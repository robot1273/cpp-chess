#include "evaluation.hpp"
#include "board.hpp"
#include "enums.hpp"

namespace chess {
    int piece_values[6] =  {
        100,   // PAWN
        320,   // KNIGHT
        330,   // BISHOP
        500,   // ROOK
        900,   // QUEEN
        9999   // KING
    };

    float mobility_weight = 0.1;

    int naiive_eval(const Board &board, Colour player){
        float eval = 0;
        uint64_t white = board.get_piece_colour_mask(WHITE);
        uint64_t black = board.get_piece_colour_mask(BLACK);

        for (int i = 0; i < 6; i++){
            uint64_t type = board.get_piece_type_mask(Piece(i));
            int white_count = __builtin_popcountll(type & white);
            int black_count = __builtin_popcountll(type & black);

            eval += (white_count - black_count) * piece_values[i];
        }

        if (player == BLACK) eval = -eval;

        return eval;
    }

    int mobility_eval(Board &board){
        float eval = 0;
        uint64_t white = board.get_piece_colour_mask(WHITE);
        uint64_t black = board.get_piece_colour_mask(BLACK);

        for (int i = 0; i < 6; i++){
            uint64_t type = board.get_piece_type_mask(Piece(i));
            int white_count = __builtin_popcountll(type & white);
            int black_count = __builtin_popcountll(type & black);

            eval += (white_count - black_count) * piece_values[i];
        }

        int white_moves = board.generate_all_legal_moves(WHITE).size();
        int black_moves = board.generate_all_legal_moves(BLACK).size();
        int check = board.king_in_check(WHITE) ? -1 : 1;

        eval += mobility_weight * (white_moves - black_moves) + check;

        return eval;
    }
}
