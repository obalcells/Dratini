#include <sys/timeb.h>
#include <cassert>
#include "sungorus_board.h"

static U64 line_mask[4][64];
static U64 attacks[4][64][64];
static U64 p_attacks[2][64];
static U64 n_attacks[64];
static U64 k_attacks[64];
static U64 passed_mask[2][64];
static U64 adjacent_mask[8];
static int pst[6][64];
static int c_mask[64];
static const int bit_table[64] = {
   0,  1,  2,  7,  3, 13,  8, 19,
   4, 25, 14, 28,  9, 34, 20, 40,
   5, 17, 26, 38, 15, 46, 29, 48,
  10, 31, 35, 54, 21, 50, 41, 57,
  63,  6, 12, 18, 24, 27, 33, 39,
  16, 37, 45, 47, 30, 53, 49, 56,
  62, 11, 23, 32, 36, 44, 52, 55,
  61, 22, 43, 51, 60, 42, 59, 58
};
static const int passed_bonus[2][8] = {
  {0, 25, 30, 35, 40, 45, 50, 0},
  {0, 50, 45, 40, 35, 30, 25, 0}
};
static const int tp_value[7] = {
  100, 325, 325, 500, 1000, 0, 0
};
static U64 zob_piece[12][64];
static U64 zob_castle[16];
static U64 zob_ep[8];

// enum {
// 	A1, B1, C1, D1, E1, F1, G1, H1,
// 	A2, B2, C2, D2, E2, F2, G2, H2,
// 	A3, B3, C3, D3, E3, F3, G3, H3,
// 	A4, B4, C4, D4, E4, F4, G4, H4,
// 	A5, B5, C5, D5, E5, F5, G5, H5,
// 	A6, B6, C6, D6, E6, F6, G6, H6,
// 	A7, B7, C7, D7, E7, F7, G7, H7,
// 	A8, B8, C8, D8, E8, F8, G8, H8,
//   NO_SQ
// };
const int NO_SQ = 64;

static U64 Random64(void)
{
  static U64 next = 1;

  next = next * 1103515245 + 12345;
  return next;
}

