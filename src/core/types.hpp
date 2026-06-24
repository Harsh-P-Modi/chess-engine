#ifndef CHESS_CORE_TYPES_HPP
#define CHESS_CORE_TYPES_HPP

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace chess {

enum class Color : uint8_t { White, Black };
using Board = std::array<char, 64>;

struct Move {
    uint8_t from = 0;
    uint8_t to = 0;
    char piece = '.';
    char captured = '.';
    char promo = '.';
    bool is_castle = false;
    bool is_ep = false;
    bool is_double = false;

    std::string uci() const;
};

} // namespace chess

#endif // CHESS_CORE_TYPES_HPP
