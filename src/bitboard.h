#pragma once

#include <cstdint>
#include <cinttypes>

namespace {
}

class BitBoard {
    public:
        bool is_empty(int sq) const;
        int get_piece(int sq) const;
        bool get_color(int sq) const; 
        uint64_t get_white() const;
        uint64_t get_black() const;
        uint64_t get_pawns(bool side) const;
        uint64_t get_knights(bool side) const;
        uint64_t get_bishops(bool side) const;
        uint64_t get_rooks(bool side) const;
        uint64_t get_queens(bool side) const;
        uint64_t get_king(bool side) const;

        void set_enpassant(int col);
        void update_castling_rights(Move move);
        void increment_count();
        void reset_fifty_move_count();
        void update_key(const BitBoard&, Move);
    private:
        uint64_t bits[12];
        uint64_t key;
        int move_count;
        int fifty_move_count;         
        int enpassant;
        bool castling_rights[4];
        bool side;
}


