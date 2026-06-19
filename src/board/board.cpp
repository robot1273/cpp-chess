#include "board.hpp"
#include "enums.hpp"
#include "move.hpp"
#include "utility.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace chess{
    // for rook position detection
    constexpr int BOTTOM_LEFT_IDX  = 0;
    constexpr int BOTTOM_RIGHT_IDX = 7;
    constexpr int TOP_LEFT_IDX     = 56;
    constexpr int TOP_RIGHT_IDX    = 63;

    /* Does not validate the move played! */
    UndoMove Board::play_move(Move move){
        int start = move.start();
        int end = move.end();
        MoveFlag flag = move.flag();

        if (flag == MoveFlag::NULL_MOVE){
            throw std::runtime_error("ERROR - Null move was played");
        }

        Piece piece_type = get_piece_type(start);
        Colour piece_colour = get_piece_colour(start);
        Piece captured_piece = get_piece_type(end);

        if (piece_type == NONE) { return {move, NONE, en_passant_moves, castling_rights}; } // no piece found, do not simulate move or swap turns
        if (piece_colour == NO_COLOUR) { throw std::runtime_error("square has no colour but contains piece"); }

        // draw conditions and hash
        game_state_stack.push_back({current_hash, half_move_counter}); // push current hash onto stack
        update_zobrist_hash(move); //update hash before board state changes

        if (captured_piece != NONE || piece_type == PAWN) {
            half_move_counter = 0;
        } else {
            half_move_counter++;
        }

        if (current_turn == BLACK) { full_moves_played++; }
        current_turn = (current_turn == WHITE) ? BLACK : WHITE; // change for next turn

        uint8_t old_castling_rights = castling_rights; //Save old data to give to undo
        uint64_t old_en_passant = en_passant_moves;

        // Update castling rights
        if (piece_type == ROOK) { // If you ever move a rook
            if (piece_colour == WHITE) {
                if (start == BOTTOM_RIGHT_IDX)     castling_rights &= ~0b0100;
                else if (start == BOTTOM_LEFT_IDX) castling_rights &= ~0b1000;
            } else {
                if (start == TOP_RIGHT_IDX)        castling_rights &= ~0b0001;
                else if (start == TOP_LEFT_IDX)    castling_rights &= ~0b0010;
            }
        } else if (piece_type == KING) { // If you ever move a king
            if (piece_colour == WHITE) { castling_rights &= ~0b1100;} // Remove white's castling rights
            else                       { castling_rights &= ~0b0011;} // Remove black's castling rights
        }
        // Handle castling moves
        // start is the king start position, end is king final position

        if (flag == CASTLE_KINGSIDE || flag == CASTLE_QUEENSIDE) { // Move the king if castling
            uint64_t rook_mask;
            int king_dest;

            if (flag == CASTLE_KINGSIDE){  // Get rook move mask
                king_dest = (piece_colour == WHITE) ? 6 : 62; // g1 or g8
                int rook_idx = (piece_colour == WHITE) ? BOTTOM_RIGHT_IDX : TOP_RIGHT_IDX;
                rook_mask = (1ULL << rook_idx) | (1ULL << (rook_idx - 2)); //Shift rook to the left 2x
            } else {
                king_dest = (piece_colour == WHITE) ? 2 : 58; // c1 or c8
                int rook_idx = (piece_colour == WHITE) ? BOTTOM_LEFT_IDX : TOP_LEFT_IDX;
                rook_mask = (1ULL << rook_idx) | (1ULL << (rook_idx + 3)); //Shift rook to the right 3x
            }

            uint64_t king_mask = (1ULL << start) | (1ULL << king_dest);

            pieces_t[KING] ^= king_mask; //Move king
            pieces_c[piece_colour] ^= king_mask;

            pieces_t[ROOK] ^= rook_mask; //Move rook
            pieces_c[piece_colour] ^= rook_mask;

            en_passant_moves = 0ULL; // make sure no en passant move is possible
            return {move, NONE, old_en_passant, old_castling_rights};
        }


        // thanks kutay i stole this idea from you MUAHAHA
        uint64_t start_mask = (1ULL << start);
        uint64_t end_mask   = (1ULL << end);
        uint64_t full_mask  = start_mask | end_mask;

        // handle capturing pieces
        if (captured_piece != NONE){
            Colour captured_colour = get_piece_colour(end);
            pieces_t[captured_piece] ^= end_mask;
            pieces_c[captured_colour] ^= end_mask; // opposite colour

            // if we moved to a corner, remove castling rights
            switch (end) {
                case BOTTOM_LEFT_IDX:  castling_rights &= ~0b1000; break; // white queenside
                case BOTTOM_RIGHT_IDX: castling_rights &= ~0b0100; break; // white kingside
                case TOP_LEFT_IDX:     castling_rights &= ~0b0010; break; // black queenside
                case TOP_RIGHT_IDX:    castling_rights &= ~0b0001; break; // top right
            }
        }

        pieces_t[piece_type]   ^= full_mask;  // move on type board
        pieces_c[piece_colour] ^= full_mask;  // move on colour board

        // Flag handling
        if (flag == MoveFlag::EN_PASSANT){ //En passant
            uint64_t captured_space = (piece_colour == WHITE) ? end_mask >> 8 : end_mask << 8;
            pieces_t[PAWN] ^= captured_space;
            pieces_c[piece_colour == WHITE ? BLACK : WHITE] ^= captured_space;
        } else if (isPromotion(flag)) { // Promotion
            pieces_t[PAWN] ^= end_mask;  // clear from pawn bitboard
            pieces_t[Piece(flag - 1)] ^= end_mask; //promotion_flag - 1 = corresponding piece
        }

        en_passant_moves = 0ULL; //Reset en passant squares after a move is played
        if (piece_type == PAWN && abs(end - start) == 16) {
            int ep_sq = (start + end) / 2;
            en_passant_moves = (1ULL << ep_sq);
        }

        return {move, captured_piece, old_en_passant, old_castling_rights}; //return the data to undo thhe move
    };

    void Board::undo_move(UndoMove undo_move) {
        int start = undo_move.move.start();
        int end = undo_move.move.end();
        en_passant_moves = undo_move.en_passant_state; //restore en passant state
        castling_rights = undo_move.castling_rights_state; //restore castling rights

        MoveFlag flag = undo_move.move.flag();
        Piece captured_piece = undo_move.captured_piece;

        Piece moving_piece = get_piece_type(end);
        Colour mover_colour = get_piece_colour(end);

        uint64_t start_mask = (1ULL << start);
        uint64_t end_mask = (1ULL << end);
        uint64_t full_mask = start_mask | end_mask;

        if (current_turn == BLACK) { full_moves_played--; }
        current_turn = (current_turn == Colour::WHITE) ? Colour::BLACK : Colour::WHITE;

        try {
            // undo state
            const GameState& previous_state = game_state_stack.back();
            current_hash = previous_state.hash;
            half_move_counter = previous_state.half_move_counter;
            game_state_stack.pop_back();
        } catch (const std::out_of_range& e) {
            throw std::runtime_error("tried to undo move when no previous move was played");
        }

        // Undo castling moves
        if (flag == CASTLE_KINGSIDE || flag == CASTLE_QUEENSIDE) { // Move the king if castling
            uint64_t rook_mask;
            int king_dest;

            if (flag == CASTLE_KINGSIDE){  // Get rook move mask
                king_dest = (mover_colour == WHITE) ? 6 : 62; // g1 or g8
                int rook_idx = (mover_colour == WHITE) ? BOTTOM_RIGHT_IDX : TOP_RIGHT_IDX;
                rook_mask = (1ULL << rook_idx) | (1ULL << (rook_idx - 2)); //Shift rook to the left 2x
            } else {
                king_dest = (mover_colour == WHITE) ? 2 : 58; // c1 or c8
                int rook_idx = (mover_colour == WHITE) ? BOTTOM_LEFT_IDX : TOP_LEFT_IDX;
                rook_mask = (1ULL << rook_idx) | (1ULL << (rook_idx + 3)); //Shift rook to the right 3x
            }

            uint64_t king_mask = (1ULL << start) | (1ULL << king_dest);

            pieces_t[KING] ^= king_mask; //Move king
            pieces_c[mover_colour] ^= king_mask;

            pieces_t[ROOK] ^= rook_mask; //Move rook
            pieces_c[mover_colour] ^= rook_mask;

            return; //End early
        }

        if (isPromotion(flag)){ // first check if move was a promotion
            pieces_t[PAWN] ^= end_mask; // add back to pawn mask
            pieces_t[moving_piece] ^= end_mask; // remove original piece type
            moving_piece = PAWN; //set piece type to pawn
        }

        pieces_t[moving_piece] ^= full_mask; // move the original piece back
        pieces_c[mover_colour] ^= full_mask;


        //restore pawn taken by en passant (captured piece should be NONE)
        if (flag == MoveFlag::EN_PASSANT && moving_piece == PAWN) {
            uint64_t captured_square = (mover_colour == WHITE) ? (1ULL << (end - 8)) : (1ULL << (end + 8));
            pieces_t[PAWN] ^= captured_square; // restore taken pawn
            pieces_c[mover_colour == WHITE ? BLACK : WHITE] ^= captured_square;
        }

        if (captured_piece != NONE) { // restore the old piece
            Colour captured_colour = (mover_colour == WHITE) ? BLACK : WHITE;
            pieces_t[captured_piece] ^= end_mask;
            pieces_c[captured_colour] ^= end_mask;
        }

    };

    // simple I/O
    void Board::display_board(bool fancy) const {
        for (int rank = 7; rank >= 0; rank--) {
            if (fancy) {  std::cout << (rank + 1) << "  ";  }

            for (int file = 0; file < 8; file++) {
                int idx = rank * 8 + file;
                std::string piece = ".";

                bool is_white = utility::readBit(pieces_c[WHITE], idx);
                for (int i = 0; i < 6; i++) {
                    if (utility::readBit(pieces_t[i], idx)) {
                        piece = is_white ? WHITE_PIECES[i] : BLACK_PIECES[i];
                        break;
                    }
                }

                std::cout << piece << " ";
            }
            std::cout << std::endl;
        }

        if (fancy) { std::cout << std::endl << "   a b c d e f g h" << std::endl; }

        std::cout << std::endl;
    }
};
