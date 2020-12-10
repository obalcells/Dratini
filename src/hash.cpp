#include <random>
#include "position.h"

/* pseudo-random number generator */
std::string str_seed = "Dratini is fast!";
std::seed_seq seed(str_seed.begin(), str_seed.end());
std::mt19937_64 rng(seed_seq);
std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));

uint64_t get_random_64() {
    return dist(rng);
}

void BitBoard::init_zobrist() {
    if(zobrist_initialized)
        return;

    for(int piece = WHITE_PAWN; piece <= BLACK_KING; piece++) {
        std::vector<uint64_t> piece_table;
        for(int sq = 0; sq < 64; sq++) {
            piece_table.push_back(get_random_64());
        }
        zobrist_pieces.push_back(piece_table);
    }

    for(int castling_flag = 0; castling_flag < 4; castling_flag++) 
        zobrist_castling.push_back(get_random_64());
    
    for(int col = 0; col < 8; col++)
        zobrist_enpassant.push_back(get_random_64());

    /* when there is no enpassant column */
    zobrist_enpassant.push_back(0);

    zobrist_side = get_random_64();
    
    zobrist_initialized = true;
}

