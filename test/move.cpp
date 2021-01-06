#include "catch.h"
#include "../src/board.h"
#include "../src/new_position.h"
#include "../src/stats.h"
#include "../src/tt.h"

inline bool same_valid_moves(Position& position, NewPosition& new_position, std::vector<std::pair<int,int> >& moves_to_pick) {
    for(int from_sq = 0; from_sq < 64; from_sq++) if(position.piece[from_sq] != EMPTY) { 
        for(int to_sq = 0; to_sq < 64; to_sq++) { 
            bool old_legal, new_legal;
            BENCHMARK("Old move valid checking") {
                old_legal = position.move_valid(Move(from_sq, to_sq));
            };
            BENCHMARK("New move valid checking") {
                new_legal = new_position.move_valid(from_sq, to_sq);
            };
            if(new_legal != old_legal)
                return false;
            if(new_legal)
                moves_to_pick.push_back(std::make_pair(from_sq, to_sq));
        }
    }
}

TEST_CASE("Move comparison with old board") {
    Position position;
    NewPosition new_position;
    auto rng_shuffle = std::default_random_engine {};

    // we play 1000 games
	int cnt = 0;
	while(cnt++ < 1000) {
        position = Position();
        new_position = NewPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		bool game_over = false;
		while(!game_over) {
			std::vector<std::pair<int,int> > moves_to_pick;
			same_valid_moves(position, new_position, moves_to_pick);
			if(moves_to_pick.empty() || new_position.only_kings_left() || new_position.get_move_count() >= 500) {
				game_over = true;
			} else {
				std::shuffle(moves_to_pick.begin(), moves_to_pick.end(), rng_shuffle);
				const Move move = Move(moves_to_pick[0].first, moves_to_pick[0].second);
                position.make_move(move);
                new_position.board_history.back().quick_check("Before making move");
                new_position.make_move_from_str(move_to_str(move));
                new_position.board_history.back().same(position);
                new_position.board_history.back().quick_check("After making move");
			}
		}
	}
}