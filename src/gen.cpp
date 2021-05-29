#include <vector>
#include <iostream>
#include <cassert>
#include "defs.h"
#include "magicmoves.h"
#include "bitboard.h"
#include "board.h"

uint64_t get_attackers(int, bool, const Board*);
uint64_t get_blockers(int, bool, const Board*);
uint64_t get_between(int, int, const Board*);
void generate_evasions(std::vector<Move>&, const Board*);
void generate_captures(std::vector<Move>&, const Board*);
void generate_quiet(std::vector<Move>&, const Board*);
int* new_generate_captures(int* moves, const Board*);
int* new_generate_quiet(int* moves, const Board*);

#define get_side_mask(_side) (_side == WHITE ? \
	(board->bits[WHITE_PAWN] | board->bits[WHITE_KNIGHT] | board->bits[WHITE_BISHOP] | board->bits[WHITE_ROOK] | board->bits[WHITE_QUEEN] | board->bits[WHITE_KING]) : \
	(board->bits[BLACK_PAWN] | board->bits[BLACK_KNIGHT] | board->bits[BLACK_BISHOP] | board->bits[BLACK_ROOK] | board->bits[BLACK_QUEEN] | board->bits[BLACK_KING]))
#define get_piece_mask(piece) board->bits[piece]
#define get_all_mask() board->occ_mask
#define get_pawn_mask(_side) (_side == WHITE ? board->bits[WHITE_PAWN] : board->bits[BLACK_PAWN])
#define get_knight_mask(_side) (_side == WHITE ? board->bits[WHITE_KNIGHT] : board->bits[BLACK_KNIGHT])
#define get_bishop_mask(_side) (_side == WHITE ? board->bits[WHITE_BISHOP] : board->bits[BLACK_BISHOP])
#define get_rook_mask(_side) (_side == WHITE ? board->bits[WHITE_ROOK] : board->bits[BLACK_ROOK])
#define get_queen_mask(_side) (_side == WHITE ? board->bits[WHITE_QUEEN] : board->bits[BLACK_QUEEN])
#define get_king_mask(_side) (_side == WHITE ? board->bits[WHITE_KING] : board->bits[BLACK_KING])
#define get_piece(sq) (board->piece_at[sq] + (board->color_at[sq] == BLACK ? 6 : 0))
#define get_color(sq) (board->color_at[sq])
#define in_check() bool(board->king_attackers)

void generate_moves(std::vector<Move>& moves, const Board* board, bool quiesce) {
    std::vector<Move> tmp_moves;
    if(board->king_attackers) {
        generate_evasions(tmp_moves, board);
    } else {
        generate_captures(tmp_moves, board);
        if(!quiesce) {
            generate_quiet(tmp_moves, board);
        }
    }
    for(int i = 0; i < (int)tmp_moves.size(); i++) {
        if(board->fast_move_valid(tmp_moves[i])) {
            moves.push_back(tmp_moves[i]);
        }
    }
}

// returns a bitboard containing all the pieces which are attacking sq 
uint64_t get_blockers(int sq, bool attacker_side, const Board* board) {
    uint64_t blockers = 0;
    
    if(get_piece(sq) != EMPTY) {
        if(attacker_side == BLACK) {
            if((mask_sq(sq) & ~COL_0) && get_piece(sq + 7) == BLACK_PAWN)
                blockers |= mask_sq(sq + 7); 
            if((mask_sq(sq) & ~COL_7) && get_piece(sq + 9) == BLACK_PAWN)
                blockers |= mask_sq(sq + 9);
        } else {
            if((mask_sq(sq) & ~COL_0) && get_piece(sq - 9) == WHITE_PAWN)
                blockers |= mask_sq(sq - 9);
            if((mask_sq(sq) & ~COL_7) && get_piece(sq - 7) == WHITE_PAWN) 
                blockers |= mask_sq(sq - 7);
        }
    } else {
        if(attacker_side == WHITE) {
            if(row(sq) > 0 && get_piece(sq - 8) == WHITE_PAWN)
                blockers |= mask_sq(sq - 8);
            else if(row(sq) == 3 && get_piece(sq - 8) == EMPTY && get_piece(sq - 16) == WHITE_PAWN)
                blockers |= mask_sq(sq - 16);
        } else {
            if(row(sq) < 7 && get_piece(sq + 8) == BLACK_PAWN)
                blockers |= mask_sq(sq + 8);
            else if(row(sq) == 4 && get_piece(sq + 8) == EMPTY && get_piece(sq + 16) == BLACK_PAWN)
                blockers |= mask_sq(sq + 16);
        }
    }

    blockers |= (Rmagic(sq, get_all_mask()) & (get_rook_mask(attacker_side) | get_queen_mask(attacker_side)))
             |  (Bmagic(sq, get_all_mask()) & (get_bishop_mask(attacker_side) | get_queen_mask(attacker_side)))
             |  (knight_attacks[sq] & get_knight_mask(attacker_side));

    return blockers;
}

