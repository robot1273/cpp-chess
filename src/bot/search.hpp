#pragma once
#include "./board/board.hpp"
#include <random>

namespace chess{
    extern uint64_t nodes_evaluated;

    struct SearchResult {
        Move best_move;
        int eval;
    };

    int negamax(Board& board, Colour player, int depth, float alpha, float beta, int moves_made);
    SearchResult find_best_move(Board &board, Colour player, int max_depth, int time_limit_ms = -1);
    Move find_random_move(Board &board, Colour player);

    std::mt19937& global_gen();

    // -1 for no seed (do nothing)
    inline void set_global_seed(int seed) {
        if (seed == -1) { return; }
        global_gen().seed(seed);
    }
}
