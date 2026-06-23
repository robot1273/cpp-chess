#pragma once
#include "./board/board.hpp"
#include "evaluation.hpp"
#include <cstdint>
#include <random>
#include <algorithm>
#include <stdexcept>
#include <chrono>

namespace chess{
    extern uint64_t nodes_evaluated;
    extern int MATE_SCORE;
    extern int MATE_THRESHOLD;

    extern int DRAW_PENALTY;
    extern int MAX_QUIESCENCE_DEPTH;

    enum NodeType { // forgive the docs, this is kind of hard to wrap my head around lol
        EXACT,          // eval is between alpha and beta       -> store exact score
        LOWER_BOUND,    // eval is more than or equal to beta   -> store lower bound score (move so good opponent wont consider it)
        UPPER_BOUND     // eval is less than or equal to alpha  -> store upper bound score (move so bad we dont consider it)
    };

    struct TranspositionTableEntry {
        int depth;
        int score;
        Move move;
        NodeType type;
        uint64_t zobrist_hash;

        static TranspositionTableEntry new_entry(int score, int alpha, int beta, int depth, Move move, uint64_t zobrist_hash) {
            NodeType type = EXACT;
            if (score <= alpha) { type = UPPER_BOUND; }
            if (score >= beta) { type = LOWER_BOUND; }
            return TranspositionTableEntry{depth, score, move, type, zobrist_hash};
        }
    };

    struct SearchResult {
        Move best_move;
        int eval;
    };

    struct TranspositionTable {
        const size_t TT_SIZE;
        TranspositionTableEntry* entries;

        TranspositionTable(size_t exponent) :
            TT_SIZE(1 << exponent),  // size must be a power of 2 for fast idx, hence why we do this
            entries(new TranspositionTableEntry[1 << exponent]()) {}

        ~TranspositionTable() { delete[] entries; }

        inline size_t index(uint64_t hash) const { return hash & (TT_SIZE - 1); } // fast idx
        inline TranspositionTableEntry* get(uint64_t hash) const { return &entries[index(hash)]; }
        inline void insert(const TranspositionTableEntry& entry) { entries[index(entry.zobrist_hash)] = entry; }

        /*
         * return entry if found hash, nullptr otherwise
         */
        inline TranspositionTableEntry* probe(uint64_t hash) {
            TranspositionTableEntry* entry = get(hash);
            if (entry->zobrist_hash == hash) return entry;
            return nullptr;
        }

        /*
         * returns true if entry should be used for search
         * false if we need to keep searching (and later update TT)
         */
        inline bool is_cutoff(TranspositionTableEntry* entry, int depth, int alpha, int beta) {
            if (entry == nullptr || entry->depth < depth) return false;

            switch (entry->type) {
                case EXACT: return true;
                case LOWER_BOUND: return entry->score >= beta;
                case UPPER_BOUND: return entry->score <= alpha;
                default: throw std::runtime_error("Invalid node type");
            }
        }

    };

    inline Colour opposite(Colour c) {
        return c == WHITE ? BLACK : WHITE;
    }

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

    inline void order_moves(MoveList& moves, const Board& board) {
        std::sort(moves.begin(), moves.end(), MVVLVAMoveOrder{board});
    }

    struct Duration {
        const std::chrono::steady_clock::time_point start;
        const int limit_ms;
        Duration(int limit_ms = -1) : start(std::chrono::steady_clock::now()), limit_ms(limit_ms) {}
        inline bool time_up() const { return limit_ms != -1 && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() >= limit_ms; }
    };

    int negamax(Board& board, Colour player, int depth, float alpha, float beta, int moves_made, std::chrono::steady_clock::time_point start, int time_limit_ms = 0);
    SearchResult find_best_move(Board &board, Colour player, int max_depth, int time_limit_ms = -1);
    Move find_random_move(Board &board, Colour player);

    std::mt19937& global_gen();

    // -1 for no seed (do nothing)
    inline void set_global_seed(int seed) {
        if (seed == -1) { return; }
        global_gen().seed(seed);
    }
}
