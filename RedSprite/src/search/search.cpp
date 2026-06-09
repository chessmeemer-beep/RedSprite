#include "search.h"
#include "movegen/move.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

namespace RedSprite {

Search::Search(TranspositionTable& t) : tt(t), pv_length(0) {
    memset(killer_moves, 0, sizeof(killer_moves));
    memset(history, 0, sizeof(history));
    memset(counter_moves, 0, sizeof(counter_moves));
}

Search::~Search() {
    stop();
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
}

bool Search::should_stop() {
    if (stop_flag.load()) return true;
    
    if (limits.nodes > 0 && stats.nodes.load() >= limits.nodes) {
        return true;
    }
    
    if (limits.time_ms > 0 && !limits.infinite) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count() - limits.start_time;
        
        if (elapsed >= static_cast<int64_t>(limits.move_time)) {
            return true;
        }
    }
    
    return false;
}

void Search::check_time() {
    if (stats.nodes.load() % NODES_PER_CHECK == 0) {
        if (should_stop()) {
            stop_flag.store(true);
        }
    }
}

void Search::score_moves(ScoredMove* moves, int count, Move tt_move, int ply) {
    for (int i = 0; i < count; ++i) {
        Move m = moves[i].move;
        int score = 0;
        
        // TT move gets highest priority
        if (m == tt_move) {
            score = SCORE_TT_MOVE;
        }
        // Killer moves
        else if (m == killer_moves[ply][0]) {
            score = SCORE_KILLER_1;
        } else if (m == killer_moves[ply][1]) {
            score = SCORE_KILLER_2;
        }
        // Counter move
        else if (ply > 0 && m == counter_moves[ply]) {
            score = SCORE_COUNTER;
        }
        // History heuristic
        else {
            Piece p = NO_PIECE; // Would get from position
            score = history[p][m.to_sq()];
        }
        
        moves[i].score = score;
    }
    
    // Sort moves by score (descending)
    std::sort(moves, moves + count, [](const ScoredMove& a, const ScoredMove& b) {
        return a.score > b.score;
    });
}

void Search::update_history(Move m, int bonus, int ply) {
    Piece p = NO_PIECE; // Would get actual piece
    int& h = history[p][m.to_sq()];
    *h += bonus - *h * std::abs(bonus) / MAX_HISTORY;
    
    // Clamp to valid range
    if (*h > MAX_HISTORY) *h = MAX_HISTORY;
    if (*h < -MAX_HISTORY) *h = -MAX_HISTORY;
}

void Search::update_killer(Move m, int ply) {
    if (ply < MAX_PLY) {
        if (m != killer_moves[ply][0]) {
            killer_moves[ply][1] = killer_moves[ply][0];
            killer_moves[ply][0] = m;
        }
    }
}

int Search::quiescence(Position& pos, int alpha, int beta, int ply) {
    stats.qsearch_calls++;
    stats.nodes++;
    
    // Check for draw
    if (pos.is_insufficient_material()) {
        return 0;
    }
    
    // Stand pat evaluation
    int stand_pat = evaluator.evaluate(pos);
    stats.eval_calls++;
    
    if (stand_pat >= beta) {
        return beta;
    }
    
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }
    
    // Generate captures
    ScoredMove moves[MAX_MOVES];
    MoveGenerator mg(&pos);
    int move_count = mg.generate_captures(moves);
    
    // Score and sort captures
    score_moves(moves, move_count, Move(), ply);
    
    // Try captures
    for (int i = 0; i < move_count; ++i) {
        if (should_stop()) break;
        
        Move m = moves[i].move;
        
        // Skip bad captures (MVV-LVA would go here)
        if (moves[i].score < 0) continue;
        
        Piece captured = pos.piece_at(m.to_sq());
        bool was_promotion = (m.type() == MOVE_PROMOTION);
        PieceType promo_type = m.promotion_type();
        
        pos.make_move(m);
        
        // Check legality
        Color us = static_cast<Color>(1 - pos.get_side_to_move());
        Color them = pos.get_side_to_move();
        
        Square ksq = SQUARE_NONE;
        for (int s = SQ_A1; s <= SQ_H8; ++s) {
            if (pos.piece_at(static_cast<Square>(s)) == (us == WHITE ? W_KING : B_KING)) {
                ksq = static_cast<Square>(s);
                break;
            }
        }
        
        bool legal = (ksq != SQUARE_NONE) && !pos.is_attacked_by(ksq, them);
        
        if (legal) {
            int score = -quiescence(pos, -beta, -alpha, ply + 1);
            
            pos.unmake_move(m, captured, was_promotion, promo_type);
            
            if (score >= beta) {
                return beta;
            }
            
            if (score > alpha) {
                alpha = score;
            }
        } else {
            pos.unmake_move(m, captured, was_promotion, promo_type);
        }
    }
    
    return alpha;
}

