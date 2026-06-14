#include <iostream>
#include <chrono>

#include "board.hpp"
#include "search.hpp"
#include "evaluation.hpp"

int main() {
    chess::Board board;
    board.display_board();

    int i = 0;
    double total_time = 0;

    int max_depth = 1;
        
    while (true) {
        chess::Colour player = chess::Colour(i % 2);
        std::cout << "Press Enter to play a random white move..." << player << std::endl;
        std::cin.get();

        auto start_time = std::chrono::high_resolution_clock::now();

        //chess::Move move = chess::find_best_move(board, player, max_depth);
        chess::Move move = chess::find_random_move(board, player);

        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        total_time += duration.count();
        std::cout << "Generation took " << duration.count() << " microseconds." << std::endl;
        std::cout << "Current average: " << total_time/(i+1) << " microseconds." << std::endl;
        
        if (!(move.flag() == chess::MoveFlag::NULL_MOVE)){
            std::string uci = chess::moveToUCI(move);
            std::cout << "Move played: " << uci << std::endl;
            board.play_move(move);
            
        } else {
            std::cout << "No valid moves"  << std::endl;
            return 0;
        }

        board.display_board();
        std::cout << "Evaluation: " << chess::mobility_eval(board) << std::endl;
        std::cout << std::endl;
        i++;
    }

    return 0;
}
