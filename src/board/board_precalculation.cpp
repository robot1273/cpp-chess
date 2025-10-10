#include "utility.hpp"
#include "board_precalculation.hpp"
#include <vector>
#include <random>

namespace chess
{
    int rook_directions[4]   = {8, -8, 1, -1};
    int bishop_directions[4] = {9, -9, 7, -7};
    
    /* 
       Makes a mask for a rook or bishop given starting position
    */
    uint64_t get_sliding_piece_mask(int piece_idx, bool is_rook){
        uint64_t mask = 0ULL;
        int *directions = is_rook ? rook_directions : bishop_directions;

        for (int i = 0; i < 4; i++){
            int direction = directions[i];
            int current_pos = piece_idx;

            while (true) {
                int next_pos = current_pos + direction;
                if (next_pos < 0 || next_pos >= 64) break;

                int old_rank = current_pos >> 3; int old_file = current_pos & 7;
                int new_rank = next_pos >> 3; int new_file = next_pos & 7;

                //if (new_rank == 0 || new_rank == 7 || new_file == 0 || new_file == 7) { break; } // do not include edges
                
                if (is_rook) {
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
        int *directions = is_rook ? rook_directions : bishop_directions;

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
    std::vector<uint64_t> generate_attacks_for_square(int square, bool is_rook) {
        std::vector<uint64_t> attacks;
        uint64_t moves_mask = get_sliding_piece_mask(square, is_rook);
        int max_occupancy_n = (1 << __builtin_popcountll(moves_mask));

        for (int occupancy_n = 0; occupancy_n < max_occupancy_n; occupancy_n++){
            uint64_t occupancy_mask = generate_occupancy(moves_mask, occupancy_n);
            attacks.emplace_back(get_sliding_moves(square, occupancy_mask, is_rook));
        }
        
        return attacks;
    }
    
    uint64_t random_u64() {
        static uint64_t x = 127369420ULL;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        return x;
    }

    /**
     * Returns a vector of length 64 for a magic number for each tile
     * Sets the attack table to the corresponding values for these magic numbers, modifying it
     */
    std::vector<uint64_t> generate_magic_numbers(bool is_rook, std::vector<std::vector<uint64_t>>& attack_table){
        int max_iterations = 100000000;
        std::vector<uint64_t> magics;

        for (int sq = 0; sq < 64; sq++){
            uint64_t mask = get_sliding_piece_mask(sq, is_rook);
            std::vector<uint64_t> attacks = generate_attacks_for_square(sq, is_rook);
            
            int mask_bits = __builtin_popcountll(mask);
            int shift_value = 64 - mask_bits;
            int max_occupancy_n = (1 << mask_bits);
            
            bool failed = true; // assume failiure until magic is found for square
            std::vector<uint64_t> used_attacks(max_occupancy_n, 0ULL);
            std::vector<bool> has_been_used(max_occupancy_n);

            for (int i = 0; i < max_iterations; i++){    
                uint64_t magic = random_u64() & random_u64() & random_u64(); //sparse magic number
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
