#include "board.hpp"
#include "move.hpp"
#include "utility.hpp"
#include "board_precalculation.hpp"

#include <iostream>

namespace chess{
    Board::Board(){ 
        //Initialise board state
        pieces_t[PAWN]   = 0x00FF00000000FF00;
        pieces_t[KNIGHT] = 0x4200000000000042;
        pieces_t[BISHOP] = 0x2400000000000024;
        pieces_t[ROOK]   = 0x8100000000000081;
        pieces_t[QUEEN]  = 0x0800000000000008;
        pieces_t[KING]   = 0x1000000000000010;

        pieces_c[WHITE]  = 0x000000000000FFFF;
        pieces_c[BLACK]  = 0xFFFF000000000000;

        rook_attacks.resize(64);
        bishop_attacks.resize(64);

        std::cout << "Generating magic numbers (wait a few seconds) ..." << std::endl;

        rook_magics = generate_magic_numbers(true, rook_attacks);
        bishop_magics = generate_magic_numbers(false, bishop_attacks);

        std::cout << "\033[1A\033[2K\r"; //erase debug line
    } 

    Piece Board::get_piece_type(int idx){
        for (int i = 0; i < 6; i++){
            if (utility::readBit(pieces_t[i], idx)) { return Piece(i); }
        }
        return NONE;
    }

    Colour Board::get_piece_colour(int idx){
        return utility::readBit(pieces_c[WHITE], idx) ? WHITE : BLACK;
    }

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
        
        Piece piece_type = get_piece_type(start);
        if (piece_type == NONE) { return UndoMove(move, NONE, en_passant_moves, castling_rights); } // no piece found, end early

        Colour piece_colour = get_piece_colour(start);

        // Update castling rights
        if (piece_type == ROOK) { // If you ever move a rook
            if (piece_colour == WHITE) {
                if (start == BOTTOM_RIGHT_IDX) castling_rights &= ~0b0100; 
                else if (start == BOTTOM_LEFT_IDX) castling_rights &= ~0b1000;
            } else {
                if (start == TOP_RIGHT_IDX) castling_rights &= ~0b0010; 
                else if (start == TOP_LEFT_IDX) castling_rights &= ~0b0001;
            }
        } else if (piece_type == KING) { // If you ever move a king
            if (piece_colour == WHITE) { castling_rights &= ~0b1100;} // Remove white's castling rights
            else                       { castling_rights &= ~0b0011;} // Remove black's castling rights
        }

        // Handle castling moves
        // start is the king start position, end is king final position
        if (flag == CASTLE_KINGSIDE || flag == CASTLE_QUEENSIDE) { // Move the king if castling
            
            uint64_t king_mask = (1ULL << start) | (1ULL << end);
            uint64_t rook_mask;
            
            if (flag == CASTLE_KINGSIDE){  // Get rook move mask
                int rook_idx = (piece_colour == WHITE) ? 7 : 63;
                rook_mask = (1ULL << rook_idx) | (1ULL << (rook_idx - 2)); //Shift rook to the left 2x
            } else if (flag == CASTLE_QUEENSIDE) {
                int rook_idx = (piece_colour == WHITE) ? 0 : 56;
                rook_mask = (1ULL << rook_idx) | (1ULL << (rook_idx + 3)); //Shift rook to the right 3x
            }

            pieces_t[KING] ^= king_mask; //Move king
            pieces_c[piece_colour] ^= king_mask;

            pieces_t[ROOK] ^= rook_mask; //Move rook
            pieces_c[piece_colour] ^= rook_mask;
            
            return {move, NONE, en_passant_moves, castling_rights};
        }    

        
        // thanks kutay i stole this idea from you MUAHAHA
        uint64_t start_mask = (1ULL << start); 
        uint64_t end_mask   = (1ULL << end);
        uint64_t full_mask  = start_mask | end_mask; 
        
