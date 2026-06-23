#include <cstdint>
#include <iostream>
#include <chrono>
#include <string>
#include <sstream>
#include <vector>
#include "board.hpp"
#include "search.hpp"
#include "run.hpp"

int CUTECHESS_MAX_DEPTH = -1; // -negative for (virtually) infinite depth
int FALLBACK_TIME_LIMIT_MS = 5000;

// ------------------------------------TERMINAL PLAY MODE ------------------------------------

// -1 = bot v bot, 0 = white, 1 = black, 2 = both
int terminal_play(int human) {
    chess::Board board;

    int i = 0;
    double total_time = 0;

    int seed = -1;
    chess::set_global_seed(seed);

    // 0 = random moves
    int white_bot_depth = 6;
    int black_bot_depth = 6;

    bool fancy_print = true;

    board.display_board(fancy_print);

    while (true) {
        // zobrist hash sanity check
        chess::Colour player = board.get_current_player();
        if (board.generate_zobrist_hash(player) != board.get_current_hash()) {
            std::cout << "HASH MISMATCH :( " << std::endl;
        }

        std::string strplayer = (player == chess::WHITE) ? "WHITE" : "BLACK";
        std::cout << "\n" << strplayer << " to play" << std::endl;

        if (human == 2 || human == player) {
            bool move_successful = false;
            while (!move_successful) {
                std::cout << "Type move here: ";
                std::string move_str;
                std::getline(std::cin, move_str);

                try {
                    board.play_move(move_str);
                    std::cout << "Move played: " << move_str << std::endl;
                    move_successful = true;
                } catch (const std::exception& e) {
                    std::cout << e.what() << " Try again.\n";
                }
            }
        } else {
            std::cout << "Press Enter to let bot play...\n";
            std::cin.get();
            std::cout << "\033[1A\033[2K\r";

            auto start_time = std::chrono::high_resolution_clock::now();

            int depth = (player == chess::WHITE) ? white_bot_depth : black_bot_depth;
            chess::SearchResult result;
            if (depth == 0) {
                result.best_move = chess::find_random_move(board, player);
                result.eval = 0;
            } else {
                result = chess::find_best_move(board, player, depth);
            }

            chess::Move move = result.best_move;
            int eval = result.eval;

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

            total_time += duration.count();
            std::cout << "Generation took " << duration.count() << " microseconds." << std::endl;
            std::cout << "Current average: " << total_time / (i + 1) << " microseconds." << std::endl;
            std::cout << "Evaluation: " << eval << std::endl;

            if (move.flag() != chess::MoveFlag::NULL_MOVE) {
                std::string uci = chess::moveToUCI(move);
                std::cout << "Move played: " << uci << std::endl;
                board.play_move(move);
            } else {
                if (board.king_in_check(chess::BLACK)) {
                    std::cout << "Checkmate, WHITE wins" << std::endl;
                } else if (board.king_in_check(chess::WHITE)) {
                    std::cout << "Checkmate, BLACK wins" << std::endl;
                } else {
                    std::cout << "Stalemate" << std::endl;
                }
                return 0;
            }
        }

        board.display_board(fancy_print);

        if (board.is_draw()) {
            std::cout << "Draw" << std::endl;
            return 0;
        }

        i++;
    }

    return 0;
}

// ------------------------------------ ENGINE BENCHMARKS ------------------------------------