int Search::alpha_beta(Position& pos, int depth, int alpha, int beta, int ply, bool cutnode) {
    stats.nodes++;
    check_time();
    
    bool is_root = (ply == 0);
    bool is_pv = (beta - alpha > 1);
    
    // Check for terminal nodes
    if (pos.is_insufficient_material()) {
        return 0;
    }
    
    // Draw detection (50-move rule, repetition would go here)
    if (pos.get_halfmove_clock() >= 100) {
        return 0;
    }
    
    // Transposition table lookup
    TTEntry* tt_entry = nullptr;
    Move tt_move;
    uint64_t key = pos.get_zobrist_key().value();
    
    if (!is_pv) {
        tt_entry = tt.probe(key);
        if (tt_entry && tt_entry->depth >= depth) {
            stats.tt_hits++;
            
            int score = tt_entry->score;
            // Adjust mate scores for ply
            if (score > -VALUE_INFINITE && score < VALUE_INFINITE) {
                // Normal score
            }
            
            if (tt_entry->flags == 0) {
                return score; // Exact score
            } else if (tt_entry->flags == 1 && score >= beta) {
                stats.tt_cutoffs++;
                return score; // Lower bound
            } else if (tt_entry->flags == 2 && score <= alpha) {
                stats.tt_cutoffs++;
                return score; // Upper bound
            }
            
            tt_move = tt_entry->best_move;
        }
    }
    
    // Check extension
    bool in_check = pos.is_in_check();
    if (in_check) {
        depth++;
    }
    
    // Quiescence search at leaf nodes
    if (depth <= 0) {
        return quiescence(pos, alpha, beta, ply);
    }
    
    // Null move pruning
    if (!is_pv && !in_check && depth >= 2 && !pos.is_insufficient_material()) {
        Position null_pos = pos;
        null_pos.make_move(Move()); // Null move
        
        int R = 3 + depth / 5;
        int score = -alpha_beta(null_pos, depth - R, -beta, -beta + 1, ply + 1, !cutnode);
        
        if (score >= beta) {
            return beta;
        }
    }
    
    // Internal iterative reduction
    if (is_pv && depth >= 4 && !tt_entry) {
        depth--;
    }
    
    // Generate moves
    ScoredMove moves[MAX_MOVES];
    MoveGenerator mg(&pos);
    int move_count = mg.generate_legal(moves);
    
    if (move_count == 0) {
        // Checkmate or stalemate
        if (in_check) {
            return -VALUE_INFINITE + ply;
        }
        return 0; // Stalemate
    }
    
    // Score moves
    score_moves(moves, move_count, tt_move, ply);
    
    Move best_move;
    int best_score = -VALUE_INFINITE;
    int old_alpha = alpha;
    int moves_searched = 0;
    
    // Iterate through moves
    for (int i = 0; i < move_count; ++i) {
        if (should_stop()) break;
        
        Move m = moves[i].move;
        
        Piece captured = pos.piece_at(m.to_sq());
        bool was_promotion = (m.type() == MOVE_PROMOTION);
        PieceType promo_type = m.promotion_type();
        
        pos.make_move(m);
        
        // Verify legality
        Color us = static_cast<Color>(1 - pos.get_side_to_move());
        Color them = pos.get_side_to_move();
        
        Square ksq = SQUARE_NONE;
        for (int s = SQ_A1; s <= SQ_H8; ++s) {
            if (pos.piece_at(static_cast<Square>(s)) == (us == WHITE ? W_KING : B_KING)) {
                ksq = static_cast<Square>(s);
                break;
            }
        }
        
        bool legal = (ksq != SQUARE_NONE) && !pos.is_attacked_by(ksq, them);
        
        if (!legal) {
            pos.unmake_move(m, captured, was_promotion, promo_type);
            continue;
        }
        
        int score;
        int new_depth = depth - 1;
        
        // Late Move Reduction (LMR)
        if (moves_searched >= 3 && !is_pv && !in_check && !was_promotion) {
            int R = 1 + static_cast<int>(std::log(moves_searched) * std::log(depth) / 2.5);
            if (cutnode) R++;
            if (is_pv) R--;
            R = std::max(1, std::min(R, depth - 1));
            
            score = -alpha_beta(pos, new_depth - R, -alpha - 1, -alpha, ply + 1, true);
            
            // Re-search if LMR failed high
            if (score > alpha && R > 1) {
                score = -alpha_beta(pos, new_depth, -alpha - 1, -alpha, ply + 1, !cutnode);
            }
        }
        // Principal Variation Search (PVS)
        else if (moves_searched == 0 || is_pv) {
            score = -alpha_beta(pos, new_depth, -beta, -alpha, ply + 1, !cutnode);
        } else {
            score = -alpha_beta(pos, new_depth, -alpha - 1, -alpha, ply + 1, !cutnode);
            
            if (score > alpha && score < beta) {
                score = -alpha_beta(pos, new_depth, -beta, -alpha, ply + 1, !cutnode);
            }
        }
        
        pos.unmake_move(m, captured, was_promotion, promo_type);
        
        if (score > best_score) {
            best_score = score;
            best_move = m;
            
            if (score > alpha) {
                alpha = score;
                
                // Update PV
                if (is_root) {
                    pv_line[0] = m;
                    pv_length = 1;
                }
                
                if (alpha >= beta) {
                    // Beta cutoff
                    update_killer(m, ply);
                    update_history(m, depth * depth, ply);
                    break;
                }
            }
        }
        
        moves_searched++;
    }
    
    // Store in transposition table
    if (!should_stop()) {
        int flag = 0; // Exact
        if (best_score <= old_alpha) {
            flag = 2; // Upper bound
        } else if (best_score >= beta) {
            flag = 1; // Lower bound
        }
        
        tt.store(key, best_move, best_score, depth, flag);
    }
    
    return best_score;
}

