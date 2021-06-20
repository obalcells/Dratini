#include <iostream>
#include <sys/stat.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdint.h>
#include "defs.h"
#include "board.h"
#include "nnue.h"

typedef int8_t clipped_t;
typedef uint32_t mask_t;
typedef uint64_t mask2_t;
typedef int8_t weight_t;

#define INCREMENTAL_NNUE
#define USE_AVX2

#if defined(USE_AVX2)
#include <immintrin.h>
#define SIMD_WIDTH 256
typedef __m256i vec16_t;
typedef __m256i vec8_t;
#define vec_add_16(a,b) _mm256_add_epi16(a,b)
#define vec_sub_16(a,b) _mm256_sub_epi16(a,b)
#define vec_packs(a,b) _mm256_packs_epi16(a,b)
#define vec_mask_pos(a) _mm256_movemask_epi8(_mm256_cmpgt_epi8(a,_mm256_setzero_si256()))
#define NUM_REGS 16
#endif

static int16_t ft_weights alignas(64) [256 * 41024];
static weight_t hidden_1_weights alignas(64) [2 * 256 * 32];
static weight_t hidden_2_weights alignas(64) [32 * 32];
static clipped_t output_weights alignas(64) [1 * 32];

static int16_t ft_biases alignas(64) [256];
static int32_t hidden_1_biases alignas(64) [32];
static int32_t hidden_2_biases alignas(64) [32];
static int32_t output_biases[1];

enum ACC_INDEXES {
    INDEX_WHITE_PAWN = 1,
    INDEX_BLACK_PAWN = 65,
    INDEX_WHITE_KNIGHT = 129,
    INDEX_BLACK_KNIGHT = 193,
    INDEX_WHITE_BISHOP = 257,
    INDEX_BLACK_BISHOP = 321,
    INDEX_WHITE_ROOK = 385,
    INDEX_BLACK_ROOK = 449,
    INDEX_WHITE_QUEEN = 513,
    INDEX_BLACK_QUEEN = 577,
    INDEX_END = 641
};

// If side is black we want to turn WHITE_PAWN into BLACK_PAWN, WHITE_KNIGHT into BLACK_KNIGHT, etc
// because we want to invert the view
static const int pieceToIndex[2][12] {
    { INDEX_WHITE_PAWN, INDEX_WHITE_KNIGHT, INDEX_WHITE_BISHOP, INDEX_WHITE_ROOK, INDEX_WHITE_QUEEN, 0,
      INDEX_BLACK_PAWN, INDEX_BLACK_KNIGHT, INDEX_BLACK_BISHOP, INDEX_BLACK_ROOK, INDEX_BLACK_QUEEN, 0 },
    { INDEX_BLACK_PAWN, INDEX_BLACK_KNIGHT, INDEX_BLACK_BISHOP, INDEX_BLACK_ROOK, INDEX_BLACK_QUEEN, 0,
      INDEX_WHITE_PAWN, INDEX_WHITE_KNIGHT, INDEX_WHITE_BISHOP, INDEX_WHITE_ROOK, INDEX_WHITE_QUEEN, 0 }
};

static const uint32_t nnueVersion = 0x7AF32F16u;
static const int TransformerStart = 3 * 4 + 177;
static const int NetworkStart = TransformerStart + 4 + 2 * 256 + 2 * 256 * 64 * 641;
static const int SHIFT = 6;
static const int FV_SCALE = 16;
static const int kHalfDimensions = 256;
static const int FtInDims = INDEX_END * 64;
static bool nnue_initialized = false;

struct IndexList {
    int values[2][32];
    size_t size;
};

inline uint32_t readu_le_u32(const void *p) {
  const uint8_t *q = (const uint8_t*) p;
  return q[0] | (q[1] << 8) | (q[2] << 16) | (q[3] << 24);
}

inline uint16_t readu_le_u16(const void *p) {
  const uint8_t *q = (const uint8_t*) p;
  return q[0] | (q[1] << 8);
}

inline uint16_t readu_le_u8(const void *p) {
  const uint8_t *q = (const uint8_t*) p;
  return q[0];
}

inline int orient(int sq, bool side) {
    return side ? (sq ^ 63) : sq;
}

