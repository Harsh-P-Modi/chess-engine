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

    uint32_t pack() const;
    static Move unpack(uint32_t packed);
};

inline constexpr int FLAG_EXACT = 0;
inline constexpr int FLAG_UPPER = 1;
inline constexpr int FLAG_LOWER = 2;

} // namespace chess

#endif // CHESS_CORE_TYPES_HPP
