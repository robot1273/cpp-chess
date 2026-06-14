#pragma once

#include <cstdint> //for uint64
#include <vector>

namespace chess
{
    std::vector<std::vector<uint64_t>> generate_attack_board(bool is_rook);
    std::vector<uint64_t> generate_magic_numbers(bool is_rook, std::vector<std::vector<uint64_t>>& attack_table);
    uint64_t get_sliding_piece_mask(int piece_idx, bool is_rook);
}
