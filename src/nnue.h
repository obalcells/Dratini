
#pragma once

int nnue_eval(Board* board);
void nnue_init(const char* file_name);

struct Accumulator {
    bool has_been_computed;
    alignas(64) int16_t accumulation[2][256];
};

struct DirtyPiece {
    int dirtyNum;
    bool no_king;
    int pc[3];
    int from[3];
    int to[3];
};