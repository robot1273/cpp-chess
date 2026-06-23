#include "search.hpp"

#include "./board/board.hpp"
#include "enums.hpp"
#include "evaluation.hpp"
#include "move.hpp"

#include <cstdint>
#include <random>

namespace chess{
    int INFTY = 10000000;
    int MATE_SCORE = 100000;
    int MATE_THRESHOLD = 99000;

    int DRAW_PENALTY = 10; //positive
    int MAX_QUIESCENCE_DEPTH = 15;

    uint64_t nodes_evaluated = 0;

    static const int TRANSPOSITION_TABLE_SIZE_EXP = 22; // 2^20 entries (HAS TO BE POWER OF 2 for fast idx)
    TranspositionTable transposition_table(TRANSPOSITION_TABLE_SIZE_EXP);

    int quiescence(Board& board, int alpha, int beta, Colour player, int depth = 0) {
        nodes_evaluated++;
        if (depth >= MAX_QUIESCENCE_DEPTH) { return eval(board, player); }
        bool in_check = board.king_in_check(player);
        int best_eval = -INFTY;

        if (!in_check) {
            int static_eval = eval(board, player);
            best_eval = static_eval;
            if (best_eval >= beta) { return beta; }
            if (best_eval > alpha) { alpha = best_eval; }
        }

        MoveList moves;
        if (!in_check) {
            board.generate_all_legal_capture_moves(player, moves);
        } else {
            board.generate_all_legal_moves(player, moves);
            if (moves.empty()) { return -MATE_SCORE + depth; } //checkmate
        }

        order_moves(moves, board);

        for (const Move& move : moves) {
            UndoMove undo = board.play_move(move);
            int eval = -quiescence(board, -beta, -alpha, opposite(player), depth+1);
            board.undo_move(undo);

            if (eval > best_eval) { best_eval = eval; }
            if (eval > alpha) { alpha = eval; }
            if (alpha >= beta) { break; }
        }

        return best_eval;
    }

    int negamax(Board& board, Colour player, int depth, int alpha, int beta, int moves_made, Duration duration = Duration{}){
        nodes_evaluated++;
        if (depth == 0) { return quiescence(board, alpha, beta, player); }
        if (board.is_draw()) { return -DRAW_PENALTY; } // make a draw slightly undesirable

        if ((nodes_evaluated & 2047) == 0) { // mod 2048
            if (duration.time_up()) { return 0; }
        }

        // zugzwang check
        bool has_non_pawn_material = (board.get_piece_colour_mask(player) &
                                    ~(board.get_piece_type_mask(chess::PAWN) |
                                     board.get_piece_type_mask(chess::KING))) != 0;

        if (depth >= 3 && !board.king_in_check(player) && eval(board, player) >= beta && has_non_pawn_material) {
            UndoMove undo_move = board.play_null_move();
            int null_eval = -negamax(board, opposite(player), depth - 1 - 2, -beta, -beta + 1, moves_made + 1, duration);
            board.undo_null_move(undo_move);

            if (null_eval >= beta) {
                return beta;
            }
        }

        int original_alpha = alpha;
        uint64_t hash = board.get_current_hash();
        TranspositionTableEntry* tt_entry = transposition_table.probe(hash);
        if (transposition_table.is_cutoff(tt_entry, depth, alpha, beta)) {
            int tt_score = tt_entry->score; //update eval

            if (tt_score >= MATE_THRESHOLD) tt_score -= moves_made; // mate score re-adjusted
            else if (tt_score <= -MATE_THRESHOLD) tt_score += moves_made;

            return tt_score;
        }

        MoveList moves;
        board.generate_all_legal_moves(player, moves);
        if (moves.empty()) {
            return board.king_in_check(player)
                ? -MATE_SCORE + moves_made // checkmate in n moves
                : -DRAW_PENALTY;           // otherwise, draw (extra check but prob unessesary)
        }

        order_moves(moves, board);
        if (tt_entry != nullptr) { // tt move ordering
            for (int i = 0; i < moves.size(); ++i) {
                if (moves.begin()[i].start() == tt_entry->move.start() && moves.begin()[i].end() == tt_entry->move.end()) {
                    std::swap(moves.begin()[0], moves.begin()[i]); break;
                }
            }
        }

        int best_eval = -INFTY;
        Move best_move = Move{};

        for (const Move& move : moves) {
            UndoMove undo = board.play_move(move);
            int eval = -negamax(board, opposite(player), depth - 1, -beta, -alpha, moves_made + 1);
            board.undo_move(undo);

            if (eval > best_eval) { best_eval = eval; best_move = move; }
            if (eval > alpha) { alpha = eval; }
            if (alpha >= beta) { break; }
        }

        if (duration.time_up()) { return best_eval; } // make sure not to add time up to tt

        int tt_store_score = best_eval;
        if (tt_store_score >= MATE_THRESHOLD) tt_store_score += moves_made; // adjust for mate
        else if (tt_store_score <= -MATE_THRESHOLD) tt_store_score -= moves_made;

        transposition_table.insert(TranspositionTableEntry::new_entry(
            tt_store_score, original_alpha, beta, depth, best_move, hash
        ));

        return best_eval;
    }

    SearchResult find_best_move(Board &board, Colour player, int max_depth, int time_limit_ms) {
        nodes_evaluated = 0;
        MoveList moves;
        board.generate_all_legal_moves(player, moves);
        if (moves.empty()) return SearchResult{Move{}, board.is_draw() ? 0 : -INFTY};

        Move best_move = Move{};
        int best_eval = -INFTY;

        Duration duration(time_limit_ms);

        order_moves(moves, board);

        for (int depth = 1; depth <= max_depth; depth++) {
            Move best_iteration_move = Move{};
            int best_iteration_eval = -INFTY;
            int alpha = -INFTY;
            int beta = INFTY;

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
                if (duration.time_up()) break;

                UndoMove undo = board.play_move(move);
                int eval = -negamax(board, opposite(player), depth - 1, -beta, -alpha, 1, duration);
                board.undo_move(undo);

                if (eval > best_iteration_eval) {
                    best_iteration_eval = eval;
                    best_iteration_move = move;
                }

                alpha = std::max(alpha, eval);
            }

            if (duration.time_up()) break;

            best_move = best_iteration_move;
            best_eval = best_iteration_eval;

            // exit early if forced mate (score is close to mate value)
            if (best_eval >= MATE_THRESHOLD) break;
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
