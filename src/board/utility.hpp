#pragma once

#include <cstdint>
#include <iostream>

#include <execinfo.h>
#include <sstream>
#include <cstdio>


namespace chess::utility {
    inline void setBit(uint64_t &board, int idx) {board |= 1ULL << idx;}
    inline void unsetBit(uint64_t &board, int idx) {board &= ~(1ULL << idx);}
    inline void toggleBit(uint64_t &board, int idx) {board ^= 1ULL << idx;}
    inline bool readBit(const uint64_t &board, int idx) {return (board >> idx) & 1ULL;}

    inline uint64_t shift(const uint64_t b, int offset) {
        return (offset > 0) ? (b << offset) : (b >> -offset);
    }

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

    inline void print_stacktrace() {
        void* buffer[100];
        int n = backtrace(buffer, 100);

        for (int i = 0; i < n; ++i) {
            std::ostringstream cmd;
            cmd << "addr2line -f -C -e /proc/self/exe " << buffer[i];

            FILE* pipe = popen(cmd.str().c_str(), "r");
            if (!pipe) {
                continue;
            }

            char line[1024];
            while (fgets(line, sizeof(line), pipe)) {
                std::cerr << line;
            }

            pclose(pipe);
        }
    }
}
