#include "move.h"
#include "position.h"

namespace RedSprite {

Bitboard MoveGenerator::compute_attacks(Color c) const {
    Bitboard attacks(0);
    Bitboard our_pieces = pos->get_pieces(c);
    
    // Pawn attacks
    if (c == WHITE) {
        Bitboard pawns = pos->get_pieces(W_PAWN);
        attacks |= pawns.shift_ne() | pawns.shift_nw();
    } else {
        Bitboard pawns = pos->get_pieces(B_PAWN);
        attacks |= pawns.shift_se() | pawns.shift_sw();
    }
    
    // Knight attacks
    Bitboard knights = pos->get_pieces(KNIGHT) & our_pieces;
    while (!knights.empty()) {
        Square s = knights.lsb();
        knights = knights.pop_lsb();
        attacks |= AttackGenerator::get_knight_attacks(s);
    }
    
    // Bishop attacks
    Bitboard bishops = pos->get_pieces(BISHOP) & our_pieces;
    while (!bishops.empty()) {
        Square s = bishops.lsb();
        bishops = bishops.pop_lsb();
        attacks |= AttackGenerator::get_bishop_attacks(s, pos->get_occupied());
    }
    
    // Rook attacks
    Bitboard rooks = pos->get_pieces(ROOK) & our_pieces;
    while (!rooks.empty()) {
        Square s = rooks.lsb();
        rooks = rooks.pop_lsb();
        attacks |= AttackGenerator::get_rook_attacks(s, pos->get_occupied());
    }
    
    // Queen attacks
    Bitboard queens = pos->get_pieces(QUEEN) & our_pieces;
    while (!queens.empty()) {
        Square s = queens.lsb();
        queens = queens.pop_lsb();
        attacks |= AttackGenerator::get_queen_attacks(s, pos->get_occupied());
    }
    
    // King attacks
    Bitboard kings = pos->get_pieces(KING) & our_pieces;
    while (!kings.empty()) {
        Square s = kings.lsb();
        kings = kings.pop_lsb();
        attacks |= AttackGenerator::get_king_attacks(s);
    }
    
    return attacks;
}

Bitboard MoveGenerator::compute_slider_attacks(PieceType pt, Square sq, Bitboard occupied) const {
    if (pt == BISHOP || pt == QUEEN) {
        return AttackGenerator::get_bishop_attacks(sq, occupied);
    } else if (pt == ROOK || pt == QUEEN) {
        return AttackGenerator::get_rook_attacks(sq, occupied);
    }
    return Bitboard(0);
}

void MoveGenerator::generate_pawn_moves(ScoredMove* moves, int& count, Bitboard targets, bool captures_only) {
    Color us = pos->get_side_to_move();
    Color them = static_cast<Color>(1 - us);
    
    Bitboard pawns = pos->get_pieces(PAWN) & pos->get_pieces(us);
    Bitboard ep_sq = pos->get_ep_square() != SQUARE_NONE ? Bitboard(pos->get_ep_square()) : Bitboard(0);
    
    if (us == WHITE) {
        Bitboard single_push = pawns.shift_north() & ~pos->get_occupied();
        Bitboard double_push = (single_push & Bitboard(0x00FF000000000000ULL)).shift_north() & ~pos->get_occupied();
        Bitboard capt_ne = pawns.shift_ne() & pos->get_pieces(them);
        Bitboard capt_nw = pawns.shift_nw() & pos->get_pieces(them);
        
        // En passant
        Bitboard ep_captures = ((pawns.shift_ne() | pawns.shift_nw()) & ep_sq);
        
        if (!captures_only) {
            // Single push
            Bitboard promos = single_push & Bitboard(0xFF00000000000000ULL);
            Bitboard quiet = single_push & ~Bitboard(0xFF00000000000000ULL);
            
            while (!quiet.empty()) {
                Square to = quiet.lsb();
                quiet = quiet.pop_lsb();
                Square from = static_cast<Square>(to - 8);
                moves[count++] = ScoredMove(Move(from, to));
            }
            
            // Double push
            while (!double_push.empty()) {
                Square to = double_push.lsb();
                double_push = double_push.pop_lsb();
                Square from = static_cast<Square>(to - 16);
                moves[count++] = ScoredMove(Move(from, to));
            }
            
            // Promotions
            PieceType promos_list[] = {KNIGHT, BISHOP, ROOK, QUEEN};
            while (!promos.empty()) {
                Square to = promos.lsb();
                promos = promos.pop_lsb();
                Square from = static_cast<Square>(to - 8);
                for (PieceType pt : promos_list) {
                    moves[count++] = ScoredMove(Move(from, to, MOVE_PROMOTION, pt));
                }
            }
        }
        
        // Captures
        Bitboard all_captures = capt_ne | capt_nw | ep_captures;
        while (!all_captures.empty()) {
            Square to = all_captures.lsb();
            all_captures = all_captures.pop_lsb();
            
            if (ep_sq.bb & (Bitboard::U64(1) << to)) {
                Square from = (rank_of(to) == 5) ? static_cast<Square>(to + 7) : static_cast<Square>(to + 9);
                if (file_of(from) > 7 || file_of(from) < 0) {
                    from = (rank_of(to) == 5) ? static_cast<Square>(to + 9) : static_cast<Square>(to + 7);
                }
                moves[count++] = ScoredMove(Move(from, to, MOVE_ENPASSANT));
            } else if (rank_of(to) == 7) {
                Square from = static_cast<Square>(to - 8);
                PieceType promos_list[] = {KNIGHT, BISHOP, ROOK, QUEEN};
                for (PieceType pt : promos_list) {
                    moves[count++] = ScoredMove(Move(from, to, MOVE_PROMOTION, pt));
                }
            } else {
                Square from = (file_of(to) < file_of(pawns.lsb())) ? static_cast<Square>(to + 7) : static_cast<Square>(to + 9);
                // Recalculate from based on which capture
                if (capt_ne.bb & (Bitboard::U64(1) << to)) {
                    from = static_cast<Square>(to - 7);
                } else {
                    from = static_cast<Square>(to - 9);
                }
                moves[count++] = ScoredMove(Move(from, to));
            }
        }
    } else {
        Bitboard single_push = pawns.shift_south() & ~pos->get_occupied();
        Bitboard double_push = (single_push & Bitboard(0x000000000000FF00ULL)).shift_south() & ~pos->get_occupied();
        Bitboard capt_se = pawns.shift_se() & pos->get_pieces(them);
        Bitboard capt_sw = pawns.shift_sw() & pos->get_pieces(them);
        
        // En passant
        Bitboard ep_captures = ((pawns.shift_se() | pawns.shift_sw()) & ep_sq);
        
        if (!captures_only) {
            // Single push
            Bitboard promos = single_push & Bitboard(0x00000000000000FFULL);
            Bitboard quiet = single_push & ~Bitboard(0x00000000000000FFULL);
            
            while (!quiet.empty()) {
                Square to = quiet.lsb();
                quiet = quiet.pop_lsb();
                Square from = static_cast<Square>(to + 8);
                moves[count++] = ScoredMove(Move(from, to));
            }
            
            // Double push
            while (!double_push.empty()) {
                Square to = double_push.lsb();
                double_push = double_push.pop_lsb();
                Square from = static_cast<Square>(to + 16);
                moves[count++] = ScoredMove(Move(from, to));
            }
            
            // Promotions
            PieceType promos_list[] = {KNIGHT, BISHOP, ROOK, QUEEN};
            while (!promos.empty()) {
                Square to = promos.lsb();
                promos = promos.pop_lsb();
                Square from = static_cast<Square>(to + 8);
                for (PieceType pt : promos_list) {
                    moves[count++] = ScoredMove(Move(from, to, MOVE_PROMOTION, pt));
                }
            }
        }
        
        // Captures
        Bitboard all_captures = capt_se | capt_sw | ep_captures;
        while (!all_captures.empty()) {
            Square to = all_captures.lsb();
            all_captures = all_captures.pop_lsb();
            
            if (ep_sq.bb & (Bitboard::U64(1) << to)) {
                Square from = (rank_of(to) == 4) ? static_cast<Square>(to - 7) : static_cast<Square>(to - 9);
                if (file_of(from) > 7 || file_of(from) < 0) {
                    from = (rank_of(to) == 4) ? static_cast<Square>(to - 9) : static_cast<Square>(to - 7);
                }
                moves[count++] = ScoredMove(Move(from, to, MOVE_ENPASSANT));
            } else if (rank_of(to) == 0) {
                Square from = static_cast<Square>(to + 8);
                PieceType promos_list[] = {KNIGHT, BISHOP, ROOK, QUEEN};
                for (PieceType pt : promos_list) {
                    moves[count++] = ScoredMove(Move(from, to, MOVE_PROMOTION, pt));
                }
            } else {
                Square from = (file_of(to) < file_of(pawns.lsb())) ? static_cast<Square>(to + 7) : static_cast<Square>(to + 9);
                if (capt_se.bb & (Bitboard::U64(1) << to)) {
                    from = static_cast<Square>(to + 7);
                } else {
                    from = static_cast<Square>(to + 9);
                }
                moves[count++] = ScoredMove(Move(from, to));
            }
        }
    }
}

void MoveGenerator::generate_knight_moves(ScoredMove* moves, int& count, Bitboard targets) {
    Color us = pos->get_side_to_move();
    Bitboard knights = pos->get_pieces(KNIGHT) & pos->get_pieces(us);
    
    while (!knights.empty()) {
        Square from = knights.lsb();
        knights = knights.pop_lsb();
        Bitboard attacks = AttackGenerator::get_knight_attacks(from) & targets;
        
        while (!attacks.empty()) {
            Square to = attacks.lsb();
            attacks = attacks.pop_lsb();
            moves[count++] = ScoredMove(Move(from, to));
        }
    }
}

void MoveGenerator::generate_bishop_moves(ScoredMove* moves, int& count, Bitboard targets) {
    Color us = pos->get_side_to_move();
    Bitboard bishops = pos->get_pieces(BISHOP) & pos->get_pieces(us);
    
    while (!bishops.empty()) {
        Square from = bishops.lsb();
        bishops = bishops.pop_lsb();
        Bitboard attacks = AttackGenerator::get_bishop_attacks(from, pos->get_occupied()) & targets;
        
        while (!attacks.empty()) {
            Square to = attacks.lsb();
            attacks = attacks.pop_lsb();
            moves[count++] = ScoredMove(Move(from, to));
        }
    }
}

void MoveGenerator::generate_rook_moves(ScoredMove* moves, int& count, Bitboard targets) {
    Color us = pos->get_side_to_move();
    Bitboard rooks = pos->get_pieces(ROOK) & pos->get_pieces(us);
    
    while (!rooks.empty()) {
        Square from = rooks.lsb();
        rooks = rooks.pop_lsb();
        Bitboard attacks = AttackGenerator::get_rook_attacks(from, pos->get_occupied()) & targets;
        
        while (!attacks.empty()) {
            Square to = attacks.lsb();
            attacks = attacks.pop_lsb();
            moves[count++] = ScoredMove(Move(from, to));
        }
    }
}

void MoveGenerator::generate_queen_moves(ScoredMove* moves, int& count, Bitboard targets) {
    Color us = pos->get_side_to_move();
    Bitboard queens = pos->get_pieces(QUEEN) & pos->get_pieces(us);
    
    while (!queens.empty()) {
        Square from = queens.lsb();
        queens = queens.pop_lsb();
        Bitboard attacks = AttackGenerator::get_queen_attacks(from, pos->get_occupied()) & targets;
        
        while (!attacks.empty()) {
            Square to = attacks.lsb();
            attacks = attacks.pop_lsb();
            moves[count++] = ScoredMove(Move(from, to));
        }
    }
}

void MoveGenerator::generate_king_moves(ScoredMove* moves, int& count, Bitboard targets, bool check_evasion) {
    Color us = pos->get_side_to_move();
    Bitboard kings = pos->get_pieces(KING) & pos->get_pieces(us);
    
    if (kings.empty()) return;
    
    Square from = kings.lsb();
    Bitboard attacks = AttackGenerator::get_king_attacks(from) & targets;
    
    while (!attacks.empty()) {
        Square to = attacks.lsb();
        attacks = attacks.pop_lsb();
        moves[count++] = ScoredMove(Move(from, to));
    }
    
    // Castling (only if not in check and not evasion)
    if (!check_evasion && !pos->is_in_check()) {
        int castling_rights = pos->get_castling_rights();
        int rights = static_cast<int>(castling_rights);
        
        if (us == WHITE) {
            if ((rights & 1) && pos->piece_at(SQ_F1) == NO_PIECE && pos->piece_at(SQ_G1) == NO_PIECE) {
                if (!pos->is_attacked_by(SQ_E1, BLACK) && !pos->is_attacked_by(SQ_F1, BLACK) && !pos->is_attacked_by(SQ_G1, BLACK)) {
                    moves[count++] = ScoredMove(Move(SQ_E1, SQ_G1, MOVE_CASTLING));
                }
            }
            if ((rights & 2) && pos->piece_at(SQ_D1) == NO_PIECE && pos->piece_at(SQ_C1) == NO_PIECE && pos->piece_at(SQ_B1) == NO_PIECE) {
                if (!pos->is_attacked_by(SQ_E1, BLACK) && !pos->is_attacked_by(SQ_D1, BLACK) && !pos->is_attacked_by(SQ_C1, BLACK)) {
                    moves[count++] = ScoredMove(Move(SQ_E1, SQ_C1, MOVE_CASTLING));
                }
            }
        } else {
            if ((rights & 4) && pos->piece_at(SQ_F8) == NO_PIECE && pos->piece_at(SQ_G8) == NO_PIECE) {
                if (!pos->is_attacked_by(SQ_E8, WHITE) && !pos->is_attacked_by(SQ_F8, WHITE) && !pos->is_attacked_by(SQ_G8, WHITE)) {
                    moves[count++] = ScoredMove(Move(SQ_E8, SQ_G8, MOVE_CASTLING));
                }
            }
            if ((rights & 8) && pos->piece_at(SQ_D8) == NO_PIECE && pos->piece_at(SQ_C8) == NO_PIECE && pos->piece_at(SQ_B8) == NO_PIECE) {
                if (!pos->is_attacked_by(SQ_E8, WHITE) && !pos->is_attacked_by(SQ_D8, WHITE) && !pos->is_attacked_by(SQ_C8, WHITE)) {
                    moves[count++] = ScoredMove(Move(SQ_E8, SQ_C8, MOVE_CASTLING));
                }
            }
        }
    }
}

int MoveGenerator::generate_legal(ScoredMove* moves) {
    int count = 0;
    Color us = pos->get_side_to_move();
    Color them = static_cast<Color>(1 - us);
    
    if (pos->is_in_check()) {
        return generate_evasions(moves);
    }
    
    Bitboard their_pieces = pos->get_pieces(them);
    Bitboard empty_sq = ~pos->get_occupied();
    
    // Generate captures
    generate_pawn_moves(moves, count, their_pieces, true);
    generate_knight_moves(moves, count, their_pieces);
    generate_bishop_moves(moves, count, their_pieces);
    generate_rook_moves(moves, count, their_pieces);
    generate_queen_moves(moves, count, their_pieces);
    generate_king_moves(moves, count, their_pieces, false);
    
    // Generate quiet moves
    generate_pawn_moves(moves, count, empty_sq, false);
    generate_knight_moves(moves, count, empty_sq);
    generate_bishop_moves(moves, count, empty_sq);
    generate_rook_moves(moves, count, empty_sq);
    generate_queen_moves(moves, count, empty_sq);
    generate_king_moves(moves, count, empty_sq, false);
    
    // Filter illegal moves (leaving king in check)
    int legal_count = 0;
    for (int i = 0; i < count; ++i) {
        Piece captured = pos->piece_at(moves[i].move.to_sq());
        bool was_promotion = (moves[i].move.type() == MOVE_PROMOTION);
        PieceType promo_type = moves[i].move.promotion_type();
        
        // Create a temporary copy to test the move
        Position temp_pos = *pos;
        temp_pos.make_move(moves[i].move);
        
        // Check if our king is attacked
        Square ksq = SQUARE_NONE;
        for (int s = SQ_A1; s <= SQ_H8; ++s) {
            if (temp_pos.piece_at(static_cast<Square>(s)) == (us == WHITE ? W_KING : B_KING)) {
                ksq = static_cast<Square>(s);
                break;
            }
        }
        
        bool legal = (ksq != SQUARE_NONE) && !temp_pos.is_attacked_by(ksq, them);
        
        if (legal) {
            moves[legal_count++] = moves[i];
        }
    }
    
    return legal_count;
}

int MoveGenerator::generate_captures(ScoredMove* moves) {
    int count = 0;
    Color us = pos->get_side_to_move();
    Color them = static_cast<Color>(1 - us);
    Bitboard their_pieces = pos->get_pieces(them);
    
    generate_pawn_moves(moves, count, their_pieces, true);
    generate_knight_moves(moves, count, their_pieces);
    generate_bishop_moves(moves, count, their_pieces);
    generate_rook_moves(moves, count, their_pieces);
    generate_queen_moves(moves, count, their_pieces);
    generate_king_moves(moves, count, their_pieces, pos->is_in_check());
    
    return count;
}

int MoveGenerator::generate_quiets(ScoredMove* moves) {
    int count = 0;
    Color us = pos->get_side_to_move();
    Bitboard empty_sq = ~pos->get_occupied();
    
    generate_pawn_moves(moves, count, empty_sq, false);
    generate_knight_moves(moves, count, empty_sq);
    generate_bishop_moves(moves, count, empty_sq);
    generate_rook_moves(moves, count, empty_sq);
    generate_queen_moves(moves, count, empty_sq);
    generate_king_moves(moves, count, empty_sq, false);
    
    return count;
}

int MoveGenerator::generate_evasions(ScoredMove* moves) {
    int count = 0;
    Color us = pos->get_side_to_move();
    Color them = static_cast<Color>(1 - us);
    
    Bitboard checkers = pos->get_checkers();
    
    if (checkers.count() > 1) {
        // Double check: only king moves
        generate_king_moves(moves, count, ~pos->get_pieces(us), true);
        return count;
    }
    
    // Single check: king moves or capture checker or block
    Square checker_sq = checkers.lsb();
    Piece checker = pos->piece_at(checker_sq);
    
    // King moves
    generate_king_moves(moves, count, ~pos->get_pieces(us), true);
    
    // Find squares between checker and king
    Bitboard blocks(0);
    Square ksq = SQUARE_NONE;
    for (int s = SQ_A1; s <= SQ_H8; ++s) {
        if (pos->piece_at(static_cast<Square>(s)) == (us == WHITE ? W_KING : B_KING)) {
            ksq = static_cast<Square>(s);
            break;
        }
    }
    
    if (type_of(checker) == KNIGHT || type_of(checker) == PAWN || type_of(checker) == KING) {
        // Can only capture
        blocks = Bitboard(checker_sq);
    } else {
        // Sliding piece: can capture or block
        int df = file_of(checker_sq) - file_of(ksq);
        int dr = rank_of(checker_sq) - rank_of(ksq);
        int step_f = (df == 0) ? 0 : (df > 0 ? 1 : -1);
        int step_r = (dr == 0) ? 0 : (dr > 0 ? 1 : -1);
        
        int f = file_of(ksq) + step_f;
        int r = rank_of(ksq) + step_r;
        
        while (f != file_of(checker_sq) || r != rank_of(checker_sq)) {
            blocks.bb |= (Bitboard::U64(1) << make_square(f, r));
            f += step_f;
            r += step_r;
        }
        blocks.bb |= (Bitboard::U64(1) << checker_sq);
    }
    
    Bitboard targets = blocks & ~pos->get_pieces(us);
    
    generate_pawn_moves(moves, count, targets, true);
    generate_knight_moves(moves, count, targets);
    generate_bishop_moves(moves, count, targets);
    generate_rook_moves(moves, count, targets);
    generate_queen_moves(moves, count, targets);
    
    return count;
}

bool MoveGenerator::is_legal(const Move& move) const {
    // Create a temporary copy to test the move
    Position temp_pos = *pos;
    
    Piece captured = temp_pos.piece_at(move.to_sq());
    bool was_promotion = (move.type() == MOVE_PROMOTION);
    PieceType promo_type = move.promotion_type();
    
    temp_pos.make_move(move);
    
    Color us = static_cast<Color>(1 - temp_pos.get_side_to_move());
    Color them = temp_pos.get_side_to_move();
    
    Square ksq = SQUARE_NONE;
    for (int s = SQ_A1; s <= SQ_H8; ++s) {
        if (temp_pos.piece_at(static_cast<Square>(s)) == (us == WHITE ? W_KING : B_KING)) {
            ksq = static_cast<Square>(s);
            break;
        }
    }
    
    bool legal = (ksq != SQUARE_NONE) && !temp_pos.is_attacked_by(ksq, them);
    
    return legal;
}

} // namespace RedSprite
