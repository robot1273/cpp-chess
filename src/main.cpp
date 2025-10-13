#include <iostream>
#include <vector>
#include <random>
#include <chrono>

#include "board.hpp"

int main() {
    chess::Board board;
    board.display_board();
    
    std::random_device rd;
    std::mt19937 gen(rd());

    int i = 0;
    double total_time = 0;

    while (true) {
        std::cout << "Press Enter to play a random white move..." << chess::Colour(i % 2) << std::endl;
        std::cin.get();

        auto start_time = std::chrono::high_resolution_clock::now();

        std::vector<chess::Move> moves = board.generate_all_legal_moves(chess::Colour(i % 2));

        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        total_time += duration.count();
        std::cout << "Generation took " << duration.count() << " microseconds." << std::endl;
        std::cout << "Current average: " << total_time/(i+1) << " microseconds." << std::endl;

        if (!moves.empty()) {
            std::uniform_int_distribution<> distrib(0, moves.size() - 1);
            chess::Move random_move = moves[distrib(gen)];
            
            std::string uci = chess::moveToUCI(random_move);
            std::cout << "Move played: " << uci << std::endl;
            
            board.play_move(random_move);
        } else {
            std::cout << "No legal moves!" << std::endl;
            return 0;
        }

        board.display_board();
        std::cout << std::endl;
        i++;
    }

    return 0;
}
