#include <iostream>
#include "defs.h"
#include "board.h"

enum {WC, BC, NO_CL};
enum {P, N, B, R, Q, K, NO_TP};
enum {FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H};
enum {RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8};

#define Opp(x)          ((x) ^ 1)
#define PcBb(p, x, y)   (p.get_piece_mask(y + (x == BLACK ? 6 : 0))) 
#define OccBb(p)        (p.occ_mask)  
#define File(x)         ((x) & 7)
#define Rank(x)         ((x) >> 3)
#define Sq(x, y)        (((y) << 3) | (x))
#define SqBb(x)         ((uint64_t)1 << (x))

#define U64 uint64_t
#define FILE_A_BB       (U64)0x0101010101010101
#define FILE_B_BB       (U64)0x0202020202020202
#define DIAG_A1H8_BB    (U64)0x8040201008040201
#define DIAG_A8H1_BB    (U64)0x0102040810204080
#define DIAG_B8H2_BB    (U64)0x0204081020408000
#define RANK_1_BB       (U64)0x00000000000000FF

#define Map0x88(x)      (((x) & 7) | (((x) & ~7) << 1))
#define Unmap0x88(x)    (((x) & 7) | (((x) & ~7) >> 1))
#define Sq0x88Off(x)    ((unsigned)(x) & 0x88)

#define RankIndex(o, x) (((o) >> ((070 & (x)) + 1)) & 63)
#define FileIndex(o, x) (((FILE_A_BB & ((o) >> File(x))) * DIAG_B8H2_BB) >> 58)
#define DiagIndex(o, x) ((((o) & line_mask[2][x]) * FILE_B_BB) >> 58)
#define AntiIndex(o, x) ((((o) & line_mask[3][x]) * FILE_B_BB) >> 58)

#define L1Attacks(o, x) attacks[0][x][RankIndex(o, x)]
#define L2Attacks(o, x) attacks[1][x][FileIndex(o, x)]
#define L3Attacks(o, x) attacks[2][x][DiagIndex(o, x)]
#define L4Attacks(o, x) attacks[3][x][AntiIndex(o, x)]

#define RAttacks(o, x)  (L1Attacks(o, x) | L2Attacks(o, x))
#define BAttacks(o, x)  (L3Attacks(o, x) | L4Attacks(o, x))
#define QAttacks(o, x)  (RAttacks(o, x) | BAttacks(o, x))

static const int MAX_EVAL = 29999;
static bool data_initialized = false;
static uint64_t attacks[4][64][64];
static uint64_t line_mask[4][64];
static uint64_t passed_mask[2][64];
static uint64_t adjacent_mask[8];
static const int dirs[4][2] = {{1, -1}, {16, -16}, {17, -17}, {15, -15}};
static const int line[8] = {0, 2, 4, 5, 5, 4, 2, 0};
static const int passed_bonus[2][8] = {
  {0, 25, 30, 35, 40, 45, 50, 0},
  {0, 50, 45, 40, 35, 30, 25, 0}
};