inline int make_acc_index(int pc, int sq, int ksq, bool perspective) {
    return 256 * (orient(sq, perspective) + pieceToIndex[perspective][pc] + ksq * INDEX_END); // * kHalfDimensions; 
}

void append_active_indices(IndexList* index_list, Board* board) {
    int w_ksq = lsb(board->bits[WHITE_KING]);
    int b_ksq = orient(lsb(board->bits[BLACK_KING]), BLACK);
    for(int sq = 0; sq < 64; sq++) if(board->piece_at[sq] != EMPTY && board->piece_at[sq] != KING) {
        index_list->values[WHITE][index_list->size] = make_acc_index(make_piece(board->piece_at[sq], board->color_at[sq]), sq, w_ksq, WHITE);
        index_list->values[BLACK][index_list->size++] = make_acc_index(make_piece(board->piece_at[sq], board->color_at[sq]), sq, b_ksq, BLACK);
    }
} 

void append_changed_indices(IndexList* added_indices, IndexList* removed_indices, Board* board, DirtyPiece* dp) {
    int w_ksq = lsb(board->bits[WHITE_KING]);
    int b_ksq = orient(lsb(board->bits[BLACK_KING]), BLACK);
    for(int i = 0; i < dp->dirtyNum; i++) if(dp->pc[i] != WHITE_KING && dp->pc[i] != BLACK_KING) {
        if(dp->from[i] != NO_SQ) {
            removed_indices->values[WHITE][removed_indices->size] = make_acc_index(dp->pc[i], dp->from[i], w_ksq, WHITE);
            removed_indices->values[BLACK][removed_indices->size++] = make_acc_index(dp->pc[i], dp->from[i], b_ksq, BLACK);
        }
        if(dp->to[i] != NO_SQ) {
            added_indices->values[WHITE][added_indices->size] = make_acc_index(dp->pc[i], dp->to[i], w_ksq, WHITE);
            added_indices->values[BLACK][added_indices->size++] = make_acc_index(dp->pc[i], dp->to[i], b_ksq, BLACK);
        }
    }
}

void compute_acc(Accumulator* acc, IndexList* indices) {
// #if defined(AVX2)
//     to be implemented...
// #else
    memcpy(acc->accumulation[WHITE], ft_biases, 256 * sizeof(int16_t));
    memcpy(acc->accumulation[BLACK], ft_biases, 256 * sizeof(int16_t));

    unsigned i, j;

    for(i = 0; i < indices->size; i++) {
        for(j = 0; j < kHalfDimensions; j++) {
            acc->accumulation[WHITE][j] += ft_weights[indices->values[WHITE][i] + j]; 
            acc->accumulation[BLACK][j] += ft_weights[indices->values[BLACK][i] + j]; 
        }
    }

    acc->has_been_computed = true;
// #endif
}

void update_acc(Accumulator* acc, Accumulator* prev_acc, IndexList* added_indices, IndexList* removed_indices) {
// #ifdef USE_AVX2
//     to be implemented...
// #else
    assert(prev_acc->has_been_computed);

    memcpy(acc, prev_acc, sizeof(Accumulator));

    unsigned i, j;

    for(i = 0; i < removed_indices->size; i++) {
        for(j = 0; j < kHalfDimensions; j++) {
            acc->accumulation[WHITE][j] -= ft_weights[removed_indices->values[WHITE][i] + j]; 
            acc->accumulation[BLACK][j] -= ft_weights[removed_indices->values[BLACK][i] + j]; 
        }
    }

    for(i = 0; i < added_indices->size; i++) {
        for(j = 0; j < kHalfDimensions; j++) {
            acc->accumulation[WHITE][j] += ft_weights[added_indices->values[WHITE][i] + j]; 
            acc->accumulation[BLACK][j] += ft_weights[added_indices->values[BLACK][i] + j]; 
        }
    }

    assert(acc->has_been_computed);
// #endif
}

static bool verify_net(const void *eval_data, size_t size) {
  if (size != 21022697)
      return false;

  const char *d = (const char*)eval_data;

  if (readu_le_u32(d) != nnueVersion) return false;
  if (readu_le_u32(d + 4) != 0x3e5aa6eeU) return false;
  if (readu_le_u32(d + 8) != 177) return false;
  if (readu_le_u32(d + TransformerStart) != 0x5d69d7b8) return false;
  if (readu_le_u32(d + NetworkStart) != 0x63337156) return false;

  return true;
}

