#include <vector>
#include <cassert>
#include "defs.h"
#include "gen.h"
#include "new_move_picker.h"

NewMovePicker::NewMovePicker(Thread& _thread, Move _tt_move, bool _quiesce) {
    thread = &_thread;
    board = &_thread.board;
    tt_move = _tt_move;
    killer_1 = _thread.killers[_thread.ply][0];
    killer_2 = _thread.killers[_thread.ply][1];
    quiesce = _quiesce;
    phase = 0;
    move_p = moves;
    moves_end = moves;
    scores_end = scores;
    bad_captures_end = bad_captures;
}

Move NewMovePicker::next_move() {
    switch(phase) {
        case 0: {
            if(tt_move != NULL_MOVE
            && board->move_valid(tt_move)
            && !((get_flag(tt_move) == QUIET_MOVE || get_flag(tt_move) != CASTLING_MOVE) && quiesce)) {
                phase = 1;
                return tt_move;
            }
        }
        case 1: {
            moves_end = move_p = moves;
            bad_captures_end = bad_captures;
            moves_end = new_generate_captures(moves, board);
            if(moves_end - move_p < 0) {
                thread->board.print_board();
            }
            score_captures();
            phase = 2;
        }
        case 2: {
            while(move_p < moves_end) {
                move = select_best();
                if(move == tt_move) {
                    continue;
                } else if(bad_capture(move)) {
                    *(bad_captures_end++) = move;
                    continue;
                }
                return move;
            }
            assert(move_p == moves_end);
            if(quiesce) {
                return NULL_MOVE;
            }
        }
        case 3: {
            phase = 4;
            move = killer_1;
            if(move != NULL_MOVE 
            && move != tt_move
            && get_flag(move) != CAPTURE_MOVE
            && board->new_move_valid(move)) {
                return move;
            } 
        }
        case 4: {
            phase = 5;
            move = killer_2;
            if(move != NULL_MOVE
            && move != tt_move
            && get_flag(move) != CAPTURE_MOVE
            && board->new_move_valid(move)) {
                return move;
            }
        }
        case 5: {
            assert(move_p == moves_end);
            Move* quiets_start = moves_end;
            moves_end = new_generate_quiet(move_p, board);
            score_quiet(quiets_start);
            phase = 6;
        }
        case 6: {
            while(move_p < moves_end) {
                move = select_best();
                if(move == tt_move
                || move == killer_1
                || move == killer_2) {
                    continue;
                }
                return move;
            } 
            phase = 7;
            move_p = bad_captures; // we reset move pointer
        }
        case 7: { // bad captures
            while(move_p < bad_captures_end) {
                return *(move_p++);
            }
        }
    }
    return NULL_MOVE;
}

Move NewMovePicker::select_best() {
    score_p_aux = scores_end - 1;
    move_p_aux = moves_end - 1;
    while(move_p_aux > move_p) {
        if(*score_p_aux > *(score_p_aux - 1)) {
            aux = *score_p_aux;
            *score_p_aux = *(score_p_aux - 1);
            *(score_p_aux - 1) = aux;
            aux = *move_p_aux;
            *move_p_aux = *(move_p_aux - 1);
            *(move_p_aux - 1) = aux;
        }
        move_p_aux--;
        score_p_aux--;
    }
    return *(move_p++);
}

bool NewMovePicker::bad_capture(Move _move) const {
    assert(get_flag(_move) != QUIET_MOVE); 
    if(get_flag(_move) != CAPTURE_MOVE
    || piece_value[board->piece_at[get_from(_move)]] >= piece_value[board->piece_at[get_to(_move)]])
        return false; 
    return board->fast_see(_move) < 0; // static exchange eval
}

void NewMovePicker::score_captures() {
    for(move_p_aux = moves; move_p_aux < moves_end; move_p_aux++)
        *(scores_end++) = thread->capture_history[board->piece_at[get_from(*move_p_aux)]][get_to(*move_p_aux)][board->piece_at[get_to(*move_p_aux)]];
}

void NewMovePicker::score_quiet(Move* quiets_start) {
    for(move_p_aux = quiets_start; move_p_aux < moves_end; move_p_aux++)
        *(scores_end++) = thread->quiet_history[board->side][get_from(*move_p_aux)][get_to(*move_p_aux)];
}

