#include "search.hpp"

#include "./board/board.hpp"
#include "enums.hpp"
#include "evaluation.hpp"
#include "move.hpp"

#include <algorithm>
#include <random>
#include <iostream>

namespace chess{
    constexpr int infinity = 1000067;
    constexpr int draw_penalty = 10; //positive
    constexpr int max_quiescence_depth = 15;

    uint64_t nodes_evaluated = 0;

    inline int mvv_lva_score(const Move& m, const Board& board) {
        int to = m.end();
        int from = m.start();

        Piece victim = board.get_piece_type(to);
        Piece attacker = board.get_piece_type(from);

        if (victim == NONE) return -1; // non-captures last

        return 1000 * piece_values[victim] - piece_values[attacker];
    }

    struct MVVLVAMoveOrder {
        const Board& board;

        bool operator()(const Move& a, const Move& b) const {
            return mvv_lva_score(a, board) > mvv_lva_score(b, board);
        }
    };

    int quiescence(Board& board, int alpha, int beta, Colour player, int depth = 0) {
        nodes_evaluated++;
        if (depth == 0 || depth >= max_quiescence_depth) { return eval(board, player); }

        int static_eval = eval(board, player);
        int best_eval = static_eval;
        if (best_eval >= beta) { return beta; }
        if (best_eval > alpha) { alpha = best_eval; }

        auto moves = board.generate_all_legal_capture_moves(player);
        std::sort(moves.begin(), moves.end(), MVVLVAMoveOrder{board});
        Colour enemy = (player == WHITE) ? BLACK : WHITE;

        for (const Move& move : moves) {
            UndoMove undo = board.play_move(move);
            int eval = -quiescence(board, -beta, -alpha, enemy, depth+1);
            board.undo_move(undo);

            if (eval > best_eval) { best_eval = eval; }
            if (eval > alpha) { alpha = eval; }
            if (alpha >= beta) { break; }
        }

        return best_eval;
    }

    int negamax(Board& board, Colour player, int depth, int alpha, int beta, int moves_made){
        nodes_evaluated++;
        if (depth == 0) { return quiescence(board, alpha, beta, player); }
        if (board.is_draw()) { return -draw_penalty; } // make a draw slightly undesirable

        auto moves = board.generate_all_legal_moves(player);
        if (moves.empty()) {
            if (board.king_in_check(player)) {
                return -100000 + moves_made; // checkmate in depth moves
            }
            return -draw_penalty;  // stalemate
        }

        std::sort(moves.begin(), moves.end(), MVVLVAMoveOrder{board});
        Colour enemy = (player == WHITE) ? BLACK : WHITE;

        int best_eval = -infinity;

        for (const Move& move : moves) {
            UndoMove undo = board.play_move(move);
            int eval = -negamax(board, enemy, depth - 1, -beta, -alpha, moves_made + 1);
            board.undo_move(undo);

            if (eval > best_eval) { best_eval = eval; }
            if (eval > alpha) { alpha = eval; }
            if (alpha >= beta) { break; }
        }
        return best_eval;
    }

    Move find_best_move(Board &board, Colour player, int depth) {
        std::vector<Move> moves = board.generate_all_legal_moves(player);
        if (moves.empty()) return Move{};

        Colour enemy = (player == WHITE) ? BLACK : WHITE;
        Move best_move = Move{};

        int best_eval = -infinity;
        int alpha = -infinity;
        int beta = infinity;

        std::sort(moves.begin(), moves.end(), MVVLVAMoveOrder{board});
        for (Move move : moves) {
            UndoMove undo = board.play_move(move);
            int eval = -negamax(board, enemy, depth - 1, -beta, -alpha, 1);
            board.undo_move(undo);

            if (eval > best_eval) {
                best_eval = eval;
                best_move = move;
            }

            alpha = std::max(alpha, eval);
        }

        std::cout << "Eval: " << best_eval << std::endl;
        return best_move;
    }

    std::mt19937& global_gen() {
        static std::mt19937 gen(std::random_device{}());
        return gen;
    }

    Move find_random_move(Board& board, Colour player) {
        std::vector<Move> moves = board.generate_all_legal_moves(player);
        if (moves.empty()) return Move{};

        std::uniform_int_distribution<> distrib(0, moves.size() - 1);
        return moves[distrib(global_gen())];
    }
}
