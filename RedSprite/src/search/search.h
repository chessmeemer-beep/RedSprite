#pragma once

#include "types.h"
#include "move.h"
#include "position.h"
#include "ttable/tt.h"
#include "eval/nnue.h"
#include <atomic>
#include <thread>
#include <vector>
#include <array>
#include <mutex>
#include <condition_variable>

namespace RedSprite {

// Search constants
constexpr int MAX_PLY = 256;
constexpr int NODES_PER_CHECK = 4096;

// Move ordering scores
constexpr int SCORE_TT_MOVE = 1000000;
constexpr int SCORE_KILLER_1 = 900000;
constexpr int SCORE_KILLER_2 = 800000;
constexpr int SCORE_COUNTER = 700000;
constexpr int MAX_HISTORY = 32768;

struct SearchStats {
    std::atomic<uint64_t> nodes{0};
    std::atomic<uint64_t> tt_hits{0};
    std::atomic<uint64_t> tt_cutoffs{0};
    std::atomic<uint64_t> eval_calls{0};
    std::atomic<uint64_t> qsearch_calls{0};
};

struct SearchLimits {
    int depth = -1;
    int time_ms = -1;
    uint64_t nodes = 0;
    bool infinite = false;
    bool ponder = false;
    
    Color side_to_move = WHITE;
    uint64_t start_time = 0;
    uint64_t move_time = 0;
};

class Search {
private:
    Position root_pos;
    SearchLimits limits;
    SearchStats stats;
    TranspositionTable& tt;
    MultiHeadEvaluator evaluator;
    
    // Move ordering
    Move killer_moves[MAX_PLY][2];
    int history[PIECE_NB][SQUARE_NB];
    Move counter_moves[MAX_PLY];
    
    // PV line
    std::array<Move, MAX_PLY> pv_line;
    int pv_length;
    
    // Multi-threading
    std::vector<std::thread> threads;
    std::atomic<bool> stop_flag{false};
    std::atomic<bool> use_nnue{true};
    
    // Root moves
    std::vector<ScoredMove> root_moves;
    
    // Search functions
    int alpha_beta(Position& pos, int depth, int alpha, int beta, int ply, bool cutnode);
    int quiescence(Position& pos, int alpha, int beta, int ply);
    
    // Move ordering
    void score_moves(ScoredMove* moves, int count, Move tt_move, int ply);
    void update_history(Move m, int bonus, int ply);
    void update_killer(Move m, int ply);
    
    // Time management
    bool should_stop();
    void check_time();
    
public:
    explicit Search(TranspositionTable& t);
    ~Search();
    
    // Main search entry point
    Move search(const Position& pos, int depth, int time_ms = -1);
    
    // Iterative deepening
    Move iterative_deepening(int max_depth);
    
    // Get best move
    Move get_best_move() const;
    
    // Get PV line
    const std::array<Move, MAX_PLY>& get_pv_line() const { return pv_line; }
    int get_pv_length() const { return pv_length; }
    
    // Search info
    uint64_t get_nodes() const { return stats.nodes.load(); }
    uint64_t get_tt_hits() const { return stats.tt_hits.load(); }
    
    // Stop search
    void stop() { stop_flag.store(true); }
    bool is_stopped() const { return stop_flag.load(); }
    
    // Set limits
    void set_limits(const SearchLimits& l) { limits = l; }
    
    // Enable/disable NNUE
    void set_nnue_enabled(bool enabled) { use_nnue.store(enabled); }
};

} // namespace RedSprite
