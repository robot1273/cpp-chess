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
            Piece get_piece_type(int idx);
            Colour get_piece_colour(int idx);
            uint64_t generate_all_attacks_bitboard(Colour player);
            std::vector<Move> generate_all_moves(Colour player);
            std::vector<Move> filter_legal_moves(const std::vector<Move> &moves, Colour player);
            bool king_in_check(Colour player, uint64_t opponent_attacks);

        public: 
            Board();
            
            std::vector<Move> generate_all_legal_moves(Colour player);
            UndoMove play_move(Move move);
            UndoMove play_move(std::string uci);
            void undo_move(UndoMove undo_move);
            void display_board();
    };  

}