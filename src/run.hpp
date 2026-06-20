#include "board.hpp"
#include <sstream>

namespace CuteChessAPI {
    int run();
    void uci_command();
    void position_command(std::istringstream& iss, chess::Board& board);
    void go_command(std::istringstream& iss, chess::Board& board);
};

int terminal_play();
int benchmark(int bot_depth);
int run_perft();
