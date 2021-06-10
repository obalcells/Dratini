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
    uint64_t king_attackers;

    // UndoData() {}

    // UndoData(
    //     const Move _move, const uint8_t _enpassant, const uint8_t _castling_flag,
    //     const uint8_t _moved_piece, const uint8_t _captured_piece, const uint8_t _fifty_move_ply
    //     ) {
    //     move = _move;
    //     enpassant = _enpassant;
    //     castling_flag = _castling_flag;
    //     moved_piece = _moved_piece;
    //     captured_piece = _captured_piece;
    //     fifty_move_ply = _fifty_move_ply;
    // }

    UndoData(const uint64_t _king_attackers) {
        king_attackers = _king_attackers;
    }
};

struct Board {
    Board();
    Board(const std::string&);
    bool opp_king_attacked() const;
	bool is_attacked(const int) const;
    bool is_attacked(const int, bool) const;
    bool in_check() const;
    bool is_draw() const;
    bool checkmate();
    bool stalemate();
	bool move_valid(const Move);
    bool new_move_valid(const Move);
    bool fast_move_valid(const Move) const;
    bool new_fast_move_valid(const Move) const;
    void print_board() const;
    void print_bitboard(uint64_t) const;
    void print_board_data() const;
    std::string get_data() const;
    bool make_move_from_str(const std::string&);
	void make_move(const Move, UndoData&);
    void new_make_move(const Move, UndoData&);
    void take_back(const UndoData&);
    void new_take_back(const UndoData&);
    int slow_see(const Move);
    int next_lva(const uint64_t&, const bool) const;
    int lva(int) const;
    int fast_see(const Move) const;
    void error_check() const;
    bool same(const Board& other) const;
    void check_classic();
    void clear_board();
    void set_from_fen(const std::string&);
    void update_material_values();
    uint64_t calculate_key(bool is_assert = true) const;

    uint64_t king_attackers;
    uint64_t key;
    uint8_t color_at[64];
    uint8_t piece_at[64];
    bool side, xside;
    uint8_t enpassant;
    uint64_t bits[12];
    uint16_t move_count;
    uint8_t castling_flag;
    uint64_t occ_mask;
    std::vector<uint64_t> keys;
    uint8_t fifty_move_ply;
    int b_mat[2]; // for sungorus eval
    int b_pst[2];


private:
	void update_key(const UndoData&);
    bool move_diagonal(const Move) const;
    bool castling_valid(const Move) const;
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
