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

extern std::vector<std::vector<uint64_t> > zobrist_pieces;
extern std::vector<uint64_t> zobrist_castling;
extern std::vector<uint64_t> zobrist_enpassant;
extern std::vector<uint64_t> zobrist_side;
extern std::vector<int> castling_bitmasks;

#define clear_square(sq, piece) bits[piece] ^= mask_sq(sq); \
	b_pst[piece >= BLACK_PAWN ? BLACK : WHITE] -= pst[piece_at[sq]][sq]; \
	b_mat[piece >= BLACK_PAWN ? BLACK : WHITE] -= piece_value[piece_at[sq]]; \
	color_at[sq] = piece_at[sq] = EMPTY; \
	occ_mask ^= mask_sq(sq);

#define set_square(sq, piece) bits[piece] |= mask_sq(sq); \
	color_at[sq] = (piece >= BLACK_PAWN ? BLACK : WHITE); \
	piece_at[sq] = (piece >= BLACK_PAWN ? piece - 6 : piece); \
    b_pst[color_at[sq]] += pst[piece_at[sq]][sq]; \
	b_mat[color_at[sq]] += piece_value[piece_at[sq]]; \
	occ_mask |= mask_sq(sq);

struct UndoData {
    Move move;
    uint8_t enpassant, castling_flag, moved_piece, captured_piece, fifty_move_ply;

    UndoData() {}

    UndoData(
        const Move _move, const uint8_t _enpassant, const uint8_t _castling_flag,
        const uint8_t _moved_piece, const uint8_t _captured_piece, const uint8_t _fifty_move_ply
        ) {
        move = _move;
        enpassant = _enpassant;
        castling_flag = _castling_flag;
        moved_piece = _moved_piece;
        captured_piece = _captured_piece;
        fifty_move_ply = _fifty_move_ply;
    }
};

struct Board {
    Board();
    Board(const std::string&);

    // board state functions
	bool is_attacked(const int) const;
    bool in_check() const;
    bool is_draw() const;
    bool checkmate();
    bool stalemate();
	bool move_valid(const Move);
    bool fast_move_valid(const Move) const;
    void print_board() const;
    void print_bitboard(uint64_t) const;
    void print_board_data() const;
    std::string get_data() const;

    // take back and make move
    bool make_move_from_str(const std::string&);
	UndoData make_move(const Move);
    void new_make_move(const Move, UndoData&);
    void take_back(const UndoData&);

    // error checking
    void error_check() const;
    bool same(const Board& other) const;
    void check_classic();
    void update_classic_settings();

    // variables that are accessed externally
    uint64_t king_attackers;
    uint64_t key;
    uint8_t color_at[64];
    uint8_t piece_at[64];
    bool side, xside;
    uint8_t enpassant;
    uint64_t bits[12];
    uint16_t move_count;
    std::vector<bool> castling_rights;
    int castling_flag; // flag we will use in the future
    uint64_t occ_mask;
    std::vector<uint64_t> keys;

    // for sungorus' eval
    int b_mat[2];
    int b_pst[2];

    // bitboard stuff
    void clear_board();
    void set_from_fen(const std::string&);
    void set_from_data();
    // void set_square(const int, const int);
    // void set_square(const int, const int, bool);
    void set_enpassant(const int);
    // void clear_square(const int, const int);
    // void clear_square(const int, const int, bool);
    // uint64_t get_piece_mask(int) const;
    // uint64_t get_all_mask() const;
    // uint64_t get_side_mask(bool) const;
    // uint64_t get_pawn_mask(bool) const;
    // uint64_t get_knight_mask(bool) const;
    // uint64_t get_bishop_mask(bool) const;
    // uint64_t get_rook_mask(bool) const;
    // uint64_t get_queen_mask(bool) const;
    // uint64_t get_king_mask(bool) const;
    // int get_piece(const int) const;
    // int get_color(const int) const;

    void update_material_values();
    uint64_t calculate_key(bool is_assert = true) const;

    uint8_t fifty_move_ply;

private:
    // internal functions for checking validity and making moves
	void update_castling_rights(const Move);
	void update_key(const UndoData&);
    bool castling_valid(const Move) const;
    bool move_diagonal(const Move) const;
    bool check_pawn_move(const Move) const;

    friend bool operator==(const Board& a, const Board& b);
    friend bool operator!=(const Board& a, const Board& b);
};

inline bool operator==(const Board& a, const Board& b) {
    return a.same(b);
}

inline bool operator!=(const Board& a, const Board& b) {
    return !a.same(b);
}
