#ifndef CHESS_SEARCH_SEARCH_HPP
#define CHESS_SEARCH_SEARCH_HPP

#include "../movegen/movegen.hpp"
#include "../evaluation/evaluation.hpp"
#include "tt.hpp"

#include <cstdint>
#include <chrono>

namespace chess {

struct TimeState {
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
    int64_t max_nodes = 0;
    int64_t nodes = 0;
    bool stop_requested = false;
};

struct SearchResult {
    Move move;
    int score = 0;
    int64_t nodes = 0;
    bool has_move = false;
};

bool should_stop(const TimeState& time_state);

SearchResult search(const State& state, int depth, TranspositionTable& tt);
SearchResult search(const State& state, int depth, TranspositionTable& tt, TimeState& time_state);

}

#endif // CHESS_SEARCH_SEARCH_HPP
