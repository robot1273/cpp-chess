#pragma once

#include "board.hpp"

namespace chess{
    /* Super simple eval function that only considers material value */
    float naiive_eval(Board &board);

    float mobility_eval(Board &board);
}   