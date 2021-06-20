#include <iostream>
#include <cassert>
#include "defs.h"
#include "board.h"
#include "misc.h"
#include "nnue.h"

enum {WC, BC, NO_CL};
enum {P, N, B, R, Q, K, NO_TP};
enum {FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H};
enum {RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8};

#define Opp(x)          ((x) ^ 1)
#define PcBb(p, x, y)   (p.bits[y + (x == BLACK ? 6 : 0)])
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

#define get_side_mask(_side) (_side == WHITE ? \
	(board.bits[WHITE_PAWN] | board.bits[WHITE_KNIGHT] | board.bits[WHITE_BISHOP] | board.bits[WHITE_ROOK] | board.bits[WHITE_QUEEN] | board.bits[WHITE_KING]) : \
	(board.bits[BLACK_PAWN] | board.bits[BLACK_KNIGHT] | board.bits[BLACK_BISHOP] | board.bits[BLACK_ROOK] | board.bits[BLACK_QUEEN] | board.bits[BLACK_KING]))

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
static const int pst[6][64] = {
  { 0, 4, 8, 10, 10, 8, 4, 0, 4, 8, 12, 14, 14, 12, 8, 4, 8, 12, 16, 18, 18, 16, 12, 8, 10, 14, 18, 20, 20, 18, 14, 10, 10, 14, 18, 20, 20, 18, 14, 10, 8, 12, 16, 18, 18, 16, 12, 8, 4, 8, 12, 14, 14, 12, 8, 4, 0, 4, 8, 10, 10, 8, 4, 0 },
  { 0, 8, 16, 20, 20, 16, 8, 0, 8, 16, 24, 28, 28, 24, 16, 8, 16, 24, 32, 36, 36, 32, 24, 16, 20, 28, 36, 40, 40, 36, 28, 20, 20, 28, 36, 40, 40, 36, 28, 20, 16, 24, 32, 36, 36, 32, 24, 16, 8, 16, 24, 28, 28, 24, 16, 8, 0, 8, 16, 20, 20, 16, 8, 0 },
  { 0, 4, 8, 10, 10, 8, 4, 0, 4, 8, 12, 14, 14, 12, 8, 4, 8, 12, 16, 18, 18, 16, 12, 8, 10, 14, 18, 20, 20, 18, 14, 10, 10, 14, 18, 20, 20, 18, 14, 10, 8, 12, 16, 18, 18, 16, 12, 8, 4, 8, 12, 14, 14, 12, 8, 4, 0, 4, 8, 10, 10, 8, 4, 0 },
  { 0, 2, 4, 5, 5, 4, 2, 0, 0, 2, 4, 5, 5, 4, 2, 0, 0, 2, 4, 5, 5, 4, 2, 0, 0, 2, 4, 5, 5, 4, 2, 0, 0, 2, 4, 5, 5, 4, 2, 0, 0, 2, 4, 5, 5, 4, 2, 0, 0, 2, 4, 5, 5, 4, 2, 0, 0, 2, 4, 5, 5, 4, 2, 0 },
  { 0, 2, 4, 5, 5, 4, 2, 0, 2, 4, 6, 7, 7, 6, 4, 2, 4, 6, 8, 9, 9, 8, 6, 4, 5, 7, 9, 10, 10, 9, 7, 5, 5, 7, 9, 10, 10, 9, 7, 5, 4, 6, 8, 9, 9, 8, 6, 4, 2, 4, 6, 7, 7, 6, 4, 2, 0, 2, 4, 5, 5, 4, 2, 0 },
  { 0, 12, 24, 30, 30, 24, 12, 0, 12, 24, 36, 42, 42, 36, 24, 12, 24, 36, 48, 54, 54, 48, 36, 24, 30, 42, 54, 60, 60, 54, 42, 30, 30, 42, 54, 60, 60, 54, 42, 30, 24, 36, 48, 54, 54, 48, 36, 24, 12, 24, 36, 42, 42, 36, 24, 12, 0, 12, 24, 30, 30, 24, 12, 0 }
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
  return -2 * pst[K][lsb(board.bits[KING + (side == BLACK ? 6 : 0)])];
}

