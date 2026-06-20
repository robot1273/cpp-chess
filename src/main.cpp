#include "run.hpp"

enum Process {
    TERMINAL_PLAY,  // play in-terminal
    CUTECHESS,      // cutechess binary
    BENCHMARK,      // benchmark bot performance
    PERFT,          // benchmark and test suite for engine performance
};

Process process = CUTECHESS;

int main() {
    switch (process) {
        case TERMINAL_PLAY:
            return terminal_play();
        case CUTECHESS:
            return CuteChessAPI::run();
        case BENCHMARK:
            return benchmark(9);
        case PERFT:
            return run_perft();
        default:
            return 1;
    }
    return CuteChessAPI::run();
}