void initialize_data() {
  int i, j, k, l, x, y;
    for (i = 0; i < 4; i++)
    for (j = 0; j < 64; j++)
      for (k = 0; k < 64; k++) {
        attacks[i][j][k] = 0;
        for (l = 0; l < 2; l++) {
          x = Map0x88(j) + dirs[i][l];
          while (!Sq0x88Off(x)) {
            y = Unmap0x88(x);
            attacks[i][j][k] |= SqBb(y);
            if ((k << 1) & (1 << (i != 1 ? File(y) : Rank(y))))
              break;
            x += dirs[i][l];
          }
        }
      }
  for (i = 0; i < 64; i++) {
    line_mask[0][i] = RANK_1_BB << (i & 070);
    line_mask[1][i] = FILE_A_BB << (i & 007);
    j = File(i) - Rank(i);
    if (j > 0)
      line_mask[2][i] = DIAG_A1H8_BB >> (j * 8);
    else
      line_mask[2][i] = DIAG_A1H8_BB << (-j * 8);
    j = File(i) - (RANK_8 - Rank(i));
    if (j > 0)
      line_mask[3][i] = DIAG_A8H1_BB << (j * 8);
    else
      line_mask[3][i] = DIAG_A8H1_BB >> (-j * 8);
  }
  for (i = 0; i < 64; i++) {
    passed_mask[WC][i] = 0;
    for (j = File(i) - 1; j <= File(i) + 1; j++) {
      if ((File(i) == FILE_A && j == -1) ||
          (File(i) == FILE_H && j == 8))
        continue;
      for (k = Rank(i) + 1; k <= RANK_8; k++)
        passed_mask[WC][i] |= SqBb(Sq(j, k));
    }
  }
  for (i = 0; i < 64; i++) {
    passed_mask[BC][i] = 0;
    for (j = File(i) - 1; j <= File(i) + 1; j++) {
      if ((File(i) == FILE_A && j == -1) ||
          (File(i) == FILE_H && j == 8))
        continue;
      for (k = Rank(i) - 1; k >= RANK_1; k--)
        passed_mask[BC][i] |= SqBb(Sq(j, k));
    }
  }
  for (i = 0; i < 8; i++) {
    adjacent_mask[i] = 0;
    if (i > 0)
      adjacent_mask[i] |= FILE_A_BB << (i - 1);
    if (i < 7)
      adjacent_mask[i] |= FILE_A_BB << (i + 1);
  }
  for (i = 0; i < 64; i++) {
    j = line[File(i)] + line[Rank(i)];
    pst[P][i] = j * 2;
    pst[N][i] = j * 4;
    pst[B][i] = j * 2;
    pst[R][i] = line[File(i)];
    pst[Q][i] = j;
    pst[K][i] = j * 6;
  }
  data_initialized = true;
}

int mobility(const Board& board, int side)
{
  
  uint64_t pieces;
  int from, mob;

  mob = 0;
  pieces = PcBb(board, side, B);
  while (pieces) {
    from = pop_first_bit(pieces);
    mob += popcnt(BAttacks(OccBb(board), from)) * 4;
  }
  pieces = PcBb(board, side, R);
  while (pieces) {
    from = pop_first_bit(pieces);
    mob += popcnt(RAttacks(OccBb(board), from)) * 2;
  }
  pieces = PcBb(board, side, Q);
  while (pieces) {
    from = pop_first_bit(pieces);
    mob += popcnt(QAttacks(OccBb(board), from));
  }
  return mob;
}

int evaluate_pawns(const Board& board, int side)
{
  U64 pieces;
  int from, score;

  score = 0;
  pieces = PcBb(board, side, P);
  while (pieces) {
    from = pop_first_bit(pieces);
    if (!(passed_mask[side][from] & PcBb(board, Opp(side), P)))
      score += passed_bonus[side][Rank(from)];
    if (!(adjacent_mask[File(from)] & PcBb(board, side, P)))
      score -= 20;
  }
  return score;
}

int evaluate_king(const Board& board, int side)
{
  if (!PcBb(board, Opp(side), Q) || board.b_mat[Opp(side)] <= 1600)
    return 0;
  return -2 * pst[K][lsb(board.get_king_mask(side))];
}

int calculate_mat(const Board& board) {
  uint64_t pieces;
  int score;

  score = 0;
  pieces = board.get_side_mask(WHITE);
  while(pieces) {
     score += piece_value[board.piece_at[pop_first_bit(pieces)]];
  }
  pieces = board.get_side_mask(BLACK);
  while(pieces) {
     score -= piece_value[board.piece_at[pop_first_bit(pieces)]];
  }
  return score;
}

int evaluate(const Board& board) {
  if(!data_initialized) {
    cerr << "Initializing sungorus data" << endl;
    initialize_data();
  }
  cerr << "Running evaluate" << endl;

  int score;

  // if((board.mat[WHITE] - board.mat[BLACK]) != calculate_mat(board)) {
  //   cout << RED_COLOR << "Material calculation error" << RESET_COLOR << endl;
  //   assert((board.mat[WHITE] - board.mat[BLACK]) == calculate_mat(board));
  // }
  // score = board.mat[WHITE] - board.mat[BLACK];
  score = calculate_mat(board);
  if (score > -200 && score < 200)
    score += mobility(board, WHITE) - mobility(board, BLACK);
  // score += board.pst[WHITE] - board.pst[BLACK];
  score += evaluate_pawns(board, WHITE) - evaluate_pawns(board, BLACK);
  score += evaluate_king(board, WHITE) - evaluate_king(board, BLACK);
  if (score < -MAX_EVAL)
    score = -MAX_EVAL;
  else if (score > MAX_EVAL)
    score = MAX_EVAL;
  return board.side == WHITE ? score : -score;
}

