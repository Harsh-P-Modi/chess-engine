#ifndef CHESS_SEARCH_TT_HPP
#define CHESS_SEARCH_TT_HPP

#include "../core/types.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace chess {

struct TTEntry {
    uint64_t hash;
    int16_t score;
    uint8_t depth;
    uint8_t flag;
    uint32_t best_move;
};

static_assert(sizeof(TTEntry) == 16, "TTEntry must be 16 bytes");

class TranspositionTable {
public:
    explicit TranspositionTable(size_t mb = 16);
    void clear();
    std::optional<TTEntry> probe(uint64_t hash, int depth, int alpha, int beta) const;
    void store(uint64_t hash, int depth, int score, int flag, uint32_t best_move);

private:
    size_t size;
    std::vector<TTEntry> entries;
};

}

#endif
