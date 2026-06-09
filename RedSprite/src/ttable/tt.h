#pragma once

#include "types.h"
#include "move.h"
#include <cstdint>
#include <vector>
#include <mutex>

namespace RedSprite {

struct TTEntry {
    uint64_t key;
    Move best_move;
    int16_t score;
    uint8_t depth;
    uint8_t flags;  // 0: exact, 1: lower bound (alpha), 2: upper bound (beta)
    uint8_t generation;
    
    constexpr TTEntry() : key(0), best_move(), score(0), depth(0), flags(0), generation(0) {}
};

class TranspositionTable {
private:
    std::vector<TTEntry> table;
    size_t size;
    size_t mask;
    uint8_t current_generation;
    std::mutex mutex;
    
public:
    TranspositionTable();
    ~TranspositionTable();
    
    void resize(size_t mb);
    void clear();
    
    void store(uint64_t key, Move move, int score, int depth, int flag);
    TTEntry* probe(uint64_t key);
    
    void increment_generation();
    uint8_t get_generation() const { return current_generation; }
    
    size_t get_size() const { return size; }
    size_t get_entries_count() const;
};

} // namespace RedSprite
