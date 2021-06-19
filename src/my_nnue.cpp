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
#include "my_nnue.h"

using clipped_t = int8_t;

#define clamp(a, b, c) ((a < b) ? b : ((a > c) ? c : a))

#define INCREMENTAL_NNUE
// #define AVX2

static int16_t ft_weights alignas(64) [256 * 41024];
static clipped_t hidden_1_weights alignas(64) [2 * 256 * 32];
static clipped_t hidden_2_weights alignas(64) [32 * 32];
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

// struct Accumulator {
//     bool has_been_computed;
//     alignas(64) int16_t accumulation[2][256];
//     int computedAccumulation;
// };

// struct DirtyPiece {
//     uint8_t piece[3];
//     Move moves[3]; // contains from and to but no flag
//     uint8_t dirty_num;
// };

struct IndexList {
    int values[2][32];
    size_t size;
};

inline int orient(int sq, bool side) {
    return side ? (sq ^ 63) : sq;
}

inline int make_acc_index(int pc, int sq, int ksq, bool perspective) {
    return 256 * (orient(sq, perspective) + pieceToIndex[perspective][pc] + ksq * INDEX_END); // * kHalfDimensions; 
}

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
    for(int i = 0; i < dp->dirtyNum; i++) {
        if(dp->to[i] != NO_SQ) {
            added_indices->values[WHITE][added_indices->size] = make_acc_index(dp->pc[i], dp->to[i], w_ksq, WHITE);
            added_indices->values[BLACK][added_indices->size++] = make_acc_index(dp->pc[i], dp->to[i], b_ksq, BLACK);
        }
        if(dp->from[i] != NO_SQ) {
            removed_indices->values[WHITE][removed_indices->size] = make_acc_index(dp->pc[i], dp->from[i], w_ksq, WHITE);
            removed_indices->values[BLACK][removed_indices->size++] = make_acc_index(dp->pc[i], dp->from[i], b_ksq, BLACK);
        }
    }
}

void compute_acc(Accumulator* acc, IndexList* indices) {
#if defined(AVX2)
    assert(false);
#else
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
#endif
}

