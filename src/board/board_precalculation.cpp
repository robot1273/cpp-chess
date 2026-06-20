#include "board_precalculation.hpp"
#include "utility.hpp"
#include <vector>

#include <cmath>
#include <iostream>

namespace chess::Global {
    // create references
    std::vector<uint64_t> rook_magics;
    std::vector<uint64_t> bishop_magics;

    std::vector<std::vector<uint64_t>> rook_attacks;
    std::vector<std::vector<uint64_t>> bishop_attacks;

    uint64_t rook_masks[64];
    uint64_t bishop_masks[64];
    uint64_t king_masks[64];
    uint64_t knight_masks[64];
    uint64_t pawn_masks[2][64];

    uint64_t zobrist_piece_keys[64][zobrist_key_length];
    uint64_t zobrist_player_turn_key;

    /* initialise global constants */
    GlobalInit::GlobalInit() {
         // zobrist hash keys
         for (int sq = 0; sq < 64; ++sq) {
             for (int piece = 0; piece < zobrist_key_length; ++piece) {
                 zobrist_piece_keys[sq][piece] = utility::random_u64();
             }
         }
         zobrist_player_turn_key = utility::random_u64();

        // sliding pieces
        for (int sq = 0; sq < 64; ++sq) {
            rook_masks[sq] = get_sliding_piece_mask(sq, true);
            bishop_masks[sq] = get_sliding_piece_mask(sq, false);
        }

        //knights, kings and pawns
        for (int sq = 0; sq < 64; ++sq) {
            uint64_t sq_bb = 1ULL << sq;
            uint64_t a = sq_bb & 0xfefefefefefefefeULL;
            uint64_t h = sq_bb & 0x7f7f7f7f7f7f7f7fULL;

            // knight moves
            uint64_t knights = 0ULL;
            for (int i = 0; i < 8; i++) {
                knights |= (utility::shift(sq_bb, knight_offsets[i]) & knight_offset_masks[i]);
            }
            knight_masks[sq] = knights;

            // king moves
            king_masks[sq] = (a << 7) | (h << 9)  |
                             (a >> 1) | (h << 1)  |
                             (a >> 9) | (h >> 7)  |
                             (sq_bb << 8) | (sq_bb >> 8);

            // pawn moves (attack mask, not actually movement)
            pawn_masks[0][sq] = (a << 7)  | (h << 9); //white
            pawn_masks[1][sq] = (h >> 7)  | (a >> 9); //black
        }

        rook_attacks.resize(64);
        bishop_attacks.resize(64);

        rook_magics = generate_magic_numbers(true, rook_attacks, rook_masks);
        bishop_magics = generate_magic_numbers(false, bishop_attacks, bishop_masks);
    };

/*
 * CODE FOR PRECOMPUTATIONS BELOW
 */

    uint64_t get_sliding_piece_mask(int piece_idx, bool is_rook){
        uint64_t mask = 0ULL;
        const int *directions = is_rook ? rook_directions : bishop_directions;

        for (int i = 0; i < 4; i++){
            int direction = directions[i];
            int current_pos = piece_idx;

            while (true) {
                int next_pos = current_pos + direction;
                if (next_pos < 0 || next_pos >= 64) break;

                int old_rank = current_pos >> 3; int old_file = current_pos & 7;
                int new_rank = next_pos >> 3; int new_file = next_pos & 7;

                if (is_rook) { // wrap-around
                    if ((abs(direction) == 1) && new_rank != old_rank) { break; }
                } else {
                    if (abs(new_rank - old_rank) != abs(new_file - old_file)) { break; }
                }

                mask |= 1ULL << next_pos;
                current_pos = next_pos;
            }
        }

        return mask;
    }

    /*
        Generate the nth occupancy for any given mask
        occupancy_n is in range [0, (2^n)-1] where n is the number of bits in mask
    */
    uint64_t generate_occupancy(uint64_t mask, int occupancy_n) {
        uint64_t occupancy = 0ULL;
        int num_mask_bits = __builtin_popcountll(mask);
        for (int i = 0; i < num_mask_bits; i++){ //Iterate for every bit in the mask
            if (occupancy_n & (1 << i)){  // Check if this value is in occupancy_n
                occupancy |= 1ULL << __builtin_ctzll(mask);  // Add the current LSB to the occupancy
            }
            mask &= mask - 1; //remove the LSB
        }
        return occupancy;
    }