void Init(void)
{
  int i, j, k, l, x, y;
  static const int dirs[4][2] = {{1, -1}, {16, -16}, {17, -17}, {15, -15}};
  static const int p_moves[2][2] = {{15, 17}, {-17, -15}};
  static const int n_moves[8] = {-33, -31, -18, -14, 14, 18, 31, 33};
  static const int k_moves[8] = {-17, -16, -15, -1, 1, 15, 16, 17};
  static const int line[8] = {0, 2, 4, 5, 5, 4, 2, 0};

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
  for (i = 0; i < 2; i++)
    for (j = 0; j < 64; j++) {
      p_attacks[i][j] = 0;
      for (k = 0; k < 2; k++) {
        x = Map0x88(j) + p_moves[i][k];
        if (!Sq0x88Off(x))
          p_attacks[i][j] |= SqBb(Unmap0x88(x));
      }
    }
  for (i = 0; i < 64; i++) {
    n_attacks[i] = 0;
    for (j = 0; j < 8; j++) {
      x = Map0x88(i) + n_moves[j];
      if (!Sq0x88Off(x))
        n_attacks[i] |= SqBb(Unmap0x88(x));
    }
  }
  for (i = 0; i < 64; i++) {
    k_attacks[i] = 0;
    for (j = 0; j < 8; j++) {
      x = Map0x88(i) + k_moves[j];
      if (!Sq0x88Off(x))
        k_attacks[i] |= SqBb(Unmap0x88(x));
    }
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
  for (i = 0; i < 64; i++)
    c_mask[i] = 15;
  c_mask[A1] = 13;
  c_mask[E1] = 12;
  c_mask[H1] = 14;
  c_mask[A8] = 7;
  c_mask[E8] = 3;
  c_mask[H8] = 11;
  for (i = 0; i < 12; i++)
    for (j = 0; j < 64; j++)
      zob_piece[i][j] = Random64();
  for (i = 0; i < 16; i++)
    zob_castle[i] = Random64();
  for (i = 0; i < 8; i++)
    zob_ep[i] = Random64();
}

U64 AttacksFrom(POS *p, int sq)
{
  switch (TpOnSq(p, sq)) {
  case P:
    return p_attacks[Cl(p->pc[sq])][sq];
  case N:
    return n_attacks[sq];
  case B:
    return BAttacks(OccBb(p), sq);
  case R:
    return RAttacks(OccBb(p), sq);
  case Q:
    return QAttacks(OccBb(p), sq);
  case K:
    return k_attacks[sq];
  }
  return 0;
}

U64 AttacksTo(POS *p, int sq)
{
  return (PcBb(p, WC, P) & p_attacks[BC][sq]) |
         (PcBb(p, BC, P) & p_attacks[WC][sq]) |
         (p->tp_bb[N] & n_attacks[sq]) |
         ((p->tp_bb[B] | p->tp_bb[Q]) & BAttacks(OccBb(p), sq)) |
         ((p->tp_bb[R] | p->tp_bb[Q]) & RAttacks(OccBb(p), sq)) |
         (p->tp_bb[K] & k_attacks[sq]);
}

int Attacked(POS *p, int sq, int side)
{
  return (PcBb(p, side, P) & p_attacks[Opp(side)][sq]) ||
         (PcBb(p, side, N) & n_attacks[sq]) ||
         ((PcBb(p, side, B) | PcBb(p, side, Q)) & BAttacks(OccBb(p), sq)) ||
         ((PcBb(p, side, R) | PcBb(p, side, Q)) & RAttacks(OccBb(p), sq)) ||
         (PcBb(p, side, K) & k_attacks[sq]);
}

void DoMove(POS *p, int move, UNDO *u)
{
  int side, fsq, tsq, ftp, ttp;

  side = p->side;
  fsq = Fsq(move);
  tsq = Tsq(move);
  ftp = TpOnSq(p, fsq);
  assert(p->pc[fsq] != NO_PC);
  ttp = TpOnSq(p, tsq);
  u->ttp = ttp;
  u->c_flags = p->c_flags;
  u->ep_sq = p->ep_sq;
  u->rev_moves = p->rev_moves;
  u->key = p->key;
  p->rep_list[p->head++] = p->key;
  if (ftp == P || ttp != NO_TP)
    p->rev_moves = 0;
  else
    p->rev_moves++;
  p->key ^= zob_castle[p->c_flags];
  p->c_flags &= c_mask[fsq] & c_mask[tsq];
  p->key ^= zob_castle[p->c_flags];
  if (p->ep_sq != NO_SQ) {
    p->key ^= zob_ep[File(p->ep_sq)];
    p->ep_sq = NO_SQ;
  }
  p->pc[fsq] = NO_PC;
  p->pc[tsq] = Pc(side, ftp);
  p->key ^= zob_piece[Pc(side, ftp)][fsq] ^ zob_piece[Pc(side, ftp)][tsq];
  p->cl_bb[side] ^= SqBb(fsq) | SqBb(tsq);
  p->tp_bb[ftp] ^= SqBb(fsq) | SqBb(tsq);
  p->pst[side] += pst[ftp][tsq] - pst[ftp][fsq];
  if (ftp == K)
    p->king_sq[side] = tsq;
  if (ttp != NO_TP) {
    p->key ^= zob_piece[Pc(Opp(side), ttp)][tsq];
    p->cl_bb[Opp(side)] ^= SqBb(tsq);
    p->tp_bb[ttp] ^= SqBb(tsq);
    p->mat[Opp(side)] -= tp_value[ttp];
    p->pst[Opp(side)] -= pst[ttp][tsq];
  }
  switch (MoveType(move)) {
  case NORMAL:
    break;
  case CASTLE:
    if (tsq > fsq) {
      fsq += 3;
      tsq -= 1;
    } else {
      fsq -= 4;
      tsq += 1;
    }
    p->pc[fsq] = NO_PC;
    p->pc[tsq] = Pc(side, R);
    p->key ^= zob_piece[Pc(side, R)][fsq] ^ zob_piece[Pc(side, R)][tsq];
    p->cl_bb[side] ^= SqBb(fsq) | SqBb(tsq);
    p->tp_bb[R] ^= SqBb(fsq) | SqBb(tsq);
    p->pst[side] += pst[R][tsq] - pst[R][fsq];
    break;
  case EP_CAP:
    tsq ^= 8;
    p->pc[tsq] = NO_PC;
    p->key ^= zob_piece[Pc(Opp(side), P)][tsq];
    p->cl_bb[Opp(side)] ^= SqBb(tsq);
    p->tp_bb[P] ^= SqBb(tsq);
    p->mat[Opp(side)] -= tp_value[P];
    p->pst[Opp(side)] -= pst[P][tsq];
    break;
  case EP_SET:
    tsq ^= 8;
    if (p_attacks[side][tsq] & PcBb(p, Opp(side), P)) {
      p->ep_sq = tsq;
      p->key ^= zob_ep[File(tsq)];
    }
    break;
  case N_PROM: case B_PROM: case R_PROM: case Q_PROM:
    ftp = PromType(move);
    p->pc[tsq] = Pc(side, ftp);
    p->key ^= zob_piece[Pc(side, P)][tsq] ^ zob_piece[Pc(side, ftp)][tsq];
    p->tp_bb[P] ^= SqBb(tsq);
    p->tp_bb[ftp] ^= SqBb(tsq);
    p->mat[side] += tp_value[ftp] - tp_value[P];
    p->pst[side] += pst[ftp][tsq] - pst[P][tsq];
    break;
  }
  p->side ^= 1;
  p->key ^= SIDE_RANDOM;
}

void DoNull(POS *p, UNDO *u)
{
  u->ep_sq = p->ep_sq;
  u->key = p->key;
  p->rep_list[p->head++] = p->key;
  p->rev_moves++;
  if (p->ep_sq != NO_SQ) {
    p->key ^= zob_ep[File(p->ep_sq)];
    p->ep_sq = NO_SQ;
  }
  p->side ^= 1;
  p->key ^= SIDE_RANDOM;
}

void UndoMove(POS *p, int move, UNDO *u)
{
  int side, fsq, tsq, ftp, ttp;

  side = Opp(p->side);
  fsq = Fsq(move);
  tsq = Tsq(move);
  ftp = TpOnSq(p, tsq);
  ttp = u->ttp;
  p->c_flags = u->c_flags;
  p->ep_sq = u->ep_sq;
  p->rev_moves = u->rev_moves;
  p->key = u->key;
  p->head--;
  p->pc[fsq] = Pc(side, ftp);
  p->pc[tsq] = NO_PC;
  p->cl_bb[side] ^= SqBb(fsq) | SqBb(tsq);
  p->tp_bb[ftp] ^= SqBb(fsq) | SqBb(tsq);
  p->pst[side] += pst[ftp][fsq] - pst[ftp][tsq];
  if (ftp == K)
    p->king_sq[side] = fsq;
  if (ttp != NO_TP) {
    p->pc[tsq] = Pc(Opp(side), ttp);
    p->cl_bb[Opp(side)] ^= SqBb(tsq);
    p->tp_bb[ttp] ^= SqBb(tsq);
    p->mat[Opp(side)] += tp_value[ttp];
    p->pst[Opp(side)] += pst[ttp][tsq];
  }
  switch (MoveType(move)) {
  case NORMAL:
    break;
  case CASTLE:
    if (tsq > fsq) {
      fsq += 3;
      tsq -= 1;
    } else {
      fsq -= 4;
      tsq += 1;
    }
    p->pc[tsq] = NO_PC;
    p->pc[fsq] = Pc(side, R);
    p->cl_bb[side] ^= SqBb(fsq) | SqBb(tsq);
    p->tp_bb[R] ^= SqBb(fsq) | SqBb(tsq);
    p->pst[side] += pst[R][fsq] - pst[R][tsq];
    break;
  case EP_CAP:
    tsq ^= 8;
    p->pc[tsq] = Pc(Opp(side), P);
    p->cl_bb[Opp(side)] ^= SqBb(tsq);
    p->tp_bb[P] ^= SqBb(tsq);
    p->mat[Opp(side)] += tp_value[P];
    p->pst[Opp(side)] += pst[P][tsq];
    break;
  case EP_SET:
    break;
  case N_PROM: case B_PROM: case R_PROM: case Q_PROM:
    p->pc[fsq] = Pc(side, P);
    p->tp_bb[P] ^= SqBb(fsq);
    p->tp_bb[ftp] ^= SqBb(fsq);
    p->mat[side] += tp_value[P] - tp_value[ftp];
    p->pst[side] += pst[P][fsq] - pst[ftp][fsq];
    break;
  }
  p->side ^= 1;
}

void UndoNull(POS *p, UNDO *u)
{
  p->ep_sq = u->ep_sq;
  p->key = u->key;
  p->head--;
  p->rev_moves--;
  p->side ^= 1;
}

U64 Key(POS *p)
{
  int i;
  U64 key;

  key = 0;
  for (i = 0; i < 64; i++)
    if (p->pc[i] != NO_PC)
      key ^= zob_piece[p->pc[i]][i];
  key ^= zob_castle[p->c_flags];
  if (p->ep_sq != NO_SQ)
    key ^= zob_ep[File(p->ep_sq)];
  if (p->side == BC)
    key ^= SIDE_RANDOM;
  return key;
}

void SetPosition(POS *p, char *epd)
{
  int i, j, pc;
  static const char pc_char[13] = "PpNnBbRrQqKk";

  for (i = 0; i < 2; i++) {
    p->cl_bb[i] = 0;
    p->mat[i] = 0;
    p->pst[i] = 0;
  }
  for (i = 0; i < 6; i++)
    p->tp_bb[i] = 0;
  p->c_flags = 0;
  p->rev_moves = 0;
  p->head = 0;
  for (i = 56; i >= 0; i -= 8) {
    j = 0;
    while (j < 8) {
      if (*epd >= '1' && *epd <= '8')
        for (pc = 0; pc < *epd - '0'; pc++) {
          p->pc[i + j] = NO_PC;
          j++;
        }
      else {
        for (pc = 0; pc_char[pc] != *epd; pc++)
          ;
        p->pc[i + j] = pc;
        p->cl_bb[Cl(pc)] ^= SqBb(i + j);
        p->tp_bb[Tp(pc)] ^= SqBb(i + j);
        if (Tp(pc) == K)
          p->king_sq[Cl(pc)] = i + j;
        p->mat[Cl(pc)] += tp_value[Tp(pc)];
        p->pst[Cl(pc)] += pst[Tp(pc)][i + j];
        j++;
      }
      epd++;
    }
    epd++;
  }
  if (*epd++ == 'w')
    p->side = WC;
  else
    p->side = BC;
  epd++;
  if (*epd == '-')
    epd++;
  else {
    if (*epd == 'K') {
      p->c_flags |= 1;
      epd++;
    }
    if (*epd == 'Q') {
      p->c_flags |= 2;
      epd++;
    }
    if (*epd == 'k') {
      p->c_flags |= 4;
      epd++;
    }
    if (*epd == 'q') {
      p->c_flags |= 8;
      epd++;
    }
  }
  epd++;
  if (*epd == '-')
    p->ep_sq = NO_SQ;
  else {
    p->ep_sq = Sq(*epd - 'a', *(epd + 1) - '1');
    if (!(p_attacks[Opp(p->side)][p->ep_sq] & PcBb(p, p->side, P)))
      p->ep_sq = NO_SQ;
  }
  p->key = Key(p);
}

bool Legal(POS *p, int move)
{
  int side, fsq, tsq, ftp, ttp;

  side = p->side;
  fsq = Fsq(move);
  tsq = Tsq(move);
  ftp = TpOnSq(p, fsq);
  ttp = TpOnSq(p, tsq);
  if (ftp == NO_TP || Cl(p->pc[fsq]) != side)
    return 0;
  if (ttp != NO_TP && Cl(p->pc[tsq]) == side)
    return 0;
  switch (MoveType(move)) {
  case NORMAL:
    break;
  case CASTLE:
    if (side == WC) {
      if (fsq != E1)
        return 0;
      if (tsq > fsq) {
        if ((p->c_flags & 1) && !(OccBb(p) & (U64)0x0000000000000060))
          if (!Attacked(p, E1, BC) && !Attacked(p, F1, BC))
            return 1;
      } else {
        if ((p->c_flags & 2) && !(OccBb(p) & (U64)0x000000000000000E))
          if (!Attacked(p, E1, BC) && !Attacked(p, D1, BC))
            return 1;
      }
    } else {
      if (fsq != E8)
        return 0;
      if (tsq > fsq) {
        if ((p->c_flags & 4) && !(OccBb(p) & (U64)0x6000000000000000))
          if (!Attacked(p, E8, WC) && !Attacked(p, F8, WC))
            return 1;
      } else {
        if ((p->c_flags & 8) && !(OccBb(p) & (U64)0x0E00000000000000))
          if (!Attacked(p, E8, WC) && !Attacked(p, D8, WC))
            return 1;
      }
    }
    return 0;
  case EP_CAP:
    if (ftp == P && tsq == p->ep_sq)
      return 1;
    return 0;
  case EP_SET:
    if (ftp == P && ttp == NO_TP && p->pc[tsq ^ 8] == NO_PC)
      if ((tsq > fsq && side == WC) ||
          (tsq < fsq && side == BC))
        return 1;
    return 0;
  }
  if (ftp == P) {
    if (side == WC) {
      if (Rank(fsq) == RANK_7 && !IsProm(move))
        return 0;
      if (tsq - fsq == 8)
        if (ttp == NO_TP)
          return 1;
      if ((tsq - fsq == 7 && File(fsq) != FILE_A) ||
          (tsq - fsq == 9 && File(fsq) != FILE_H))
        if (ttp != NO_TP)
          return 1;
    } else {
      if (Rank(fsq) == RANK_2 && !IsProm(move))
        return 0;
      if (tsq - fsq == -8)
        if (ttp == NO_TP)
          return 1;
      if ((tsq - fsq == -9 && File(fsq) != FILE_A) ||
          (tsq - fsq == -7 && File(fsq) != FILE_H))
        if (ttp != NO_TP)
          return 1;
    }
    return 0;
  }
  if (IsProm(move))
    return 0;
  return (AttacksFrom(p, fsq) & SqBb(tsq)) != 0;
}

int Swap(POS *p, int from, int to)
{
  int side, ply, type, score[32];
  U64 attackers, occ, type_bb;

  attackers = AttacksTo(p, to);
  occ = OccBb(p);
  score[0] = tp_value[TpOnSq(p, to)];
  type = TpOnSq(p, from);
  occ ^= SqBb(from);
  attackers |= (BAttacks(occ, to) & (p->tp_bb[B] | p->tp_bb[Q])) |
               (RAttacks(occ, to) & (p->tp_bb[R] | p->tp_bb[Q]));
  attackers &= occ;
  side = Opp(p->side);
  ply = 1;
  while (attackers & p->cl_bb[side]) {
    if (type == K) {
      score[ply++] = INF;
      break;
    }
    score[ply] = -score[ply - 1] + tp_value[type];
    for (type = P; type <= K; type++)
      if ((type_bb = PcBb(p, side, type) & attackers))
        break;
    occ ^= type_bb & -type_bb;
    attackers |= (BAttacks(occ, to) & (p->tp_bb[B] | p->tp_bb[Q])) |
                 (RAttacks(occ, to) & (p->tp_bb[R] | p->tp_bb[Q]));
    attackers &= occ;
    side ^= 1;
    ply++;
  }
  while (--ply)
    score[ply - 1] = -Max(-score[ply - 1], score[ply]);
  return score[0];
}
