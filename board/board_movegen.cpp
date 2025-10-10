#include "board.hpp"
#include "board_precalculation.hpp"
#include <vector>
#include "utility.hpp"

namespace chess{
    inline uint64_t shift(const uint64_t b, int offset) {
        return (offset > 0) ? (b << offset) : (b >> -offset);
    }

    static constexpr int knight_offsets[8] = {-17, -15, -10, -6, 6, 10, 15, 17};
    static constexpr uint64_t knight_masks[8] = {
        0x7F7F7F7F7F7F7F7FULL,
        0xFEFEFEFEFEFEFEFEULL,
        0x3F3F3F3F3F3F3F3FULL, 
        0xFCFCFCFCFCFCFCFCULL,
        0x3F3F3F3F3F3F3F3FULL,
        0xFCFCFCFCFCFCFCFCULL,
        0xFEFEFEFEFEFEFEFEULL,
        0x7F7F7F7F7F7F7F7FULL
    };
    
    /*
      Generates a bitboard with every possible attacked square, regardless of whats on the attacked square
      Used to efficiently identify king checks
     */
    uint64_t Board::generate_all_attacks_bitboard(Colour player) {
        uint64_t attacks = 0ULL;

        uint64_t full_spaces = pieces_c[WHITE] | pieces_c[BLACK];

        // -------------------------- Pawn moves -------------------------- //
        uint64_t pawns_take_left, pawns_take_right;
        uint64_t pawns = pieces_t[PAWN] & pieces_c[player]; //inital positions

        int take_left_offset   = (player == WHITE) ? 7 : -9;
        int take_right_offset  = (player == WHITE) ? 9 : -7;

        pawns_take_left   = shift(pawns & 0xFEFEFEFEFEFEFEFEULL, take_left_offset); 
        pawns_take_right  = shift(pawns & 0x7F7F7F7F7F7F7F7FULL, take_right_offset); 

        attacks |= pawns_take_left | pawns_take_right; //add pawn attacks 
        
        // -------------------------- Knight moves -------------------------- //
        uint64_t knights = pieces_t[KNIGHT] & pieces_c[player]; 

        for (int i = 0; i < 8; i++){
            attacks |= shift(knights, knight_offsets[i]) & knight_masks[i]; //add knight attacks
        }

        // -------------------------- Sliding moves -------------------------- //

        //Helper lambda function to generate sliding moves  
        auto gen_sliding = [&](uint64_t pieces, bool is_rook) { 
            for (uint64_t board = pieces; board; board &= board - 1) { //for each straight slider
                int square = __builtin_ctzll(board);
                
                uint64_t mask = get_sliding_piece_mask(square, is_rook);
                uint64_t occupancy = full_spaces & mask;

                uint64_t magic = is_rook ? rook_magics[square] : bishop_magics[square];
                int mask_bits = __builtin_popcountll(mask);
                int index = (occupancy * magic) >> (64 - mask_bits);
            
                uint64_t possible_moves = is_rook ? rook_attacks[square][index] : bishop_attacks[square][index];
                attacks |= possible_moves;
            }
        };

        uint64_t straight_sliding_pieces = (pieces_t[ROOK] | pieces_t[QUEEN]) & pieces_c[player];
        uint64_t diagonal_sliding_pieces = (pieces_t[BISHOP] | pieces_t[QUEEN]) & pieces_c[player];

        gen_sliding(straight_sliding_pieces, true);
        gen_sliding(diagonal_sliding_pieces, false);

        // -------------------------- King moves -------------------------- //
        uint64_t king = pieces_t[KING] & pieces_c[player]; 
        uint64_t a = king & 0xfefefefefefefefeULL; //prevent wraparound
        uint64_t h = king & 0x7f7f7f7f7f7f7f7fULL;

        uint64_t moves_mask = (a << 7) | (king << 8) | (h << 9) |
                              (a >> 1) | (h << 1) |
                              (a >> 9) | (king >> 8) | (h >> 7);

        attacks |= moves_mask;

        return attacks;
    }