#ifdef USE_AVX2
static void permute_biases(int32_t *biases) {
  __m128i *b = (__m128i *)biases;
  __m128i tmp[8];
  tmp[0] = b[0];
  tmp[1] = b[4];
  tmp[2] = b[1];
  tmp[3] = b[5];
  tmp[4] = b[2];
  tmp[5] = b[6];
  tmp[6] = b[3];
  tmp[7] = b[7];
  memcpy(b, tmp, 8 * sizeof(__m128i));
}
#endif

inline unsigned wt_idx(unsigned r, unsigned c, unsigned dims) {
    (void)dims;
#ifdef USE_AVX2
  if (dims > 32) {
    unsigned b = c & 0x18;
    b = (b << 1) | (b >> 1);
    c = (c & ~0x18) | (b & 0x18);
  }
#endif
  return c * 32 + r;
}

void read_net(const void* eval_data) {
    const char* d = (const char*)eval_data + TransformerStart + 4;
    int i, j;

    for(i = 0; i < kHalfDimensions; i++, d += 2)
        ft_biases[i] = readu_le_u16(d);
    for(i = 0; i < kHalfDimensions * FtInDims; i++, d += 2)
        ft_weights[i] = readu_le_u16(d);

    d += 4; // very important!

    for(i = 0; i < 32; i++, d += 4)
        hidden_1_biases[i] = readu_le_u32(d);
    for(j = 0; j < 32; j++)
        for(i = 0; i < 512; i++, d += 1)
            hidden_1_weights[wt_idx(j, i, 512)] = *d;
    for(i = 0; i < 32; i++, d += 4)
        hidden_2_biases[i] = readu_le_u32(d);
    for(j = 0; j < 32; j++)
        for(i = 0; i < 32; i++, d += 1)
            hidden_2_weights[wt_idx(j, i, 32)] = *d;
    for(i = 0; i < 1; i++, d += 4)
        output_biases[i] = readu_le_u32(d);
    for(i = 0; i < 32; i++, d += 1)
        output_weights[i] = readu_le_u8(d);

#ifdef USE_AVX2
    permute_biases(hidden_1_biases);
    permute_biases(hidden_2_biases);
#endif
}

static size_t file_size(int fd) {
    struct stat statbuf;
    fstat(fd, &statbuf);
    return statbuf.st_size;
}

static bool load_eval_file(const char *file_name) {
    const void *data;
    size_t size;

    {
        int fd = open(file_name, 0x0000); 
        if (fd == -1)
            return false;
        size = file_size(fd);
        void* _data = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
        data = _data == MAP_FAILED ? NULL : _data;
        close(fd);
    }

    bool success = verify_net(data, size);
    if (success)
        read_net(data);
    if(size && data)
        munmap((void *)data, size);
    return success;
}

void nnue_init(const char* file_name) {
    if (load_eval_file(file_name)) {
        cerr << GREEN_COLOR << "NNUE loaded " << file_name << "!" << endl << RESET_COLOR;
        nnue_initialized = true;
        return;
    }
    cerr << RED_COLOR << "Error loading NNUE file " << file_name << endl << RESET_COLOR;
    assert(false);
    while(1); // in case we have -DNDEBUG flag 
}

#ifdef USE_AVX2
void transform(const bool side, Accumulator* acc, clipped_t* output, mask_t* out_mask) {
    const bool xside = !side;
    unsigned i;
    int16_t (*accumulation)[2][256] = &acc->accumulation;

    const unsigned num_chunks = (16 * kHalfDimensions) / SIMD_WIDTH;

    vec8_t* out = (vec8_t*)&output[0];
    for(i = 0; i < num_chunks / 2; i++) {
        vec16_t s0 = ((vec16_t*)(*accumulation)[side])[i * 2];
        vec16_t s1 = ((vec16_t *)(*accumulation)[side])[i * 2 + 1];
        out[i] = _mm256_packs_epi16(s0, s1);
        *(out_mask++) = _mm256_movemask_epi8(_mm256_cmpgt_epi8(out[i] ,_mm256_setzero_si256()));
    }

    out = (vec8_t*)&output[kHalfDimensions];
    for(i = 0; i < num_chunks / 2; i++) {
        vec16_t s0 = ((vec16_t*)(*accumulation)[xside])[i * 2];
        vec16_t s1 = ((vec16_t *)(*accumulation)[xside])[i * 2 + 1];
        out[i] = _mm256_packs_epi16(s0, s1);
        *(out_mask++) = _mm256_movemask_epi8(_mm256_cmpgt_epi8(out[i] ,_mm256_setzero_si256()));
    }
}
#endif