void update_acc(Accumulator* acc, Accumulator* prev_acc, IndexList* added_indices, IndexList* removed_indices) {
#if defined(AVX2)

#else
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
#endif
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

void read_net(const void* eval_data) {
    const char* d = (const char*)eval_data + TransformerStart + 4;
    int i, j;

    for(i = 0; i < kHalfDimensions; i++, d += 2)
        ft_biases[i] = readu_le_u16(d);
    for(i = 0; i < kHalfDimensions * FtInDims; i++, d += 2)
        ft_weights[i] = readu_le_u16(d);

    d += 4;

    for(i = 0; i < 32; i++, d += 4)
        hidden_1_biases[i] = readu_le_u32(d);
    for(j = 0; j < 32; j++)
        for(i = 0; i < 512; i++, d += 1)
            hidden_1_weights[i * 32 + j] = *d;
    for(i = 0; i < 32; i++, d += 4)
        hidden_2_biases[i] = readu_le_u32(d);
    for(j = 0; j < 32; j++)
        for(i = 0; i < 32; i++, d += 1)
            hidden_2_weights[i * 32 + j] = *d;
    for(i = 0; i < 1; i++, d += 4)
        output_biases[i] = readu_le_u32(d);
    for(i = 0; i < 32; i++, d += 1)
        output_weights[i] = readu_le_u8(d);
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

void my_nnue_init(const char* file_name) {
    if (load_eval_file(file_name)) {
        cerr << GREEN_COLOR << "NNUE loaded " << file_name << "!" << endl << RESET_COLOR;
        nnue_initialized = true;
        return;
    }
    cerr << RED_COLOR << "Error loading NNUE file " << file_name << endl << RESET_COLOR;
    assert(false);
    while(1); // in case we have -DNDEBUG flag 
}

void affine_txfm(clipped_t *input, clipped_t *output, clipped_t *weights, int32_t* biases,
                 unsigned input_dim, unsigned output_dim) {
#if defined(AVX2)

#else
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
#endif
}

int32_t affine_propagate(clipped_t* input, clipped_t* weights, int32_t* biases, 
                         unsigned input_dim) {
#if defined(AVX2)  

#else
    int32_t ans = biases[0];
    for(unsigned i = 0; i < input_dim; i++) {
        ans += input[i] * weights[i];
    }
    return ans;
#endif
}

// InputLayer = InputSlice<256 * 2>
// out: 512 x clipped_t

// Hidden1Layer = ClippedReLu<AffineTransform<InputLayer, 32>>
// 512 x clipped_t -> 32 x int32_t -> 32 x clipped_t

// Hidden2Layer = ClippedReLu<AffineTransform<hidden1, 32>>
// 32 x clipped_t -> 32 x int32_t -> 32 x clipped_t

// OutputLayer = AffineTransform<HiddenLayer2, 1>
// 32 x clipped_t -> 1 x int32_t

struct NetData {
    alignas(64) clipped_t input[512];
    clipped_t hidden_1_out[32];
    clipped_t hidden_2_out[32];
};

#define NNUE_PATH "/Users/balce/maia-net.bin"

// #undef INCREMENTAL_NNUE

bool is_acc_ok(Accumulator* acc1, Accumulator* acc2) {
    if(acc1->has_been_computed != acc2->has_been_computed)
        return false;
    for(int c = 0; c < 2; c++) {
        for(int i = 0; i < 256; i++) {
            if(acc1->accumulation[c][i] != acc2->accumulation[c][i]) {
                cerr << "Accs don't match at c = " << c << ", i = " << i << " "; 
                cerr << (int)acc1->accumulation[c][i] << " " << (int)acc2->accumulation[c][i] << endl;
                return false;
            }
        }
    }
    return true;
}

// static int total_calcs = 0;
// static int no_calcs = 0;
// static int incremental_calcs = 0;
// static int normal_calcs = 0;

int my_nnue_eval(Board* board) {
    if(!nnue_initialized)
        my_nnue_init(NNUE_PATH);

    // total_calcs++;
    // if(total_calcs % 1000000 == 0) {
    //     cerr << "No calc   " << (no_calcs * 100) / total_calcs << "%" << endl;
    //     cerr << "Inc calc  " << (incremental_calcs * 100) / total_calcs << "%" << endl;
    //     cerr << "Norm calc " << (normal_calcs * 100) / total_calcs << "%" << endl;
    //     cerr << "******" << endl;
    // }

    Accumulator* acc = &board->acc_stack[board->acc_stack_size];
    DirtyPiece* dp = board->acc_stack_size ? &board->dp_stack[board->acc_stack_size - 1] : NULL;
    Accumulator* prev_acc = board->acc_stack_size ? &board->acc_stack[board->acc_stack_size - 1] : NULL;

    // cerr << "We will calculate score for the following board" << endl;
    // board->print_board();

    acc->has_been_computed = false;

    if(acc->has_been_computed) {
        // don't do anything
        // no_calcs++;
#if defined(INCREMENTAL_NNUE)
    } else if(prev_acc != NULL && prev_acc->has_been_computed && dp->no_king) {
        // incremental_calcs++;
        IndexList added_indices, removed_indices;
        added_indices.size = removed_indices.size = 0;
        append_changed_indices(&added_indices, &removed_indices, board, dp);
        update_acc(acc, prev_acc, &added_indices, &removed_indices);

        // // we check that the updated acc is okay
        // Accumulator acc_tmp;
        // IndexList index_list;
        // index_list.size = 0;
        // append_active_indices(&index_list, board);
        // compute_acc(&acc_tmp, &index_list);
        // assert(is_acc_ok(acc, &acc_tmp));
#endif
    } else {
        // normal_calcs++;
        IndexList index_list;
        index_list.size = 0;
        append_active_indices(&index_list, board);
        compute_acc(acc, &index_list);
    }

    assert(acc->has_been_computed);

    struct NetData buf;
    unsigned i, j;

    for(i = 0; i < kHalfDimensions; i++) {
        buf.input[i] = clamp(acc->accumulation[board->side][i], 0, 127);
        buf.input[i + 256] = clamp(acc->accumulation[board->xside][i], 0, 127);
    }

    affine_txfm(buf.input, buf.hidden_1_out, hidden_1_weights, hidden_1_biases, 512, 32);

    affine_txfm(buf.hidden_1_out, buf.hidden_2_out, hidden_2_weights, hidden_2_biases, 32, 32);

    int32_t nnue_score = affine_propagate(buf.hidden_2_out, output_weights, output_biases, 32);

    return (nnue_score / FV_SCALE);
}
