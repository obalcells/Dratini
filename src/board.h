#pragma once

#include <random>
#include <cstdint>
#include <cinttypes>
#include <vector>

#include "defs.h"

extern std::vector<std::vector<uint64_t> > pawn_attacks;
extern std::vector<uint64_t> knight_attacks;
extern std::vector<uint64_t> king_attacks;
extern std::vector<uint64_t> castling_mask;

struct Board {
    Board(const std::string&);
    /*
     {
		init_data();
        cerr << "Finished doing the things with the data" << endl;
        if(read_from_file == false) {
            set_from_fen(str);
        } else {
            set_from_file(str);
        }
        update_classic_board();
	}
    */

	Board();
    /*
     {
		init_data();
		set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        update_classic_board();
	}
    */

    void clear_board();
    void set_from_fen(const std::string&);
    void set_from_data();
    void set_square(const int, const int);
    void set_square(const int, const int, bool);
    void set_enpassant(const int);
    void clear_square(const int, const int);
    void clear_square(const int, const int, bool);

    bool is_empty(const int);
    uint64_t get_piece_mask(int) const;
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

	void update_castling_rights(const Move);
	void update_key(const Board&, const Move);
    uint64_t calculate_key() const;

	bool is_attacked(const int) const;
    bool in_check() const;
    bool is_draw() const;
    bool checkmate();
    bool stalemate();
    bool castling_valid(const Move) const;
    bool move_diagonal(const Move) const;
    bool check_pawn_move(const Move) const;

	void make_move(const Move);
	bool move_valid(const Move);
    bool fast_move_valid(const Move) const;
    void print_board() const;
    void print_bitboard(uint64_t) const;
    void print_bitboard_data() const;

    void quick_check(const std::string&);
    // void same(const Position&) const;
    bool same(const Board& other) const;

    void update_classic_board();

    uint64_t bits[12];
    uint64_t key;
    uint16_t move_count;
    uint8_t fifty_move_ply;
    uint8_t enpassant;
    bool castling_rights[4];
    bool side;
    bool xside;

    // classical board
    uint8_t color_at[64];
    uint8_t piece_at[64];

    friend bool operator==(const Board& a, const Board& b);
};

inline bool operator==(const Board& a, const Board& b) {
    return a.same(b);
}
