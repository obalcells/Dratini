#include <vector>
#include "defs.h"
#include "protos.h"
#include "board.h"
#include "data.h"

// Things we should evaluate
// 1 Pawn 
// 1.1 Position (how close to promotion)
// 1.2 Double pawn in same column
// 1.3 How isolated is it
// 1.4 Does it have any enemy pawn in its way
// 2 King
// 2.1 Is it being attacked -> Safety
// 2.2 Are its sourrounding squares being attacked -> Safety
// 2.3 Position (safe in early/mid-game, close to middle in late-game)
// 3 Rook, Bishop and Queen
// 3.1 Position
// 3.2 How cleared its attack lanes are
// 4 Knight
// 4.1 Position

#define PENALTY_ISOLATED_PAWN -10
#define PENALTY_FRONTAL_ENEMY_PAWN  -10
#define PENALTY_DOUBLE_PAWN -40
#define REWARD_DISTANCE_TO_PROMOTION_PAWN 5 
#define PENALTY_DISTANCE_FROM_START_PAWN -2 
#define PENALTY_EMPTY_FILE_KING -50 // probably should be different for endgame
#define PENALTY_EMPTY_FILE_ENDGAME_KING -10 // yep
#define PENALTY_DISTANCE_TO_PAWN_KING -2
#define PENALTY_FRONTAL_PAWN_KING -10 
#define PENALTY_FRIENDLY_BLOCKING_PIECE 0 
#define REWARD_EMPTY_SQUARE_NON_SLIDING 10 // maybe it should be 0 and friendly +10?
#define REWARD_ENEMY_SQUARE_NON_SLIDING 40 
#define REWARD_EMPTY_SQUARE_SLIDING 2
#define REWARD_ENEMY_SQUARE_SLIDING 5

int piece_value[6] = {
  100, 300, 300, 500, 900
};

int pawn_pcsq[64] = {
   0,   0,   0,   0,   0,   0,   0,   0,  
   5,  10,  15,  20,  20,  15,  10,   5, 
   4,   8,  12,  16,  16,  12,   8,   4,
   3,   6,   9,  12,  12,   9,   6,   3, 
   2,   4,   6,  10,  10,   6,   4,   2, 
   1,   2,   3,   3,   3,   3,   2,   1, 
  10,  10,   6,   6,   6,   6,  10,  10, 
   0,   0,   0,   0,   0,   0,   0,   0,
};

int knight_pcsq[64] = {
	-10, -10, -10, -10, -10, -10, -10, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-10, -30, -10, -10, -10, -10, -30, -10
};

int bishop_pcsq[64] = {
	-10, -10, -10, -10, -10, -10, -10, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-10, -10, -20, -10, -10, -20, -10, -10
};

int king_pcsq[64] = {
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-20, -20, -20, -20, -20, -20, -20, -20,
	  0,  20,  40, -20,   0, -20,  40,  20
};

int king_endgame_pcsq[64] = {
	  0,  10,  20,  30,  30,  20,  10,   0,
	 10,  20,  30,  40,  40,  30,  20,  10,
	 20,  30,  40,  50,  50,  40,  30,  20,
	 30,  40,  50,  60,  60,  50,  40,  30,
	 30,  40,  50,  60,  60,  50,  40,  30,
	 20,  30,  40,  50,  50,  40,  30,  20,
	 10,  20,  30,  40,  40,  30,  20,  10,
	  0,  10,  20,  30,  30,  20,  10,   0
};

int flip(int pos) {
  return (7 - row(pos)) * 8 + col(pos);
}

int pawn_ranks[2][8] = {
  { 0, 0, 0, 0, 0, 0, 0, 0 },  
  { 0, 0, 0, 0, 0, 0, 0, 0 } 
};

