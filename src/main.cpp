#include "run.hpp"

// 0 - > terminal play
// 1 - > cutechess
// 2 - > benchmark
enum Process {
    TERMINAL_PLAY,
    CUTECHESS,
    BENCHMARK
};

Process process = Process::CUTECHESS;

int main() {
    switch (process) {
        case 0:
            return terminal_play();
        case 1:
            return CuteChessAPI::run();
        case 2:
            return benchmark(7);
        default:
            return 1;
    }
    return CuteChessAPI::run();
}