// returns a bitboard containing all the pieces from a certain side which are attacking sq */
uint64_t get_attackers(int sq, bool attacker_side, const Board* board) {
    uint64_t attackers = 0;

    if(attacker_side == BLACK) {
        if((mask_sq(sq) & ~COL_0) && sq + 7 < 64 && get_piece(sq + 7) == BLACK_PAWN)
            attackers |= mask_sq(sq + 7); 
        if((mask_sq(sq) & ~COL_7) && sq + 9 < 64 && get_piece(sq + 9) == BLACK_PAWN)
            attackers |= mask_sq(sq + 9);
    } else {
        if((mask_sq(sq) & ~COL_0) && sq - 9 >= 0 && get_piece(sq - 9) == WHITE_PAWN)
            attackers |= mask_sq(sq - 9);
        if((mask_sq(sq) & ~COL_7) && sq - 7 >= 0 && get_piece(sq - 7) == WHITE_PAWN) 
            attackers |= mask_sq(sq - 7);
    }

    return      attackers 
              | (Rmagic(sq, board->occ_mask) & (get_rook_mask(attacker_side) | get_queen_mask(attacker_side)))
              | (Bmagic(sq, board->occ_mask) & (get_bishop_mask(attacker_side) | get_queen_mask(attacker_side)))
              | (Bmagic(sq, get_all_mask()) & (get_bishop_mask(attacker_side) | get_queen_mask(attacker_side)))
              | (knight_attacks[sq] & get_knight_mask(attacker_side)); 

    // return attackers;
}

// returns a bitboard with the squares between from_sq and to_sq
// it takes into account the type of piece at from_sq
uint64_t get_between(int from_sq, int to_sq, const Board* board) {
    const int piece = get_piece(from_sq);

    if(row(from_sq) == row(to_sq) || col(from_sq) == col(to_sq)) {
        if(piece == WHITE_ROOK || piece == BLACK_ROOK || piece == WHITE_QUEEN || piece == BLACK_QUEEN) {
            return Rmagic(from_sq, board->occ_mask) & Rmagic(to_sq, board->occ_mask) & ~board->occ_mask;
        }       
    } else if(abs(row(from_sq) - row(to_sq)) == abs(col(from_sq) - col(to_sq))) {
        if(piece == WHITE_BISHOP || piece == BLACK_BISHOP || piece == WHITE_QUEEN || piece == BLACK_QUEEN) {
            return Bmagic(from_sq, board->occ_mask) & Bmagic(to_sq, board->occ_mask) & ~board->occ_mask;
        }
    }

    return 0;
}

