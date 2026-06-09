#include "bitboard.h"

namespace RedSprite {

Bitboard AttackGenerator::pawn_attacks[COLOR_NB][SQUARE_NB];
Bitboard AttackGenerator::knight_attacks[SQUARE_NB];
Bitboard AttackGenerator::king_attacks[SQUARE_NB];
Bitboard AttackGenerator::bishop_attacks[SQUARE_NB][512];
Bitboard AttackGenerator::rook_attacks[SQUARE_NB][4096];
Bitboard AttackGenerator::bishop_masks[SQUARE_NB];
Bitboard AttackGenerator::rook_masks[SQUARE_NB];

Bitboard::U64 AttackGenerator::sliding_attack(const Bitboard& mask, Square sq, const Bitboard& occupied, int delta) {
    Bitboard attacks = 0;
    int s = static_cast<int>(sq);
    
    while (true) {
        s += delta;
        if (s < 0 || s > 63 || !(mask.bb & (Bitboard::U64(1) << s))) break;
        attacks.bb |= (Bitboard::U64(1) << s);
        if (occupied.bb & (Bitboard::U64(1) << s)) break;
    }
    return attacks;
}

int AttackGenerator::get_bishop_shift(Square s) {
    // Bishop: 4 directions, varying bits per square
    // Max 9 bits needed for most squares
    return 64 - __builtin_popcountll(bishop_masks[s].bb);
}

int AttackGenerator::get_rook_shift(Square s) {
    // Rook: 4 directions, varying bits per square  
    // Max 12 bits needed for center squares
    return 64 - __builtin_popcountll(rook_masks[s].bb);
}

void AttackGenerator::init() {
    // Initialize pawn attacks
    for (int c = WHITE; c <= BLACK; ++c) {
        for (int s = SQ_A1; s <= SQ_H8; ++s) {
            Bitboard b = Bitboard(static_cast<Square>(s));
            if (c == WHITE) {
                Bitboard attacks = 0;
                if (rank_of(static_cast<Square>(s)) < 7) {
                    if (file_of(static_cast<Square>(s)) > 0) attacks |= b.shift_nw();
                    if (file_of(static_cast<Square>(s)) < 7) attacks |= b.shift_ne();
                }
                pawn_attacks[c][s] = attacks;
            } else {
                Bitboard attacks = 0;
                if (rank_of(static_cast<Square>(s)) > 0) {
                    if (file_of(static_cast<Square>(s)) > 0) attacks |= b.shift_sw();
                    if (file_of(static_cast<Square>(s)) < 7) attacks |= b.shift_se();
                }
                pawn_attacks[c][s] = attacks;
            }
        }
    }
    
    // Initialize knight attacks
    for (int s = SQ_A1; s <= SQ_H8; ++s) {
        Bitboard b = Bitboard(static_cast<Square>(s));
        Bitboard attacks = 0;
        attacks |= b.shift_north().shift_east().shift_east();
        attacks |= b.shift_north().shift_west().shift_west();
        attacks |= b.shift_south().shift_east().shift_east();
        attacks |= b.shift_south().shift_west().shift_west();
        attacks |= b.shift_east().shift_north().shift_north();
        attacks |= b.shift_east().shift_south().shift_south();
        attacks |= b.shift_west().shift_north().shift_north();
        attacks |= b.shift_west().shift_south().shift_south();
        knight_attacks[s] = attacks;
    }
    
    // Initialize king attacks
    for (int s = SQ_A1; s <= SQ_H8; ++s) {
        Bitboard b = Bitboard(static_cast<Square>(s));
        Bitboard attacks = 0;
        attacks |= b.shift_north();
        attacks |= b.shift_south();
        attacks |= b.shift_east();
        attacks |= b.shift_west();
        attacks |= b.shift_ne();
        attacks |= b.shift_nw();
        attacks |= b.shift_se();
        attacks |= b.shift_sw();
        king_attacks[s] = attacks;
    }
    
    // Initialize sliding piece masks and attacks
    for (int s = SQ_A1; s <= SQ_H8; ++s) {
        Bitboard edge = Bitboard(0x8181818181818181ULL); // A and H files
        Bitboard rank_edge = Bitboard(0xFF00000000000000ULL | 0xFFULL); // Rank 1 and 8
        
        // Bishop mask
        bishop_masks[s] = ~(edge | rank_edge) & (
            sliding_attack(~edge, static_cast<Square>(s), 0, 9) |
            sliding_attack(~edge, static_cast<Square>(s), 0, 7) |
            sliding_attack(~edge, static_cast<Square>(s), 0, -9) |
            sliding_attack(~edge, static_cast<Square>(s), 0, -7)
        );
        
        // Rook mask
        rook_masks[s] = ~(rank_edge) & (
            sliding_attack(rank_edge, static_cast<Square>(s), 0, 8) |
            sliding_attack(rank_edge, static_cast<Square>(s), 0, -8) |
            sliding_attack(Bitboard(0x0101010101010101ULL << file_of(static_cast<Square>(s))), 
                          static_cast<Square>(s), 0, 1) |
            sliding_attack(Bitboard(0x8080808080808080ULL >> (7 - file_of(static_cast<Square>(s)))), 
                          static_cast<Square>(s), 0, -1)
        );
        
        // Generate bishop attacks for all occupancy combinations
        int bishop_bits = bishop_masks[s].count();
        for (int i = 0; i < (1 << bishop_bits); ++i) {
            Bitboard occupied = 0;
            Bitboard mask = bishop_masks[s];
            for (int j = 0; j < bishop_bits; ++j) {
                if (i & (1 << j)) {
                    occupied.bb |= (Bitboard::U64(1) << mask.lsb());
                    mask = mask.pop_lsb();
                }
            }
            
            Bitboard attacks = 0;
            attacks |= sliding_attack(~edge, static_cast<Square>(s), occupied, 9);
            attacks |= sliding_attack(~edge, static_cast<Square>(s), occupied, 7);
            attacks |= sliding_attack(~edge, static_cast<Square>(s), occupied, -9);
            attacks |= sliding_attack(~edge, static_cast<Square>(s), occupied, -7);
            bishop_attacks[s][i] = attacks;
        }
        
        // Generate rook attacks for all occupancy combinations
        int rook_bits = rook_masks[s].count();
        for (int i = 0; i < (1 << rook_bits); ++i) {
            Bitboard occupied = 0;
            Bitboard mask = rook_masks[s];
            for (int j = 0; j < rook_bits; ++j) {
                if (i & (1 << j)) {
                    occupied.bb |= (Bitboard::U64(1) << mask.lsb());
                    mask = mask.pop_lsb();
                }
            }
            
            Bitboard attacks = 0;
            attacks |= sliding_attack(rank_edge, static_cast<Square>(s), occupied, 8);
            attacks |= sliding_attack(rank_edge, static_cast<Square>(s), occupied, -8);
            attacks |= sliding_attack(Bitboard(0x0101010101010101ULL << file_of(static_cast<Square>(s))), 
                                     static_cast<Square>(s), occupied, 1);
            attacks |= sliding_attack(Bitboard(0x8080808080808080ULL >> (7 - file_of(static_cast<Square>(s)))), 
                                     static_cast<Square>(s), occupied, -1);
            rook_attacks[s][i] = attacks;
        }
    }
}

} // namespace RedSprite
