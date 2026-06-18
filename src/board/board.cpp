#include "board.hpp"
#include "enums.hpp"
#include "move.hpp"
#include "utility.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include "board_precalculation.hpp"

namespace chess{
    /* Reset board state to starting position
     */
    void Board::reset() {
        pieces_t[PAWN]   = 0x00FF00000000FF00;
        pieces_t[KNIGHT] = 0x4200000000000042;
        pieces_t[BISHOP] = 0x2400000000000024;
        pieces_t[ROOK]   = 0x8100000000000081;
        pieces_t[QUEEN]  = 0x0800000000000008;
        pieces_t[KING]   = 0x1000000000000010;

        pieces_c[WHITE]  = 0x000000000000FFFF;
        pieces_c[BLACK]  = 0xFFFF000000000000;

        en_passant_moves = 0ULL;
        castling_rights = 0b1111;
        current_turn = Colour::WHITE;

        current_hash = generate_zobrist_hash(current_turn);
    }

    Board::Board(){
        reset(); //setup initial board state
    }

    // returns 0-5 for white pieces, 6-11 black pieces and -1 for none
    int Board::get_zobrist_piece_index(int square) const {
        Piece p = get_piece_type(square);
        Colour c = get_piece_colour(square);
        return get_zobrist_piece_index(p, c);
    }

    int Board::get_zobrist_piece_index(Piece p, Colour c) const {
        if (p == NONE) return -1; // allow c no colour to pass (error should raise)
        return static_cast<int>(p) + 6 * static_cast<int>(c);
    }

    uint64_t Board::generate_zobrist_hash(Colour player_to_move) const {
        uint64_t hash = 0;

        for (int sq = 0; sq < 64; sq++) {
            int idx = get_zobrist_piece_index(sq);
            if (idx != -1) {
                hash ^= Global::zobrist_piece_keys[sq][idx];
            }
        }

        if (player_to_move == BLACK) {
            hash ^= Global::zobrist_player_turn_key;
        }

        return hash;
    }

    void Board::update_zobrist_hash(const Move& move) {
        int start = move.start();
        int end = move.end();
        MoveFlag flag = move.flag();
        Colour player = get_piece_colour(start);
        auto& ZOBRIST_KEYS = Global::zobrist_piece_keys;

        int moving_piece_idx = get_zobrist_piece_index(start);
        current_hash ^= ZOBRIST_KEYS[start][moving_piece_idx];

        if (isPromotion(flag)) { // handle promotion
            Piece promotion_piece = promotionPiece(flag);
            int promotion_piece_idx = get_zobrist_piece_index(promotion_piece, player);
            current_hash ^= ZOBRIST_KEYS[end][promotion_piece_idx];
        } else { // non-promotion move
            current_hash ^= ZOBRIST_KEYS[end][moving_piece_idx];
        }

        // handle capture
        int capture_square = (flag == EN_PASSANT) ? end + (player == WHITE ? -8 : 8) : end;
        int captured_piece_idx = get_zobrist_piece_index(capture_square);

        if (captured_piece_idx != -1) { // if capture
            current_hash ^= ZOBRIST_KEYS[capture_square][captured_piece_idx];
        }

        // king move is sorted but we also need to handle the rook move
        if (flag == CASTLE_KINGSIDE || flag == CASTLE_QUEENSIDE) {
            int rook_start = 0;
            int rook_end = 0;

            if (flag == CASTLE_KINGSIDE) {
                rook_start = (player == WHITE) ? 7 : 63;
                rook_end = rook_start - 2;
            } else {
                rook_start = (player == WHITE) ? 0 : 56;
                rook_end = rook_start + 3;
            }

            int rook_idx = get_zobrist_piece_index(rook_start);
            current_hash ^= ZOBRIST_KEYS[rook_start][rook_idx];
            current_hash ^= ZOBRIST_KEYS[rook_end][rook_idx];
        }

        // swap turn
        current_hash ^= Global::zobrist_player_turn_key;
    }

    Piece Board::get_piece_type(int idx) const {
        uint64_t mask = 1ULL << idx;
        for (int i = 0; i < 6; i++) {
            if (pieces_t[i] & mask) return Piece(i);
        }
        return NONE;
    }

    Colour Board::get_piece_colour(int idx) const {
        if ((pieces_c[WHITE] >> idx) & 1ULL) return WHITE;
        if ((pieces_c[BLACK] >> idx) & 1ULL) return BLACK;
        return NO_COLOUR;
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

        if (flag == MoveFlag::NULL_MOVE){
            throw std::runtime_error("ERROR - Null move was played");
        }

        Piece piece_type = get_piece_type(start);
        Piece captured_piece = get_piece_type(end);

        // draw conditions and hash
        game_state_stack.push_back({current_hash, half_move_counter}); // push current hash onto stack
        update_zobrist_hash(move); //update hash before board state changes

        current_turn = (current_turn == Colour::WHITE) ? Colour::BLACK : Colour::WHITE;

        if (captured_piece != NONE || piece_type == PAWN) {
            half_move_counter = 0;
        } else {
            half_move_counter++;
        }

        if (piece_type == NONE) { return {move, NONE, en_passant_moves, castling_rights}; } // no piece found, end early
        Colour piece_colour = get_piece_colour(start);

        if (piece_colour == NO_COLOUR) {
            utility::print_stacktrace();
            throw std::runtime_error("square has no piece to read colour from");
        }

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

    bool Board::insufficient_material() const {
        if (pieces_t[PAWN] || pieces_t[ROOK] || pieces_t[QUEEN]) { return false; } //always possible to mate

        int knights = __builtin_popcountll(pieces_t[KNIGHT]);
        int bishops = __builtin_popcountll(pieces_t[BISHOP]);
        int total_minors = knights + bishops;

        if (total_minors == 0) { return true; } // king v king

        if (total_minors == 1) { return true; } // knight or bishop vs king

        if (knights == 0 && bishops == 2) { // 2 bishops game, need to check square colors
            uint64_t white_squares = 0xAA55AA55AA55AA55ULL;
            bool white_bishop_on_light = pieces_t[BISHOP] & pieces_c[WHITE] & white_squares;
            bool black_bishop_on_light = pieces_t[BISHOP] & pieces_c[BLACK] & white_squares;

            if (white_bishop_on_light == black_bishop_on_light) { return true; } // if both on light or dark then cant mate
        }

        return false;
    }

    bool Board::is_draw() const {
        if (half_move_counter >= 100) { return true; } // 50 move rule draw
        if (half_move_counter < 4) { return false; }   // impossible for repetition draw
        if (insufficient_material()) { return true; }  // insufficient material draw

        // 3-fold repetition draw
        int stack_size = game_state_stack.size();
        int limit = (stack_size > half_move_counter) ? (stack_size - half_move_counter) : 0;

        // look only as far as the counter allows
        for (int i = stack_size - 2; i >= limit; i -= 2) {
            if (game_state_stack[i].hash == current_hash) return true;
        }

        return false;
    }

    // simple I/O
    void Board::display_board() const {
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
