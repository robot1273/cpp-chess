#pragma once
#include "./board/board.hpp"

namespace chess{
    float minimax(Board& board, Colour player, int depth);
    Move find_best_move(Board &board, Colour player, int depth);
    Move find_random_move(Board &board, Colour player);
}