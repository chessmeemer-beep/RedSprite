#pragma once

#include "types.h"
#include "bitboard.h"

namespace RedSprite {

enum MoveType : int {
    MOVE_NONE = 0,
    MOVE_NORMAL = 1,
    MOVE_PROMOTION = 2,
    MOVE_ENPASSANT = 3,
    MOVE_CASTLING = 4
};

struct Move {
private:
    uint16_t data;
    
public:
    constexpr Move() : data(0) {}
    constexpr Move(Square from, Square to, MoveType type = MOVE_NORMAL, PieceType promotion = NO_PIECE_TYPE) 
        : data(static_cast<uint16_t>(from | (to << 6) | (type << 12) | (promotion << 14))) {}
    
    constexpr Square from_sq() const { return static_cast<Square>(data & 0x3F); }
    constexpr Square to_sq() const { return static_cast<Square>((data >> 6) & 0x3F); }
    constexpr MoveType type() const { return static_cast<MoveType>((data >> 12) & 0x3); }
    constexpr PieceType promotion_type() const { return static_cast<PieceType>((data >> 14) & 0x7); }
    
    constexpr bool operator==(const Move& o) const { return data == o.data; }
    constexpr bool operator!=(const Move& o) const { return data != o.data; }
    constexpr bool is_valid() const { return data != 0; }
    
    constexpr uint16_t value() const { return data; }
};

struct ScoredMove {
    Move move;
    int score;
    
    constexpr ScoredMove() : move(), score(0) {}
    constexpr ScoredMove(Move m, int s = 0) : move(m), score(s) {}
};

class Position;

class MoveGenerator {
private:
    const Position* pos;
    Color us;
    Bitboard their_attacks;
    
    void generate_pawn_moves(ScoredMove* moves, int& count, Bitboard targets, bool captures_only);
    void generate_knight_moves(ScoredMove* moves, int& count, Bitboard targets);
    void generate_bishop_moves(ScoredMove* moves, int& count, Bitboard targets);
    void generate_rook_moves(ScoredMove* moves, int& count, Bitboard targets);
    void generate_queen_moves(ScoredMove* moves, int& count, Bitboard targets);
    void generate_king_moves(ScoredMove* moves, int& count, Bitboard targets, bool check_evasion);
    
public:
    explicit MoveGenerator(const Position* p) : pos(p), us(NO_PIECE), their_attacks(0) {}
    
    // Generate all legal moves
    int generate_legal(ScoredMove* moves);
    
    // Generate only captures
    int generate_captures(ScoredMove* moves);
    
    // Generate quiet moves (non-captures)
    int generate_quiets(ScoredMove* moves);
    
    // Generate evasion moves (when in check)
    int generate_evasions(ScoredMove* moves);
    
    // Check if a move is legal (for quick validation)
    bool is_legal(const Move& move);
    
    // Compute attack bitboards
    Bitboard compute_attacks(Color c) const;
    Bitboard compute_slider_attacks(PieceType pt, Square sq, Bitboard occupied) const;
};

} // namespace RedSprite
