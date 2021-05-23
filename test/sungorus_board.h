#pragma once

#include <cstdint>
#include <cinttypes>
#include "../src/board.h"

#define U64 uint64_t

enum {WC, BC, NO_CL};
enum {P, N, B, R, Q, K, NO_TP};
enum {WP, BP, WN, BN, WB, BB, WR, BR, WQ, BQ, WK, BK, NO_PC};
enum {FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H};
enum {RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8};
enum {NORMAL, CASTLE, EP_CAP, EP_SET, N_PROM, B_PROM, R_PROM, Q_PROM};
enum {NONE, UPPER, LOWER, EXACT};

#define RANK_1_BB       (U64)0x00000000000000FF
#define RANK_2_BB       (U64)0x000000000000FF00
#define RANK_3_BB       (U64)0x0000000000FF0000
#define RANK_4_BB       (U64)0x00000000FF000000
#define RANK_5_BB       (U64)0x000000FF00000000
#define RANK_6_BB       (U64)0x0000FF0000000000
#define RANK_7_BB       (U64)0x00FF000000000000
#define RANK_8_BB       (U64)0xFF00000000000000

#define FILE_A_BB       (U64)0x0101010101010101
#define FILE_B_BB       (U64)0x0202020202020202
#define FILE_C_BB       (U64)0x0404040404040404
#define FILE_D_BB       (U64)0x0808080808080808
#define FILE_E_BB       (U64)0x1010101010101010
#define FILE_F_BB       (U64)0x2020202020202020
#define FILE_G_BB       (U64)0x4040404040404040
#define FILE_H_BB       (U64)0x8080808080808080

#define DIAG_A1H8_BB    (U64)0x8040201008040201
#define DIAG_A8H1_BB    (U64)0x0102040810204080
#define DIAG_B8H2_BB    (U64)0x0204081020408000

#define SIDE_RANDOM     (~((U64)0))

#define START_POS       "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -"

#define SqBb(x)         ((U64)1 << (x))

#define Cl(x)           ((x) & 1)
#define Tp(x)           ((x) >> 1)
#define Pc(x, y)        (((y) << 1) | (x))

#define File(x)         ((x) & 7)
#define Rank(x)         ((x) >> 3)
#define Sq(x, y)        (((y) << 3) | (x))

#define Abs(x)          ((x) > 0 ? (x) : -(x))
#define Max(x, y)       ((x) > (y) ? (x) : (y))
#define Min(x, y)       ((x) < (y) ? (x) : (y))
#define Map0x88(x)      (((x) & 7) | (((x) & ~7) << 1))
#define Unmap0x88(x)    (((x) & 7) | (((x) & ~7) >> 1))
#define Sq0x88Off(x)    ((unsigned)(x) & 0x88)

#define Fsq(x)          ((x) & 63)
#define Tsq(x)          (((x) >> 6) & 63)
#define MoveType(x)     ((x) >> 12)
#define IsProm(x)       ((x) & 0x4000)
#define PromType(x)     (((x) >> 12) - 3)

#define Opp(x)          ((x) ^ 1)

#define InCheck(p)      Attacked(p, KingSq(p, p->side), Opp(p->side))
#define Illegal(p)      Attacked(p, KingSq(p, Opp(p->side)), p->side)
#define MayNull(p)      (((p)->cl_bb[(p)->side] & ~((p)->tp_bb[P] | (p)->tp_bb[K])) != 0)

#define PcBb(p, x, y)   ((p)->cl_bb[x] & (p)->tp_bb[y])
#define OccBb(p)        ((p)->cl_bb[WC] | (p)->cl_bb[BC])
#define UnoccBb(p)      (~OccBb(p))
#define TpOnSq(p, x)    (Tp((p)->pc[x]))
#define KingSq(p, x)    ((p)->king_sq[x])

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

#define FirstOne(x)     bit_table[(((x) & (~(x) + 1)) * (U64)0x0218A392CD3D5DBF) >> 58]

struct POS {
  U64 cl_bb[2];
  U64 tp_bb[6];
  int pc[64];
  int king_sq[2];
  int mat[2];
  int pst[2];
  int side;
  int c_flags;
  int ep_sq;
  int rev_moves;
  int head;
  U64 key;
  U64 rep_list[256];
};

struct UNDO {
  int ttp;
  int c_flags;
  int ep_sq;
  int rev_moves;
  U64 key;
};

void Init();
void UndoMove(POS *, int, UNDO *);
void UndoNull(POS *, UNDO *);
int Legal(POS *, int);
U64 AttacksFrom(POS *, int);
U64 AttacksTo(POS *, int);
int Attacked(POS *, int, int);
int Legal(POS *, int);
void DoMove(POS *, int, UNDO *);
void DoNull(POS *, UNDO *);
void SetPosition(POS *, char *);
U64 Key(POS *);
