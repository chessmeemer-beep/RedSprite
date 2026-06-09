#include "nnue.h"
#include "position.h"
#include <cstring>
#include <cmath>

namespace RedSprite {

// Feature index calculation
inline int make_feature(PieceType pt, Color c, Square sq) {
    return (c * N_PIECE_TYPES + pt) * N_INPUT_SQUARES + sq;
}

void NNUEFeatures::remove_feature(int feature) {
    for (auto it = active_features.begin(); it != active_features.end(); ++it) {
        if (*it == feature) {
            active_features.erase(it);
            return;
        }
    }
}

void NNUEFeatures::update_move(const Position& pos, Move m) {
    // Remove old position features
    // Add new position features
    // This is simplified - full implementation would handle all piece moves
    (void)pos;
    (void)m;
}

NNUEEvaluator::NNUEEvaluator() : out_bias(0) {
    memset(ft_weights, 0, sizeof(ft_weights));
    memset(ft_biases, 0, sizeof(ft_biases));
    memset(out_weights, 0, sizeof(out_weights));
    memset(accumulators, 0, sizeof(accumulators));
}

void NNUEEvaluator::initialize_weights() {
    // Initialize with small random values (simplified - would load from file)
    uint64_t seed = 0xDEADBEEF;
    
    for (int i = 0; i < N_FEATURES; ++i) {
        for (int j = 0; j < N_HIDDEN; ++j) {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            ft_weights[i][j] = static_cast<int16_t>((seed % 256) - 128);
        }
    }
    
    for (int j = 0; j < N_HIDDEN; ++j) {
        seed ^= seed << 13;
        seed ^= seed >> 7;
        seed ^= seed << 17;
        ft_biases[j] = static_cast<int16_t>(seed % 256);
    }
    
    for (int j = 0; j < N_HIDDEN * 2; ++j) {
        seed ^= seed << 13;
        seed ^= seed >> 7;
        seed ^= seed << 17;
        out_weights[j] = static_cast<int16_t>((seed % 256) - 128);
    }
    
    seed ^= seed << 13;
    seed ^= seed >> 7;
    seed ^= seed << 17;
    out_bias = static_cast<int16_t>(seed % 100);
}

void NNUEEvaluator::refresh_accumulator(const Position& pos) {
    memset(accumulators, 0, sizeof(accumulators));
    
    // Add biases
    for (int c = WHITE; c <= BLACK; ++c) {
        for (int j = 0; j < N_HIDDEN; ++j) {
            accumulators[c][j] = ft_biases[j];
        }
    }
    
    // Add all piece features
    for (int sq = SQ_A1; sq <= SQ_H8; ++sq) {
        Piece p = pos.piece_at(static_cast<Square>(sq));
        if (p != NO_PIECE) {
            Color c = color_of(p);
            PieceType pt = type_of(p);
            
            // Perspective: white's view and black's view
            int feat_white = make_feature(pt, WHITE, static_cast<Square>(sq));
            int feat_black = make_feature(pt, BLACK, static_cast<Square>(sq ^ 56)); // Mirror for black
            
            for (int j = 0; j < N_HIDDEN; ++j) {
                accumulators[WHITE][j] += ft_weights[feat_white][j];
                accumulators[BLACK][j] += ft_weights[feat_black][j];
            }
        }
    }
}

int NNUEEvaluator::evaluate(const Position& pos, Color perspective) {
    refresh_accumulator(pos);
    
    const int16_t* acc = accumulators[perspective];
    int sum = out_bias;
    
    // Output layer with clipped ReLU activation
    for (int j = 0; j < N_HIDDEN; ++j) {
        int16_t activated = clipped_relu(acc[j]);
        sum += activated * out_weights[j];
    }
    
    // Scale to centipawns
    return sum / 64;
}

void NNUEEvaluator::update_incremental(const Position& pos, Move m, Piece captured, bool was_promotion, PieceType promo_type) {
    // Incremental update of accumulators based on move
    // Simplified - full implementation would track exact feature changes
    (void)pos;
    (void)m;
    (void)captured;
    (void)was_promotion;
    (void)promo_type;
    
    // Would update accumulators here instead of full refresh
}

// Multi-Head Evaluator Implementation

MultiHeadEvaluator::MultiHeadEvaluator() {
    nnue.initialize_weights();
    
    // Initialize head weights (equal initially)
    for (int i = 0; i < HEAD_COUNT; ++i) {
        head_weights[i] = 1.0f;
    }
    
    mg_weight = 1.0f;
    eg_weight = 0.0f;
}

HeadEvaluation MultiHeadEvaluator::evaluate_tactical(const Position& pos) {
    // Tactical evaluation: material imbalances, threats, captures
    int score = 0;
    
    // Material count
    const int piece_values[] = {0, 100, 320, 330, 500, 900, 0};
    
    for (int pt = PAWN; pt <= QUEEN; ++pt) {
        int white_count = pos.material_count(static_cast<PieceType>(pt), WHITE);
        int black_count = pos.material_count(static_cast<PieceType>(pt), BLACK);
        score += (white_count - black_count) * piece_values[pt];
    }
    
    // Threat detection (simplified)
    // Would analyze attacks on valuable pieces
    
    float confidence = 0.9f; // High confidence for material
    return HeadEvaluation(score, confidence);
}

HeadEvaluation MultiHeadEvaluator::evaluate_strategic(const Position& pos) {
    // Strategic evaluation: pawn structure, space, piece activity
    int score = 0;
    (void)pos;
    
    // Pawn structure bonuses (doubled, isolated, passed)
    // Space advantage
    // Piece mobility
    
    float confidence = 0.7f;
    return HeadEvaluation(score, confidence);
}

HeadEvaluation MultiHeadEvaluator::evaluate_positional(const Position& pos) {
    // Positional evaluation: piece placement, outposts, weak squares
    int score = 0;
    
    // Piece-square tables
    static const int pawn_table[64] = {
        0,  0,  0,  0,  0,  0,  0,  0,
        50, 50, 50, 50, 50, 50, 50, 50,
        10, 10, 20, 30, 30, 20, 10, 10,
        5,  5, 10, 25, 25, 10,  5,  5,
        0,  0,  0, 20, 20,  0,  0,  0,
        5, -5,-10,  0,  0,-10, -5,  5,
        5, 10, 10,-20,-20, 10, 10,  5,
        0,  0,  0,  0,  0,  0,  0,  0
    };
    
    // Apply PST for pawns (simplified)
    Bitboard white_pawns = pos.get_pieces(W_PAWN);
    while (!white_pawns.empty()) {
        Square s = white_pawns.lsb();
        white_pawns = white_pawns.pop_lsb();
        score += pawn_table[s];
    }
    
    Bitboard black_pawns = pos.get_pieces(B_PAWN);
    while (!black_pawns.empty()) {
        Square s = black_pawns.lsb();
        black_pawns = black_pawns.pop_lsb();
        score -= pawn_table[s ^ 56]; // Mirror for black
    }
    
    float confidence = 0.8f;
    return HeadEvaluation(score, confidence);
}

HeadEvaluation MultiHeadEvaluator::evaluate_opening(const Position& pos) {
    // Opening evaluation: development, center control, king safety
    int score = 0;
    (void)pos;
    
    // Development bonus for minor pieces moved out
    // Center control (pawns and pieces on d4, e4, d5, e5)
    // Early castling bonus
    
    float phase = get_phase(pos);
    float confidence = phase; // High confidence in opening
    return HeadEvaluation(score, confidence);
}

HeadEvaluation MultiHeadEvaluator::evaluate_endgame(const Position& pos) {
    // Endgame evaluation: king activity, passed pawns, promotion chances
    int score = 0;
    (void)pos;
    
    // King centralization
    // Passed pawn advancement
    // Opposition and triangulation
    
    float phase = 1.0f - get_phase(pos);
    float confidence = phase; // High confidence in endgame
    return HeadEvaluation(score, confidence);
}

HeadEvaluation MultiHeadEvaluator::evaluate_king_safety(const Position& pos) {
    // King safety evaluation: pawn shield, attacking pieces near king
    int score = 0;
    
    // Find kings
    Square white_king = SQUARE_NONE, black_king = SQUARE_NONE;
    for (int s = SQ_A1; s <= SQ_H8; ++s) {
        Piece p = pos.piece_at(static_cast<Square>(s));
        if (p == W_KING) white_king = static_cast<Square>(s);
        if (p == B_KING) black_king = static_cast<Square>(s);
    }
    
    // Pawn shield evaluation (simplified)
    if (white_king != SQUARE_NONE) {
        // Check pawns near king
        Bitboard shield = AttackGenerator::get_king_attacks(white_king);
        shield &= pos.get_pieces(PAWN);
        score += shield.count() * 20;
    }
    
    if (black_king != SQUARE_NONE) {
        Bitboard shield = AttackGenerator::get_king_attacks(black_king);
        shield &= pos.get_pieces(PAWN);
        score -= shield.count() * 20;
    }
    
    float confidence = 0.75f;
    return HeadEvaluation(score, confidence);
}

HeadEvaluation MultiHeadEvaluator::evaluate_prediction(const Position& pos) {
    // Prediction head: predict opponent's best response, danger assessment
    int score = 0;
    (void)pos;
    
    // Would use neural network to predict likely outcomes
    // Assess immediate threats
    
    float confidence = 0.6f;
    return HeadEvaluation(score, confidence);
}

int MultiHeadEvaluator::fuse_evaluations(const std::array<HeadEvaluation, HEAD_COUNT>& heads) {
    int total = 0;
    float weight_sum = 0;
    
    for (int i = 0; i < HEAD_COUNT; ++i) {
        total += static_cast<int>(heads[i].score * heads[i].confidence * head_weights[i]);
        weight_sum += heads[i].confidence * head_weights[i];
    }
    
    return (weight_sum > 0) ? static_cast<int>(total / weight_sum) : 0;
}

int MultiHeadEvaluator::evaluate(const Position& pos) {
    // Evaluate all heads
    std::array<HeadEvaluation, HEAD_COUNT> heads;
    heads[HEAD_TACTICAL] = evaluate_tactical(pos);
    heads[HEAD_STRATEGIC] = evaluate_strategic(pos);
    heads[HEAD_POSITIONAL] = evaluate_positional(pos);
    heads[HEAD_OPENING] = evaluate_opening(pos);
    heads[HEAD_ENDGAME] = evaluate_endgame(pos);
    heads[HEAD_KING_SAFETY] = evaluate_king_safety(pos);
    heads[HEAD_PREDICTION] = evaluate_prediction(pos);
    
    // Fuse evaluations
    int fused_score = fuse_evaluations(heads);
    
    // Add NNUE evaluation as base
    int nnue_score = nnue.evaluate(pos, pos.get_side_to_move());
    
    // Combine fused multi-head score with NNUE
    return fused_score + nnue_score;
}

float MultiHeadEvaluator::get_phase(const Position& pos) {
    // Calculate game phase based on material
    const int piece_phase[] = {0, 0, 1, 1, 2, 4, 0};
    
    int total_phase = 0;
    for (int pt = KNIGHT; pt <= QUEEN; ++pt) {
        int count = pos.material_count(static_cast<PieceType>(pt), WHITE) +
                    pos.material_count(static_cast<PieceType>(pt), BLACK);
        total_phase += count * piece_phase[pt];
    }
    
    // Phase goes from 0 (endgame) to 1 (opening)
    constexpr int MAX_PHASE = 24;
    return static_cast<float>(total_phase) / MAX_PHASE;
}

void MultiHeadEvaluator::set_head_weight(EvalHead head, float weight) {
    if (head >= 0 && head < HEAD_COUNT) {
        head_weights[head] = weight;
    }
}

} // namespace RedSprite
