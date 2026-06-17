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

            // precalculated masks and attack tables are globally initialised
            // using the chess::Global namespace

            Colour current_turn = Colour::WHITE;

        private:
            Piece get_piece_type(int idx) const;
            Colour get_piece_colour(int idx) const;

            bool is_square_attacked(int sq, Colour player) const;
            bool king_in_check(Colour player) const;
            bool is_legal(const Move& move, Colour player);

            std::vector<Move> generate_all_moves(Colour player) const;
        public:
            Board();
            void reset();

            std::vector<Move> generate_all_legal_moves(Colour player);
            UndoMove play_move(Move move);
            UndoMove play_move(std::string uci);
            void undo_move(UndoMove undo_move);
            void display_board() const;
            bool validate() const;

            inline uint64_t get_piece_type(Piece piece) const { return pieces_t[piece]; } //External read
            inline uint64_t get_piece_colour(Colour colour) const { return pieces_c[colour]; }
    };

}
