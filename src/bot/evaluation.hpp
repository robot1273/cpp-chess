#pragma once

#include "board.hpp"

namespace chess{
    extern int piece_values[6];

    /* Super simple eval function that only considers material value */
    int naiive_eval(const Board &board, Colour player);
    int mobility_eval(Board &board);
}