int benchmark(int bot_depth) {
    std::vector<std::string> test_fens = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", // start pos
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", // kiwipete
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", // endgame
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", // middlegame
    };

    uint64_t total_nodes = 0;
    double total_time_sec = 0.0;

    std::cout << "benchmarking at bot depth " << bot_depth << "\n";

    for (size_t i = 0; i < test_fens.size(); i++) {
        std::cout << "testing FEN position no. " << i + 1 << " (" << test_fens[i] << ")\n";

        chess::Board board;
        board.load_position(test_fens[i]);

        chess::nodes_evaluated = 0;

        auto start = std::chrono::high_resolution_clock::now();
        chess::find_best_move(board, board.get_current_player(), bot_depth);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> diff = end - start;

        double time_taken = diff.count();
        uint64_t nodes = chess::nodes_evaluated;

        total_nodes += nodes;
        total_time_sec += time_taken;

        int nps = (time_taken > 0.0) ? static_cast<int>(nodes / time_taken) : 0;

        std::cout << "Pos " << i + 1 << " | Nodes: " << nodes
                  << " | Time: " << time_taken << "s | NPS: " << nps << "\n";
    }

    int overall_nps = (total_time_sec > 0.0) ? static_cast<int>(total_nodes / total_time_sec) : 0;

    std::cout << "Total Nodes: " << total_nodes << "\n";
    std::cout << "Total Time:  " << total_time_sec << "s\n";
    std::cout << "Overall NPS: " << overall_nps << "\n\n";

    return 0;
}

// ------------------------------------ PERFT TESTING ------------------------------------

uint64_t perft(chess::Board& board, int depth) {
    if (depth == 0) return 1ULL;

    uint64_t nodes = 0;

    chess::MoveList legal_moves;
    board.generate_all_legal_moves(board.get_current_player(), legal_moves);

    for (const chess::Move& move : legal_moves) {
        chess::UndoMove undo = board.play_move(move);
        nodes += perft(board, depth - 1);
        board.undo_move(undo);
    }

    return nodes;
}

std::vector<uint64_t> test_perft(const std::string& fen, int depth, uint64_t expected_nodes) {
    chess::Board board;
    if (fen == "startpos") {
        board.reset();
    } else {
        board.load_position(fen);
    }

    std::cout << "Running perft on " << fen << ", depth " << depth << "\n";

    auto start_time = std::chrono::high_resolution_clock::now();
    uint64_t actual_nodes = perft(board, depth);
    auto end_time = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    if (actual_nodes != expected_nodes) {
        std::string error_msg = "PERFT FAILED expected " + std::to_string(expected_nodes) + " but got " + std::to_string(actual_nodes);
        // std::cout << error_msg << std::endl;
        throw std::runtime_error(error_msg);
    }

    uint64_t time_passed = duration.count();
    uint64_t nps = (time_passed > 0) ? (actual_nodes * 1000000ULL) / time_passed : 0;

    std::cout << "PERFT PASSED! Nodes: " << actual_nodes << " | Time: " << time_passed << "us | NPS: " << nps << "\n\n";
    return {time_passed, actual_nodes};
}

int run_perft() {
    uint64_t total_time = 0;
    uint64_t nodes_processed = 0;

    auto run_test = [&total_time, &nodes_processed](const std::string& fen, int depth, uint64_t expected_nodes) {
        std::vector<uint64_t> result = test_perft(fen, depth, expected_nodes);
        total_time += result[0];
        nodes_processed += result[1];
    };

    try {
        // start pos
        std::string start_pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        std::string kiwipete = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
        std::string endgame_pos = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
        std::string edgecase_pos = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";

        run_test(start_pos, 4, 197281);
        run_test(start_pos, 5, 4865609);
        run_test(start_pos, 6, 119060324);

        run_test(kiwipete, 4, 4085603);
        run_test(kiwipete, 5, 193690690);

        run_test(endgame_pos, 4, 43238);
        run_test(endgame_pos, 5, 674624);

        run_test(edgecase_pos, 1, 44);
        run_test(edgecase_pos, 2, 1486);
        run_test(edgecase_pos, 3, 62379);
        run_test(edgecase_pos, 4, 2103487);
        run_test(edgecase_pos, 5, 89941194);

        uint64_t nps = (total_time > 0) ? (nodes_processed * 1000000ULL) / total_time : 0;
        std::cout << "Completed test suite successfully." << "\n";
        std::cout << "Overall NPS: "  << nps << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n" << e.what() << "\nMOVE GENERATOR IS BROKEN.\n";
        return 1;
    }

}


