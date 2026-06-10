#pragma once

#include "../core/types.h"
#include "../movegen/move.h"
#include <cstdint>
#include <vector>
#include <memory>

namespace RedSprite {

class Position;

// NNUE Feature Transformer
constexpr int N_INPUT_SQUARES = 64;
constexpr int N_PIECE_TYPES = 7;
constexpr int N_COLORS = 2;
constexpr int N_FEATURES = N_INPUT_SQUARES * N_PIECE_TYPES * N_COLORS;
constexpr int N_HIDDEN = 512;
constexpr int N_OUTPUT = 1;

// Clipped ReLU activation
inline int16_t clipped_relu(int16_t x) {
    return static_cast<int16_t>(x > 0 ? (x < 127 ? x : 127) : 0);
}

struct NNUEFeatures {
    std::vector<int> active_features;
    
    void clear() { active_features.clear(); }
    void add_feature(int feature) { active_features.push_back(feature); }
    void remove_feature(int feature);
    
    // Incremental update for moves
    void update_move(const Position& pos, Move m);
};

class NNUEEvaluator {
private:
    // Weights (would be loaded from file in production)
    alignas(64) int16_t ft_weights[N_FEATURES][N_HIDDEN];
    alignas(64) int16_t ft_biases[N_HIDDEN];
    alignas(64) int16_t out_weights[N_HIDDEN * 2];
    int16_t out_bias;
    
    // Accumulators for incremental updates
    alignas(64) int16_t accumulators[COLOR_NB][N_HIDDEN];
    
public:
    NNUEEvaluator();
    
    // Initialize weights (random for now, would load from file)
    void initialize_weights();
    
    // Full evaluation from scratch
    int evaluate(const Position& pos, Color perspective);
    
    // Incremental update
    void update_incremental(const Position& pos, Move m, Piece captured, bool was_promotion, PieceType promo_type);
    
    // Get accumulator for current position
    const int16_t* get_accumulator(Color c) const { return accumulators[c]; }
    
    // Refresh accumulator from scratch
    void refresh_accumulator(const Position& pos);
};

// Multi-head evaluation framework
enum EvalHead {
    HEAD_TACTICAL = 0,
    HEAD_STRATEGIC = 1,
    HEAD_POSITIONAL = 2,
    HEAD_OPENING = 3,
    HEAD_ENDGAME = 4,
    HEAD_KING_SAFETY = 5,
    HEAD_PREDICTION = 6,
    HEAD_COUNT = 7
};

struct HeadEvaluation {
    int score;
    float confidence;
    
    constexpr HeadEvaluation() : score(0), confidence(1.0f) {}
    constexpr HeadEvaluation(int s, float c) : score(s), confidence(c) {}
};

class MultiHeadEvaluator {
private:
    NNUEEvaluator nnue;
    
    // Head-specific weights for fusion
    float head_weights[HEAD_COUNT];
    
    // Material phase weight (MG vs EG)
    float mg_weight;
    float eg_weight;
    
public:
    MultiHeadEvaluator();
    
    // Evaluate all heads independently
    HeadEvaluation evaluate_tactical(const Position& pos);
    HeadEvaluation evaluate_strategic(const Position& pos);
    HeadEvaluation evaluate_positional(const Position& pos);
    HeadEvaluation evaluate_opening(const Position& pos);
    HeadEvaluation evaluate_endgame(const Position& pos);
    HeadEvaluation evaluate_king_safety(const Position& pos);
    HeadEvaluation evaluate_prediction(const Position& pos);
    
    // Fusion layer: combine all head scores
    int fuse_evaluations(const std::array<HeadEvaluation, HEAD_COUNT>& heads);
    
    // Main evaluation function
    int evaluate(const Position& pos);
    
    // Get material phase
    float get_phase(const Position& pos);
    
    // Update head weights (for learning)
    void set_head_weight(EvalHead head, float weight);
    float get_head_weight(EvalHead head) const { return head_weights[head]; }
};

} // namespace RedSprite
