#include <iostream>
#include <chrono>
#include <iostream>
#include "board.hpp"
#include "enums.hpp"
#include "search.hpp"

int main() {
    chess::Board board;
    board.display_board();

    int i = 0;
    double total_time = 0;

    int seed = -1;
    chess::set_global_seed(seed);

    // 0 = random moves
    int white_bot_depth = 6;
    int black_bot_depth = 5;

    while (true) {
        // zobrist hash sanity check
        chess::Colour player = board.get_current_player();
        uint64_t hash = board.generate_zobrist_hash(player);
        if (hash != board.get_current_hash()) {
            std::cout << "HASH MISMATCH :( " << std::endl;
        }

        std::string strplayer = (player == chess::WHITE) ? "WHITE" : "BLACK";
        std::cout << strplayer << " to play" << std::endl;
        std::cout << "Press Enter to play next move...\n";
        std::cin.get();
        std::cout << "\033[1A\033[2K\r"; //erase whitespace

        chess::Move move;

        auto start_time = std::chrono::high_resolution_clock::now();

        int depth = (player == chess::WHITE) ? white_bot_depth : black_bot_depth;
        if (depth == 0) {
            move = chess::find_random_move(board, player);
        } else {
            move = chess::find_best_move(board, player, depth);
        }

        auto end_time = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        total_time += duration.count();
        std::cout << "Generation took " << duration.count() << " microseconds." << std::endl;
        std::cout << "Current average: " << total_time/(i+1) << " microseconds." << std::endl;

        board.display_board();
        std::cout << std::endl;

        if (!(move.flag() == chess::MoveFlag::NULL_MOVE)){
            std::string uci = chess::moveToUCI(move);
            std::cout << "Move played: " << uci << std::endl;
            board.play_move(move);
        } else {
            if (board.king_in_check(chess::BLACK)) {
                std::cout << "Checkmate, WHITE wins" << std::endl;
                return 0;
            } else if (board.king_in_check(chess::WHITE)) {
                std::cout << "Checkmate, BLACK wins" << std::endl;
                return 0;
            } else {
                std::cout << "fatal error, no moves..." << std::endl;
                if (chess::find_random_move(board, player).flag() == chess::MoveFlag::NULL_MOVE) {
                    std::cout << "really fucking bad error !!! move gen is broken" << std::endl;
                    return 1;
                }
                return 1;
            }
        }

        if (board.is_draw()) {
            std::cout << "Draw" << std::endl;
            return 0;
        }

        i++;
    }

    return 0;
}

/// some silly mostly vibes benvchmark code

// int main() {
//     const int N = 200;
//     const int TRIALS = 100000;

//     chess::Board board;
//     std::vector<double> averages;
//     averages.reserve(TRIALS);

//     std::cout << "Starting benchmark: "
//               << TRIALS << " trials, "
//               << N << " moves each\n";

//     for (int t = 0; t < TRIALS; t++) {
//         board.reset();
//         double total_time = 0;
//         int actual_moves = 0; // Track actual moves in case of early exit

//         for (int i = 0; i < N; i++) {
//             chess::Colour player = chess::Colour(i % 2);

//             auto start = std::chrono::high_resolution_clock::now();
//             chess::Move move = chess::find_random_move(board, player);
//             auto end = std::chrono::high_resolution_clock::now();

//             if (move.flag() == chess::MoveFlag::NULL_MOVE) {
//                 break;
//             }

//             board.play_move(move);

//             total_time += std::chrono::duration_cast<std::chrono::microseconds>(
//                 end - start
//             ).count();
//             actual_moves++;
//         }

//         if (actual_moves > 0) {
//             double avg = total_time / actual_moves; // Divide by actual moves, not N
//             averages.push_back(avg);
//         }
//     }

//     if (averages.empty()) return 0;

//     double sum = 0;
//     double min_v = *std::min_element(averages.begin(), averages.end());
//     double max_v = *std::max_element(averages.begin(), averages.end());

//     for (double v : averages) sum += v;

//     double mean = sum / averages.size();

//     // Standard deviation calculation
//     double variance_sum = 0;
//     for (double v : averages) {
//         variance_sum += (v - mean) * (v - mean);
//     }
//     double std_dev = std::sqrt(variance_sum / (averages.size() - 1));

