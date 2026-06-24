#ifndef CHESS_MOVEGEN_MOVEGEN_HPP
#define CHESS_MOVEGEN_MOVEGEN_HPP

#include "../core/board.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace chess {

std::vector<Move> gen_pseudo_moves(const State& state);
std::vector<Move> legal_moves(const State& state);
State make_move(const State& state, const Move& move);
std::optional<Move> parse_move_input(const State& state, std::string_view text);
std::optional<Move> find_move(const State& state, std::string_view uci);

} // namespace chess

#endif // CHESS_MOVEGEN_MOVEGEN_HPP
