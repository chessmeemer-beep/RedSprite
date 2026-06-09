#pragma once

#include "types.h"
#include <cstdint>

namespace RedSprite {

class Bitboard {
public:
    using U64 = uint64_t;
    
    U64 bb;
    
    constexpr Bitboard() : bb(0) {}
    constexpr explicit Bitboard(U64 b) : bb(b) {}
    constexpr explicit Bitboard(Square s) : bb(U64(1) << s) {}
    
    constexpr Bitboard operator|(const Bitboard& o) const { return Bitboard(bb | o.bb); }
    constexpr Bitboard operator&(const Bitboard& o) const { return Bitboard(bb & o.bb); }
    constexpr Bitboard operator^(const Bitboard& o) const { return Bitboard(bb ^ o.bb); }
    constexpr Bitboard operator~() const { return Bitboard(~bb); }
    
    constexpr Bitboard& operator|=(const Bitboard& o) { bb |= o.bb; return *this; }
    constexpr Bitboard& operator&=(const Bitboard& o) { bb &= o.bb; return *this; }
    constexpr Bitboard& operator^=(const Bitboard& o) { bb ^= o.bb; return *this; }
    
    constexpr bool operator==(const Bitboard& o) const { return bb == o.bb; }
    constexpr bool empty() const { return bb == 0; }
    constexpr int count() const { return __builtin_popcountll(bb); }
    constexpr Square lsb() const { return static_cast<Square>(__builtin_ctzll(bb)); }
    constexpr Square msb() const { return static_cast<Square>(63 - __builtin_clzll(bb)); }
    
    constexpr Bitboard pop_lsb() const { return Bitboard(bb & (bb - 1)); }
    
    constexpr U64 value() const { return bb; }
    
    // Shift operations for move generation
    constexpr Bitboard shift_north() const { return Bitboard(bb << 8); }
    constexpr Bitboard shift_south() const { return Bitboard(bb >> 8); }
    constexpr Bitboard shift_east() const { return Bitboard((bb & 0x7f7f7f7f7f7f7f7fULL) << 1); }
    constexpr Bitboard shift_west() const { return Bitboard((bb & 0xfefefefefefefefeULL) >> 1); }
    constexpr Bitboard shift_ne() const { return Bitboard((bb & 0x7f7f7f7f7f7f7f7fULL) << 9); }
    constexpr Bitboard shift_nw() const { return Bitboard((bb & 0xfefefefefefefefeULL) << 7); }
    constexpr Bitboard shift_se() const { return Bitboard((bb & 0x7f7f7f7f7f7f7f7fULL) >> 7); }
    constexpr Bitboard shift_sw() const { return Bitboard((bb & 0xfefefefefefefefeULL) >> 9); }
};

// Precomputed attack tables
class AttackGenerator {
private:
    static Bitboard pawn_attacks[COLOR_NB][SQUARE_NB];
    static Bitboard knight_attacks[SQUARE_NB];
    static Bitboard king_attacks[SQUARE_NB];
    static Bitboard bishop_attacks[SQUARE_NB][512];
    static Bitboard rook_attacks[SQUARE_NB][4096];
    static Bitboard bishop_masks[SQUARE_NB];
    static Bitboard rook_masks[SQUARE_NB];
    
    static U64 sliding_attack(const Bitboard& mask, Square sq, const Bitboard& occupied, int delta);
    
public:
    static void init();
    
    static constexpr Bitboard get_pawn_attacks(Color c, Square s) { return pawn_attacks[c][s]; }
    static constexpr Bitboard get_knight_attacks(Square s) { return knight_attacks[s]; }
    static constexpr Bitboard get_king_attacks(Square s) { return king_attacks[s]; }
    static constexpr Bitboard get_bishop_attacks(Square s, Bitboard occupied) { 
        return bishop_attacks[s][(occupied.value() & bishop_masks[s].value()) >> get_bishop_shift(s)]; 
    }
    static constexpr Bitboard get_rook_attacks(Square s, Bitboard occupied) { 
        return rook_attacks[s][(occupied.value() & rook_masks[s].value()) >> get_rook_shift(s)]; 
    }
    static constexpr Bitboard get_queen_attacks(Square s, Bitboard occupied) {
        return get_bishop_attacks(s, occupied) | get_rook_attacks(s, occupied);
    }
    
    static int get_bishop_shift(Square s);
    static int get_rook_shift(Square s);
};

} // namespace RedSprite
