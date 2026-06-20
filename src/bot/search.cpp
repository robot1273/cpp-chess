#include "search.hpp"

#include "./board/board.hpp"
#include "enums.hpp"
#include "evaluation.hpp"
#include "move.hpp"

#include <algorithm>
#include <random>
#include <chrono>

namespace chess{
    constexpr int infinity = 10000000;
    constexpr int mate_score = 100000;
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

        MoveList moves;
        board.generate_all_legal_capture_moves(player, moves);
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


        MoveList moves;
        board.generate_all_legal_moves(player, moves);
        if (moves.empty()) {
            if (board.king_in_check(player)) {
                return -mate_score + moves_made; // checkmate in depth moves
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

    inline bool time_up(int limit_ms, std::chrono::steady_clock::time_point start) {
        if (limit_ms == -1) return false;
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >= limit_ms;
    }

    SearchResult find_best_move(Board &board, Colour player, int max_depth, int time_limit_ms) {
        MoveList moves;
        board.generate_all_legal_moves(player, moves);
        if (moves.empty()) return SearchResult{Move{}, board.is_draw() ? 0 : -infinity};

        Colour enemy = (player == WHITE) ? BLACK : WHITE;
        Move best_move = Move{};
        int best_eval = -infinity;

        auto start = std::chrono::steady_clock::now();

        std::sort(moves.begin(), moves.end(), MVVLVAMoveOrder{board});

        for (int depth = 1; depth <= max_depth; depth++) {
            Move best_iteration_move = Move{};
            int best_iteration_eval = -infinity;
            int alpha = -infinity;
            int beta = infinity;

            // get best move from previous iteration to front
            if (depth > 1) {
                for (int i = 0; i < moves.size(); ++i) {
                    if (moves.begin()[i].start() == best_move.start() && moves.begin()[i].end() == best_move.end()) {
                        std::swap(moves.begin()[0], moves.begin()[i]);
                        break;
                    }
                }
            }

            for (Move move : moves) {
                if (time_up(time_limit_ms, start)) break;

                UndoMove undo = board.play_move(move);
                int eval = -negamax(board, enemy, depth - 1, -beta, -alpha, 1);
                board.undo_move(undo);

                if (eval > best_iteration_eval) {
                    best_iteration_eval = eval;
                    best_iteration_move = move;
                }

                alpha = std::max(alpha, eval);
            }

            if (time_up(time_limit_ms, start)) break;

            best_move = best_iteration_move;
            best_eval = best_iteration_eval;

            // exit early if forced mate (score is close to mate value)
            if (best_eval >= mate_score * 0.9) break;
        }

        return {best_move, best_eval};
    }



    std::mt19937& global_gen() {
        static std::mt19937 gen(std::random_device{}());
        return gen;
    }

    Move find_random_move(Board& board, Colour player) {
        MoveList moves;
        board.generate_all_legal_moves(player, moves);
        if (moves.empty()) return Move{};

        std::uniform_int_distribution<> distrib(0, moves.size() - 1);
        return moves.get(distrib(global_gen()));
    }
}
