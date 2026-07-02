#include "evaluation.hpp"

#include "core/board.hpp"
#include "core/constants.hpp"

#include <algorithm>
#include <cctype>
#include <array>

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

static constexpr int mg_pawn[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
    5, 5, 10, 25, 25, 10, 5, 5,
    0, 0, 0, 20, 20, 0, 0, 0,
    5, -5, -10, 0, 0, -10, -5, 5,
    5, 10, 10, -20, -20, 10, 10, 5,
    0, 0, 0, 0, 0, 0, 0, 0
};

static constexpr int eg_pawn[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    80, 80, 80, 80, 80, 80, 80, 80,
    50, 50, 50, 50, 50, 50, 50, 50,
    30, 30, 30, 30, 30, 30, 30, 30,
    20, 20, 20, 20, 20, 20, 20, 20,
    10, 10, 10, 10, 10, 10, 10, 10,
    5, 5, 5, 5, 5, 5, 5, 5,
    0, 0, 0, 0, 0, 0, 0, 0
};

static constexpr int mg_knight[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20, 0, 0, 0, 0, -20, -40,
    -30, 0, 10, 15, 15, 10, 0, -30,
    -30, 5, 15, 20, 20, 15, 5, -30,
    -30, 0, 15, 20, 20, 15, 0, -30,
    -30, 5, 10, 15, 15, 10, 5, -30,
    -40, -20, 0, 5, 5, 0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50
};

static constexpr int eg_knight[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20, 0, 0, 0, 0, -20, -40,
    -30, 0, 10, 15, 15, 10, 0, -30,
    -30, 5, 15, 20, 20, 15, 5, -30,
    -30, 0, 15, 20, 20, 15, 0, -30,
    -30, 5, 10, 15, 15, 10, 5, -30,
    -40, -20, 0, 5, 5, 0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50
};

static constexpr int mg_bishop[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10, 0, 0, 0, 0, 0, 0, -10,
    -10, 0, 5, 10, 10, 5, 0, -10,
    -10, 5, 5, 10, 10, 5, 5, -10,
    -10, 0, 10, 10, 10, 10, 0, -10,
    -10, 10, 10, 10, 10, 10, 10, -10,
    -10, 5, 0, 0, 0, 0, 5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
};

static constexpr int eg_bishop[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10, 0, 0, 0, 0, 0, 0, -10,
    -10, 0, 5, 10, 10, 5, 0, -10,
    -10, 5, 5, 10, 10, 5, 5, -10,
    -10, 0, 10, 10, 10, 10, 0, -10,
    -10, 10, 10, 10, 10, 10, 10, -10,
    -10, 5, 0, 0, 0, 0, 5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
};

static constexpr int mg_rook[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    5, 10, 10, 10, 10, 10, 10, 5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    0, 0, 0, 5, 5, 0, 0, 0
};

static constexpr int eg_rook[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    5, 10, 10, 10, 10, 10, 10, 5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    0, 0, 0, 5, 5, 0, 0, 0
};

static constexpr int mg_queen[64] = {
    -20, -10, -10, -5, -5, -10, -10, -20,
    -10, 0, 0, 0, 0, 0, 0, -10,
    -10, 0, 5, 5, 5, 5, 0, -10,
    -5, 0, 5, 5, 5, 5, 0, -5,
    0, 0, 5, 5, 5, 5, 0, -5,
    -10, 5, 5, 5, 5, 5, 0, -10,
    -10, 0, 5, 0, 0, 0, 0, -10,
    -20, -10, -10, -5, -5, -10, -10, -20
};

static constexpr int eg_queen[64] = {
    -20, -10, -10, -5, -5, -10, -10, -20,
    -10, 0, 0, 0, 0, 0, 0, -10,
    -10, 0, 5, 5, 5, 5, 0, -10,
    -5, 0, 5, 5, 5, 5, 0, -5,
    0, 0, 5, 5, 5, 5, 0, -5,
    -10, 5, 5, 5, 5, 5, 0, -10,
    -10, 0, 5, 0, 0, 0, 0, -10,
    -20, -10, -10, -5, -5, -10, -10, -20
};

static constexpr int mg_king[64] = {
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10,
    20, 20, 0, 0, 0, 0, 20, 20,
    20, 30, 10, 0, 0, 10, 30, 20
};

static constexpr int eg_king[64] = {
    -50, -40, -30, -20, -20, -30, -40, -50,
    -30, -20, -10, 0, 0, -10, -20, -30,
    -30, -10, 20, 30, 30, 20, -10, -30,
    -30, -10, 30, 40, 40, 30, -10, -30,
    -30, -10, 30, 40, 40, 30, -10, -30,
    -30, -10, 20, 30, 30, 20, -10, -30,
    -30, -30, 0, 0, 0, 0, -30, -30,
    -50, -30, -30, -30, -30, -30, -30, -50
};

