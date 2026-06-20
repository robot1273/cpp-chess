#include "board.hpp"
#include "board_precalculation.hpp"
#include "utility.hpp"

#include <vector>

namespace chess{
    // squares that must be empty for castling to be legal
    static constexpr uint64_t white_kingside_castle_mask  = 0x0000000000000060;
    static constexpr uint64_t white_queenside_castle_mask = 0x000000000000000E;

    static constexpr uint64_t black_kingside_castle_mask  = 0x6000000000000000;
    static constexpr uint64_t black_queenside_castle_mask = 0x0E00000000000000;

    /*
      Generates all pseudolegal moves
    */
    void Board::generate_all_moves(Colour player, MoveList& moves) const {
        uint64_t full_spaces = pieces_c[WHITE] | pieces_c[BLACK];
        uint64_t empty_spaces = ~full_spaces;
        Colour enemy = (player == WHITE) ? BLACK : WHITE;

        // -------------------------- Pawn moves -------------------------- //
        uint64_t pawns_single_move, pawns_double_move, pawns_take_left, pawns_take_right;
        uint64_t pawns = pieces_t[PAWN] & pieces_c[player]; //inital positions
        uint64_t double_move_mask = (player == WHITE) ? 0x0000000000FF0000ULL : 0x0000FF0000000000ULL;

        int single_move_offset = (player == WHITE) ? 8 : -8;
        int take_left_offset   = (player == WHITE) ? 7 : -9;
        int take_right_offset  = (player == WHITE) ? 9 : -7;

        pawns_single_move = utility::shift(pawns, single_move_offset) & empty_spaces;
        pawns_double_move = utility::shift(pawns_single_move & double_move_mask, single_move_offset) & empty_spaces;
        pawns_take_left   = utility::shift(pawns & 0xFEFEFEFEFEFEFEFEULL, take_left_offset) & (pieces_c[enemy] | en_passant_moves); // Can take on an imaginary en passant square
        pawns_take_right  = utility::shift(pawns & 0x7F7F7F7F7F7F7F7FULL, take_right_offset) & (pieces_c[enemy] | en_passant_moves);

        // generate promotion moves
        uint64_t final_rank_mask = (player == WHITE) ? 0xFF00000000000000ULL : 0x00000000000000FFULL;
        uint64_t final_rank_promoting_moves = (pawns_single_move | pawns_take_left | pawns_take_right) & final_rank_mask;

        auto add_pawn_moves = [&](uint64_t pawn_moves, int offset) {
            uint64_t promoting_moves = pawn_moves & final_rank_promoting_moves;
            uint64_t en_passant_captures = pawn_moves & en_passant_moves;  // overlap between en passant spaces and possible moves
            uint64_t regular_moves = pawn_moves & ~final_rank_promoting_moves & ~en_passant_moves; //no en passant or promotion

            for (uint64_t board = regular_moves; board; board &= board - 1) { //non-promoting moves
                int move = __builtin_ctzll(board);
                moves.push_back(Move(move - offset, move, MoveFlag::NONE_FLAG));
            }

            for (uint64_t board = promoting_moves; board; board &= board - 1) { //promoting moves
                int move = __builtin_ctzll(board);
                moves.push_back(Move(move - offset, move, MoveFlag::PROMOTE_QUEEN));
                moves.push_back(Move(move - offset, move, MoveFlag::PROMOTE_ROOK));
                moves.push_back(Move(move - offset, move, MoveFlag::PROMOTE_BISHOP));
                moves.push_back(Move(move - offset, move, MoveFlag::PROMOTE_KNIGHT));
            }

            for (uint64_t board = en_passant_captures; board; board &= board - 1) { //en passant captures
                int move = __builtin_ctzll(board);
                moves.push_back(Move(move - offset, move, MoveFlag::EN_PASSANT));
            }

        };

        add_pawn_moves(pawns_single_move, single_move_offset);
        add_pawn_moves(pawns_double_move, single_move_offset * 2);
        add_pawn_moves(pawns_take_left,   take_left_offset);
        add_pawn_moves(pawns_take_right,  take_right_offset);

        // -------------------------- Knight moves -------------------------- //
        uint64_t knights = pieces_t[KNIGHT] & pieces_c[player];

        for (uint64_t board = knights; board; board &= board - 1) {
            int start_sq = __builtin_ctzll(board);

            uint64_t targets = Global::knight_masks[start_sq] & ~pieces_c[player];

            for (uint64_t t_board = targets; t_board; t_board &= t_board - 1) {
                moves.push_back(Move(start_sq, __builtin_ctzll(t_board)));
            }
        }

        // -------------------------- Sliding moves -------------------------- //

        //Helper lambda function to generate sliding moves
        auto gen_sliding = [&](uint64_t pieces, bool is_rook) {
            for (uint64_t board = pieces; board; board &= board - 1) { //for each straight slider
                int sq = __builtin_ctzll(board);

                uint64_t mask = is_rook ? Global::rook_masks[sq] : Global::bishop_masks[sq];
                uint64_t occupancy = full_spaces & mask;

                uint64_t magic = is_rook ? Global::rook_magics[sq] : Global::bishop_magics[sq];
                int mask_bits = __builtin_popcountll(mask);
                int index = (occupancy * magic) >> (64 - mask_bits);

                uint64_t possible_moves = is_rook ? Global::rook_attacks[sq][index] : Global::bishop_attacks[sq][index];

                possible_moves &= ~pieces_c[player]; //exclude friendly pieces

                for (uint64_t move = possible_moves; move; move &= move - 1) { //addd the moves !
                    int target = __builtin_ctzll(move);
                    moves.push_back(Move(sq, target));
                }
            }
        };

        uint64_t straight_sliding_pieces = (pieces_t[ROOK] | pieces_t[QUEEN]) & pieces_c[player];
        uint64_t diagonal_sliding_pieces = (pieces_t[BISHOP] | pieces_t[QUEEN]) & pieces_c[player];

        gen_sliding(straight_sliding_pieces, true);
        gen_sliding(diagonal_sliding_pieces, false);

        // -------------------------- King moves -------------------------- //
        uint64_t king = pieces_t[KING] & pieces_c[player];
        if (!king) return; // UH OH!

        int king_idx = __builtin_ctzll(king);

        uint64_t moves_mask = Global::king_masks[king_idx] & ~pieces_c[player];

        for (uint64_t board = moves_mask; board; board &= board - 1) {
            moves.push_back(Move(king_idx, __builtin_ctzll(board)));
        }

        // -------------------------- Castling moves --------------------------
        // A castling move has start = the king square, end = the king end square

        bool can_castle_maybe = (player == WHITE) ? ((castling_rights & 0b1100) != 0) : ((castling_rights & 0b0011) != 0);  // Should we even bother with castling computation?

        if (!can_castle_maybe) { return; } //If no castling available, end now!

        bool queenside_is_free, kingside_is_free;

        // OK so we might have the rights, but are the spaces free?
        if (player == WHITE){
            queenside_is_free = (castling_rights & 0b1000) && !(full_spaces & white_queenside_castle_mask);
            kingside_is_free  = (castling_rights & 0b0100) && !(full_spaces & white_kingside_castle_mask);
        } else {
            queenside_is_free = (castling_rights & 0b0010) && !(full_spaces & black_queenside_castle_mask);
            kingside_is_free  = (castling_rights & 0b0001) && !(full_spaces & black_kingside_castle_mask);
        }

        if (!queenside_is_free && !kingside_is_free){ return; }  //The spaces on both sides are not free! End now!!!!!!!!

        if (king_in_check(player)) { return; } // If the king is in check, we can't castle! END NOW!!!!
        bool queenside_can_castle = false;
        bool kingside_can_castle = false;

        if (player == WHITE) {
            if (queenside_is_free) { // white queenside path safety (C1 and D1)
                queenside_can_castle = !is_square_attacked(2, WHITE) && !is_square_attacked(3, WHITE);
            }
            if (kingside_is_free) { // white kingside path safety (F1 and G1)
                kingside_can_castle  = !is_square_attacked(5, WHITE) && !is_square_attacked(6, WHITE);
            }
        } else {
            if (queenside_is_free) { // black queenside path safety (C8 and D8)
                queenside_can_castle = !is_square_attacked(58, BLACK) && !is_square_attacked(59, BLACK);
            }
            if (kingside_is_free) { // black kingside path safety (F8 and G8)
                kingside_can_castle  = !is_square_attacked(61, BLACK) && !is_square_attacked(62, BLACK);
            }
        }

        if (queenside_can_castle) {
            moves.push_back(Move(king_idx, king_idx-2, MoveFlag::CASTLE_QUEENSIDE));
        }
        if (kingside_can_castle)  {
            moves.push_back(Move(king_idx, king_idx+2, MoveFlag::CASTLE_KINGSIDE));
        }
    }