// ------------------------------------ CUTE CHESS API ------------------------------------

int CuteChessAPI::run() {
    chess::Board board;
    std::string line;

    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "uci") {
            uci_command();
        } else if (command == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (command == "ucinewgame") {
            board.reset();
        } else if (command == "position") {
            position_command(iss, board);
        } else if (command == "go") {
            go_command(iss, board);
        } else if (command == "quit") {
            break;
        }
    }
    return 0;

};

void CuteChessAPI::uci_command() {
    std::cout << "id name kutays_demise" << std::endl;
    std::cout << "id author bobot_chan" << std::endl;
    std::cout << "uciok" << std::endl;
}

void CuteChessAPI::position_command(std::istringstream& iss, chess::Board& board) {
    std::string token;
    iss >> token;

    if (token == "startpos") {
        board.reset();
        iss >> token;
    } else if (token == "fen") {
        std::string fen = "";
        while (iss >> token && token != "moves") {
            fen += token + " ";
        }
        board.load_position(fen);
    }

    if (token == "moves") {
        std::string move_uci;
        while (iss >> move_uci) {
            try {
                board.play_move(move_uci);
            } catch (const std::exception& e) {
                std::cerr << "ERROR: " << e.what() << std::endl;
            }
        }
    }
}

/* helper to convert an evaluation score into num mates if mate, otherwise standard eval
 * outputs "mate <moves till mate>" if mate, otherwise "cp <eval>"
 */
std::string uci_eval_to_mate_string(int eval) {
    if (std::abs(eval) >= chess::MATE_THRESHOLD) { // if mate
        int mate = (chess::MATE_SCORE - std::abs(eval) + 1) / 2; // no. moves till mate
        if (eval < 0) { mate = -mate; } // if black player
        return "mate " + std::to_string(mate);
    }
    return "cp " + std::to_string(eval);
}


void CuteChessAPI::go_command(std::istringstream& iss, chess::Board& board) {
    std::string token;

    int movetime = -1;
    int depth = CUTECHESS_MAX_DEPTH;
    int wtime = -1, btime = -1, winc = 0, binc = 0;

    while (iss >> token) { //parse
        if (token == "depth") iss >> depth;
        else if (token == "movetime") iss >> movetime;
        else if (token == "wtime") iss >> wtime;
        else if (token == "btime") iss >> btime;
        else if (token == "winc") iss >> winc;
        else if (token == "binc") iss >> binc;
    }

    int time_limit = FALLBACK_TIME_LIMIT_MS; // fallback limit

    // calculate good time limit based on remaining time and increment
    if (movetime > 0) {
        time_limit = movetime; // forced move time
    } else {
        chess::Colour player = board.get_current_player();
        int my_time = (player == chess::WHITE) ? wtime : btime;
        int my_inc = (player == chess::WHITE) ? winc : binc;

        if (my_time > 0) { // standard time control as detailed on chess programming wiki
            int lag = 50; // reduce thinking time in case of lag
            time_limit = (my_time / 20) + (my_inc / 2) - lag;
            if (time_limit < lag) time_limit = 50; // minimum thinking time of 50ms
        }
    }

    if (depth < 0) { depth = 256; }

    auto start = std::chrono::steady_clock::now();
    chess::SearchResult result = chess::find_best_move(board, board.get_current_player(), depth, time_limit);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    int elapsed_ms = static_cast<int>(elapsed.count());

    uint64_t nps = (elapsed_ms > 0 ? chess::nodes_evaluated * 1000ULL / elapsed_ms : 0);

    std::cout
        << "info"
        << " depth " << result.depth_reached
        << " score " << uci_eval_to_mate_string(result.eval)
        << " time " << elapsed_ms
        << " nodes " << chess::nodes_evaluated
        << " nps " << nps
        << std::endl;

    std::cout << "bestmove " << chess::moveToUCI(result.best_move) << std::endl;

}
