#include <vector>
#include <random>
#include <algorithm>
#include <cassert>
#include <string>
#include "../src/defs.h"
#include "../src/tt.h"
#include "../src/board.h"
#include "../src/gen.h"
#include "../src/engine.h"
#include "board_speed.h"

TranspositionTable tt;
Engine engine;

static std::vector<std::string> split(const std::string &str) {
    std::vector<std::string> tokens;
    std::string::size_type start = 0;
    std::string::size_type end = 0;
    while ((end = str.find(" ", start)) != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(str.substr(start));
    return tokens;
}

uint64_t new_perft(Board&, int); 
uint64_t old_perft(Board&, int); 

static const int max_depth = 5;
static const int n_runs = 10;
static long long time_gen = 0;
static std::chrono::time_point<std::chrono::high_resolution_clock> t;

int main() {
    // test_board_speed();
    test_move_valid_speed();
    // test_see_speed();
    return 0;

    Board board = Board("r3k2r/2pb1ppp/2pp1q2/p7/1nP1B3/1P2P3/P2N1PPP/R2QK2R w KQkq a6 0 14");
    std::chrono::time_point < std::chrono::high_resolution_clock > initial_time;

    uint64_t duration_1 = 0, duration_2 = 0, duration_3 = 0;
    uint64_t p1, p2, p3;

    for(int i = 0; i < n_runs; i++) {
        // initial_time = std::chrono::high_resolution_clock::now();
        // p1 = old_perft(board, 0);
        // duration_1 += (long long)(std::chrono::high_resolution_clock::now() - initial_time).count();

        initial_time = std::chrono::high_resolution_clock::now();
        p2 = new_perft(board, 0);
        duration_2 += (long long)(std::chrono::high_resolution_clock::now() - initial_time).count();
    } 

    // adjusted rates
    duration_1 /= (long long)1e3 * n_runs;
    duration_2 /= (long long)1e3 * n_runs;
    // time_gen /= (long long)1e3 * n_runs;

    // cerr << BLUE_COLOR << "Dratini with vector" << endl << RESET_COLOR;
    // cerr << duration_1 << endl;
    cerr << MAGENTA_COLOR << "Dratini with array" << endl << RESET_COLOR;
    cerr << duration_2 << endl;

    return 0;
}

static int flag_converter(int move, POS* pos) {
    switch(MoveType(move)) {
        case CASTLE: return CASTLING_MOVE;
        case EP_CAP: return ENPASSANT_MOVE;
        case EP_SET: return QUIET_MOVE;  
        case N_PROM: return KNIGHT_PROMOTION;
        case B_PROM: return BISHOP_PROMOTION;
        case R_PROM: return ROOK_PROMOTION;
        case Q_PROM: return Q_PROM;
        default: {
            if(pos->pc[Tsq(move)] == NO_PC) {
                // cerr << MAGENTA_COLOR << "There is piece " << TpOnSq(pos, Tsq(move)) << " at sq " << Tsq(move) << endl << RESET_COLOR;
                return QUIET_MOVE;
            } 
            return CAPTURE_MOVE;
        }
    }
    assert(false);
}

uint64_t s_perft(POS *pos, int depth) {
    int move[256];
	// t = std::chrono::high_resolution_clock::now();
    GenerateCaptures(pos, move);
    int* last = GenerateQuiet(pos, move);
    // time_gen += (long long)(std::chrono::high_resolution_clock::now() - t).count();
    if(depth == max_depth)
        return last - move;
    UNDO undo_data;
    uint64_t ans = 0;
    for(int* it = move; it < last; it++) {
        DoMove(pos, *it, &undo_data);
        if(Illegal(pos)) {
            UndoMove(pos, *it, &undo_data);
            continue;
        } 
        ans += s_perft(pos, depth + 1);
        UndoMove(pos, *it, &undo_data);
    }
    return ans;
}

// using new board functions with array pointer to add moves 
uint64_t new_perft(Board& board, int depth) {
    Move moves[256];
	// t = std::chrono::high_resolution_clock::now();
    new_generate_captures(moves, &board);
    Move* last = new_generate_quiet(moves, &board);
    // time_gen += (long long)(std::chrono::high_resolution_clock::now() - t).count();
    if(depth == max_depth)
        return last - moves;
    UndoData undo_data = UndoData(board.king_attackers);
    uint64_t ans = 0;
    for(Move* movep = moves; movep < last; movep++) {
        board.new_make_move(*movep, undo_data);
        if(!board.is_attacked(lsb(board.bits[KING + (board.xside ? 6 : 0)]), board.side))
            ans += new_perft(board, depth + 1);
        board.new_take_back(undo_data);
    }
    return ans;
}

// using new board functions but vector for storing moves
uint64_t old_perft(Board& board, int depth) {
    std::vector<Move> moves;
    moves.reserve(128);
    generate_captures(moves, &board);
    generate_quiet(moves, &board);
    if(depth == max_depth)
        return moves.size();
    UndoData undo_data = UndoData(board.king_attackers);
    uint64_t ans = 0;
    for(auto move : moves) {
        board.new_make_move(move, undo_data);
        if(!board.is_attacked(lsb(board.bits[KING + (board.xside ? 6 : 0)]), board.side))
            ans += old_perft(board, depth + 1);
        board.new_take_back(undo_data);
    }
    return ans;
}

// using new board functions with array pointer to add moves 
void perft_check(Board& board, POS* pos, int depth) {
    UndoData d_undo_data = UndoData(board.king_attackers);
    UNDO s_undo_data;

    Move d1_moves[256];
    int s1_moves[256];

    new_generate_captures(d1_moves, &board);
    Move* d_last = new_generate_quiet(d1_moves, &board);

    GenerateCaptures(pos, s1_moves);
    int* s_last = GenerateQuiet(pos, s1_moves);

    std::vector<Move> d3_moves, s3_moves;

    for(Move* movep = d1_moves; movep < d_last; movep++) {
        // cerr << "Making move " << move_to_str(*movep) << " f " << (int)get_flag(*movep) << endl;
        // cerr << "Board is " << endl;
        // board.print_board();
        board.new_make_move(*movep, d_undo_data);
        if(board.is_attacked(lsb(board.bits[KING + (board.xside ? 6 : 0)]), board.side)) {
            board.new_take_back(d_undo_data);
            if(board.new_fast_move_valid(*movep)) {
                cerr << "Move " << move_to_str(*movep) << " is detected as valid by fmv and invalid by MAR" << endl;
                cerr << "Board is" << endl;
                board.print_board();
            }
            assert(!board.new_fast_move_valid(*movep));
            continue;
        }
        d3_moves.push_back(*movep);
        board.new_take_back(d_undo_data);
        if(!board.new_fast_move_valid(*movep)) {
            cerr << "Move " << move_to_str(*movep) << " is detected as invalid by fmv and valid by MAR" << endl;
            cerr << "Board is" << endl;
            board.print_board();
        }
        assert(board.new_fast_move_valid(*movep));
    }

    for(int* movep = s1_moves; movep < s_last; movep++) {
        DoMove(pos, *movep, &s_undo_data);
        if(Illegal(pos)) {
            UndoMove(pos, *movep, &s_undo_data);
            continue;
        }
        UndoMove(pos, *movep, &s_undo_data);
        s3_moves.push_back(Move(Fsq(*movep), Tsq(*movep), flag_converter(*movep, pos))); // flag_converter(*movep, pos)));
    }

    std::sort(d3_moves.begin(), d3_moves.end());
    std::sort(s3_moves.begin(), s3_moves.end());
    bool diff = false; 

    if(d3_moves.size() == s3_moves.size()) {
        cerr << BLUE_COLOR;
        for(int i = 0; i < d3_moves.size(); i++) {
            if(d3_moves[i] != s3_moves[i]) {
                diff = true;
                cerr << move_to_str(d3_moves[i]) << " f is " << get_flag(d3_moves[i]) << endl; 
                cerr << move_to_str(s3_moves[i]) << " f is " << get_flag(s3_moves[i]) << endl; 
                cerr << "***********" << endl;
            }
        }
        cerr << RESET_COLOR;
    }

    if(d3_moves.size() != s3_moves.size() || diff) {
        cerr << RED_COLOR << "Gen moves differ at depth " << depth << endl << RESET_COLOR;
        cerr << "Board is" << endl;
        board.print_board();
        cerr << "Moves generated by Dratini are" << endl; 
        for(auto move : d3_moves) {
            cerr << move_to_str(move) << " f " << get_flag(move) << " ";
        }
        cerr << endl;
        cerr << "Moves generated by new move picker are" << endl;
        for(auto move : s3_moves) {
            cerr << move_to_str(move) << " f " << get_flag(move) << " ";
        }
        cerr << endl;
    }

    assert(d3_moves.size() == s3_moves.size());
    assert(!diff);

    if(depth == max_depth)
        return;
    UndoData undo_data = UndoData(board.king_attackers);
    // long long ans = 0;
    int s_move;
    bool b1, b2;
    for(auto move : d3_moves) {
        board.new_make_move(move, undo_data);
        s_move = StrToMove(pos, const_cast<char*>(move_to_str(move).c_str()));
        DoMove(pos, s_move, &s_undo_data); 
        b1 = !board.is_attacked(lsb(board.bits[KING + (board.xside ? 6 : 0)]), board.side);
        b2 = !Illegal(pos);
        if(b1 != b2) {
            cerr << "Differing move is " << move_to_str(move) << endl;
            cerr << b1 << " " << b2 << endl; 
            cerr << "Board is " << endl;
            board.print_board();
        }
        assert(b1 == b2);
        if(b1)
            perft_check(board, pos, depth + 1);
        UndoMove(pos, s_move, &s_undo_data);
        board.new_take_back(undo_data);
    }
}
