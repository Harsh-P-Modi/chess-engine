#ifndef CHESS_CORE_CONSTANTS_HPP
#define CHESS_CORE_CONSTANTS_HPP

#include <array>
#include <utility>
#include <cstdint>

namespace chess {

inline constexpr char FILES[] = "abcdefgh";
inline constexpr std::array<std::pair<int, int>, 8> KNIGHT_OFFSETS{{
    {1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}
}};
inline constexpr std::array<std::pair<int, int>, 8> KING_OFFSETS{{
    {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}
}};
inline constexpr std::array<std::pair<int, int>, 4> BISHOP_DIRS{{
    {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
}};
inline constexpr std::array<std::pair<int, int>, 4> ROOK_DIRS{{
    {1, 0}, {-1, 0}, {0, 1}, {0, -1}
}};

inline constexpr uint8_t CASTLE_K = 1;
inline constexpr uint8_t CASTLE_Q = 2;
inline constexpr uint8_t CASTLE_k = 4;
inline constexpr uint8_t CASTLE_q = 8;

} // namespace chess

#endif // CHESS_CORE_CONSTANTS_HPP