// we assume that king is in check
void generate_evasions(std::vector<Move>& moves, const Board* board) {
    int from_sq, to_sq, king_pos = lsb(get_king_mask(board->side));
    assert(get_attackers(king_pos, board->xside, board) == board->king_attackers);

    if(popcnt(board->king_attackers) == 1) {
        // a piece (different than the checked king) will try to eat the attacker
        int attacker_pos = lsb(board->king_attackers);

        // special case: the attacker is a pawn and we eat it enpass 
        if(board->side == BLACK && (mask_sq(attacker_pos) & ROW_3) && board->enpassant == col(attacker_pos)) {
            if(col(attacker_pos) > 0 && get_piece(attacker_pos - 1) == BLACK_PAWN)
                moves.push_back(Move(attacker_pos - 1, attacker_pos - 8, ENPASSANT_MOVE));
            if(col(attacker_pos) < 7 && get_piece(attacker_pos + 1) == BLACK_PAWN) {
                moves.push_back(Move(attacker_pos + 1, attacker_pos - 8, ENPASSANT_MOVE));
            }
        } else if(board->side == WHITE && (mask_sq(attacker_pos) & ROW_4) && board->enpassant == col(attacker_pos)) {
            if(col(attacker_pos) > 0 && get_piece(attacker_pos - 1) == WHITE_PAWN)  
                moves.push_back(Move(attacker_pos - 1, attacker_pos + 8, ENPASSANT_MOVE));
            if(col(attacker_pos) < 7 && get_piece(attacker_pos + 1) == WHITE_PAWN)    
                moves.push_back(Move(attacker_pos + 1, attacker_pos + 8, ENPASSANT_MOVE));
        }

        uint64_t tmp_mask = get_attackers(attacker_pos, board->side, board), blockers;

        while(tmp_mask) {
            from_sq = pop_first_bit(tmp_mask);
            if(board->piece_at[from_sq] == PAWN && (mask_sq(attacker_pos) & (ROW_0 | ROW_7))) {
                moves.push_back(Move(from_sq, attacker_pos, QUEEN_PROMOTION));
                moves.push_back(Move(from_sq, attacker_pos, KNIGHT_PROMOTION));
            } else {
                moves.push_back(Move(from_sq, attacker_pos, CAPTURE_MOVE));
            }
        }

        // a piece (different than the checked king) will try to block the attack without eating anything
        tmp_mask = get_between(attacker_pos, king_pos, board);
        while(tmp_mask) {
            to_sq = pop_first_bit(tmp_mask);
            assert(get_piece(to_sq) == EMPTY);
            blockers = get_blockers(to_sq, board->side, board);

            // special case: we block a square by eating enpass
            if(board->enpassant == col(to_sq)) {
                if(board->side == WHITE && row(to_sq) == 5) {
                    if(col(to_sq) > 0 && get_piece(to_sq - 9) == WHITE_PAWN)
                        moves.push_back(Move(to_sq - 9, to_sq, ENPASSANT_MOVE));
                    if(col(to_sq) < 7 && get_piece(to_sq - 7) == WHITE_PAWN)
                        moves.push_back(Move(to_sq - 7, to_sq, ENPASSANT_MOVE));
                } else if(board->side == BLACK && row(to_sq) == 2) {
                    if(col(to_sq) > 0 && get_piece(to_sq + 7) == BLACK_PAWN)
                        moves.push_back(Move(to_sq + 7, to_sq, ENPASSANT_MOVE));
                    if(col(to_sq) < 7 && get_piece(to_sq + 9) == BLACK_PAWN)
                        moves.push_back(Move(to_sq + 9, to_sq, ENPASSANT_MOVE));
                }
            }

            while(blockers) {
                from_sq = pop_first_bit(blockers);
                assert(get_piece(to_sq) == EMPTY);
                if(board->piece_at[from_sq] == PAWN && (mask_sq(to_sq) & (ROW_0 | ROW_7))) {
                    moves.push_back(Move(from_sq, to_sq, QUEEN_PROMOTION));
                    moves.push_back(Move(from_sq, to_sq, KNIGHT_PROMOTION));
                } else {
                    moves.push_back(Move(from_sq, to_sq, QUIET_MOVE));
                }
            }
        }
    }

    // we just move the king, remember that we will check whether a move is valid later
    uint64_t attack_mask = king_attacks[king_pos] & ~get_side_mask(board->side);
    while(attack_mask) {
        to_sq = pop_first_bit(attack_mask);
        if(board->piece_at[to_sq] != EMPTY) {
            moves.push_back(Move(king_pos, to_sq, CAPTURE_MOVE)); 
        } else {
            moves.push_back(Move(king_pos, to_sq, QUIET_MOVE)); 
        }
    }
}

