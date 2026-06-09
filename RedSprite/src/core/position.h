#pragma once

#include "types.h"
#include "bitboard.h"
#include "move.h"
#include <array>
#include <cstring>

namespace RedSprite {

class Position {
private:
    // Piece lists: piece_lists[piece_type][square] = count
    std::array<Bitboard, PIECE_NB> pieces;
    std::array<Bitboard, COLOR_NB> side_pieces;
    std::array<Bitboard, PIECE_TYPE_NB> piece_types;
    
    Bitboard occupied;
    Bitboard checkers;      // Pieces giving check
    Bitboard pinned;        // Pinned pieces
    Bitboard king_attacks;  // Squares attacked by king
    
    Square castling_rights; // Bitmask of castling rights (KQkq)
    Square ep_square;       // En passant square (SQUARE_NONE if none)
    
    Color side_to_move;
    int halfmove_clock;
    int fullmove_number;
    
    ZobristHash zobrist_key;
    
    // For move legality checking
    bool in_check;
    
public:
    Position();
    
    // Initialize from FEN string
    bool set_from_fen(const char* fen);
    
    // Get FEN string
    std::string get_fen() const;
    
    // Accessors
    constexpr Piece piece_at(Square s) const {
        for (int p = W_PAWN; p <= B_KING; ++p) {
            if (pieces[p].bb & (Bitboard::U64(1) << s)) {
                return static_cast<Piece>(p);
            }
        }
        return NO_PIECE;
    }
    
    constexpr Bitboard get_pieces(Piece p) const { return pieces[p]; }
    constexpr Bitboard get_pieces(Color c) const { return side_pieces[c]; }
    constexpr Bitboard get_pieces(PieceType pt) const { return piece_types[pt]; }
    constexpr Bitboard get_occupied() const { return occupied; }
    constexpr Bitboard get_checkers() const { return checkers; }
    constexpr Bitboard get_pinned() const { return pinned; }
    
    constexpr Color get_side_to_move() const { return side_to_move; }
    constexpr Square get_ep_square() const { return ep_square; }
    constexpr int get_halfmove_clock() const { return halfmove_clock; }
    constexpr int get_fullmove_number() const { return fullmove_number; }
    constexpr Square get_castling_rights() const { return castling_rights; }
    
    constexpr bool is_in_check() const { return in_check; }
    constexpr ZobristHash get_zobrist_key() const { return zobrist_key; }
    
    // Check if square is attacked by color
    bool is_attacked_by(Square s, Color by) const;
    
    // Make/unmake moves
    void make_move(const Move& m);
    void unmake_move(const Move& m, Piece captured, bool was_promotion, PieceType promo_type);
    
    // Give/lose castling rights
    void update_castling_rights(Move m);
    
    // Detect check
    void update_check_info();
    
    // Material count
    int material_count(PieceType pt, Color c) const;
    
    // Is it a draw by insufficient material?
    bool is_insufficient_material() const;
};

// Zobrist hashing
class ZobristHash {
private:
    uint64_t key;
    
public:
    static uint64_t piece_keys[PIECE_NB][SQUARE_NB];
    static uint64_t ep_keys[SQUARE_NB];
    static uint64_t castling_keys[16];
    static uint64_t side_key;
    
    static void init();
    
    constexpr ZobristHash() : key(0) {}
    constexpr uint64_t value() const { return key; }
    
    constexpr void xor_piece(Piece p, Square s) { key ^= piece_keys[p][s]; }
    constexpr void xor_ep(Square s) { key ^= ep_keys[s]; }
    constexpr void xor_castling(int rights) { key ^= castling_keys[rights]; }
    constexpr void xor_side() { key ^= side_key; }
    
    constexpr ZobristHash operator^(uint64_t v) const { 
        ZobristHash h; h.key = key ^ v; return h; 
    }
};

} // namespace RedSprite
