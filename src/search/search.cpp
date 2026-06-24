#include "search.hpp"

#include <algorithm>
#include <limits>

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

static std::vector<Move> order_moves(const std::vector<Move>& moves) {
    std::vector<Move> ordered = moves;
    std::sort(ordered.begin(), ordered.end(), [](const Move& a, const Move& b) {
        int a_val = a.captured != '.' ? piece_value(a.captured) : 0;
        int b_val = b.captured != '.' ? piece_value(b.captured) : 0;
        return a_val > b_val;
    });
    return ordered;
}

static std::pair<int, std::optional<Move>> negamax(const State& state, int depth, int alpha, int beta, int64_t& nodes) {
    const int INF = std::numeric_limits<int>::max() / 4;
    const int MATE_SCORE = 100000;

    auto moves = legal_moves(state);
    if (moves.empty()) {
        if (is_in_check(state)) {
            return {-(MATE_SCORE + depth), std::nullopt};
        }
        return {0, std::nullopt};
    }

    if (depth == 0) {
        nodes += 1;
        return {relative_eval(state), std::nullopt};
    }

    int best_score = -INF;
    std::optional<Move> best_move;
    for (auto const& move : order_moves(moves)) {
        State next = make_move(state, move);
        auto [score, _] = negamax(next, depth - 1, -beta, -alpha, nodes);
        score = -score;
        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            break;
        }
    }

    return {best_score, best_move};
}

SearchResult search(const State& state, int depth) {
    int64_t nodes = 0;
    auto [score, best_move] = negamax(state, depth, std::numeric_limits<int>::min() / 2, std::numeric_limits<int>::max() / 2, nodes);
    SearchResult result;
    result.score = score;
    result.nodes = nodes;
    if (best_move.has_value()) {
        result.move = *best_move;
        result.has_move = true;
    }
    return result;
}

} // namespace chess
