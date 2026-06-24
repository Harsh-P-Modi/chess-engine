#include "core/board.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace chess {

std::string Move::uci() const {
    auto result = square_name(from) + square_name(to);
    if (promo != '.') {
        result.push_back(static_cast<char>(std::tolower(promo)));
    }
    return result;
}

int sq(int file, int rank) {
    return rank * 8 + file;
}

int file_of(int square) {
    return square % 8;
}

int rank_of(int square) {
    return square / 8;
}

bool on_board(int file, int rank) {
    return file >= 0 && file < 8 && rank >= 0 && rank < 8;
}

std::string square_name(uint8_t square) {
    int file = file_of(square);
    int rank = rank_of(square);
    return std::string{FILES[file]} + static_cast<char>('1' + rank);
}

uint8_t from_algebraic(std::string_view text) {
    if (text.size() != 2) {
        throw std::invalid_argument("Invalid square notation");
    }

    char file_char = static_cast<char>(text[0]);
    char rank_char = static_cast<char>(text[1]);
    const char* pos = std::find(std::begin(FILES), std::end(FILES) - 1, file_char);
    if (pos == std::end(FILES) - 1) {
        throw std::invalid_argument("Invalid file in square notation");
    }

    int file = static_cast<int>(pos - std::begin(FILES));
    int rank = rank_char - '1';
    if (rank < 0 || rank >= 8) {
        throw std::invalid_argument("Invalid rank in square notation");
    }
    return static_cast<uint8_t>(sq(file, rank));
}

bool is_white(char p) {
    return p >= 'A' && p <= 'Z';
}

Color color_of(char p) {
    return is_white(p) ? Color::White : Color::Black;
}

Color opposite(Color color) {
    return color == Color::White ? Color::Black : Color::White;
}

State initial_state() {
    State state;
    state.board.fill('.');
    const char back[] = {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'};
    for (int file = 0; file < 8; ++file) {
        state.board[sq(file, 0)] = back[file];
        state.board[sq(file, 1)] = 'P';
        state.board[sq(file, 6)] = 'p';
        state.board[sq(file, 7)] = static_cast<char>(std::tolower(back[file]));
    }
    return state;
}

State fen_to_state(std::string_view fen) {
    State state;
    state.board.fill('.');
    std::string fen_string(fen);
    std::istringstream iss(fen_string);
    std::string placement;
    iss >> placement;
    std::vector<std::string> parts;
    parts.push_back(placement);
    std::string token;
    while (iss >> token) {
        parts.push_back(token);
    }

    const std::string& side_str = parts.size() > 1 ? parts[1] : "w";
    const std::string& castling_str = parts.size() > 2 ? parts[2] : "";
    const std::string& ep_str = parts.size() > 3 ? parts[3] : "-";

    auto rows = std::vector<std::string>{};
    std::string current;
    for (char ch : placement) {
        if (ch == '/') {
            rows.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    rows.push_back(current);

    if (rows.size() != 8) {
        throw std::invalid_argument("Invalid FEN board layout");
    }

    for (size_t rank_index = 0; rank_index < 8; ++rank_index) {
        const std::string& row = rows[rank_index];
        int rank = 7 - static_cast<int>(rank_index);
        int file = 0;
        for (char ch : row) {
            if (std::isdigit(static_cast<unsigned char>(ch))) {
                file += ch - '0';
            } else {
                if (file < 0 || file >= 8) {
                    throw std::invalid_argument("Invalid FEN row");
                }
                state.board[sq(file, rank)] = ch;
                ++file;
            }
        }
    }

    state.side = side_str == "b" ? Color::Black : Color::White;
    state.castling = 0;
    if (castling_str != "-") {
        for (char c : castling_str) {
            switch (c) {
                case 'K': state.castling |= CASTLE_K; break;
                case 'Q': state.castling |= CASTLE_Q; break;
                case 'k': state.castling |= CASTLE_k; break;
                case 'q': state.castling |= CASTLE_q; break;
                default: break;
            }
        }
    }

    if (ep_str != "-") {
        state.ep = from_algebraic(ep_str);
    } else {
        state.ep.reset();
    }

    return state;
}

bool attacked_by(const Board& board, uint8_t target, Color by_color) {
    int tf = file_of(target);
    int tr = rank_of(target);

    int pawn_rank = tr + (by_color == Color::White ? -1 : 1);
    for (int df : {-1, 1}) {
        int f = tf + df;
        if (on_board(f, pawn_rank)) {
            int square = sq(f, pawn_rank);
            char p = board[square];
            if (p != '.' && color_of(p) == by_color && std::toupper(p) == 'P') {
                return true;
            }
        }
    }

    for (auto [df, dr] : KNIGHT_OFFSETS) {
        int f = tf + df;
        int r = tr + dr;
        if (on_board(f, r)) {
            char p = board[sq(f, r)];
            if (p != '.' && color_of(p) == by_color && std::toupper(p) == 'N') {
                return true;
            }
        }
    }

    for (auto [df, dr] : KING_OFFSETS) {
        int f = tf + df;
        int r = tr + dr;
        if (on_board(f, r)) {
            char p = board[sq(f, r)];
            if (p != '.' && color_of(p) == by_color && std::toupper(p) == 'K') {
                return true;
            }
        }
    }

    for (auto [df, dr] : BISHOP_DIRS) {
        int f = tf + df;
        int r = tr + dr;
        while (on_board(f, r)) {
            char p = board[sq(f, r)];
            if (p != '.') {
                if (color_of(p) == by_color) {
                    char pt = std::toupper(p);
                    if (pt == 'B' || pt == 'Q') {
                        return true;
                    }
                }
                break;
            }
            f += df;
            r += dr;
        }
    }

    for (auto [df, dr] : ROOK_DIRS) {
        int f = tf + df;
        int r = tr + dr;
        while (on_board(f, r)) {
            char p = board[sq(f, r)];
            if (p != '.') {
                if (color_of(p) == by_color) {
                    char pt = std::toupper(p);
                    if (pt == 'R' || pt == 'Q') {
                        return true;
                    }
                }
                break;
            }
            f += df;
            r += dr;
        }
    }

    return false;
}

int find_king(const Board& board, Color color) {
    char king = color == Color::White ? 'K' : 'k';
    for (int i = 0; i < 64; ++i) {
        if (board[i] == king) {
            return i;
        }
    }
    return -1;
}

bool is_in_check(const State& state) {
    int king_pos = find_king(state.board, state.side);
    if (king_pos < 0) {
        return false;
    }
    return attacked_by(state.board, static_cast<uint8_t>(king_pos), opposite(state.side));
}

static std::string piece_symbol(char piece) {
    switch (piece) {
        case 'P': return u8"\u2659";
        case 'N': return u8"\u2658";
        case 'B': return u8"\u2657";
        case 'R': return u8"\u2656";
        case 'Q': return u8"\u2655";
        case 'K': return u8"\u2654";
        case 'p': return u8"\u265F";
        case 'n': return u8"\u265E";
        case 'b': return u8"\u265D";
        case 'r': return u8"\u265C";
        case 'q': return u8"\u265B";
        case 'k': return u8"\u265A";
        default: return ".";
    }
}

void print_board(const State& state) {
    std::cout << '\n';
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << (rank + 1) << ' ';
        for (int file = 0; file < 8; ++file) {
            char piece = state.board[sq(file, rank)];
            std::cout << piece_symbol(piece);
            if (file < 7) {
                std::cout << ' ';
            }
        }
        std::cout << '\n';
    }
    std::cout << "  a b c d e f g h\n\n";
}

} // namespace chess