    /* Checks if a king is in check in the current board state */
    bool Board::king_in_check(Colour player) const {
        uint64_t king_bb = pieces_t[KING] & pieces_c[player];
        return king_bb ? is_square_attacked(__builtin_ctzll(king_bb), player) : false;
    }

    /* Generalisation of if a square is attacked */
    bool Board::is_square_attacked(int sq, Colour player) const {
        uint64_t enemy_pieces = pieces_c[(player == WHITE) ? BLACK : WHITE];

        // knights
        if (Global::knight_masks[sq] & (pieces_t[KNIGHT] & enemy_pieces)) return true;

        // pawns
        int enemy_pawn_idx = (player == WHITE) ? 0 : 1;
        if (Global::pawn_masks[enemy_pawn_idx][sq] & (pieces_t[PAWN] & enemy_pieces)) return true;

        // king
        if (Global::king_masks[sq] & (pieces_t[KING] & enemy_pieces)) return true;

        // sliders
        auto check_sliders = [&](bool is_rook, uint64_t attackers) {
            if (!attackers) return false;
            uint64_t mask = is_rook ? Global::rook_masks[sq] : Global::bishop_masks[sq];
            uint64_t occ = (pieces_c[WHITE] | pieces_c[BLACK]) & mask;

            uint64_t magic = is_rook ? Global::rook_magics[sq] : Global::bishop_magics[sq];
            int index = (occ * magic) >> (64 - __builtin_popcountll(mask));

            uint64_t attacks = is_rook ? Global::rook_attacks[sq][index] : Global::bishop_attacks[sq][index];
            return (attacks & attackers) != 0;
        };

        if (check_sliders(true, (pieces_t[ROOK] | pieces_t[QUEEN]) & enemy_pieces)) return true;
        if (check_sliders(false, (pieces_t[BISHOP] | pieces_t[QUEEN]) & enemy_pieces)) return true;

        return false;
    }