Move Search::search(const Position& pos, int depth, int time_ms) {
    root_pos = pos;
    limits.depth = depth;
    limits.time_ms = time_ms;
    limits.move_time = time_ms;
    limits.start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    stop_flag.store(false);
    stats.nodes.store(0);
    stats.tt_hits.store(0);
    
    // Generate root moves
    root_moves.clear();
    ScoredMove moves[MAX_MOVES];
    MoveGenerator mg(&root_pos);
    int count = mg.generate_legal(moves);
    
    for (int i = 0; i < count; ++i) {
        root_moves.push_back(moves[i]);
    }
    
    if (root_moves.empty()) {
        return Move();
    }
    
    // Iterative deepening
    return iterative_deepening(depth);
}

Move Search::iterative_deepening(int max_depth) {
    Move best_move;
    int best_score = -VALUE_INFINITE;
    
    for (int d = 1; d <= max_depth && !should_stop(); ++d) {
        int alpha = -VALUE_INFINITE;
        int beta = VALUE_INFINITE;
        
        // Aspiration window
        if (d >= 4) {
            int window = 25;
            alpha = best_score - window;
            beta = best_score + window;
        }
        
        while (!should_stop()) {
            pv_length = 0;
            int score = alpha_beta(root_pos, d, alpha, beta, 0, false);
            
            if (should_stop()) break;
            
            if (score <= alpha) {
                // Failed low, widen window
                beta = (alpha + beta) / 2;
                alpha -= 50;
            } else if (score >= beta) {
                // Failed high, widen window
                beta += 50;
            } else {
                // Success
                best_score = score;
                if (pv_length > 0) {
                    best_move = pv_line[0];
                }
                break;
            }
        }
        
        // Print progress (would go to UCI interface)
        // std::cout << "info depth " << d << " score " << best_score << " nodes " << stats.nodes.load() << std::endl;
    }
    
    return best_move;
}

Move Search::get_best_move() const {
    if (pv_length > 0) {
        return pv_line[0];
    }
    return Move();
}

} // namespace RedSprite