//     std::cout << "\nFinal stats:\n";
//     std::cout << "Mean avg move time: " << mean << " us\n";
//     std::cout << "Standard Deviation: " << std_dev << " us\n";
//     std::cout << "Min avg: "             << min_v << " us\n";
//     std::cout << "Max avg: "             << max_v << " us\n";

//     return 0;
// }

// #include <iostream>
// #include <chrono>
// #include <vector>
// #include <string>
// #include "board.hpp"
// #include "move.hpp"
// #include "board_precalculation.hpp"

// namespace chess::Test {

//     // Fast, recursive perft worker
//     uint64_t perft(int depth, Board& board, Colour player) {
//         if (depth == 0) return 1ULL;

//         uint64_t nodes = 0;
//         std::vector<Move> moves = board.generate_all_legal_moves(player);
//         Colour next_player = (player == Colour::WHITE) ? Colour::BLACK : Colour::WHITE;

//         for (const auto& move : moves) {
//             UndoMove undo = board.play_move(move);
//             nodes += perft(depth - 1, board, next_player);
//             board.undo_move(undo);
//         }

//         return nodes;
//     }

//     // Top-level divide function to print move-by-move nodes
//     uint64_t perft_divide(int depth, Board& board, Colour player) {
//         if (depth <= 0) return 0;

//         std::cout << "\n========================================\n";
//         std::cout << "  PERFT DIVIDE (Depth " << depth << ")\n";
//         std::cout << "========================================\n";

//         uint64_t total_nodes = 0;
//         std::vector<Move> moves = board.generate_all_legal_moves(player);
//         Colour next_player = (player == Colour::WHITE) ? Colour::BLACK : Colour::WHITE;

//         auto start_time = std::chrono::high_resolution_clock::now();

//         for (const auto& move : moves) {
//             UndoMove undo = board.play_move(move);
//             uint64_t nodes = perft(depth - 1, board, next_player);
//             board.undo_move(undo);

//             total_nodes += nodes;
//             std::cout << moveToUCI(move) << ": " << nodes << "\n";
//         }

//         auto end_time = std::chrono::high_resolution_clock::now();
//         auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

//         std::cout << "----------------------------------------\n";
//         std::cout << "Total Nodes : " << total_nodes << "\n";
//         std::cout << "Time Taken  : " << duration << " ms\n";
//         if (duration > 0) {
//             std::cout << "NPS         : " << (total_nodes * 1000) / duration << "\n";
//         }
//         std::cout << "========================================\n\n";
//         return total_nodes;
//     }
// }

// int main() {
//     // The global precalculation ('initialiser' in your header) has automatically
//     // run before entering this main block.

//     chess::Board board;

//     // Reference values for Chess Starting Position:
//     // Depth 1: 20
//     // Depth 2: 400
//     // Depth 3: 8,902
//     // Depth 4: 197,281
//     // Depth 5: 4,865,609
//     // Depth 6: 119,060,324
//     // Depth 7: 3,195,901,860

//     std::cout << "[!] Starting engine verification testing...\n";

//     const int target_depths[] = {1, 2, 3, 4, 5, 6, 7};
//     const uint64_t expected_nodes[] = {20ULL, 400ULL, 8902ULL, 197281ULL, 4865609ULL, 119060324ULL, 3195901860ULL};

//     for (int i = 0; i < 7; ++i) {
//         int d = target_depths[i];
//         std::cout << "[>] Running Perft at Depth " << d << "... " << std::flush;

//         board.reset();
//         auto start = std::chrono::high_resolution_clock::now();
//         uint64_t result = chess::Test::perft_divide(d, board, chess::Colour::WHITE);
//         auto end = std::chrono::high_resolution_clock::now();

//         auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

//         if (result == expected_nodes[i]) {
//             std::cout << "PASSED (" << result << " nodes in " << ms << " ms)\n";
//         } else {
//             std::cout << "FAILED!\n";
//             std::cout << "    Expected: " << expected_nodes[i] << "\n";
//             std::cout << "    Got     : " << result << "\n";
//             std::cout << "[!] Aborting.\n";
//             return 1;
//         }
//     }

//     std::cout << "\n[+] Success! Move generator matches reference values up to depth 7.\n";
//     return 0;
// }
