#include "catch.h"
#include "../src/board.h"
#include "../src/new_position.h"
#include "../src/stats.h"
#include "../src/tt.h"

inline bool same_valid_moves(Position& position, NewPosition& new_position, std::vector<std::pair<int,int> >& moves_to_pick) {
    for(int from_sq = 0; from_sq < 64; from_sq++) if(position.piece[from_sq] != EMPTY) { 
        for(int to_sq = 0; to_sq < 64; to_sq++) { 
            bool old_legal, new_legal;
            old_legal = position.move_valid(Move(from_sq, to_sq));
            new_legal = new_position.move_valid(from_sq, to_sq);
            if(new_legal != old_legal)
                return false;
            if(new_legal)
                moves_to_pick.push_back(std::make_pair(from_sq, to_sq));
        }
    }
}

TEST_CASE("Checking correctness of move making and validation") {
    Position position;
    NewPosition new_position;
    auto rng_shuffle = std::default_random_engine {};

    // we play 1000 games
	int cnt = 0;
	while(cnt++ < 100) {
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

TEST_CASE("Benchmark validating and making move") {
    Position position;
    NewPosition new_position;
    std::string rng_seed_str = "Dratini";
    std::seed_seq seed1 (rng_seed_str.begin(), rng_seed_str.end());
    std::vector<std::vector<std::pair<int,int> > > all_games;
    int cnt = 0;
    int move_cnt = 0;
    auto rng_shuffle = std::default_random_engine { seed1 };

    while(cnt++ < 50) {      
        position = Position();
        new_position = NewPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        move_cnt = 0;
        std::vector<std::pair<int,int> > moves;
        bool game_over = false;
        while(!game_over) {
            std::vector<std::pair<int,int> > moves_to_pick;
            for(int from_sq = 0; from_sq < 64; from_sq++) if(position.piece[from_sq] != EMPTY) { 
                for(int to_sq = 0; to_sq < 64; to_sq++) { 
                    bool old_legal, new_legal;
                    old_legal = position.move_valid(Move(from_sq, to_sq));
                    new_legal = new_position.move_valid(from_sq, to_sq);
                    assert(new_legal == old_legal);
                    if(new_legal)
                        moves_to_pick.push_back(std::make_pair(from_sq, to_sq));
                }
            }
            if(moves_to_pick.empty() || new_position.only_kings_left() || move_cnt >= 300)
                break;
            std::shuffle(moves_to_pick.begin(), moves_to_pick.end(), rng_shuffle);
            const Move move = Move(moves_to_pick[0].first, moves_to_pick[0].second);
            moves.push_back(moves_to_pick[0]);
            position.make_move(move);
            new_position.board_history.back().quick_check("Before making move");
            new_position.make_move_from_str(move_to_str(move));
            new_position.board_history.back().same(position);
            move_cnt++;
        }
        all_games.push_back(moves);
    }

    BENCHMARK("OLD only move making function") {
        for(int i = 0; i < (int)all_games.size(); i++) {
            position = Position();
            for(int j = 0; j < (int)all_games[i].size(); j++)
                position.make_move(Move(all_games[i][j].first, all_games[i][j].second));
        }
    };

    BENCHMARK("NEW only move making function") {
        for(int i = 0; i < (int)all_games.size(); i++) {
            new_position = NewPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            for(int j = 0; j < (int)all_games[i].size(); j++)
                new_position.make_move_from_str(move_to_str(Move(all_games[i][j].first, all_games[i][j].second)));
        }
    };

    BENCHMARK("OLD move making + validity checking") {
        rng_shuffle = std::default_random_engine { seed1 };
        // we play 1000 games
        cnt = 0;
        move_cnt = 0;
        while(cnt++ < 500) {
            position = Position();
            bool game_over = false;
            while(!game_over) {
                std::vector<std::pair<int,int> > moves_to_pick;
                for(int from_sq = 0; from_sq < 64; from_sq++)  
                    for(int to_sq = 0; to_sq < 64; to_sq++)
                        if(position.move_valid(Move(from_sq, to_sq)))
                            moves_to_pick.push_back(std::make_pair(from_sq, to_sq));
                if(moves_to_pick.empty() || position.only_kings_left() || move_cnt >= 300)
                    break;
                std::shuffle(moves_to_pick.begin(), moves_to_pick.end(), rng_shuffle);
                const Move move = Move(moves_to_pick[0].first, moves_to_pick[0].second);
                position.make_move(move);
                move_cnt++;
            }
        }
    };

    BENCHMARK("NEW move making and validity checking") {
        rng_shuffle = std::default_random_engine { seed1 };
        cnt = 0;
        move_cnt = 0;
        while(cnt++ < 500) {
            new_position = NewPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            bool game_over = false;
            while(!game_over) {
                std::vector<std::pair<int,int> > moves_to_pick;
                for(int from_sq = 0; from_sq < 64; from_sq++) 
                    for(int to_sq = 0; to_sq < 64; to_sq++)
                        if(new_position.move_valid(from_sq, to_sq))
                            moves_to_pick.push_back(std::make_pair(from_sq, to_sq));
                if(moves_to_pick.empty() || new_position.only_kings_left() || move_cnt >= 300)
                    break;
                std::shuffle(moves_to_pick.begin(), moves_to_pick.end(), rng_shuffle);
                const Move move = Move(moves_to_pick[0].first, moves_to_pick[0].second);
                new_position.make_move_from_str(move_to_str(move));
                move_cnt++;
            }
        }
    };
}