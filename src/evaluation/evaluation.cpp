#include "evaluation.hpp"

#include <cctype>

namespace chess {

static int piece_value(char piece) {
    switch (std::toupper(static_cast<unsigned char>(piece))) {
        case 'P': return 100;
        case 'N': return 320;
        case 'B': return 330;
        case 'R': return 500;
        case 'Q': return 900;
        case 'K': return 20000;
        default: return 0;
    }
}

int evaluate(const State& state) {
    int score = 0;
    for (char piece : state.board) {
        if (piece == '.') {
            continue;
        }
        int value = piece_value(piece);
        if (is_white(piece)) {
            score += value;
        } else {
            score -= value;
        }
    }
    return score;
}

int relative_eval(const State& state) {
    int value = evaluate(state);
    return state.side == Color::White ? value : -value;
}

} // namespace chess
