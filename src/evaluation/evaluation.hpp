#ifndef CHESS_EVALUATION_EVALUATION_HPP
#define CHESS_EVALUATION_EVALUATION_HPP

#include "../core/board.hpp"

namespace chess {

int evaluate(const State& state);
int relative_eval(const State& state);

} // namespace chess

#endif // CHESS_EVALUATION_EVALUATION_HPP
