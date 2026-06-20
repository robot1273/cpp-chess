#pragma once

#include "board.hpp"

namespace chess{
    constexpr int piece_values[6] = { 100, 330, 360, 500, 900,  0}; //for move ordering
    int eval(const Board& board, Colour player);
}
