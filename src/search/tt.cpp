#include "tt.hpp"

#include <cstring>

namespace chess {

TranspositionTable::TranspositionTable(size_t mb) {
    size_t bytes = mb * 1024 * 1024;
    size = bytes / sizeof(TTEntry);
    if (size < 1) size = 1;
    entries.resize(size);
    clear();
}

void TranspositionTable::clear() {
    for (auto& entry : entries) {
        entry.hash = 0;
        entry.depth = 0;
    }
}

std::optional<TTEntry> TranspositionTable::probe(uint64_t hash, int depth, int alpha, int beta) const {
    size_t index = static_cast<size_t>(hash) & (size - 1);
    const TTEntry& entry = entries[index];
    if (entry.hash != hash) {
        return std::nullopt;
    }
    if (entry.depth < depth) {
        return std::nullopt;
    }

    if (entry.flag == FLAG_EXACT) {
        return entry;
    } else if (entry.flag == FLAG_UPPER && entry.score <= alpha) {
        return entry;
    } else if (entry.flag == FLAG_LOWER && entry.score >= beta) {
        return entry;
    }

    return std::nullopt;
}

void TranspositionTable::store(uint64_t hash, int depth, int score, int flag, uint32_t best_move) {
    size_t index = static_cast<size_t>(hash) & (size - 1);
    TTEntry& entry = entries[index];
    if (entry.hash != hash || depth >= entry.depth) {
        entry.hash = hash;
        entry.depth = static_cast<uint8_t>(depth);
        entry.score = static_cast<int16_t>(score);
        entry.flag = static_cast<uint8_t>(flag);
        entry.best_move = best_move;
    }
}

}
