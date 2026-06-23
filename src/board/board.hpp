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

    struct MoveList {
        Move moves[256];
        int count = 0;

        inline void push_back(const Move& move) {
            moves[count++] = move;
        }

        // Allow range-based for loops
        inline Move* begin() { return &moves[0]; }
        inline Move* end() { return &moves[count]; }
        inline int size() const { return count; }
        inline bool empty() const { return count == 0; }
        inline Move get(int index) const { return moves[index]; }
    };

    // to track a hashed game state
    struct GameState {
        uint64_t hash;
        int half_move_counter;
    };

    class Board {
        private:
            uint64_t pieces_t[6]; // Pieces grouped by type
            uint64_t pieces_c[2]; // Pieces grouped by colour
            uint64_t en_passant_moves = 0ULL; //imaginary spaces that can be taken by pawns only. (only ever a single bit should be set)
            uint8_t castling_rights = 0b1111; // All rooks can castle. WL, WR, BL, BR respectively

            // precalculated masks and attack tables are globally initialised
            // using the chess::Global namespace

            Colour current_turn = Colour::WHITE;
            uint64_t current_hash;
            int half_move_counter = 0; // tracks the number of *half* moves since capture/pawn mov
            int full_moves_played = 1; // incremented after each black move

            std::vector<GameState> game_state_stack;

        private:
            bool is_square_attacked(int sq, Colour player) const;
            int get_zobrist_piece_index(int square) const;
            int get_zobrist_piece_index(Piece p, Colour c) const;
        public:
            void reset() {
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
                current_turn = WHITE;

                current_hash = generate_zobrist_hash(current_turn);
            }

            Board(){
                reset(); //setup initial board state
            }

            void generate_all_moves(Colour player, MoveList& moves) const;
            void generate_all_legal_moves(Colour player, MoveList& moves) const;
            void generate_all_legal_capture_moves(Colour player, MoveList& moves) const;

            UndoMove play_move(Move move);
            UndoMove play_move(std::string uci);
            void undo_move(UndoMove undo_move);

            UndoMove play_null_move();
            void undo_null_move(UndoMove undo_move);

            void display_board(bool fancy) const;

            bool king_in_check(Colour player) const;
            bool is_legal(const Move& move, Colour player) const;
            bool is_legal(const Move& move, Colour player, int king_sq, Colour enemy) const;

            bool is_draw() const; // checks if current game state is a draw
            bool insufficient_material() const;

            void update_zobrist_hash(const Move& move); // update hash with a given move
            uint64_t generate_zobrist_hash(Colour player_to_move) const; // of current board state

            void load_position(const std::string& fen);
            std::string get_fen() const;

            inline Colour get_current_player() const { return current_turn; }
            inline uint64_t get_piece_type_mask(Piece piece) const { return pieces_t[piece]; }
            inline uint64_t get_piece_colour_mask(Colour colour) const { return pieces_c[colour]; }
            inline uint64_t get_current_hash() const { return current_hash; }

            inline Piece get_piece_type(int idx) const {
                uint64_t mask = 1ULL << idx;
                for (int i = 0; i < 6; i++) {
                    if (pieces_t[i] & mask) return Piece(i);
                }
                return NONE;
            }

            inline Colour get_piece_colour(int idx) const {
                if ((pieces_c[WHITE] >> idx) & 1ULL) return WHITE;
                if ((pieces_c[BLACK] >> idx) & 1ULL) return BLACK;
                return NO_COLOUR;
            }

    };

}
