#pragma once

#include <cstdint>
#include "enums.hpp"

namespace chess {
    enum MoveFlag : uint8_t {
        NONE_FLAG       = 0,
        EN_PASSANT      = 1, 
        PROMOTE_KNIGHT  = 2, //promotion - 1 = corresponding piece enum
        PROMOTE_BISHOP  = 3,
        PROMOTE_ROOK    = 4,
        PROMOTE_QUEEN   = 5,
        CASTLE_KINGSIDE = 6,
        CASTLE_QUEENSIDE= 7
    };

    inline bool isPromotion(MoveFlag flag){
        return (flag >= MoveFlag::PROMOTE_KNIGHT && flag <= MoveFlag::PROMOTE_QUEEN);
    }

    struct Move{
        uint16_t move = 0;
        Move(int start, int end){ move = start | (end << 6); }
        Move(int start, int end, MoveFlag flag){ move = start | (end << 6) | (flag << 12); }
        inline int start() const { return move & 0x3F; }
        inline int end() const { return (move >> 6) & 0x3F; } 
        inline MoveFlag flag() const { return MoveFlag(move >> 12); }
    };

    struct UndoMove {
        Move move;
        Piece captured_piece;
        uint64_t en_passant_state;
        uint8_t castling_rights_state;
        UndoMove(Move m, Piece cp, uint64_t enpassant, uint8_t castling_rights) :
             move(m), captured_piece(cp), en_passant_state(enpassant), castling_rights_state(castling_rights) {}
    };
}
