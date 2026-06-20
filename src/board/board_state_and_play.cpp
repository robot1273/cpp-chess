#include "board.hpp"
#include "enums.hpp"
#include "move.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include "board_precalculation.hpp"
#include <sstream>

/*
 * All code here is related to the state of the board and handling move input.
 */

namespace chess{

    // --------------------------------------- BOARD STATE GEN --------------------------------------- //

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

    // --------------------------------------- BOARD STATE QUERY --------------------------------------- //

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

    // --------------------------------------- MOVE INPUTS AND FEN --------------------------------------- //


    UndoMove Board::play_move(std::string uci){
        if (!is_valid_UCI_string(uci)) {
            throw std::runtime_error("Invalid UCI format: " + uci);
        }

        int start = (uci[1] - '1') * 8 + (uci[0] - 'a');
        int end = (uci[3] - '1') * 8 + (uci[2] - 'a');

        MoveFlag promo_flag = MoveFlag::NONE_FLAG;
        if (uci.length() == 5) {
            switch (std::tolower(uci[4])) {
                case 'n': promo_flag = MoveFlag::PROMOTE_KNIGHT; break;
                case 'b': promo_flag = MoveFlag::PROMOTE_BISHOP; break;
                case 'r': promo_flag = MoveFlag::PROMOTE_ROOK;   break;
                case 'q': promo_flag = MoveFlag::PROMOTE_QUEEN;  break;
            }
        }

        MoveList legal_moves;
        generate_all_legal_moves(current_turn, legal_moves);
        for (const Move& legal_move : legal_moves) {
            if (legal_move.start() == start && legal_move.end() == end) {
                if (promo_flag != MoveFlag::NONE_FLAG) {
                    if (legal_move.flag() == promo_flag) {
                        return play_move(legal_move);
                    }
                } else {
                    return play_move(legal_move);
                }
            }
        }

        throw std::runtime_error("Illegal move attempted: " + uci);
    }

    void Board::load_position(const std::string& fen) {
        if (fen.empty()) { reset(); return; }

        for (int i = 0; i < 6; i++) pieces_t[i] = 0ULL;
        for (int i = 0; i < 2; i++) pieces_c[i] = 0ULL;
        game_state_stack.clear();

        std::istringstream iss(fen);
        std::string board_fen, turn_fen, castling_fen, ep_fen; //space separated fieds
        int half_moves = 0, full_moves = 1;

        iss >> board_fen >> turn_fen >> castling_fen >> ep_fen;
        if (!(iss >> half_moves)) half_moves = 0;
        if (!(iss >> full_moves)) full_moves = 1;

        // place pieces
        int rank = 7;
        int file = 0;
        for (char c : board_fen) {
            if (c == '/') {
                rank--;
                file = 0;
            } else if (std::isdigit(c)) {
                file += (c - '0');
            } else {
                int sq = rank * 8 + file;
                uint64_t mask = 1ULL << sq;
                Colour piece_colour = std::isupper(c) ? WHITE : BLACK;

                Piece piece_type;
                switch (std::tolower(c)) {
                    case 'p': piece_type = PAWN; break;
                    case 'n': piece_type = KNIGHT; break;
                    case 'b': piece_type = BISHOP; break;
                    case 'r': piece_type = ROOK; break;
                    case 'q': piece_type = QUEEN; break;
                    case 'k': piece_type = KING; break;
                    default: throw std::runtime_error("FEN contains invalid piece character");
                }

                pieces_t[piece_type] |= mask;
                pieces_c[piece_colour] |= mask;
                file++;
            }
        }

        current_turn = (turn_fen == "b") ? BLACK : WHITE;

        // castling rights
        castling_rights = 0;
        if (castling_fen != "-") {
            for (char c : castling_fen) {
                switch (c) {
                    case 'K': castling_rights |= 0b0100; break; // white kingside
                    case 'Q': castling_rights |= 0b1000; break; // white queenside
                    case 'k': castling_rights |= 0b0001; break; // black kingside
                    case 'q': castling_rights |= 0b0010; break; // black queenside
                }
            }
        }

        // en passant
        en_passant_moves = 0ULL;
        if (ep_fen != "-") {
            int ep_file = ep_fen[0] - 'a';
            int ep_rank = ep_fen[1] - '1';
            en_passant_moves = 1ULL << (ep_rank * 8 + ep_file);
        }

        // counter and hash generation
        half_move_counter = half_moves;
        current_hash = generate_zobrist_hash(current_turn);
    }

    std::string Board::get_fen() const {
        std::string fen = "";

        // get piece placement
        char piece_chars[] = {'p', 'n', 'b', 'r', 'q', 'k'};
        for (int rank = 7; rank >= 0; rank--) {
            int empty_count = 0;
            for (int file = 0; file < 8; file++) {
                int sq = rank * 8 + file;
                Piece p = get_piece_type(sq);
                Colour c = get_piece_colour(sq);

                if (p == NONE) { // empty square count
                    empty_count++;
                } else {
                    if (empty_count > 0) {
                        fen += std::to_string(empty_count);
                        empty_count = 0;
                    }

                    char pc = piece_chars[p];
                    fen += (c == WHITE) ? std::toupper(pc) : pc;
                }
            }

            if (empty_count > 0) {
                fen += std::to_string(empty_count);
            }
            if (rank > 0) {
                fen += "/";
            }
        }

        fen += " ";

        // turn
        fen += (current_turn == WHITE) ? "w " : "b ";

        // castling rights
        std::string castling = "";
        if (castling_rights & 0b0100) castling += "K";
        if (castling_rights & 0b1000) castling += "Q";
        if (castling_rights & 0b0001) castling += "k";
        if (castling_rights & 0b0010) castling += "q";

        fen += castling.empty() ? "-" : castling;
        fen += " ";

        // en passant
        if (en_passant_moves == 0ULL) {
            fen += "-";
        } else {
            int en_passant_square = __builtin_ctzll(en_passant_moves);
            fen += char('a' + (en_passant_square % 8)); //hehe
            fen += char('1' + (en_passant_square / 8));
        }
        fen += " ";

        // half move count
        fen += std::to_string(half_move_counter) + " ";

        // full move count
        fen += std::to_string(full_moves_played);

        return fen;
    }

}
