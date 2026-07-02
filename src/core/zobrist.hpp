#ifndef CHESS_CORE_ZOBRIST_HPP
#define CHESS_CORE_ZOBRIST_HPP

#include <cstdint>

namespace chess {

enum class Color : uint8_t;
struct State;
struct Move;

uint64_t compute_hash(const State& state);
uint64_t compute_hash(const State& state, const Move& move);

}

#endif
