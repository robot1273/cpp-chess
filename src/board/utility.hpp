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

    inline void print_bitboard(uint64_t &board) {
        for (int rank = 7; rank >= 0; rank--) {         
            for (int file = 0; file < 8; file++) {
                int idx = rank * 8 + file;
                bool bit = readBit(board, idx);
                std::cout << bit;
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    inline void print_bitboard_solid(uint64_t &board) {
        for (int rank = 7; rank >= 0; rank--) {         
            for (int file = 0; file < 8; file++) {
                int idx = rank * 8 + file;
                bool bit = readBit(board, idx);
                std::string c = bit ? "█" : ".";
                std::cout << c;
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

}
