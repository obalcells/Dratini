#pragma once

#include <random>
#include <cstdint>
#include <cinttypes>
#include <vector>

#include "board.h"
#include "new_defs.h"
#include "defs.h"

namespace {
    /* pseudo-random number generator */
    std::string str_seed = "Dratini is fast!";
    std::seed_seq seed(str_seed.begin(), str_seed.end());
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));

    /* important to set required_data_initialized to false at the beginning */
    bool required_data_initialized = false;
    std::vector<std::vector<uint64_t> > zobrist_pieces;
    std::vector<uint64_t> zobrist_castling;
    std::vector<uint64_t> zobrist_enpassant;
    std::vector<uint64_t> zobrist_side;
    std::vector<std::vector<uint64_t> > pawn_attacks;
    std::vector<uint64_t> knight_attacks;
    std::vector<uint64_t> king_attacks;
    std::vector<uint64_t> castling_mask;

    const char piece_char[12] = { 
        'P', 'N', 'B', 'R', 'Q', 'K',
        'p', 'n', 'b', 'r', 'q', 'k',
    };
}

struct BitBoard {
	BitBoard(const std::string& fen) {
		init_data();
		set_from_fen(fen);
	}

	BitBoard() {
		init_data();
		set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	}

    /* initializes zobrist hashes and bitboard tables */
    static void init_data();

    static uint64_t mask_sq(int sq) {
        return (uint64_t(1) << sq);
    }

    static uint64_t get_random_64() {
        return dist(rng);
    }

    void clear_board();
    void set_from_fen(const std::string&);
    void set_square(const int, const int);
    void set_square(const int, const int, bool);
    void set_enpassant(const int);
    void clear_square(const int, const int);
    void clear_square(const int, const int, bool);

    bool is_empty(const int);
    uint64_t get_all_mask() const;
    uint64_t get_side_mask(bool) const;
    uint64_t get_pawn_mask(bool) const;
    uint64_t get_knight_mask(bool) const;
    uint64_t get_bishop_mask(bool) const;
    uint64_t get_rook_mask(bool) const;
    uint64_t get_queen_mask(bool) const;
    uint64_t get_king_mask(bool) const;

    int get_piece(const int) const;
    int get_color(const int) const;

	void update_castling_rights(const NewMove&);
	void update_key(const BitBoard&, const NewMove&);
    uint64_t calculate_key() const;

	bool is_attacked(const int) const;
    bool in_check() const;
    bool castling_valid(const NewMove&) const;
    bool move_diagonal(const NewMove&) const;
    bool check_pawn_move(const NewMove&) const;

	void make_move(const NewMove&);
	bool move_valid(const NewMove&);
    void print_board() const;
    void print_bitboard(uint64_t) const;

    void quick_check(const std::string&);
    void same(const Position&);

    uint64_t bits[12];
    uint64_t key;
    uint16_t move_count;
    uint8_t fifty_move_ply;
    uint8_t enpassant;
    bool castling_rights[4];
    bool side, xside; /* TODO: change !side with xside and flip xside too after move */
};
