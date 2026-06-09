#pragma once

#include <cstdint>
#include <array>

namespace RedSprite {

enum Color : int { WHITE = 0, BLACK = 1, COLOR_NB = 2 };
enum PieceType : int { NO_PIECE_TYPE = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6, PIECE_TYPE_NB = 7 };
enum Piece : int { NO_PIECE = 0, W_PAWN = 1, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING, 
                   B_PAWN = 9, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING, PIECE_NB = 15 };
enum Square : int { SQ_A1 = 0, SQ_H1 = 7, SQ_A8 = 56, SQ_H8 = 63, SQUARE_NB = 64 };
enum Phase : int { PHASE_MG = 0, PHASE_EG = 1 };

constexpr int MAX_MOVES = 256;
constexpr int MAX_DEPTH = 64;
constexpr int VALUE_INFINITE = 32000;
constexpr int VALUE_NONE = 32001;

inline Square make_square(int f, int r) { return static_cast<Square>(f + (r << 3)); }
inline int file_of(Square s) { return s & 7; }
inline int rank_of(Square s) { return s >> 3; }
inline Color color_of(Piece p) { return static_cast<Color>(p >> 3); }
inline PieceType type_of(Piece p) { return static_cast<PieceType>(p & 7); }

} // namespace RedSprite
