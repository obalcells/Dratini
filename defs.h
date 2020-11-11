#pragma once
#include <vector>
#include <iostream>

#define ll long long
#define endl '\n'

#define EMPTY 6
#define WHITE 0
#define BLACK 1

#define PAWN 0
#define KNIGHT 1
#define BISHOP 2
#define ROOK 3
#define QUEEN 4
#define KING 5

#define RESET_COLOR "\033[0m"
#define EMPTY_COLOR "\033[37m"
#define BLACK_COLOR "\033[36m"
#define WHITE_COLOR "\033[37m"

#define ENPASSANT_INDEX 6
#define CASTLING_INDEX 7

#define row(x) (x >> 3)
#define col(x) (x & 7)
#define max(x, y) (x > y ? x : y)
#define min(x, y) (x < y ? x : y)
#define valid_pos(x) (x >= 0 && x < 64)

inline int abs(int x) { 
  if(x < 0)
    return x * (-1);
  return x;
}

inline int distance(char pos_1, char pos_2) {
  return abs(row(pos_1) - row(pos_2)) + abs(col(pos_1) - col(pos_2));
}

inline bool valid_distance(char pos_1, char pos_2) {
  return valid_pos(pos_1) && valid_pos(pos_2) && distance(pos_1, pos_2) <= 3; 
}	

struct Move {
    char from;
    char to;
    char captured;
    char castling;
    char enpassant;
    bool promotion;
    Move() {
      from = 64; to = 64; captured = EMPTY;
      castling = 0; enpassant = 0; promotion = false;
    }
    Move(char _from, char _to) {
      from = _from; to = _to; captured = EMPTY;
      castling = 0; enpassant = 0; promotion = false;
    }
    Move(char _from, char _to, char _captured, char _castling, char _enpassant) {
      from = _from; to = _to; captured = _captured;
      castling = _castling; enpassant = _enpassant; promotion = false;
    }
    Move(char _from, char _to, char _captured, char _castling, char _enpassant, bool _promotion) {
      from = _from; to = _to; captured = _captured;
      castling = _castling; enpassant = _enpassant; promotion = _promotion;
    }
    bool operator <(const Move & b) const {
      return false;
    }
};

struct PV_Entry {
  long long state_key;
  int alpha;
  Move move;
  PV_Entry() {
    state_key = 0;
    alpha = 0;
    move = Move();
  }
  PV_Entry(long long _state_key, int _alpha, Move _move) {
    state_key = _state_key;
    alpha = 0;
    move = _move;
  }
};