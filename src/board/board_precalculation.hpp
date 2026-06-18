#pragma once

#include <cstdint>
#include <vector>

namespace chess::Global  {
    /* rook and bishop magics to compute all sliding piece attacks */
    extern std::vector<uint64_t> rook_magics;
    extern std::vector<uint64_t> bishop_magics;

    /* rook and bishop attack tables indexed by [square][magic] */
    extern std::vector<std::vector<uint64_t>> rook_attacks;
    extern std::vector<std::vector<uint64_t>> bishop_attacks;

    /* piece movement masks */
    extern uint64_t rook_masks[64];
    extern uint64_t bishop_masks[64];
    extern uint64_t king_masks[64];
    extern uint64_t knight_masks[64];
    extern uint64_t pawn_masks[2][64]; // for first idx 0 white, 1 black

    inline constexpr int rook_directions[4]   = {8, -8, 1, -1};
    inline constexpr int bishop_directions[4] = {9, -9, 7, -7};

    inline constexpr int knight_offsets[8] = {-17, -15, -10, -6, 6, 10, 15, 17};
    inline constexpr uint64_t knight_offset_masks[8] = { // each mask corresponds to an offset
        0x7F7F7F7F7F7F7F7FULL,
        0xFEFEFEFEFEFEFEFEULL,
        0x3F3F3F3F3F3F3F3FULL,
        0xFCFCFCFCFCFCFCFCULL,
        0x3F3F3F3F3F3F3F3FULL,
        0xFCFCFCFCFCFCFCFCULL,
        0x7F7F7F7F7F7F7F7FULL,
        0xFEFEFEFEFEFEFEFEULL
    };

    constexpr int zobrist_key_length = 12;
    extern uint64_t zobrist_piece_keys[64][zobrist_key_length];
    extern uint64_t zobrist_player_turn_key;

    /**
     * generates sliding piece movement mask
     * should not be used outside of pre computation
     */
    uint64_t get_sliding_piece_mask(int piece_idx, bool is_rook);

    std::vector<uint64_t> generate_magic_numbers(bool is_rook,
        std::vector<std::vector<uint64_t>>& attack_table,
        const uint64_t* masks);

    struct GlobalInit { GlobalInit(); };
    inline GlobalInit initialiser;
}