        // handle capturing pieces
        Piece captured_piece = get_piece_type(end);
        if (captured_piece != NONE){
            Colour captured_colour = get_piece_colour(end);
            pieces_t[captured_piece] ^= end_mask; 
            pieces_c[captured_colour] ^= end_mask; // opposite colour
        }

        pieces_t[piece_type]   ^= full_mask;  // move on type board
        pieces_c[piece_colour] ^= full_mask;  // move on colour board

        // Flag handling
        if (flag == MoveFlag::EN_PASSANT){ //En passant
            uint64_t captured_space = (piece_colour == WHITE) ? end_mask >> 8 : end_mask << 8;
            pieces_t[PAWN] ^= captured_space;
            pieces_c[piece_colour == WHITE ? BLACK : WHITE] ^= captured_space;
        } else if (isPromotion(flag)) { // Promotion
            pieces_t[PAWN] ^= end_mask;  // remove pawn from original
            pieces_t[Piece(flag-1)] |= end_mask; //promotion_flag - 1 = corresponding piece
        }

        uint64_t old_en_passant = en_passant_moves; // save old state for undo
        en_passant_moves = 0ULL; //Reset en passant squares after a move is played
        if (piece_type == PAWN && abs(end - start) == 16) {
            int ep_sq = (start + end) / 2;
            en_passant_moves = (1ULL << ep_sq);
        }

        return {move, captured_piece, old_en_passant, castling_rights}; //return the data to undo thhe move
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

        // Undo castling moves
        if (flag == CASTLE_KINGSIDE || flag == CASTLE_QUEENSIDE) { // Move the king if castling
            uint64_t king_mask = (1ULL << start) | (1ULL << end);
            uint64_t rook_mask;
            
            if (flag == CASTLE_KINGSIDE){  // Get rook move mask
                int rook_idx = (mover_colour == WHITE) ? 7 : 63;
                rook_mask = (1ULL << rook_idx) | (1ULL << (rook_idx - 2)); //Shift rook to the left 2x
            } else if (flag == CASTLE_QUEENSIDE) {
                int rook_idx = (mover_colour == WHITE) ? 0 : 56;
                rook_mask = (1ULL << rook_idx) | (1ULL << (rook_idx + 3)); //Shift rook to the right 3x
            }


            pieces_t[KING] ^= king_mask; //Move king
            pieces_c[mover_colour] ^= king_mask;

            pieces_t[ROOK] ^= rook_mask; //Move rook
            pieces_c[mover_colour] ^= rook_mask;
            
            return; //End early
        }  
            
        if (isPromotion(flag)){ // first check if move was a promotion
            pieces_t[PAWN] |= end_mask; // add back to pawn mask
            pieces_t[moving_piece] ^= end_mask; // remove original piece type
            moving_piece = PAWN; //set piece type to pawn
        }
        
        pieces_t[moving_piece] ^= full_mask; // move the original piece back 
        pieces_c[mover_colour] ^= full_mask;


        //restore pawn taken by en passant (captured piece should be NONE)
        if (flag == MoveFlag::EN_PASSANT) { 
            uint64_t captured_square = (mover_colour == WHITE) ? (1ULL << (end - 8)) : (1ULL << (end + 8));
            pieces_t[PAWN] |= captured_square; // restore taken pawn
            pieces_c[mover_colour == WHITE ? BLACK : WHITE] |= captured_square; 
        }
        
        if (captured_piece != NONE) { // restore the old piece
            Colour captured_colour = (mover_colour == WHITE) ? BLACK : WHITE;
            pieces_t[captured_piece] ^= end_mask;
            pieces_c[captured_colour] ^= end_mask;
        }

    };

    // simple I/O
    void Board::display_board() {
        for (int rank = 7; rank >= 0; rank--) {        
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
    }

    UndoMove Board::play_move(std::string uci){
        Move move = uciToMove(uci);
        return play_move(move);
    }
};