void generate_captures(std::vector<Move>& moves, const Board* board) {
    uint64_t mask, attack_mask, pawn_mask = get_pawn_mask(board->side), xside_mask = get_side_mask(board->xside);
    int from_sq, to_sq;

    // first we generate pawn moves 
    if(board->side == WHITE) {
        // enpassant capture
        if(board->enpassant != NO_ENPASSANT) {
            int enpassant_sq = 40 + int(board->enpassant);
            assert(get_piece(enpassant_sq - 8) == BLACK_PAWN);
            // eating to the left (sq -> sq + 7)
            if(board->enpassant < 7 && get_piece(enpassant_sq - 7) == WHITE_PAWN)
                moves.push_back(Move(enpassant_sq - 7, enpassant_sq, ENPASSANT_MOVE));
            // eating to the right (sq -> sq + 9)
            if(board->enpassant > 0 && get_piece(enpassant_sq - 9) == WHITE_PAWN)
                moves.push_back(Move(enpassant_sq - 9, enpassant_sq, ENPASSANT_MOVE));
        }

        // promotion eating diagonally to the left (sq -> sq + 7)
        mask = ((pawn_mask & ROW_6 & ~COL_0) << 7) & xside_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            // Move constructor:  'from'    'to'     'move type'    
            moves.push_back(Move(to_sq - 7, to_sq, QUEEN_PROMOTION));
            moves.push_back(Move(to_sq - 7, to_sq, KNIGHT_PROMOTION));
        }

        // promotion eating diagonally to the right (sq -> sq + 9)
        mask = ((pawn_mask & ROW_6 & ~COL_7) << 9) & xside_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq - 9, to_sq, QUEEN_PROMOTION));
            moves.push_back(Move(to_sq - 9, to_sq, KNIGHT_PROMOTION));
        }

        // promotion front (sq -> sq + 8)
        mask = ((pawn_mask & ROW_6) << 8) & ~board->occ_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq - 8, to_sq, QUEEN_PROMOTION));
            moves.push_back(Move(to_sq - 8, to_sq, KNIGHT_PROMOTION));
        }

        // pawn capture to the left (sq -> sq + 7)
        mask = ((pawn_mask & ~ROW_6 & ~COL_0) << 7) & xside_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq - 7, to_sq, CAPTURE_MOVE));
        }

        // pawn capture to the right (sq -> sq + 9)
        mask = ((pawn_mask & ~ROW_6 & ~COL_7) << 9) & xside_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq - 9, to_sq, CAPTURE_MOVE));
        }
    } else {
        // enpassant capture
        if(board->enpassant != NO_ENPASSANT) {
            int enpassant_sq = 16 + int(board->enpassant);
            assert(get_piece(enpassant_sq + 8) == WHITE_PAWN);
            // to the left (sq -> sq - 9)
            if(board->enpassant < 7 && get_piece(enpassant_sq + 9) == BLACK_PAWN)
                moves.push_back(Move(enpassant_sq + 9, enpassant_sq, ENPASSANT_MOVE));
            // to the right (sq -> sq - 7)
            if(board->enpassant > 0 && get_piece(enpassant_sq + 7) == BLACK_PAWN)
                moves.push_back(Move(enpassant_sq + 7, enpassant_sq, ENPASSANT_MOVE));
        }

        // promotion eating diagonally to the left (sq -> sq - 9)
        mask = ((pawn_mask & ROW_1 & ~COL_0) >> 9) & xside_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq + 9, to_sq, QUEEN_PROMOTION));
            moves.push_back(Move(to_sq + 9, to_sq, KNIGHT_PROMOTION));
        }

        // promotion eating diagonally to the right (sq -> sq - 7)
        mask = ((pawn_mask & ROW_1 & ~COL_7) >> 7) & xside_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq + 7, to_sq, QUEEN_PROMOTION));
            moves.push_back(Move(to_sq + 7, to_sq, KNIGHT_PROMOTION));
        }

        // promotion front (sq -> sq - 8)
        mask = ((pawn_mask & ROW_1) >> 8) & ~board->occ_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq + 8, to_sq, QUEEN_PROMOTION));
            moves.push_back(Move(to_sq + 8, to_sq, KNIGHT_PROMOTION));
        }

        // pawn capture to the left (sq -> sq - 9)
        mask = ((pawn_mask & ~ROW_1 & ~COL_0) >> 9) & xside_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            assert(Move(to_sq + 9, to_sq, CAPTURE_MOVE) != 8394);
            assert(!(to_sq + 9 == C2 && to_sq == D1));
            moves.push_back(Move(to_sq + 9, to_sq, CAPTURE_MOVE));
        }

        // pawn capture to the right (sq -> sq - 7)
        mask = ((pawn_mask & ~ROW_1 & ~COL_7) >> 7) & xside_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            assert(!(to_sq + 7 == C2 && to_sq == D1));
            assert(Move(to_sq + 7, to_sq, CAPTURE_MOVE) != 8394);
            moves.push_back(Move(to_sq + 7, to_sq, CAPTURE_MOVE));
        }
    }

    // king captures
    mask = get_king_mask(board->side);
    assert(mask != 0);
    from_sq = pop_first_bit(mask);
    attack_mask = king_attacks[from_sq] & xside_mask;
    while(attack_mask) {
        assert(attack_mask != 0);
        to_sq = pop_first_bit(attack_mask);
        assert(to_sq >= 0 && to_sq < 64);
        moves.push_back(Move(from_sq, to_sq, CAPTURE_MOVE));
    }

    assert(mask == 0);

    // knight captures
    mask = get_knight_mask(board->side);
    while(mask) {
        from_sq = pop_first_bit(mask);
        attack_mask = knight_attacks[from_sq] & xside_mask;
        while(attack_mask) {
            to_sq = pop_first_bit(attack_mask);
            moves.push_back(Move(from_sq, to_sq, CAPTURE_MOVE));
        }
    }

    // bishop and queen captures
    mask = get_bishop_mask(board->side) | get_queen_mask(board->side);
    while(mask) {
        from_sq = pop_first_bit(mask);
        attack_mask = Bmagic(from_sq, board->occ_mask) & xside_mask;
        while(attack_mask) {
            to_sq = pop_first_bit(attack_mask);
            moves.push_back(Move(from_sq, to_sq, CAPTURE_MOVE));
        }
    }

    // rook and queen captures
    mask = get_rook_mask(board->side) | get_queen_mask(board->side);
    while(mask) {
        from_sq = pop_first_bit(mask);
        attack_mask = Rmagic(from_sq, board->occ_mask) & xside_mask;
        while(attack_mask) {
            to_sq = pop_first_bit(attack_mask);
            moves.push_back(Move(from_sq, to_sq, CAPTURE_MOVE));
        }
    }
}