    /*
      Generates all pseudolegal moves
    */
    std::vector<Move> Board::generate_all_moves(Colour player) {
        std::vector<Move> moves = {};
        
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

        pawns_single_move = shift(pawns, single_move_offset) & empty_spaces;
        pawns_double_move = shift(pawns_single_move & double_move_mask, single_move_offset) & empty_spaces;
        pawns_take_left   = shift(pawns & 0xFEFEFEFEFEFEFEFEULL, take_left_offset) & (pieces_c[enemy] | en_passant_moves); // Can take on an imaginary en passant square
        pawns_take_right  = shift(pawns & 0x7F7F7F7F7F7F7F7FULL, take_right_offset) & (pieces_c[enemy] | en_passant_moves); 

        // generate promotion moves
        uint64_t final_rank_mask = (player == WHITE) ? 0xFF00000000000000ULL : 0x00000000000000FFULL;
        uint64_t final_rank_promoting_moves = (pawns_single_move | pawns_take_left | pawns_take_right) & final_rank_mask;
        
        auto add_pawn_moves = [&](uint64_t pawn_moves, int offset) { 
            uint64_t promoting_moves = pawn_moves & final_rank_promoting_moves;
            uint64_t en_passant_captures = pawn_moves & en_passant_moves;  // overlap between en passant spaces and possible moves
            uint64_t regular_moves = pawn_moves & ~final_rank_promoting_moves & ~en_passant_moves; //no en passant or promotion
            
            for (uint64_t board = regular_moves; board; board &= board - 1) { //non-promoting moves
                int move = __builtin_ctzll(board);
                moves.emplace_back(Move(move - offset, move, MoveFlag::NONE_FLAG));
            }

            for (uint64_t board = promoting_moves; board; board &= board - 1) { //promoting moves
                int move = __builtin_ctzll(board);
                moves.emplace_back(Move(move - offset, move, MoveFlag::PROMOTE_QUEEN));
                moves.emplace_back(Move(move - offset, move, MoveFlag::PROMOTE_ROOK));
                moves.emplace_back(Move(move - offset, move, MoveFlag::PROMOTE_BISHOP));
                moves.emplace_back(Move(move - offset, move, MoveFlag::PROMOTE_KNIGHT));
            }
            
            for (uint64_t board = en_passant_captures; board; board &= board - 1) { //en passant captures
                int move = __builtin_ctzll(board);
                moves.emplace_back(Move(move - offset, move, MoveFlag::EN_PASSANT)); 
            }

        };
 
        add_pawn_moves(pawns_single_move, single_move_offset);
        add_pawn_moves(pawns_double_move, single_move_offset * 2);
        add_pawn_moves(pawns_take_left,   take_left_offset);
        add_pawn_moves(pawns_take_right,  take_right_offset);

        // -------------------------- Knight moves -------------------------- //
        uint64_t knights = pieces_t[KNIGHT] & pieces_c[player]; 
        uint64_t knight_moves[8];

        for (int i = 0; i < 8; i++){
            knight_moves[i] = shift(knights, knight_offsets[i]) & knight_masks[i] & ~pieces_c[player];
        }

        for (int i = 0; i < 8; i++){
            for (uint64_t board = knight_moves[i]; board; board &= board - 1) { 
                int move = __builtin_ctzll(board);
                moves.emplace_back(Move(move - knight_offsets[i], move));
            }
        }

        // -------------------------- Sliding moves -------------------------- //

        //Helper lambda function to generate sliding moves  
        auto gen_sliding = [&](uint64_t pieces, bool is_rook) { 
            for (uint64_t board = pieces; board; board &= board - 1) { //for each straight slider
                int square = __builtin_ctzll(board);
                
                uint64_t mask = get_sliding_piece_mask(square, is_rook);
                uint64_t occupancy = full_spaces & mask;

                uint64_t magic = is_rook ? rook_magics[square] : bishop_magics[square];
                int mask_bits = __builtin_popcountll(mask);
                int index = (occupancy * magic) >> (64 - mask_bits);
            
                uint64_t possible_moves = is_rook ? rook_attacks[square][index] : bishop_attacks[square][index];

                possible_moves &= ~pieces_c[player]; //exclude friendly pieces

                for (uint64_t move = possible_moves; move; move &= move - 1) { //addd the moves !
                    int target = __builtin_ctzll(move);
                    moves.emplace_back(Move(square, target));
                }
            }
        };

        uint64_t straight_sliding_pieces = (pieces_t[ROOK] | pieces_t[QUEEN]) & pieces_c[player];
        uint64_t diagonal_sliding_pieces = (pieces_t[BISHOP] | pieces_t[QUEEN]) & pieces_c[player];

        gen_sliding(straight_sliding_pieces, true);
        gen_sliding(diagonal_sliding_pieces, false);

        // -------------------------- King moves -------------------------- //
        uint64_t king = pieces_t[KING] & pieces_c[player]; 
        if (!king) return moves; // UH OH!

        int king_idx = __builtin_ctzll(king);

        uint64_t a = king & 0xfefefefefefefefeULL; //prevent wraparound
        uint64_t h = king & 0x7f7f7f7f7f7f7f7fULL;

        uint64_t moves_mask = (a << 7) | (king << 8) | (h << 9) |
                              (a >> 1) | (h << 1) |
                              (a >> 9) | (king >> 8) | (h >> 7);

        moves_mask &= ~pieces_c[player];

        for (uint64_t board = moves_mask; board; board &= board - 1) { 
            moves.emplace_back(Move(king_idx, __builtin_ctzll(board)));
        }   

        return moves;
    }

    /* Checks if a king is in check in the current board state */
    bool Board::king_in_check(Colour player) {
        Colour opponent = (player == WHITE) ? BLACK : WHITE;
        uint64_t opponent_attacks = generate_all_attacks_bitboard(opponent);

        return (opponent_attacks & pieces_t[KING] & pieces_c[player]) != 0;
    }

    std::vector<Move> Board::filter_legal_moves(const std::vector<Move>& moves, Colour player) {
        std::vector<Move> legal_moves;
        legal_moves.reserve(moves.size()); 

        for (const Move& move : moves) {
            UndoMove undo = play_move(move); // simulate move

            if (!king_in_check(player)) { //check move
                legal_moves.push_back(move);
            }

            undo_move(undo); // restore state
        }

        return legal_moves;
    }

    std::vector<Move> Board::generate_all_legal_moves(Colour player) {
        return filter_legal_moves(generate_all_moves(player), player);
    }
}