int calculate_mat_p(Board* board) {
  uint64_t pieces;
  int score;

  score = 0;
  pieces = board->bits[WHITE_PAWN] | board->bits[WHITE_KNIGHT] | board->bits[WHITE_BISHOP] | board->bits[WHITE_ROOK] | board->bits[WHITE_QUEEN] | board->bits[WHITE_KING];
  while(pieces) {
    int sq = pop_first_bit(pieces);
    score += piece_value[board->piece_at[sq]];
    score += pst[board->piece_at[sq]][sq];
  }
  pieces = board->bits[BLACK_PAWN] | board->bits[BLACK_KNIGHT] | board->bits[BLACK_BISHOP] | board->bits[BLACK_ROOK] | board->bits[BLACK_QUEEN] | board->bits[BLACK_KING];
  while(pieces) {
    int sq = pop_first_bit(pieces);
    score -= piece_value[board->piece_at[sq]];
    score -= pst[board->piece_at[sq]][sq];
  }
  return score;
}

int calculate_mat(const Board& board) {
  uint64_t pieces;
  int score;

  score = 0;
  pieces = get_side_mask(WHITE);
  while(pieces) {
    int sq = pop_first_bit(pieces);
    score += piece_value[board.piece_at[sq]];
    score += pst[board.piece_at[sq]][sq];
  }
  pieces = get_side_mask(BLACK);
  while(pieces) {
    int sq = pop_first_bit(pieces);
    score -= piece_value[board.piece_at[sq]];
    score -= pst[board.piece_at[sq]][sq];
  }
  return score;
}

// static const int p_conv[12] = {
//   wpawn, wknight, wbishop, wrook, wqueen, wking,
//   bpawn, bknight, bbishop, brook, bqueen, bking
// };

// int nnue_eval_centr(Board& board) {
  // int player = board.side;
  // int pieces[64], squares[64];

  // int j = 2;
  // for(int i = 0; i < 64; i++) {
  //   if(board.piece_at[i] == KING && board.color_at[i] == WHITE) {
  //     pieces[0] = wking;
  //     squares[0] = i;
  //   } else if(board.piece_at[i] == KING) {
  //     pieces[1] = bking;
  //     squares[1] = i;
  //   } else if(board.piece_at[i] != EMPTY) {
  //     pieces[j] = p_conv[board.piece_at[i] + (board.color_at[i] ? 6 : 0)];
  //     squares[j++] = i;
  //   }
  // }

  // pieces[j] = 0;
  // squares[j] = 0;

  // cerr << "Calculating NNUE score of board" << endl;
  // board.print_board();

  // int score = nnue_eval(&board);
  // int score2 = nnue_eval(&board);
  // score = score2;

  // if(score != score2) {
  //   cerr << BLUE_COLOR << "Scores are different for the following position" << endl << RESET_COLOR;
  //   cerr << score << " " << score2 << " " << score3 << endl;
  //   board.print_board();
  // }

  // assert(score == score2 && score == score3);
  // if(score != score2 || score != score3)
  //   while(1);

  // return score;
// }

int evaluate(const Board& board) {
  if(!data_initialized) {
    initialize_data();
  }
  int score;

  if((board.b_mat[WHITE] - board.b_mat[BLACK] + board.b_pst[WHITE] - board.b_pst[BLACK]) != calculate_mat(board)) {
    cout << RED_COLOR
    << "Mat calculation error" << " "
    << (board.b_mat[WHITE] - board.b_mat[BLACK] + board.b_pst[WHITE] - board.b_pst[BLACK]) << " "
    << calculate_mat(board)
    << RESET_COLOR << endl;
    cerr << "Board is " << endl;
    board.print_board();
  }
  assert((board.b_mat[WHITE] - board.b_mat[BLACK] + board.b_pst[WHITE] - board.b_pst[BLACK]) == calculate_mat(board));
  score = board.b_mat[WHITE] - board.b_mat[BLACK];
  score += board.b_pst[WHITE] - board.b_pst[BLACK];
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