// we assume that the king isn't in check
void generate_quiet(std::vector<Move>& moves, const Board* board) {
    uint64_t mask, attack_mask, pawn_mask = get_pawn_mask(board->side);
    int from_sq, to_sq;

    if(board->side == WHITE) {
        // front only one square (no promotion) (sq -> sq + 8)
        mask = ((pawn_mask & (~ROW_6)) << 8) & (~board->occ_mask);
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq - 8, to_sq, QUIET_MOVE));
        }
        
        // frontal two squares (sq -> sq + 8)
        mask = ((pawn_mask & ROW_1) << 8) & (~board->occ_mask);
        mask = ((mask & ROW_2) << 8) & (~board->occ_mask);
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq - 16, to_sq, QUIET_MOVE));
        }
    } else {
        // front only one square (no promotion) (sq -> sq - 8)
        mask = ((pawn_mask & (~ROW_1)) >> 8) & (~board->occ_mask);
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq + 8, to_sq, QUIET_MOVE));
        }

        // frontal two squares (sq -> sq - 16)
        mask = ((pawn_mask & ROW_6) >> 8) & (~board->occ_mask);
        mask = ((mask & ROW_5) >> 8) & (~board->occ_mask);
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq + 16, to_sq, QUIET_MOVE));
        }
    }

    // generate the castling moves 
    if(board->side == WHITE) {
        // white queen side castling
        if((board->castling_flag & 1)
        && !(board->occ_mask & castling_mask[WHITE_QUEEN_SIDE])
        && !board->is_attacked(D1)) {
            assert(board->piece_at[E1] == KING);
            moves.push_back(Move(E1, C1, CASTLING_MOVE));
        }
        // white king side castling
        if((board->castling_flag & 2)
        && !(board->occ_mask & castling_mask[WHITE_KING_SIDE])
        && !board->is_attacked(F1)) {
            assert(board->piece_at[E1] == KING);
            moves.push_back(Move(E1, G1, CASTLING_MOVE));
        }
    } else if(board->side == BLACK) {
        // black queen side castling
        if((board->castling_flag & 4)
        && !(board->occ_mask & castling_mask[BLACK_QUEEN_SIDE])
        && !board->is_attacked(D8)) {
            assert(board->piece_at[E8] == KING);
            moves.push_back(Move(E8, C8, CASTLING_MOVE));
        }
        // black king side castling
        if((board->castling_flag & 8)
        && !(board->occ_mask & castling_mask[BLACK_KING_SIDE])
        && !board->is_attacked(F8)) {
            assert(board->piece_at[E8] == KING);
            moves.push_back(Move(E8, G8, CASTLING_MOVE));
        }
    }

    // king
    mask = get_king_mask(board->side);
    from_sq = pop_first_bit(mask);  
    attack_mask = king_attacks[from_sq] & (~board->occ_mask);
    while(attack_mask) {
        to_sq = pop_first_bit(attack_mask);
        moves.push_back(Move(from_sq, to_sq, QUIET_MOVE));
    }

    // knight
    mask = get_knight_mask(board->side);
    while(mask) {
        from_sq = pop_first_bit(mask);
        attack_mask = knight_attacks[from_sq] & (~board->occ_mask);
        while(attack_mask) {
            to_sq = pop_first_bit(attack_mask);
            moves.push_back(Move(from_sq, to_sq, QUIET_MOVE));
        }
    }

    // bishop and queen
    mask = get_bishop_mask(board->side) | get_queen_mask(board->side);
    while(mask) {
        from_sq = pop_first_bit(mask);
        attack_mask = Bmagic(from_sq, board->occ_mask) & (~board->occ_mask);
        while(attack_mask) {
            to_sq = pop_first_bit(attack_mask);
            moves.push_back(Move(from_sq, to_sq, QUIET_MOVE));
        }
    }

    // rooks and queen
    mask = get_rook_mask(board->side) | get_queen_mask(board->side);
    while(mask) {
        from_sq = pop_first_bit(mask);
        attack_mask = Rmagic(from_sq, board->occ_mask) & (~board->occ_mask);
        while(attack_mask) {
            to_sq = pop_first_bit(attack_mask);
            moves.push_back(Move(from_sq, to_sq, QUIET_MOVE));
        }
    }
}

