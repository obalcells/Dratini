#pragma once

#include <cstdint>
#include <cinttypes>

struct BitBoard {
    BitBoard() {}

    ~BitBoard() {}

    BitBoard(std::string fen) {
        /* TODO */
    }

    static uint64_t mask_sq(int sq);

    /* TODO: Init magic moves and tables (all static things) */

    void set_square(int sq, int piece);

    void set_square(int sq, int piece, bool _side);

    void clear_square(int sq, int piece);

    void clear_square(int sq, int piece, bool _side);

    bool is_empty(int sq);

    uint64_t get_all_mask() const;

    uint64_t get_side_mask(bool side) const;

    uint64_t get_pawns_mask(bool side) const;

    uint64_t get_knights_mask(bool side) const;

    uint64_t get_bishops_mask(bool side) const;

    uint64_t get_rooks_mask(bool side) const;

    uint64_t get_queens_mask(bool side) const;

    uint64_t get_king_mask(bool side) const;

    void update_castling_rights(const Move &move);

    void increment_count();

    void reset_fifty_move_count();

    void update_key(const BitBoard &, const Move &);

    uint64_t bits[12];
    uint64_t key;
    int move_count;
    int fifty_move_count;
    int enpassant;
    bool castling_rights[4];
    bool side, xside; /* TODO: change !side with xside and flip xside too after move */
}