inline bool next_idx(unsigned *idx, unsigned *offset, mask2_t *v,
    mask_t *mask, unsigned inDims) {
  while (*v == 0) {
    *offset += 8 * sizeof(mask2_t);
    if (*offset >= inDims) return false;
    memcpy(v, (char *)mask + (*offset / 8), sizeof(mask2_t));
  }
  *idx = *offset + __builtin_ctzll(*v);
  *v &= *v - 1;
  return true;
}

#ifdef USE_AVX2
inline void affine_txfm(int8_t *input, void *output, unsigned inDims,
                        unsigned outDims, const int32_t *biases, const weight_t *weights,
                        mask_t *inMask, mask_t *outMask, const bool pack8_and_calc_mask) {
  assert(outDims == 32);

  const __m256i kZero = _mm256_setzero_si256();
  __m256i out_0 = ((__m256i *)biases)[0];
  __m256i out_1 = ((__m256i *)biases)[1];
  __m256i out_2 = ((__m256i *)biases)[2];
  __m256i out_3 = ((__m256i *)biases)[3];
  __m256i first, second;
  mask2_t v;
  unsigned idx;

  memcpy(&v, inMask, sizeof(mask2_t));
  for (unsigned offset = 0; offset < inDims;) {
    if (!next_idx(&idx, &offset, &v, inMask, inDims))
      break;
    first = ((__m256i *)weights)[idx];
    uint16_t factor = input[idx];
    if (next_idx(&idx, &offset, &v, inMask, inDims)) {
      second = ((__m256i *)weights)[idx];
      factor |= input[idx] << 8;
    } else {
      second = kZero;
    }
    __m256i mul = _mm256_set1_epi16(factor), prod, signs;
    prod = _mm256_maddubs_epi16(mul, _mm256_unpacklo_epi8(first, second));
    signs = _mm256_cmpgt_epi16(kZero, prod);
    out_0 = _mm256_add_epi32(out_0, _mm256_unpacklo_epi16(prod, signs));
    out_1 = _mm256_add_epi32(out_1, _mm256_unpackhi_epi16(prod, signs));
    prod = _mm256_maddubs_epi16(mul, _mm256_unpackhi_epi8(first, second));
    signs = _mm256_cmpgt_epi16(kZero, prod);
    out_2 = _mm256_add_epi32(out_2, _mm256_unpacklo_epi16(prod, signs));
    out_3 = _mm256_add_epi32(out_3, _mm256_unpackhi_epi16(prod, signs));
  }

  __m256i out16_0 = _mm256_srai_epi16(_mm256_packs_epi32(out_0, out_1), SHIFT);
  __m256i out16_1 = _mm256_srai_epi16(_mm256_packs_epi32(out_2, out_3), SHIFT);

  __m256i *outVec = (__m256i *)output;
  outVec[0] = _mm256_packs_epi16(out16_0, out16_1);
  if (pack8_and_calc_mask)
    outMask[0] = _mm256_movemask_epi8(_mm256_cmpgt_epi8(outVec[0], kZero));
  else
    outVec[0] = _mm256_max_epi8(outVec[0], kZero);
}
#else
void affine_txfm(clipped_t *input, clipped_t *output, clipped_t *weights, int32_t* biases,
                 unsigned input_dim, unsigned output_dim) {
    unsigned i, j;
    int32_t tmp[output_dim];

    memcpy(tmp, biases, output_dim * sizeof(int32_t));

    for(i = 0; i < input_dim; i++) if(input[i]) { 
        for(j = 0; j < output_dim; j++) {
            tmp[j] += (clipped_t)input[i] * weights[output_dim * i + j];
        }
    }

    for(i = 0; i < output_dim; i++) {
        output[i] = (int8_t)clamp((tmp[i] >> SHIFT), 0, 127);
    }
}
#endif

