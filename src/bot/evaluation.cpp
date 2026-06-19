// #include "evaluation.hpp"
// #include "board.hpp"
// #include "enums.hpp"

// namespace chess {
//     int piece_values[6] =  {
//         100,   // PAWN
//         320,   // KNIGHT
//         330,   // BISHOP
//         500,   // ROOK
//         900,   // QUEEN
//         9999   // KING
//     };

//     int eval(const Board &board, Colour player){
//         float eval = 0;
//         uint64_t white = board.get_piece_colour_mask(WHITE);
//         uint64_t black = board.get_piece_colour_mask(BLACK);

//         for (int i = 0; i < 6; i++){
//             uint64_t type = board.get_piece_type_mask(Piece(i));
//             int white_count = __builtin_popcountll(type & white);
//             int black_count = __builtin_popcountll(type & black);

//             eval += (white_count - black_count) * piece_values[i];
//         }

//         if (player == BLACK) eval = -eval;

//         return eval;
//     }
// }

#include "evaluation.hpp"
#include "board.hpp"
#include "enums.hpp"
#include <cstdint>

namespace chess {

int piece_values[6] = {
    100,   // PAWN
    320,   // KNIGHT
    330,   // BISHOP
    500,   // ROOK
    900,   // QUEEN
    9999   // KING
};

// ---------- PIECE-SQUARE TABLES (WHITE PERSPECTIVE) ----------

static const int pawn_pst[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10,-20,-20, 10, 10,  5,
     5, -5,-10,  0,  0,-10, -5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5,  5, 10, 25, 25, 10,  5,  5,
    10, 10, 20, 30, 30, 20, 10, 10,
    50, 50, 50, 50, 50, 50, 50, 50,
     0,  0,  0,  0,  0,  0,  0,  0
};

static const int knight_pst[64] = {
   -50,-40,-30,-30,-30,-30,-40,-50,
   -40,-20,  0,  0,  0,  0,-20,-40,
   -30,  0, 10, 15, 15, 10,  0,-30,
   -30,  5, 15, 20, 20, 15,  5,-30,
   -30,  0, 15, 20, 20, 15,  0,-30,
   -30,  5, 10, 15, 15, 10,  5,-30,
   -40,-20,  0,  5,  5,  0,-20,-40,
   -50,-40,-30,-30,-30,-30,-40,-50
};

static const int bishop_pst[64] = {
   -20,-10,-10,-10,-10,-10,-10,-20,
   -10,  5,  0,  0,  0,  0,  5,-10,
   -10, 10, 10, 10, 10, 10, 10,-10,
   -10,  0, 10, 10, 10, 10,  0,-10,
   -10,  5,  5, 10, 10,  5,  5,-10,
   -10,  0,  5, 10, 10,  5,  0,-10,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -20,-10,-10,-10,-10,-10,-10,-20
};

static const int rook_pst[64] = {
     0,  0,  0,  5,  5,  0,  0,  0,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     5, 10, 10, 10, 10, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

static const int queen_pst[64] = {
   -20,-10,-10, -5, -5,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
     0,  0,  5,  5,  5,  5,  0, -5,
   -10,  5,  5,  5,  5,  5,  0,-10,
   -10,  0,  5,  0,  0,  0,  0,-10,
   -20,-10,-10, -5, -5,-10,-10,-20
};

static const int king_pst[64] = {
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20
};

static inline const int* pst_table(Piece p) {
    switch (p) {
        case PAWN:   return pawn_pst;
        case KNIGHT: return knight_pst;
        case BISHOP: return bishop_pst;
        case ROOK:   return rook_pst;
        case QUEEN:  return queen_pst;
        case KING:   return king_pst;
    }
    return pawn_pst;
}

// flip square for black
static inline int mirror_square(int sq) {
    return sq ^ 56;
}

// ---------- EVAL ----------

int eval(const Board &board, Colour player) {
    int score = 0;

    uint64_t white = board.get_piece_colour_mask(WHITE);
    uint64_t black = board.get_piece_colour_mask(BLACK);

    for (int i = 0; i < 6; i++) {
        Piece p = Piece(i);

        uint64_t type = board.get_piece_type_mask(p);

        uint64_t w = type & white;
        uint64_t b = type & black;

        // material
        score += piece_values[i] * (__builtin_popcountll(w) - __builtin_popcountll(b));

        // PST white
        while (w) {
            int sq = __builtin_ctzll(w);
            score += pst_table(p)[sq];
            w &= w - 1;
        }

        // PST black (mirrored)
        while (b) {
            int sq = __builtin_ctzll(b);
            score -= pst_table(p)[mirror_square(sq)];
            b &= b - 1;
        }
    }

    return (player == WHITE) ? score : -score;
}

}
