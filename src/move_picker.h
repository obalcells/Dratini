#pragma once

#include "defs.h"
#include "new_board.h"

enum phases {
	HASH,
	GENERATE_CAPTURES,
	GOOD_CAPTURES,
	KILLERS,
	BAD_CAPTURES,
	GENERATE_QUIET,
	QUIET
};

struct NewMoveWithScore {
	 NewMove move;
	int score;

	NewMoveWithScore(NewMove _move, int _score) : move(_move), score(_score) {}

    friend bool operator==(const NewMoveWithScore& a, const NewMoveWithScore& b);
    friend bool operator!=(const NewMoveWithScore& a, const NewMoveWithScore& b);
    friend bool operator<(const NewMoveWithScore& a, const NewMoveWithScore& b);
};

inline bool operator==(const NewMoveWithScore& a, const NewMoveWithScore& b) {
	return a.score == b.score;
}

inline bool operator!=(const NewMoveWithScore& a, const NewMoveWithScore& b) {
	return a.score != b.score;
}

inline bool operator>(const NewMoveWithScore& a, const NewMoveWithScore& b) {
	return a.score > b.score;
}

inline bool operator<(const NewMoveWithScore& a, const NewMoveWithScore& b) {
	return a.score < b.score;
}

class MovePicker {
public:
	MovePicker(BitBoard& current_board) {
		board = &current_board;
		phase = HASH;
	}
	NewMove next_move();
    int slow_see(const NewMove&, bool root = true); 
    int fast_see(const NewMove&); 

private:
	int next_lva(const uint64_t&, bool) const;
    int lva(const int) const;
	void check_xrays(uint64_t& attacker_mask, const int from_sq, const int to_sq) const;
	uint64_t get_attackers(int, bool) const;
	void sort_evasions();
	void sort_captures();
	void sort_quiet();

	int phase = 0;
	NewMove hash_move;
	std::vector<NewMoveWithScore> move_stack;
	std::vector<NewMove> compatible_move_stack;
	BitBoard* board;
};