int* new_generate_captures(int* moves, const Board* board) {
    uint64_t mask, attack_mask, pawn_mask = get_pawn_mask(board->side), xside_mask = get_side_mask(board->xside);
    int from_sq, to_sq;

    // first we generate pawn moves 
    if(board->side == WHITE) {
        // enpassant capture
        if(board->enpassant != NO_ENPASSANT) {
            int enpassant_sq = 40 + int(board->enpassant);
            assert(get_piece(enpassant_sq - 8) == BLACK_PAWN);
            // eating to the left (sq -> sq + 7)
            if(board->enpassant < 7 && get_piece(enpassant_sq - 7) == WHITE_PAWN)
                moves.push_back(Move(enpassant_sq - 7, enpassant_sq, ENPASSANT_MOVE));
            // eating to the right (sq -> sq + 9)
            if(board->enpassant > 0 && get_piece(enpassant_sq - 9) == WHITE_PAWN)
                moves.push_back(Move(enpassant_sq - 9, enpassant_sq, ENPASSANT_MOVE));
        }

        // promotion eating diagonally to the left (sq -> sq + 7)
        mask = ((pawn_mask & ROW_6 & ~COL_0) << 7) & xside_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            // Move constructor:  'from'    'to'     'move type'    
            moves.push_back(Move(to_sq - 7, to_sq, QUEEN_PROMOTION));
            moves.push_back(Move(to_sq - 7, to_sq, KNIGHT_PROMOTION));
        }

        // promotion eating diagonally to the right (sq -> sq + 9)
        mask = ((pawn_mask & ROW_6 & ~COL_7) << 9) & xside_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq - 9, to_sq, QUEEN_PROMOTION));
            moves.push_back(Move(to_sq - 9, to_sq, KNIGHT_PROMOTION));
        }

        // promotion front (sq -> sq + 8)
        mask = ((pawn_mask & ROW_6) << 8) & ~board->occ_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq - 8, to_sq, QUEEN_PROMOTION));
            moves.push_back(Move(to_sq - 8, to_sq, KNIGHT_PROMOTION));
        }

        // pawn capture to the left (sq -> sq + 7)
        mask = ((pawn_mask & ~ROW_6 & ~COL_0) << 7) & xside_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq - 7, to_sq, CAPTURE_MOVE));
        }

        // pawn capture to the right (sq -> sq + 9)
        mask = ((pawn_mask & ~ROW_6 & ~COL_7) << 9) & xside_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq - 9, to_sq, CAPTURE_MOVE));
        }
    } else {
        // enpassant capture
        if(board->enpassant != NO_ENPASSANT) {
            int enpassant_sq = 16 + int(board->enpassant);
            assert(get_piece(enpassant_sq + 8) == WHITE_PAWN);
            // to the left (sq -> sq - 9)
            if(board->enpassant < 7 && get_piece(enpassant_sq + 9) == BLACK_PAWN)
                moves.push_back(Move(enpassant_sq + 9, enpassant_sq, ENPASSANT_MOVE));
            // to the right (sq -> sq - 7)
            if(board->enpassant > 0 && get_piece(enpassant_sq + 7) == BLACK_PAWN)
                moves.push_back(Move(enpassant_sq + 7, enpassant_sq, ENPASSANT_MOVE));
        }

        // promotion eating diagonally to the left (sq -> sq - 9)
        mask = ((pawn_mask & ROW_1 & ~COL_0) >> 9) & xside_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq + 9, to_sq, QUEEN_PROMOTION));
            moves.push_back(Move(to_sq + 9, to_sq, KNIGHT_PROMOTION));
        }

        // promotion eating diagonally to the right (sq -> sq - 7)
        mask = ((pawn_mask & ROW_1 & ~COL_7) >> 7) & xside_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq + 7, to_sq, QUEEN_PROMOTION));
            moves.push_back(Move(to_sq + 7, to_sq, KNIGHT_PROMOTION));
        }

        // promotion front (sq -> sq - 8)
        mask = ((pawn_mask & ROW_1) >> 8) & ~board->occ_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq + 8, to_sq, QUEEN_PROMOTION));
            moves.push_back(Move(to_sq + 8, to_sq, KNIGHT_PROMOTION));
        }

        // pawn capture to the left (sq -> sq - 9)
        mask = ((pawn_mask & ~ROW_1 & ~COL_0) >> 9) & xside_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            assert(Move(to_sq + 9, to_sq, CAPTURE_MOVE) != 8394);
            assert(!(to_sq + 9 == C2 && to_sq == D1));
            moves.push_back(Move(to_sq + 9, to_sq, CAPTURE_MOVE));
        }

        // pawn capture to the right (sq -> sq - 7)
        mask = ((pawn_mask & ~ROW_1 & ~COL_7) >> 7) & xside_mask;
        while(mask) {
            to_sq = pop_first_bit(mask);
            assert(!(to_sq + 7 == C2 && to_sq == D1));
            assert(Move(to_sq + 7, to_sq, CAPTURE_MOVE) != 8394);
            moves.push_back(Move(to_sq + 7, to_sq, CAPTURE_MOVE));
        }
    }

    // king captures
    mask = get_king_mask(board->side);
    assert(mask != 0);
    from_sq = pop_first_bit(mask);
    attack_mask = king_attacks[from_sq] & xside_mask;
    while(attack_mask) {
        assert(attack_mask != 0);
        to_sq = pop_first_bit(attack_mask);
        assert(to_sq >= 0 && to_sq < 64);
        moves.push_back(Move(from_sq, to_sq, CAPTURE_MOVE));
    }

    assert(mask == 0);

    // knight captures
    mask = get_knight_mask(board->side);
    while(mask) {
        from_sq = pop_first_bit(mask);
        attack_mask = knight_attacks[from_sq] & xside_mask;
        while(attack_mask) {
            to_sq = pop_first_bit(attack_mask);
            moves.push_back(Move(from_sq, to_sq, CAPTURE_MOVE));
        }
    }

    // bishop and queen captures
    mask = get_bishop_mask(board->side) | get_queen_mask(board->side);
    while(mask) {
        from_sq = pop_first_bit(mask);
        attack_mask = Bmagic(from_sq, board->occ_mask) & xside_mask;
        while(attack_mask) {
            to_sq = pop_first_bit(attack_mask);
            moves.push_back(Move(from_sq, to_sq, CAPTURE_MOVE));
        }
    }

    // rook and queen captures
    mask = get_rook_mask(board->side) | get_queen_mask(board->side);
    while(mask) {
        from_sq = pop_first_bit(mask);
        attack_mask = Rmagic(from_sq, board->occ_mask) & xside_mask;
        while(attack_mask) {
            to_sq = pop_first_bit(attack_mask);
            moves.push_back(Move(from_sq, to_sq, CAPTURE_MOVE));
        }
    }
}

