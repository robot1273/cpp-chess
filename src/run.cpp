#include <iostream>
#include <chrono>
#include <string>
#include <sstream>
#include "board.hpp"
#include "search.hpp"
#include "run.hpp"


// ------------------------------------TERMINAL PLAY MODE ------------------------------------

int terminal_play() {
    chess::Board board;

    std::string fen = "kqqqqqqq/qqqqqqqq/qqqqqqqq/qqqqqqqq/QQQQQQQQ/QQQQQQQQ/QQQQQQQQ/QQQQQQQK w - - 0 1";
    board.load_position(fen);

    std::cout << fen << std::endl;
    std::cout << board.get_fen() << std::endl;

    int i = 0;
    double total_time = 0;

    int seed = -1;
    chess::set_global_seed(seed);

    // 0 = random moves
    int white_bot_depth = 6;
    int black_bot_depth = 6;

    int human = -1; // -1 = bot v bot, 0 = white, 1 = black, 2 = both
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
            chess::Move move;
            if (depth == 0) {
                move = chess::find_random_move(board, player);
            } else {
                move = chess::find_best_move(board, player, depth);
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

            total_time += duration.count();
            std::cout << "Generation took " << duration.count() << " microseconds." << std::endl;
            std::cout << "Current average: " << total_time / (i + 1) << " microseconds." << std::endl;

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

#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include "board.hpp"
#include "search.hpp"

int CUTECHESS_DEPTH = 7;

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
            go_command(board);
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

void CuteChessAPI::go_command(chess::Board& board) {
    chess::Move best_move = chess::find_best_move(board, board.get_current_player(), CUTECHESS_DEPTH);
    std::cout << "bestmove " << chess::moveToUCI(best_move) << std::endl;
}