    /*
       Given a starting pos and occupied spaces, generates a bitboard of legal end positions
       is_rook -> true generates rook moves, false generates bishop moves
       Inclusive of the blocking piece!
    */
    uint64_t get_sliding_moves(int piece_idx, uint64_t occupancy, bool is_rook) {
        uint64_t moves = 0ULL;
        const int *directions = is_rook ? rook_directions : bishop_directions;

        for (int i = 0; i < 4; i++){
            int direction = directions[i];
            int current_pos = piece_idx;

            while (true) {
                int next_pos = current_pos + direction;
                if (next_pos < 0 || next_pos >= 64) break;

                int old_rank = current_pos >> 3; int old_file = current_pos & 7;
                int new_rank = next_pos >> 3; int new_file = next_pos & 7;

                if (is_rook) {
                    if ((abs(direction) == 1) && new_rank != old_rank) { break; }
                } else {
                    if (abs(new_rank - old_rank) != abs(new_file - old_file)) { break; }
                }

                uint64_t bb = 1ULL << next_pos;
                moves |= bb;
                if (occupancy & bb){ break; } //If 1, then the space is blocked, so we end this direction
                current_pos = next_pos;
            }
        }

        return moves;
    }

    /*
     * Generates all possible moves for a given square for every permutation of enemies in the way
     */
    std::vector<uint64_t> generate_attacks_for_square(int square,
        bool is_rook) {
        std::vector<uint64_t> attacks;
        uint64_t moves_mask = is_rook ? rook_masks[square] : bishop_masks[square];
        int max_occupancy_n = (1 << __builtin_popcountll(moves_mask));

        for (int occupancy_n = 0; occupancy_n < max_occupancy_n; occupancy_n++){
            uint64_t occupancy_mask = generate_occupancy(moves_mask, occupancy_n);
            attacks.emplace_back(get_sliding_moves(square, occupancy_mask, is_rook));
        }

        return attacks;
    }

    /**
     * Returns a vector of length 64 for a magic number for each tile
     * Sets the attack table to the corresponding values for these magic numbers, modifying it
     */
    std::vector<uint64_t> generate_magic_numbers(bool is_rook,
        std::vector<std::vector<uint64_t>>& attack_table,
        const uint64_t* masks){

        int max_iterations = 100000000;
        std::vector<uint64_t> magics;

        for (int sq = 0; sq < 64; sq++){
            uint64_t mask = masks[sq];
            std::vector<uint64_t> attacks = generate_attacks_for_square(sq, is_rook);

            int mask_bits = __builtin_popcountll(mask);
            int shift_value = 64 - mask_bits;
            int max_occupancy_n = (1 << mask_bits);

            bool failed = true; // assume failiure until magic is found for square
            std::vector<uint64_t> used_attacks(max_occupancy_n, 0ULL);
            std::vector<bool> has_been_used(max_occupancy_n);

            for (int i = 0; i < max_iterations; i++){
                uint64_t magic = utility::random_u64() & utility::random_u64() & utility::random_u64(); //sparse magic number
                if (__builtin_popcountll((mask * magic) & 0xFF00000000000000ULL) < 8) { continue; } // skip magics that dont use upper bits
                std::fill(has_been_used.begin(), has_been_used.end(), false); //reset used moves
                bool failed_iteration = false;

                for (int j = 0; j < max_occupancy_n; j++){
                    uint64_t occupancy_mask = generate_occupancy(mask, j);
                    int index = (occupancy_mask * magic) >> shift_value; // test the magic number

                    if (!has_been_used[index]) {
                        has_been_used[index] = true;
                        used_attacks[index] = attacks[j];
                    } else if (used_attacks[index] != attacks[j]) {
                        failed_iteration = true;
                        break;
                    }
                }

                if (!failed_iteration){ //magic found for square
                    magics.emplace_back(magic);
                    attack_table[sq] = used_attacks;
                    failed = false;
                    break;
                }
            }

            if (failed) {
                std::cout << "CRITICAL ERROR: Failed to generate a magic number for " << sq << std::endl;
                throw std::runtime_error("Fatal error - magic numbers not generated");
            }

        }

        return magics;
    }

}
