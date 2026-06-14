#pragma once

#include "enums.hpp"
#include "move.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace chess
{
    inline const std::string WHITE_PIECES[6] = {"♟","♞","♝","♜","♛","♚"};
    inline const std::string BLACK_PIECES[6] = {"♙","♘","♗","♖","♕","♔"};

    class Board {
        private:
            uint64_t pieces_t[6]; // Pieces grouped by type
            uint64_t pieces_c[2]; // Pieces grouped by colour
            uint64_t en_passant_moves = 0ULL; //imaginary spaces that can be taken by pawns only. (only ever a single bit should be set)
            uint8_t castling_rights = 0b1111; // All rooks can castle. WL, WR, BL, BR respectively

            std::vector<std::vector<uint64_t>> rook_attacks;
            std::vector<std::vector<uint64_t>> bishop_attacks;

            std::vector<uint64_t> rook_magics;
            std::vector<uint64_t> bishop_magics;

            Colour current_turn = Colour::WHITE;

        private:
            Piece const get_piece_type(int idx);
            Colour const get_piece_colour(int idx);
            uint64_t const generate_all_attacks_bitboard(Colour player);
            std::vector<Move> const generate_all_moves(Colour player);
            std::vector<Move> const filter_legal_moves(const std::vector<Move> &moves, Colour player);

            bool const is_square_attacked(int sq, Colour player);
            bool const king_in_check(Colour player);

        public:
            Board();

            std::vector<Move> const generate_all_legal_moves(Colour player);
            UndoMove play_move(Move move);
            UndoMove play_move(std::string uci);
            void undo_move(UndoMove undo_move);
            void display_board();

            inline uint64_t const get_piece_type(Piece piece){ return pieces_t[piece]; } //External read
            inline uint64_t const get_piece_colour(Colour colour){ return pieces_c[colour]; }
    };

}
