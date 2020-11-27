#pragma once

/*
#include <vector>
#include "defs.h"
#include "data.h"
#include "board.h"

struct State {
  std::vector<int> _color, _piece; 
  int _side, _xside, _castling, _enpassant;
  State() { 
    _color.resize(64);
    _piece.resize(64);
    _side = side;
    _xside = xside;
    _castling = castling;
    _enpassant = enpassant;
    for(int i = 0; i < 64; i++) {
      _color[i] = color[i];
      _piece[i] = piece[i];
    }
  }
  void set() {
    side = _side;
    xside = _xside;
    castling = _castling;
    enpassant = _enpassant;
    for(int i = 0; i < 64; i++) {
      color[i] = _color[i];
      piece[i] = _piece[i];
    }
  }
  void print() {
    State state_now = State();
    this->set(); // we temporarily set the state we want to print
    print_board();
    state_now.set(); // reverting to the actual state
  }
  bool same(const State & other) const {
    if(_side != other._side) return false;
    if(_xside != other._xside) return false;
    if(_castling != other._castling) return false; 
    if(_enpassant != other._enpassant) return false;
    for(int i = 0; i < 64; i++) {
      if(_color[i] != other._color[i]) return false;
      if(_piece[i] != other._piece[i]) return false;
    }
    return true;
  }
};
*/