#define U64 uint64_t

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