// we assume that the king isn't in check
int* generate_quiet(int* moves, const Board* board) {
    uint64_t mask, attack_mask, pawn_mask = get_pawn_mask(board->side);
    int from_sq, to_sq;

    if(board->side == WHITE) {
        // front only one square (no promotion) (sq -> sq + 8)
        mask = ((pawn_mask & (~ROW_6)) << 8) & (~board->occ_mask);
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq - 8, to_sq, QUIET_MOVE));
        }
        
        // frontal two squares (sq -> sq + 8)
        mask = ((pawn_mask & ROW_1) << 8) & (~board->occ_mask);
        mask = ((mask & ROW_2) << 8) & (~board->occ_mask);
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq - 16, to_sq, QUIET_MOVE));
        }
    } else {
        // front only one square (no promotion) (sq -> sq - 8)
        mask = ((pawn_mask & (~ROW_1)) >> 8) & (~board->occ_mask);
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq + 8, to_sq, QUIET_MOVE));
        }

        // frontal two squares (sq -> sq - 16)
        mask = ((pawn_mask & ROW_6) >> 8) & (~board->occ_mask);
        mask = ((mask & ROW_5) >> 8) & (~board->occ_mask);
        while(mask) {
            to_sq = pop_first_bit(mask);
            moves.push_back(Move(to_sq + 16, to_sq, QUIET_MOVE));
        }
    }

    // generate the castling moves 
    if(board->side == WHITE) {
        // white queen side castling
        if((board->castling_flag & 1)
        && !(board->occ_mask & castling_mask[WHITE_QUEEN_SIDE])
        && !board->is_attacked(D1)) {
            assert(board->piece_at[E1] == KING);
            moves.push_back(Move(E1, C1, CASTLING_MOVE));
        }
        // white king side castling
        if((board->castling_flag & 2)
        && !(board->occ_mask & castling_mask[WHITE_KING_SIDE])
        && !board->is_attacked(F1)) {
            assert(board->piece_at[E1] == KING);
            moves.push_back(Move(E1, G1, CASTLING_MOVE));
        }
    } else if(board->side == BLACK) {
        // black queen side castling
        if((board->castling_flag & 4)
        && !(board->occ_mask & castling_mask[BLACK_QUEEN_SIDE])
        && !board->is_attacked(D8)) {
            assert(board->piece_at[E8] == KING);
            moves.push_back(Move(E8, C8, CASTLING_MOVE));
        }
        // black king side castling
        if((board->castling_flag & 8)
        && !(board->occ_mask & castling_mask[BLACK_KING_SIDE])
        && !board->is_attacked(F8)) {
            assert(board->piece_at[E8] == KING);
            moves.push_back(Move(E8, G8, CASTLING_MOVE));
        }
    }

    // king
    mask = get_king_mask(board->side);
    from_sq = pop_first_bit(mask);  
    attack_mask = king_attacks[from_sq] & (~board->occ_mask);
    while(attack_mask) {
        to_sq = pop_first_bit(attack_mask);
        moves.push_back(Move(from_sq, to_sq, QUIET_MOVE));
    }

    // knight
    mask = get_knight_mask(board->side);
    while(mask) {
        from_sq = pop_first_bit(mask);
        attack_mask = knight_attacks[from_sq] & (~board->occ_mask);
        while(attack_mask) {
            to_sq = pop_first_bit(attack_mask);
            moves.push_back(Move(from_sq, to_sq, QUIET_MOVE));
        }
    }

    // bishop and queen
    mask = get_bishop_mask(board->side) | get_queen_mask(board->side);
    while(mask) {
        from_sq = pop_first_bit(mask);
        attack_mask = Bmagic(from_sq, board->occ_mask) & (~board->occ_mask);
        while(attack_mask) {
            to_sq = pop_first_bit(attack_mask);
            moves.push_back(Move(from_sq, to_sq, QUIET_MOVE));
        }
    }

    // rooks and queen
    mask = get_rook_mask(board->side) | get_queen_mask(board->side);
    while(mask) {
        from_sq = pop_first_bit(mask);
        attack_mask = Rmagic(from_sq, board->occ_mask) & (~board->occ_mask);
        while(attack_mask) {
            to_sq = pop_first_bit(attack_mask);
            moves.push_back(Move(from_sq, to_sq, QUIET_MOVE));
        }
    }
}

