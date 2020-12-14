#pragma once

#include <cstdint>
#include <cinttypes>
#include <vector>

struct Board {
	static bool required_data_initialized = false;
	static std::vector<std::vector<uint64_t>> zobrist_pieces;
	static std::vector<uint64_t> zobrist_castling;
	static std::vector<uint64_t> zobrist_enpassant;
	static std::vector<uint64_t> zobrist_side;
	static std::vector<std::vector<uint64_t> > pawn_attacks;
	static std::vector<uint64_t> knight_attacks;
	static std::vector<uint64_t> king_attacks;

	static void init_data();

	static uint64_t mask_sq(int sq);

	BitBoard(const std::string& fen) {
		set_from_fen(fen);
		init_data();
	}

	BitBoard() {
		set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		init_data();
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
    uint64_t get_pawns_mask(bool) const;
    uint64_t get_knights_mask(bool) const;
    uint64_t get_bishops_mask(bool) const;
    uint64_t get_rooks_mask(bool) const;
    uint64_t get_queens_mask(bool) const;
    uint64_t get_king_mask(bool) const;

    int get_piece(const int) const;
    bool get_color(const int) const;

	void update_castling_rights(const Move&);
	void update_key(const Board&, const Move&);

	bool is_attacked(const int&) const;
    bool in_check() const;
    bool castling_valid(const Move&) const;
    bool move_diagonal(const Move&) const;
    bool check_pawn_move(const Move&) const;

	void make_move(const Move&);
	bool move_valid(const Move&);

    uint64_t bits[12];
    uint64_t key;
    uint16_t move_count;
    uint8_t fifty_move_ply;
    uint8_t enpassant;
    bool castling_rights[4];
    bool side, xside; /* TODO: change !side with xside and flip xside too after move */
}