static int pst_score(char piece, int square, Color side) {
    if (side == Color::Black) {
        square = 63 - square;
    }
    switch (std::toupper(static_cast<unsigned char>(piece))) {
        case 'P': return mg_pawn[square];
        case 'N': return mg_knight[square];
        case 'B': return mg_bishop[square];
        case 'R': return mg_rook[square];
        case 'Q': return mg_queen[square];
        case 'K': return mg_king[square];
        default: return 0;
    }
}

static int eg_pst_score(char piece, int square, Color side) {
    if (side == Color::Black) {
        square = 63 - square;
    }
    switch (std::toupper(static_cast<unsigned char>(piece))) {
        case 'P': return eg_pawn[square];
        case 'N': return eg_knight[square];
        case 'B': return eg_bishop[square];
        case 'R': return eg_rook[square];
        case 'Q': return eg_queen[square];
        case 'K': return eg_king[square];
        default: return 0;
    }
}

static int pawn_structure_bonus(const Board& board, Color side) {
    int score = 0;
    uint64_t pawns = 0;
    for (int i = 0; i < 64; ++i) {
        char p = board[i];
        if (p != '.' && std::toupper(p) == 'P' && color_of(p) == side) {
            pawns |= (1ULL << i);
        }
    }

    for (int file = 0; file < 8; ++file) {
        int count = 0;
        for (int rank = 0; rank < 8; ++rank) {
            int sq_idx = rank * 8 + file;
            if ((pawns >> sq_idx) & 1ULL) {
                ++count;
            }
        }
        if (count > 1) {
            score -= 10 * (count - 1);
        }
        bool isolated = true;
        for (int df : {-1, 1}) {
            int nf = file + df;
            if (nf >= 0 && nf < 8) {
                for (int rank = 0; rank < 8; ++rank) {
                    int sq_idx = rank * 8 + nf;
                    if ((pawns >> sq_idx) & 1ULL) {
                        isolated = false;
                        break;
                    }
                }
            }
            if (!isolated) break;
        }
        if (isolated && count > 0) {
            score -= 10;
        }
    }
    return score;
}

static int king_safety(const Board& board, Color side) {
    int king_sq = find_king(board, side);
    if (king_sq < 0) return 0;
    int kf = file_of(king_sq);
    int kr = rank_of(king_sq);
    int shield = 0;
    if (side == Color::White) {
        for (int df : {-1, 0, 1}) {
            int f = kf + df;
            int r = kr + 1;
            if (on_board(f, r)) {
                if (board[sq(f, r)] == 'P') ++shield;
            }
            r = kr + 2;
            if (on_board(f, r)) {
                if (board[sq(f, r)] == 'P') ++shield;
            }
        }
    } else {
        for (int df : {-1, 0, 1}) {
            int f = kf + df;
            int r = kr - 1;
            if (on_board(f, r)) {
                if (board[sq(f, r)] == 'p') ++shield;
            }
            r = kr - 2;
            if (on_board(f, r)) {
                if (board[sq(f, r)] == 'p') ++shield;
            }
        }
    }
    return shield * 5;
}

static int game_phase(const Board& board) {
    int phase = 0;
    for (char p : board) {
        if (p == '.') continue;
        switch (std::toupper(static_cast<unsigned char>(p))) {
            case 'P': break;
            case 'N': phase += 1; break;
            case 'B': phase += 1; break;
            case 'R': phase += 2; break;
            case 'Q': phase += 4; break;
            case 'K': break;
        }
    }
    return std::min(24, std::max(0, phase));
}

static int evaluate_mg_eg(const State& state, bool eg) {
    int score = 0;
    for (int sq = 0; sq < 64; ++sq) {
        char piece = state.board[sq];
        if (piece == '.') continue;
        int val = piece_value(piece);
        int pst = eg ? eg_pst_score(piece, sq, state.side) : pst_score(piece, sq, state.side);
        if (is_white(piece)) {
            score += val + pst;
        } else {
            score -= val + pst;
        }
    }
    if (!eg) {
        int w_pawn = pawn_structure_bonus(state.board, Color::White);
        int b_pawn = pawn_structure_bonus(state.board, Color::Black);
        score += w_pawn - b_pawn;
        int w_king = king_safety(state.board, Color::White);
        int b_king = king_safety(state.board, Color::Black);
        score += w_king - b_king;
    }
    return score;
}

int evaluate(const State& state) {
    int phase = game_phase(state.board);
    int mg = evaluate_mg_eg(state, false);
    int eg = evaluate_mg_eg(state, true);
    int score = (mg * phase + eg * (24 - phase)) / 24;
    return score;
}

int relative_eval(const State& state) {
    int value = evaluate(state);
    return state.side == Color::White ? value : -value;
}

}
