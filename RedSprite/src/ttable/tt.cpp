#include "tt.h"
#include <cstring>

namespace RedSprite {

TranspositionTable::TranspositionTable() : size(0), mask(0), current_generation(0) {
    resize(1); // Default 1 MB
}

TranspositionTable::~TranspositionTable() = default;

void TranspositionTable::resize(size_t mb) {
    std::lock_guard<std::mutex> lock(mutex);
    
    size = (mb * 1024 * 1024) / sizeof(TTEntry);
    
    // Round down to power of 2
    size_t s = 1;
    while (s * 2 <= size) s *= 2;
    size = s;
    
    mask = size - 1;
    table.resize(size);
    clear();
}

void TranspositionTable::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    std::fill(table.begin(), table.end(), TTEntry());
    current_generation = 0;
}

void TranspositionTable::store(uint64_t key, Move move, int score, int depth, int flag) {
    size_t index = key & mask;
    TTEntry& entry = table[index];
    
    // Always overwrite if:
    // 1. Different position
    // 2. Same position but deeper search
    // 3. Same position and same depth but exact score preferred
    if (entry.key != key || 
        depth >= entry.depth || 
        (depth == entry.depth && flag == 0 && entry.flags != 0)) {
        
        entry.key = key;
        entry.best_move = move;
        entry.score = static_cast<int16_t>(score);
        entry.depth = static_cast<uint8_t>(depth);
        entry.flags = static_cast<uint8_t>(flag);
        entry.generation = current_generation;
    }
}

TTEntry* TranspositionTable::probe(uint64_t key) {
    size_t index = key & mask;
    TTEntry& entry = table[index];
    
    if (entry.key == key && entry.generation == current_generation) {
        return &entry;
    }
    
    return nullptr;
}

void TranspositionTable::increment_generation() {
    current_generation++;
    if (current_generation == 0) {
        // Wrap around: clear old entries
        for (auto& entry : table) {
            if (entry.generation != current_generation) {
                entry = TTEntry();
            }
        }
    }
}

size_t TranspositionTable::get_entries_count() const {
    size_t count = 0;
    for (const auto& entry : table) {
        if (entry.key != 0 && entry.generation == current_generation) {
            ++count;
        }
    }
    return count;
}

} // namespace RedSprite