// returns an integer representing
// how favorable the current board state is for WHITE
int eval() {

  for(int i = 0; i < 2; i++) 
    for(int j = 0; j < 8; j++)
      pawn_ranks[i][j] = 0;

  bool late_game = false;
  int white_score = 0, black_score = 0; 

  // first pass: evaluate phase
  int material[2] = { 0, 0 };

  for(int pos = 0; pos < 64; pos++) if(piece[pos] != EMPTY) { 
    if(piece[pos] == PAWN) {
      if(color[pos] == WHITE && pawn_ranks[WHITE][col(pos)] == 0) pawn_ranks[WHITE][col(pos)] = row(pos);
      else if(color[pos] == BLACK) pawn_ranks[BLACK][col(pos)] = row(pos);
    }
    material[color[pos]] += piece_value[piece[pos]];
  }
  
  if(material[WHITE] + material[BLACK] <= 4000) {
    late_game = true;
  } 

  // second pass: evaluate each piece
  for(int pos = 0; pos < 64; pos++) if(piece[pos] != EMPTY) { 
    switch(piece[pos]) {
      case PAWN:
        if(color[pos] == WHITE) {
          material[WHITE] += eval_white_pawn(pos, late_game);
          material[WHITE] += pawn_pcsq[pos]; 
        } else {
          material[BLACK] += eval_black_pawn(pos, late_game);
          material[BLACK] += pawn_pcsq[flip(pos)];
        }
        break;
      case KING:
        material[color[pos]] += eval_king(pos, late_game);
        if(late_game) {
          if(color[pos] == WHITE) material[WHITE] += king_endgame_pcsq[pos];
          else material[BLACK] += king_endgame_pcsq[flip(pos)];
        } else {
          if(color[pos] == WHITE) material[WHITE] += king_pcsq[pos];
          else material[BLACK] += king_pcsq[flip(pos)];
        }
        break;
      case QUEEN:
        material[color[pos]] += 2 * eval_sorroundings(pos, late_game);
        break;
      case ROOK:
        material[color[pos]] += eval_sorroundings(pos, late_game);
        break;
      case BISHOP:
        material[color[pos]] += eval_sorroundings(pos, late_game);
        if(color[pos] == WHITE) material[WHITE] = bishop_pcsq[pos];
        else material[BLACK] = bishop_pcsq[flip(pos)];
        break;
      case KNIGHT:
        if(color[pos] == WHITE) material[color[pos]] += knight_pcsq[pos];
        else material[BLACK] += knight_pcsq[flip(pos)];
    }
  }   

  return material[WHITE] - material[BLACK];
}

int eval_white_pawn(int pos, bool late_game = false) {
  // Pawn base value is 100
  int score = 0;

  if(late_game) score += row(pos) * REWARD_DISTANCE_TO_PROMOTION_PAWN; 
  else score += (7 - row(pos)) * PENALTY_DISTANCE_FROM_START_PAWN; 

  int isolated = 2; 
  if(col(pos) > 0 && abs(pawn_ranks[WHITE][col(pos - 1)] - row(pos)) <= 1) isolated--;
  if(col(pos) < 7 && abs(pawn_ranks[WHITE][col(pos + 1)] - row(pos)) <= 1) isolated--;
  score += isolated * PENALTY_ISOLATED_PAWN; 

  if(pawn_ranks[WHITE][col(pos)] < row(pos)) score += PENALTY_DOUBLE_PAWN;

  int frontal_enemy = 0;
  if(col(pos) > 0 && pawn_ranks[BLACK][col(pos - 1)] > row(pos)) frontal_enemy++;
  if(pawn_ranks[BLACK][col(pos)] > row(pos)) frontal_enemy++;
  if(col(pos) < 7 && pawn_ranks[BLACK][col(pos + 1)] > row(pos)) frontal_enemy++;
  score += frontal_enemy * PENALTY_FRONTAL_ENEMY_PAWN; 

  return score;
}

