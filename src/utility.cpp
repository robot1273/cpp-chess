#include <iostream>
#include "utility.hpp"

namespace chess::utility {
    void print_bitboard(uint64_t &board) {
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

    void print_bitboard_solid(uint64_t &board) {
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