    bool Board::is_legal(const Move& move, Colour player) const {
        MoveFlag flag = move.flag();

        // castling is guaranteed to be legal
        if (flag == MoveFlag::CASTLE_KINGSIDE || flag == MoveFlag::CASTLE_QUEENSIDE) {
            return true;
        }

        int start = move.start();
        int end = move.end();
        int king_sq = __builtin_ctzll(pieces_t[KING] & pieces_c[player]);

        // king end square should be checked if the king moves
        if (start == king_sq) {
            king_sq = end;
        }

        Colour enemy = (player == WHITE) ? BLACK : WHITE;

        // simulate the new occupancy mask and enemy pieces after move is made
        uint64_t occ = ((pieces_c[WHITE] | pieces_c[BLACK]) ^ (1ULL << start)) | (1ULL << end);
        uint64_t enemy_pieces = pieces_c[enemy] & ~(1ULL << end);

        // simulate en passant move
        if (flag == MoveFlag::EN_PASSANT) {
            int ep_capture_sq = (player == WHITE) ? end - 8 : end + 8;
            occ ^= (1ULL << ep_capture_sq);
            enemy_pieces ^= (1ULL << ep_capture_sq);
        }

        // when we check for atacks, we use the occupancy and enemy piece masks
        // so when we move a piece, we can check if a pin prevents legality

        // knight
        if (Global::knight_masks[king_sq] & pieces_t[KNIGHT] & enemy_pieces) return false;

        // pawn
        int enemy_pawn_idx = (player == WHITE) ? 0 : 1;
        if (Global::pawn_masks[enemy_pawn_idx][king_sq] & pieces_t[PAWN] & enemy_pieces) return false;

        // king
        if (Global::king_masks[king_sq] & pieces_t[KING] & enemy_pieces) return false;

        auto check_sliders = [&](uint64_t attackers, bool is_rook) {
            if (!attackers) return false;
            uint64_t mask = is_rook ? Global::rook_masks[king_sq] : Global::bishop_masks[king_sq];
            uint64_t magic = is_rook ? Global::rook_magics[king_sq] : Global::bishop_magics[king_sq];

            int index = ((occ & mask) * magic) >> (64 - __builtin_popcountll(mask));
            uint64_t attacks = is_rook ? Global::rook_attacks[king_sq][index] : Global::bishop_attacks[king_sq][index];
            return (attacks & attackers) != 0;
        };

        if (check_sliders((pieces_t[ROOK] | pieces_t[QUEEN]) & enemy_pieces, true)) return false;
        if (check_sliders((pieces_t[BISHOP] | pieces_t[QUEEN]) & enemy_pieces, false)) return false;

        return true;
    }

    void Board::generate_all_legal_moves(Colour player, MoveList& legal_moves) const {
        MoveList pseudo_moves;
        generate_all_moves(player, pseudo_moves);

        for (const Move& move : pseudo_moves) {
            if (is_legal(move, player)) {
                legal_moves.push_back(move);
            }
        }
    }

    void Board::generate_all_legal_capture_moves(Colour player, MoveList& legal_capture_moves) const {
        MoveList pseudo_moves;
        generate_all_moves(player, pseudo_moves);

        Colour enemy = (player == WHITE) ? BLACK : WHITE;

        for (const Move& move : pseudo_moves) {
            // first check if capture
            if (get_piece_colour(move.end()) == enemy) {
                if (is_legal(move, player)) {
                    legal_capture_moves.push_back(move);
                }
            }
        }
    }

}
