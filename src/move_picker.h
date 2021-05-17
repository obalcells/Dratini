#pragma once

#include "defs.h"
#include "board.h"
#include "search.h"
#include "gen.h"

enum phases {
	HASH,
	GENERATE_CAPTURES,
	GOOD_CAPTURES,
	FIRST_KILLER,
	SECOND_KILLER,
	BAD_CAPTURES,
	GENERATE_QUIET,
	QUIET,
    DONE
};

struct MoveWithScore {
	Move move;
	int score;

	MoveWithScore(Move _move, int _score) : move(_move), score(_score) {}

    friend bool operator==(const MoveWithScore& a, const MoveWithScore& b);
    friend bool operator!=(const MoveWithScore& a, const MoveWithScore& b);
    friend bool operator<(const MoveWithScore& a, const MoveWithScore& b);
};

inline bool operator==(const MoveWithScore& a, const MoveWithScore& b) {
	return a.score == b.score;
}

inline bool operator!=(const MoveWithScore& a, const MoveWithScore& b) {
	return a.score != b.score;
}

inline bool operator>(const MoveWithScore& a, const MoveWithScore& b) {
	return a.score > b.score;
}

inline bool operator<(const MoveWithScore& a, const MoveWithScore& b) {
	return a.score < b.score;
}

class MovePicker {
	public:
		MovePicker(Thread& _thread, const Move _tt_move, bool _captures_only = false) {
			thread = &_thread;
			board = &_thread.board;
			tt_move = _tt_move; 
			phase = HASH;
			captures_only = _captures_only;
		}
		Move next_move();
		int slow_see(const Move, const bool root = true); 
		int fast_see(const Move) const; 

		static Move get_random_move(const Board&);

	// private:
		void delete_move(int);
		int get_best_index(bool no_min = false) const;
		int next_lva(const uint64_t&, bool) const;
		int lva(const int) const;
		void check_xrays(uint64_t& attacker_mask, const int from_sq, const int to_sq) const;
		uint64_t get_attackers(const int, const bool) const;
		void sort_evasions();
		void sort_captures();
		void sort_quiet();

		int phase;
		bool captures_only;
		Move tt_move; 
		std::vector<Move> move_stack;
		std::vector<int> scores;
		Board* board;
		Thread* thread;
};