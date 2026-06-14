#include "search.hpp"

#include "./board/board.hpp"
#include "evaluation.hpp"

#include <algorithm>
#include <limits>
#include <random>
#include <iostream>

namespace chess{
    float infinity = std::numeric_limits<float>::infinity();

    float minimax(Board& board, Colour player, int depth){
        if (depth == 0) { return mobility_eval(board); }
        
        float value = (player == WHITE) ? -infinity : infinity;

        for (Move move : board.generate_all_legal_moves(player)){
            UndoMove undo = board.play_move(move);
            if (player == WHITE){
                value = std::max(value, minimax(board, BLACK, depth-1));
            } else{
                value = std::min(value, minimax(board, WHITE, depth-1));
            } 
            board.undo_move(undo);
        }
        return value;
    }

    Move find_best_move(Board &board, Colour player, int depth){
        Move best_move = Move();
        float best_eval = (player == WHITE) ? -infinity : infinity;
        Colour enemy = (player == WHITE) ? BLACK : WHITE;

        for (Move move : board.generate_all_legal_moves(player)){
            UndoMove undo = board.play_move(move);
            float eval = minimax(board, enemy, depth - 1);
            board.undo_move(undo);

            if (((player == WHITE) && (eval > best_eval)) || ((player == BLACK) && (eval < best_eval))){
                best_eval = eval;
                best_move = move;
            }
        }

        return best_move;
    }

    std::mt19937& global_gen() {
        static std::mt19937 gen(std::random_device{}());
        return gen;
    }

    Move find_random_move(Board& board, Colour player) {
        std::vector<Move> moves = board.generate_all_legal_moves(player);
        if (moves.empty()) throw std::runtime_error("No moves");

        std::uniform_int_distribution<> distrib(0, moves.size() - 1);
        return moves[distrib(global_gen())];
    }
} 
