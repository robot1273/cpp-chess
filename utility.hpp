#pragma once

#include <cstdint>
#include <iostream>

namespace chess::utility {
    inline void setBit(uint64_t &board, int idx) {board |= 1ULL << idx;}
    inline void unsetBit(uint64_t &board, int idx) {board &= ~(1ULL << idx);}
    inline void toggleBit(uint64_t &board, int idx) {board ^= 1ULL << idx;}
    inline bool readBit(uint64_t &board, int idx) {return (board >> idx) & 1ULL;}

    void print_bitboard(uint64_t &board);
    void print_bitboard_solid(uint64_t &board);

}
