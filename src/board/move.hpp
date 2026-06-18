#pragma once

#include <cstdint>
#include <string>
#include "enums.hpp"

namespace chess {
    enum MoveFlag : uint8_t {
        NONE_FLAG        = 0,
        EN_PASSANT       = 1,
        PROMOTE_KNIGHT   = 2, //promotion - 1 = corresponding piece enum
        PROMOTE_BISHOP   = 3,
        PROMOTE_ROOK     = 4,
        PROMOTE_QUEEN    = 5,
        CASTLE_KINGSIDE  = 6,
        CASTLE_QUEENSIDE = 7,
        NULL_MOVE        = 8,
    };

    inline bool isPromotion(MoveFlag flag){
        return (flag >= MoveFlag::PROMOTE_KNIGHT && flag <= MoveFlag::PROMOTE_QUEEN);
    }

    inline Piece promotionPiece(MoveFlag flag){ // assumes flag is a promotion flag
        return static_cast<Piece>(static_cast<int>(flag) - 1);
    }

    struct Move{
        uint16_t move = 0;
        Move() {move = NULL_MOVE << 12;}
        Move(int start, int end){ move = start | (end << 6); }
        Move(int start, int end, MoveFlag flag){ move = start | (end << 6) | (flag << 12); }
        inline int start() const { return move & 0x3F; }
        inline int end() const { return (move >> 6) & 0x3F; }
        inline MoveFlag flag() const { return MoveFlag(move >> 12); }
    };

    /* Used for simulating moves temporarialy*/
    struct UndoMove {
        Move move;
        Piece captured_piece;
        uint64_t en_passant_state;
        uint8_t castling_rights_state;
        UndoMove(Move m, Piece cp, uint64_t enpassant, uint8_t castling_rights) :
             move(m), captured_piece(cp), en_passant_state(enpassant), castling_rights_state(castling_rights) {}
    };

    // Conversion to and from Universal Chess Interface notation
    // TODO castling UCI moves
    inline std::string moveToUCI(const Move &move){
        int start = move.start(), end = move.end();
        char promotionChar = '\0';
        switch (move.flag()) {
            case MoveFlag::PROMOTE_KNIGHT: promotionChar = 'n'; break;
            case MoveFlag::PROMOTE_BISHOP: promotionChar = 'b'; break;
            case MoveFlag::PROMOTE_ROOK:   promotionChar = 'r'; break;
            case MoveFlag::PROMOTE_QUEEN:  promotionChar = 'q'; break;
            default: break;
        }
        std::string uci;
        uci += char('a' + start % 8);
        uci += char('1' + start / 8);
        uci += char('a' + end % 8);
        uci += char('1' + end / 8);
        if (promotionChar) uci += promotionChar;
        return uci;
    }

    inline Move uciToMove(const std::string &uci) {
        int start = (uci[1] - '1') * 8 + (uci[0] - 'a');
        int end   = (uci[3] - '1') * 8 + (uci[2] - 'a');
        MoveFlag flag = MoveFlag::NONE_FLAG;

        if (uci.size() == 5) {
            switch (std::tolower(uci[4])) {
                case 'n': flag = MoveFlag::PROMOTE_KNIGHT; break;
                case 'b': flag = MoveFlag::PROMOTE_BISHOP; break;
                case 'r': flag = MoveFlag::PROMOTE_ROOK;   break;
                case 'q': flag = MoveFlag::PROMOTE_QUEEN;  break;
            }
        }

        return Move(start, end, flag);
    }
}