int eval_black_pawn(int pos, bool late_game = false) {
  int score = 0;

  if(late_game) score += (7 - row(pos)) * REWARD_DISTANCE_TO_PROMOTION_PAWN; 
  else score += row(pos) * PENALTY_DISTANCE_FROM_START_PAWN; 

  int isolated = 2; 
  if(col(pos) > 0 && abs(pawn_ranks[BLACK][col(pos - 1)] - row(pos)) <= 1) isolated--;
  if(col(pos) < 7 && abs(pawn_ranks[BLACK][col(pos + 1)] - row(pos)) <= 1) isolated--;
  score += isolated * PENALTY_ISOLATED_PAWN; 

  if(pawn_ranks[BLACK][col(pos)] > row(pos)) score += PENALTY_DOUBLE_PAWN;

  int frontal_enemy = 0;
  if(col(pos) > 0 && pawn_ranks[WHITE][col(pos - 1)] < row(pos)) frontal_enemy++;
  if(pawn_ranks[WHITE][col(pos)] < row(pos)) frontal_enemy++;
  if(col(pos) < 7 && pawn_ranks[WHITE][col(pos + 1)] < row(pos)) frontal_enemy++;
  score += frontal_enemy * PENALTY_FRONTAL_ENEMY_PAWN;

  return score;
}

int eval_king(int pos, bool late_game = false) {
  auto eval_white_col = [&](int col) {
    int score = 0; 
    if(pawn_ranks[WHITE][col] == 0 || pawn_ranks[WHITE][col] < row(pos) && !late_game) score += PENALTY_EMPTY_FILE_KING;
    else if(pawn_ranks[WHITE][col] == 0 || pawn_ranks[WHITE][col] < row(pos) && late_game) score += PENALTY_EMPTY_FILE_ENDGAME_KING;
    else score += (pawn_ranks[WHITE][col] - (row(pos) + 1)) * PENALTY_DISTANCE_TO_PAWN_KING; // the less distance, the better
    if(pawn_ranks[BLACK][col] > row(pos)) score += PENALTY_FRONTAL_PAWN_KING; 
    return score;
  };

  auto eval_black_col = [&](int col) {
    int score = 0; 
    if(pawn_ranks[BLACK][col] == 0 || pawn_ranks[BLACK][col] > row(pos) && !late_game) score += PENALTY_EMPTY_FILE_KING;
    else if(pawn_ranks[BLACK][col] == 0 || pawn_ranks[BLACK][col] > row(pos) && late_game) score += PENALTY_EMPTY_FILE_ENDGAME_KING; 
    else score += (row(pos) - (pawn_ranks[BLACK][col] + 1)) * PENALTY_DISTANCE_TO_PAWN_KING;
    if(pawn_ranks[WHITE][col] < row(pos)) score += PENALTY_FRONTAL_PAWN_KING; 
    return score;
  };

  int score = 0;
  for(int c = max(0, col(pos) - 2); c < min(7, col(pos) + 2); c++) {
    // i guess that open files are much worse at the beginning
    if(color[pos] == WHITE) score += (late_game ? eval_white_col(c) : 2 * eval_white_col(c)); 
    else score += (late_game ? eval_black_col(c) : 2 * eval_black_col(c)); 
  } 
  return score;
}

/* The more squares a piece can visit, the better */
int eval_sorroundings(int pos, bool late_game = false) {
  int score = 0;

  for(int i = 0; i < 8; i++) {
    if(offset[piece[pos]][i] == 0) break; 

    if(!slide[piece[pos]] && valid_distance(pos, pos + offset[piece[pos]][i])) {
      // Must be a knight hehe
      if(color[pos + offset[piece[pos]][i]] == color[pos]); // neutral?
      else if(color[pos + offset[piece[pos]][i]] == EMPTY) score += REWARD_EMPTY_SQUARE_NON_SLIDING;
      else score += REWARD_ENEMY_SQUARE_NON_SLIDING;

    } else if(slide[piece[pos]]) {
      int new_pos = pos;

      while(valid_distance(new_pos, new_pos + offset[piece[pos]][i])) {
        new_pos += offset[piece[pos]][i];
        if(color[new_pos] == color[pos]) break; 
        if(color[new_pos] == EMPTY) score += REWARD_EMPTY_SQUARE_SLIDING;
        else {
          // attacking square with an enemy piece
          score += REWARD_ENEMY_SQUARE_SLIDING;
          // maybe do something if the piece is unprotected?
          break;
        } 
      } 
    }
  }
  return score;
}

