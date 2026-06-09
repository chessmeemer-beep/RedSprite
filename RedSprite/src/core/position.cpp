#include "position.h"
#include <cstring>
#include <sstream>
#include <iomanip>

namespace RedSprite {

uint64_t ZobristHash::piece_keys[PIECE_NB][SQUARE_NB];
uint64_t ZobristHash::ep_keys[SQUARE_NB];
uint64_t ZobristHash::castling_keys[16];
uint64_t ZobristHash::side_key;

void ZobristHash::init() {
    // Use a simple pseudo-random number generator
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    for (int p = 0; p < PIECE_NB; ++p) {
        for (int s = 0; s < SQUARE_NB; ++s) {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            piece_keys[p][s] = seed;
        }
    }
    
    for (int s = 0; s < SQUARE_NB; ++s) {
        seed ^= seed << 13;
        seed ^= seed >> 7;
        seed ^= seed << 17;
        ep_keys[s] = seed;
    }
    
    for (int i = 0; i < 16; ++i) {
        seed ^= seed << 13;
        seed ^= seed >> 7;
        seed ^= seed << 17;
        castling_keys[i] = seed;
    }
    
    seed ^= seed << 13;
    seed ^= seed >> 7;
    seed ^= seed << 17;
    side_key = seed;
}

Position::Position() 
    : occupied(0), checkers(0), pinned(0), king_attacks(0),
      castling_rights(0), ep_square(SQUARE_NONE), side_to_move(WHITE),
      halfmove_clock(0), fullmove_number(1), in_check(false) {
    memset(pieces.data(), 0, sizeof(pieces));
    memset(side_pieces.data(), 0, sizeof(side_pieces));
    memset(piece_types.data(), 0, sizeof(piece_types));
}

bool Position::set_from_fen(const char* fen) {
    // Clear position
    memset(pieces.data(), 0, sizeof(pieces));
    memset(side_pieces.data(), 0, sizeof(side_pieces));
    memset(piece_types.data(), 0, sizeof(piece_types));
    occupied = Bitboard(0);
    zobrist_key = ZobristHash();
    
    std::istringstream iss(fen);
    std::string board_str, stm_str, castling_str, ep_str, halfmove_str, fullmove_str;
    
    if (!(iss >> board_str >> stm_str >> castling_str >> ep_str)) {
        return false;
    }
    
    // Parse board
    int rank = 7, file = 0;
    for (char c : board_str) {
        if (c == '/') {
            --rank;
            file = 0;
        } else if (std::isdigit(c)) {
            file += c - '0';
        } else {
            Piece p = NO_PIECE;
            switch (c) {
                case 'P': p = W_PAWN; break;
                case 'N': p = W_KNIGHT; break;
                case 'B': p = W_BISHOP; break;
                case 'R': p = W_ROOK; break;
                case 'Q': p = W_QUEEN; break;
                case 'K': p = W_KING; break;
                case 'p': p = B_PAWN; break;
                case 'n': p = B_KNIGHT; break;
                case 'b': p = B_BISHOP; break;
                case 'r': p = B_ROOK; break;
                case 'q': p = B_QUEEN; break;
                case 'k': p = B_KING; break;
            }
            
            if (p != NO_PIECE) {
                Square sq = make_square(file, rank);
                pieces[p].bb |= (Bitboard::U64(1) << sq);
                side_pieces[color_of(p)].bb |= (Bitboard::U64(1) << sq);
                piece_types[type_of(p)].bb |= (Bitboard::U64(1) << sq);
                occupied.bb |= (Bitboard::U64(1) << sq);
                zobrist_key.xor_piece(p, sq);
                ++file;
            }
        }
    }
    
    // Side to move
    side_to_move = (stm_str == "w") ? WHITE : BLACK;
    if (side_to_move == BLACK) {
        zobrist_key.xor_side();
    }
    
    // Castling rights
    castling_rights = 0;
    int rights_mask = 0;
    for (char c : castling_str) {
        switch (c) {
            case 'K': rights_mask |= 1; break;
            case 'Q': rights_mask |= 2; break;
            case 'k': rights_mask |= 4; break;
            case 'q': rights_mask |= 8; break;
        }
    }
    castling_rights = static_cast<Square>(rights_mask);
    zobrist_key.xor_castling(rights_mask);
    
    // En passant square
    if (ep_str != "-") {
        int ep_file = ep_str[0] - 'a';
        int ep_rank = ep_str[1] - '1';
        ep_square = make_square(ep_file, ep_rank);
        zobrist_key.xor_ep(ep_square);
    } else {
        ep_square = SQUARE_NONE;
    }
    
    // Halfmove and fullmove clocks
    halfmove_clock = 0;
    fullmove_number = 1;
    if (iss >> halfmove_str >> fullmove_str) {
        halfmove_clock = std::stoi(halfmove_str);
        fullmove_number = std::stoi(fullmove_str);
    }
    
    update_check_info();
    return true;
}

std::string Position::get_fen() const {
    std::ostringstream oss;
    
    // Board
    for (int rank = 7; rank >= 0; --rank) {
        int empty = 0;
        for (int file = 0; file < 8; ++file) {
            Square sq = make_square(file, rank);
            Piece p = piece_at(sq);
            if (p == NO_PIECE) {
                ++empty;
            } else {
                if (empty > 0) {
                    oss << empty;
                    empty = 0;
                }
                char c;
                switch (p) {
                    case W_PAWN: c = 'P'; break;
                    case W_KNIGHT: c = 'N'; break;
                    case W_BISHOP: c = 'B'; break;
                    case W_ROOK: c = 'R'; break;
                    case W_QUEEN: c = 'Q'; break;
                    case W_KING: c = 'K'; break;
                    case B_PAWN: c = 'p'; break;
                    case B_KNIGHT: c = 'n'; break;
                    case B_BISHOP: c = 'b'; break;
                    case B_ROOK: c = 'r'; break;
                    case B_QUEEN: c = 'q'; break;
                    case B_KING: c = 'k'; break;
                    default: c = '?'; break;
                }
                oss << c;
            }
        }
        if (empty > 0) oss << empty;
        if (rank > 0) oss << '/';
    }
    
    oss << ' ';
    oss << (side_to_move == WHITE ? 'w' : 'b');
    
    oss << ' ';
    if (castling_rights == 0) {
        oss << '-';
    } else {
        if (castling_rights & 1) oss << 'K';
        if (castling_rights & 2) oss << 'Q';
        if (castling_rights & 4) oss << 'k';
        if (castling_rights & 8) oss << 'q';
    }
    
    oss << ' ';
    if (ep_square == SQUARE_NONE) {
        oss << '-';
    } else {
        oss << static_cast<char>('a' + file_of(ep_square));
        oss << static_cast<char>('1' + rank_of(ep_square));
    }
    
    oss << ' ' << halfmove_clock << ' ' << fullmove_number;
    
    return oss.str();
}

int Position::material_count(PieceType pt, Color c) const {
    Piece p = static_cast<Piece>((c << 3) | pt);
    return get_pieces(p).count();
}

bool Position::is_insufficient_material() const {
    // King vs King
    if (occupied.count() == 2) return true;
    
    // King + Knight vs King or King vs King + Knight
    if (occupied.count() == 3) {
        if (piece_types[KNIGHT].count() == 1) return true;
        if (piece_types[BISHOP].count() == 1) return true;
    }
    
    // King + Bishop vs King + Bishop (same color bishops)
    if (occupied.count() == 4 && piece_types[BISHOP].count() == 2) {
        Bitboard bishops = piece_types[BISHOP];
        Bitboard white_bishops = bishops & side_pieces[WHITE];
        Bitboard black_bishops = bishops & side_pieces[BLACK];
        
        if (white_bishops.count() == 1 && black_bishops.count() == 1) {
            Square wb_sq = white_bishops.lsb();
            Square bb_sq = black_bishops.lsb();
            bool wb_color = (file_of(wb_sq) + rank_of(wb_sq)) % 2;
            bool bb_color = (file_of(bb_sq) + rank_of(bb_sq)) % 2;
            if (wb_color == bb_color) return true;
        }
    }
    
    return false;
}

void Position::update_check_info() {
    Square ksq = SQUARE_NONE;
    for (int s = SQ_A1; s <= SQ_H8; ++s) {
        if (piece_at(static_cast<Square>(s)) == (side_to_move == WHITE ? W_KING : B_KING)) {
            ksq = static_cast<Square>(s);
            break;
        }
    }
    
    if (ksq == SQUARE_NONE) {
        in_check = false;
        checkers = Bitboard(0);
        pinned = Bitboard(0);
        return;
    }
    
    Color them = static_cast<Color>(1 - side_to_move);
    
    // Find attackers
    Bitboard attackers = 0;
    
    // Pawn attacks
    Bitboard pawn_attackers = AttackGenerator::get_pawn_attacks(them, ksq) & get_pieces(PAWN) & get_pieces(them);
    if (!pawn_attackers.empty()) {
        attackers |= pawn_attackers;
    }
    
    // Knight attacks
    Bitboard knight_attackers = AttackGenerator::get_knight_attacks(ksq) & get_pieces(KNIGHT) & get_pieces(them);
    if (!knight_attackers.empty()) {
        attackers |= knight_attackers;
    }
    
    // King attacks
    Bitboard king_attackers = AttackGenerator::get_king_attacks(ksq) & get_pieces(KING) & get_pieces(them);
    if (!king_attackers.empty()) {
        attackers |= king_attackers;
    }
    
    // Sliding attacks
    Bitboard bishop_queen = get_pieces(BISHOP) | get_pieces(QUEEN);
    Bitboard rook_queen = get_pieces(ROOK) | get_pieces(QUEEN);
    
    Bitboard bishop_attackers = AttackGenerator::get_bishop_attacks(ksq, occupied) & bishop_queen & get_pieces(them);
    if (!bishop_attackers.empty()) {
        attackers |= bishop_attackers;
    }
    
    Bitboard rook_attackers = AttackGenerator::get_rook_attacks(ksq, occupied) & rook_queen & get_pieces(them);
    if (!rook_attackers.empty()) {
        attackers |= rook_attackers;
    }
    
    checkers = attackers;
    in_check = !attackers.empty();
    
    // Calculate pinned pieces
    pinned = Bitboard(0);
    Bitboard our_pieces = get_pieces(side_to_move);
    
    // Check for pins along ranks/files
    Bitboard rank_pinners = AttackGenerator::get_rook_attacks(ksq, 0) & rook_queen & get_pieces(them);
    while (!rank_pinners.empty()) {
        Square pinner_sq = rank_pinners.lsb();
        rank_pinners = rank_pinners.pop_lsb();
        
        Bitboard between = (Bitboard(pinner_sq) | Bitboard(ksq)).pop_lsb().pop_lsb();
        between = between & ((Bitboard(pinner_sq) | Bitboard(ksq)) - 1);
        
        if ((between & ~our_pieces).empty() && between.count() == 1) {
            pinned |= between;
        }
    }
    
    // Check for pins along diagonals
    Bitboard diag_pinners = AttackGenerator::get_bishop_attacks(ksq, 0) & bishop_queen & get_pieces(them);
    while (!diag_pinners.empty()) {
        Square pinner_sq = diag_pinners.lsb();
        diag_pinners = diag_pinners.pop_lsb();
        
        Bitboard between = (Bitboard(pinner_sq) | Bitboard(ksq)).pop_lsb().pop_lsb();
        between = between & ((Bitboard(pinner_sq) | Bitboard(ksq)) - 1);
        
        if ((between & ~our_pieces).empty() && between.count() == 1) {
            pinned |= between;
        }
    }
}

bool Position::is_attacked_by(Square s, Color by) const {
    // Pawn attacks
    if (!(AttackGenerator::get_pawn_attacks(by, s) & get_pieces(PAWN) & get_pieces(by)).empty()) {
        return true;
    }
    
    // Knight attacks
    if (!(AttackGenerator::get_knight_attacks(s) & get_pieces(KNIGHT) & get_pieces(by)).empty()) {
        return true;
    }
    
    // King attacks
    if (!(AttackGenerator::get_king_attacks(s) & get_pieces(KING) & get_pieces(by)).empty()) {
        return true;
    }
    
    // Sliding attacks
    Bitboard bishop_queen = get_pieces(BISHOP) | get_pieces(QUEEN);
    Bitboard rook_queen = get_pieces(ROOK) | get_pieces(QUEEN);
    
    if (!(AttackGenerator::get_bishop_attacks(s, occupied) & bishop_queen & get_pieces(by)).empty()) {
        return true;
    }
    
    if (!(AttackGenerator::get_rook_attacks(s, occupied) & rook_queen & get_pieces(by)).empty()) {
        return true;
    }
    
    return false;
}

void Position::make_move(const Move& m) {
    Square from = m.from_sq();
    Square to = m.to_sq();
    PieceType pt = type_of(piece_at(from));
    Piece captured = piece_at(to);
    
    // Update zobrist
    Piece moving_piece = piece_at(from);
    zobrist_key.xor_piece(moving_piece, from);
    
    // Handle en passant capture
    if (m.type() == MOVE_ENPASSANT) {
        Square ep_capture_sq = make_square(file_of(to), rank_of(from));
        Piece ep_captured = piece_at(ep_capture_sq);
        zobrist_key.xor_piece(ep_captured, ep_capture_sq);
        pieces[ep_captured].bb &= ~(Bitboard::U64(1) << ep_capture_sq);
        side_pieces[color_of(ep_captured)].bb &= ~(Bitboard::U64(1) << ep_capture_sq);
        piece_types[PAWN].bb &= ~(Bitboard::U64(1) << ep_capture_sq);
        occupied.bb &= ~(Bitboard::U64(1) << ep_capture_sq);
        captured = ep_captured;
    }
    
    // Handle castling
    if (m.type() == MOVE_CASTLING) {
        if (to > from) { // Kingside
            Square rook_from = make_square(7, rank_of(from));
            Square rook_to = make_square(5, rank_of(from));
            Piece rook = piece_at(rook_from);
            
            zobrist_key.xor_piece(rook, rook_from);
            zobrist_key.xor_piece(rook, rook_to);
            
            pieces[rook].bb &= ~(Bitboard::U64(1) << rook_from);
            pieces[rook].bb |= (Bitboard::U64(1) << rook_to);
            side_pieces[side_to_move].bb &= ~(Bitboard::U64(1) << rook_from);
            side_pieces[side_to_move].bb |= (Bitboard::U64(1) << rook_to);
            piece_types[ROOK].bb &= ~(Bitboard::U64(1) << rook_from);
            piece_types[ROOK].bb |= (Bitboard::U64(1) << rook_to);
            occupied.bb &= ~(Bitboard::U64(1) << rook_from);
            occupied.bb |= (Bitboard::U64(1) << rook_to);
        } else { // Queenside
            Square rook_from = make_square(0, rank_of(from));
            Square rook_to = make_square(3, rank_of(from));
            Piece rook = piece_at(rook_from);
            
            zobrist_key.xor_piece(rook, rook_from);
            zobrist_key.xor_piece(rook, rook_to);
            
            pieces[rook].bb &= ~(Bitboard::U64(1) << rook_from);
            pieces[rook].bb |= (Bitboard::U64(1) << rook_to);
            side_pieces[side_to_move].bb &= ~(Bitboard::U64(1) << rook_from);
            side_pieces[side_to_move].bb |= (Bitboard::U64(1) << rook_to);
            piece_types[ROOK].bb &= ~(Bitboard::U64(1) << rook_from);
            piece_types[ROOK].bb |= (Bitboard::U64(1) << rook_to);
            occupied.bb &= ~(Bitboard::U64(1) << rook_from);
            occupied.bb |= (Bitboard::U64(1) << rook_to);
        }
    }
    
    // Remove captured piece
    if (captured != NO_PIECE && m.type() != MOVE_ENPASSANT) {
        zobrist_key.xor_piece(captured, to);
        pieces[captured].bb &= ~(Bitboard::U64(1) << to);
        side_pieces[color_of(captured)].bb &= ~(Bitboard::U64(1) << to);
        piece_types[type_of(captured)].bb &= ~(Bitboard::U64(1) << to);
        occupied.bb &= ~(Bitboard::U64(1) << to);
    }
    
    // Move piece
    pieces[moving_piece].bb &= ~(Bitboard::U64(1) << from);
    pieces[moving_piece].bb |= (Bitboard::U64(1) << to);
    side_pieces[side_to_move].bb &= ~(Bitboard::U64(1) << from);
    side_pieces[side_to_move].bb |= (Bitboard::U64(1) << to);
    piece_types[pt].bb &= ~(Bitboard::U64(1) << from);
    piece_types[pt].bb |= (Bitboard::U64(1) << to);
    occupied.bb &= ~(Bitboard::U64(1) << from);
    occupied.bb |= (Bitboard::U64(1) << to);
    
    // Handle promotion
    if (m.type() == MOVE_PROMOTION) {
        PieceType promo_type = m.promotion_type();
        Piece new_piece = static_cast<Piece>((side_to_move << 3) | promo_type);
        
        pieces[moving_piece].bb &= ~(Bitboard::U64(1) << to);
        piece_types[pt].bb &= ~(Bitboard::U64(1) << to);
        
        pieces[new_piece].bb |= (Bitboard::U64(1) << to);
        piece_types[promo_type].bb |= (Bitboard::U64(1) << to);
        
        zobrist_key.xor_piece(new_piece, to);
    } else {
        zobrist_key.xor_piece(moving_piece, to);
    }
    
    // Update castling rights
    update_castling_rights(m);
    
    // Update en passant square
    if (ep_square != SQUARE_NONE) {
        zobrist_key.xor_ep(ep_square);
    }
    
    ep_square = SQUARE_NONE;
    if (pt == PAWN && std::abs(rank_of(to) - rank_of(from)) == 2) {
        ep_square = make_square(file_of(to), (rank_of(to) + rank_of(from)) / 2);
        zobrist_key.xor_ep(ep_square);
    }
    
    // Update clocks
    if (pt == PAWN || captured != NO_PIECE) {
        halfmove_clock = 0;
    } else {
        ++halfmove_clock;
    }
    
    if (side_to_move == BLACK) {
        ++fullmove_number;
    }
    
    // Switch side
    side_to_move = static_cast<Color>(1 - side_to_move);
    zobrist_key.xor_side();
    
    // Update check info
    update_check_info();
}

void Position::unmake_move(const Move& m, Piece captured, bool was_promotion, PieceType promo_type) {
    Square from = m.from_sq();
    Square to = m.to_sq();
    
    // Switch side back
    side_to_move = static_cast<Color>(1 - side_to_move);
    zobrist_key.xor_side();
    
    Piece moving_piece = was_promotion ? static_cast<Piece>((side_to_move << 3) | PAWN) : piece_at(to);
    
    if (was_promotion) {
        Piece promo_piece = static_cast<Piece>((side_to_move << 3) | promo_type);
        pieces[promo_piece].bb &= ~(Bitboard::U64(1) << to);
        piece_types[promo_type].bb &= ~(Bitboard::U64(1) << to);
        
        moving_piece = static_cast<Piece>((side_to_move << 3) | PAWN);
        pieces[moving_piece].bb |= (Bitboard::U64(1) << to);
        piece_types[PAWN].bb |= (Bitboard::U64(1) << to);
    }
    
    // Move piece back
    pieces[moving_piece].bb &= ~(Bitboard::U64(1) << to);
    pieces[moving_piece].bb |= (Bitboard::U64(1) << from);
    side_pieces[side_to_move].bb &= ~(Bitboard::U64(1) << to);
    side_pieces[side_to_move].bb |= (Bitboard::U64(1) << from);
    piece_types[type_of(moving_piece)].bb &= ~(Bitboard::U64(1) << to);
    piece_types[type_of(moving_piece)].bb |= (Bitboard::U64(1) << from);
    occupied.bb &= ~(Bitboard::U64(1) << to);
    occupied.bb |= (Bitboard::U64(1) << from);
    
    // Restore captured piece
    if (captured != NO_PIECE) {
        if (m.type() == MOVE_ENPASSANT) {
            Square ep_capture_sq = make_square(file_of(to), rank_of(from));
            pieces[captured].bb |= (Bitboard::U64(1) << ep_capture_sq);
            side_pieces[color_of(captured)].bb |= (Bitboard::U64(1) << ep_capture_sq);
            piece_types[PAWN].bb |= (Bitboard::U64(1) << ep_capture_sq);
            occupied.bb |= (Bitboard::U64(1) << ep_capture_sq);
        } else {
            pieces[captured].bb |= (Bitboard::U64(1) << to);
            side_pieces[color_of(captured)].bb |= (Bitboard::U64(1) << to);
            piece_types[type_of(captured)].bb |= (Bitboard::U64(1) << to);
            occupied.bb |= (Bitboard::U64(1) << to);
        }
    }
    
    // Restore castling rook
    if (m.type() == MOVE_CASTLING) {
        if (to > from) { // Kingside
            Square rook_from = make_square(7, rank_of(from));
            Square rook_to = make_square(5, rank_of(from));
            Piece rook = static_cast<Piece>((side_to_move << 3) | ROOK);
            
            pieces[rook].bb &= ~(Bitboard::U64(1) << rook_to);
            pieces[rook].bb |= (Bitboard::U64(1) << rook_from);
            side_pieces[side_to_move].bb &= ~(Bitboard::U64(1) << rook_to);
            side_pieces[side_to_move].bb |= (Bitboard::U64(1) << rook_from);
            piece_types[ROOK].bb &= ~(Bitboard::U64(1) << rook_to);
            piece_types[ROOK].bb |= (Bitboard::U64(1) << rook_from);
            occupied.bb &= ~(Bitboard::U64(1) << rook_to);
            occupied.bb |= (Bitboard::U64(1) << rook_from);
        } else { // Queenside
            Square rook_from = make_square(0, rank_of(from));
            Square rook_to = make_square(3, rank_of(from));
            Piece rook = static_cast<Piece>((side_to_move << 3) | ROOK);
            
            pieces[rook].bb &= ~(Bitboard::U64(1) << rook_to);
            pieces[rook].bb |= (Bitboard::U64(1) << rook_from);
            side_pieces[side_to_move].bb &= ~(Bitboard::U64(1) << rook_to);
            side_pieces[side_to_move].bb |= (Bitboard::U64(1) << rook_from);
            piece_types[ROOK].bb &= ~(Bitboard::U64(1) << rook_to);
            piece_types[ROOK].bb |= (Bitboard::U64(1) << rook_from);
            occupied.bb &= ~(Bitboard::U64(1) << rook_to);
            occupied.bb |= (Bitboard::U64(1) << rook_from);
        }
    }
    
    // Restore clocks and other state would need to be stored in a history stack
    // For simplicity, we assume this is called immediately after make_move
    update_check_info();
}

void Position::update_castling_rights(Move m) {
    Square from = m.from_sq();
    Square to = m.to_sq();
    Piece moved = piece_at(from);
    
    // If king moves, lose all castling rights for that side
    if (type_of(moved) == KING) {
        if (side_to_move == WHITE) {
            if (castling_rights & 1) {
                zobrist_key.xor_castling(castling_rights);
                castling_rights &= ~static_cast<Square>(3);
                zobrist_key.xor_castling(castling_rights);
            }
        } else {
            if (castling_rights & 4) {
                zobrist_key.xor_castling(castling_rights);
                castling_rights &= ~static_cast<Square>(12);
                zobrist_key.xor_castling(castling_rights);
            }
        }
    }
    
    // If rook moves or is captured, update corresponding rights
    if (type_of(moved) == ROOK) {
        if (from == SQ_A1) {
            zobrist_key.xor_castling(castling_rights);
            castling_rights &= ~static_cast<Square>(2);
            zobrist_key.xor_castling(castling_rights);
        } else if (from == SQ_H1) {
            zobrist_key.xor_castling(castling_rights);
            castling_rights &= ~static_cast<Square>(1);
            zobrist_key.xor_castling(castling_rights);
        } else if (from == SQ_A8) {
            zobrist_key.xor_castling(castling_rights);
            castling_rights &= ~static_cast<Square>(8);
            zobrist_key.xor_castling(castling_rights);
        } else if (from == SQ_H8) {
            zobrist_key.xor_castling(castling_rights);
            castling_rights &= ~static_cast<Square>(4);
            zobrist_key.xor_castling(castling_rights);
        }
    }
    
    // If rook is captured
    if (to == SQ_A1) {
        zobrist_key.xor_castling(castling_rights);
        castling_rights &= ~static_cast<Square>(2);
        zobrist_key.xor_castling(castling_rights);
    } else if (to == SQ_H1) {
        zobrist_key.xor_castling(castling_rights);
        castling_rights &= ~static_cast<Square>(1);
        zobrist_key.xor_castling(castling_rights);
    } else if (to == SQ_A8) {
        zobrist_key.xor_castling(castling_rights);
        castling_rights &= ~static_cast<Square>(8);
        zobrist_key.xor_castling(castling_rights);
    } else if (to == SQ_H8) {
        zobrist_key.xor_castling(castling_rights);
        castling_rights &= ~static_cast<Square>(4);
        zobrist_key.xor_castling(castling_rights);
    }
}

} // namespace RedSprite
