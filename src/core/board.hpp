#ifndef CHESS_CORE_BOARD_HPP
#define CHESS_CORE_BOARD_HPP

#include "constants.hpp"
#include "types.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace chess {

int sq(int file, int rank);
int file_of(int square);
int rank_of(int square);
bool on_board(int file, int rank);
std::string square_name(uint8_t square);
uint8_t from_algebraic(std::string_view text);
bool is_white(char p);
Color color_of(char p);
Color opposite(Color color);

struct State {
    Board board;
    Color side = Color::White;
    uint8_t castling = CASTLE_K | CASTLE_Q | CASTLE_k | CASTLE_q;
    std::optional<uint8_t> ep;

    State copy() const { return *this; }
};

State initial_state();
State fen_to_state(std::string_view fen);

bool attacked_by(const Board& board, uint8_t target, Color by_color);
int find_king(const Board& board, Color color);
bool is_in_check(const State& state);
void print_board(const State& state);

} // namespace chess

#endif // CHESS_CORE_BOARD_HPP