int32_t affine_propagate(clipped_t* input, clipped_t* weights, int32_t* biases, 
                         unsigned input_dim) {
#ifdef USE_AVX2
  __m256i *iv = (__m256i *)input;
  __m256i *row = (__m256i *)weights;
  __m256i prod = _mm256_maddubs_epi16(iv[0], row[0]);
  prod = _mm256_madd_epi16(prod, _mm256_set1_epi16(1));
  __m128i sum = _mm_add_epi32(
      _mm256_castsi256_si128(prod), _mm256_extracti128_si256(prod, 1));
  sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0x1b));
  return _mm_cvtsi128_si32(sum) + _mm_extract_epi32(sum, 1) + biases[0];
#else
    int32_t ans = biases[0];
    for(unsigned i = 0; i < input_dim; i++) {
        ans += input[i] * weights[i];
    }
    return ans;
#endif
}

struct NetData {
    alignas(64) clipped_t input[512];
    clipped_t hidden_1_out[32];
    clipped_t hidden_2_out[32];
};

int nnue_eval(Board* board) {
    if(!nnue_initialized)
        nnue_init(NNUE_PATH);

    Accumulator* acc = &board->acc_stack[board->acc_stack_size & 7];
    struct NetData buf;

#ifdef INCREMENTAL_NNUE
    if(!acc->has_been_computed) {
        bool found_computed = false;
        // most of the updates (99%) are made with the newest or the second-newest accumulator
        // so there's no need to check older ones
        int i;
        for(i = board->acc_stack_size - 1; i >= 0 && i >= board->acc_stack_size - 2; i--) {
            if(!board->dp_stack[i & 7].no_king)
                break;
            if(board->acc_stack[i & 7].has_been_computed) {
                found_computed = true;
                break;
            }
        }

        if(found_computed) {
            IndexList added_indices, removed_indices;
            added_indices.size = removed_indices.size = 0;
            for(int j = i; j < board->acc_stack_size; j++)
                append_changed_indices(&added_indices, &removed_indices, board, &board->dp_stack[j & 7]);
            assert(board->acc_stack[i & 7].has_been_computed);
            update_acc(acc, &board->acc_stack[i & 7], &added_indices, &removed_indices);
        } else {
            IndexList index_list;
            index_list.size = 0;
            append_active_indices(&index_list, board);
            compute_acc(acc, &index_list);
        }
    }
#else
    if(!acc->has_been_computed) {
        IndexList index_list;
        index_list.size = 0;
        append_active_indices(&index_list, board);
        compute_acc(acc, &index_list);
    }
#endif

    assert(acc->has_been_computed);

#ifndef USE_AVX2
    for(unsigned i = 0; i < kHalfDimensions; i++) {
        buf.input[i] = clamp(acc->accumulation[board->side][i], 0, 127);
        buf.input[i + 256] = clamp(acc->accumulation[board->xside][i], 0, 127);
    }

    affine_txfm(buf.input, buf.hidden_1_out, hidden_1_weights, hidden_1_biases, 512, 32);

    affine_txfm(buf.hidden_1_out, buf.hidden_2_out, hidden_2_weights, hidden_2_biases, 32, 32);
#else
    alignas(8) mask_t input_mask[512 / (8 * sizeof(mask_t))];
    alignas(8) mask_t hidden1_mask[8 / sizeof(mask_t)] = { 0 };

    transform(board->side, acc, buf.input, input_mask);

    affine_txfm(buf.input, buf.hidden_1_out, 512, 32, hidden_1_biases,
        hidden_1_weights, input_mask, hidden1_mask, true); 

    affine_txfm(buf.hidden_1_out, buf.hidden_2_out, 32, 32, hidden_2_biases,
        hidden_2_weights, hidden1_mask, NULL, false); 
#endif

    int32_t nnue_score = affine_propagate(buf.hidden_2_out, output_weights, output_biases, 32);

    return (nnue_score / FV_SCALE);
}
