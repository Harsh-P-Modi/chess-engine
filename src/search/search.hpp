#ifndef CHESS_SEARCH_SEARCH_HPP
#define CHESS_SEARCH_SEARCH_HPP

#include "../movegen/movegen.hpp"
#include "../evaluation/evaluation.hpp"

#include <cstdint>

namespace chess {

struct SearchResult {
    Move move;
    int score = 0;
    int64_t nodes = 0;
    bool has_move = false;
};

SearchResult search(const State& state, int depth);

} // namespace chess

#endif // CHESS_SEARCH_SEARCH_HPP
