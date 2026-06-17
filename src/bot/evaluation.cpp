#include "evaluation.hpp"
#include "board.hpp"

namespace chess {
    float piece_values[6] =  {
        1.0,   // PAWN
        3.0,   // KNIGHT
        3.0,   // BISHOP
        5.0,   // ROOK
        9.0,   // QUEEN
        9999.0  // KING
    };

    float mobility_weight = 0.1;

    float naiive_eval(const Board &board){
        Board board_copy = board;

        float eval = 0;
        uint64_t white = board_copy.get_piece_colour(WHITE);
        uint64_t black = board_copy.get_piece_colour(BLACK);

        for (int i = 0; i < 6; i++){
            uint64_t type = board_copy.get_piece_type(Piece(i));
            int white_count = __builtin_popcountll(type & white);
            int black_count = __builtin_popcountll(type & black);

            eval += (white_count - black_count) * piece_values[i];
        }

        return eval;
    }

    float mobility_eval(const Board &board){
        Board board_copy = board;

        float eval = 0;
        uint64_t white = board_copy.get_piece_colour(WHITE);
        uint64_t black = board_copy.get_piece_colour(BLACK);

        for (int i = 0; i < 6; i++){
            uint64_t type = board_copy.get_piece_type(Piece(i));
            int white_count = __builtin_popcountll(type & white);
            int black_count = __builtin_popcountll(type & black);

            eval += (white_count - black_count) * piece_values[i];
        }


        int white_moves = board_copy.generate_all_legal_moves(WHITE).size();
        int black_moves = board_copy.generate_all_legal_moves(BLACK).size();

        eval += mobility_weight * (white_moves - black_moves);

        return eval;
    }
}
