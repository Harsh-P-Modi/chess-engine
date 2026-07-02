#include "search.hpp"

#include "tt.hpp"

#include <algorithm>
#include <chrono>
#include <limits>

namespace chess {

bool should_stop(const TimeState& time_state) {
    if (time_state.stop_requested) {
        return true;
    }
    if (time_state.max_nodes > 0 && time_state.nodes >= time_state.max_nodes) {
        return true;
    }
    if (time_state.end_time > time_state.start_time) {
        if (std::chrono::steady_clock::now() >= time_state.end_time) {
            return true;
        }
    }
    return false;
}

static TimeState dummy_time_state = []() {
    TimeState t;
    t.start_time = std::chrono::steady_clock::now();
    t.end_time = std::chrono::steady_clock::time_point::max();
    t.max_nodes = 0;
    t.nodes = 0;
    return t;
}();

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

static std::vector<Move> order_moves(const std::vector<Move>& moves, uint32_t tt_move) {
    std::vector<Move> ordered = moves;
    std::sort(ordered.begin(), ordered.end(), [&](const Move& a, const Move& b) {
        uint32_t a_pack = a.pack();
        uint32_t b_pack = b.pack();
        bool a_tt = (tt_move != 0 && a_pack == tt_move);
        bool b_tt = (tt_move != 0 && b_pack == tt_move);
        if (a_tt != b_tt) return a_tt;
        int a_val = a.captured != '.' ? piece_value(a.captured) : 0;
        int b_val = b.captured != '.' ? piece_value(b.captured) : 0;
        return a_val > b_val;
    });
    return ordered;
}

std::vector<Move> gen_captures(const State& state) {
    std::vector<Move> captures;
    const Board& board = state.board;
    Color side = state.side;

    for (uint8_t square = 0; square < 64; ++square) {
        char piece = board[square];
        if (piece == '.' || color_of(piece) != side) {
            continue;
        }
        int file = file_of(square);
        int rank = rank_of(square);
        char pt = static_cast<char>(std::toupper(piece));

        if (pt == 'P') {
            int direction = side == Color::White ? 1 : -1;
            for (int df : {-1, 1}) {
                int capture_file = file + df;
                int capture_rank = rank + direction;
                if (!on_board(capture_file, capture_rank)) {
                    continue;
                }
                uint8_t target = static_cast<uint8_t>(sq(capture_file, capture_rank));
                char captured = board[target];
                if (captured != '.' && color_of(captured) != side) {
                    if (capture_rank == (side == Color::White ? 7 : 0)) {
                        for (char promo : {'Q', 'R', 'B', 'N'}) {
                            captures.push_back(Move::unpack(Move{square, target, piece, captured, promo}.pack()));
                        }
                    } else {
                        captures.push_back(Move::unpack(Move{square, target, piece, captured}.pack()));
                    }
                } else if (state.ep.has_value() && target == *state.ep) {
                    char cap = side == Color::White ? 'p' : 'P';
                    captures.push_back(Move::unpack(Move{square, target, piece, cap, '.', false, true}.pack()));
                }
            }
        } else if (pt == 'N') {
            for (auto [df, dr] : KNIGHT_OFFSETS) {
                int nf = file + df;
                int nr = rank + dr;
                if (!on_board(nf, nr)) {
                    continue;
                }
                uint8_t target = static_cast<uint8_t>(sq(nf, nr));
                char captured = board[target];
                if (captured != '.' && color_of(captured) != side) {
                    captures.push_back(Move::unpack(Move{square, target, piece, captured}.pack()));
                }
            }
        } else if (pt == 'K') {
            for (auto [df, dr] : KING_OFFSETS) {
                int nf = file + df;
                int nr = rank + dr;
                if (!on_board(nf, nr)) {
                    continue;
                }
                uint8_t target = static_cast<uint8_t>(sq(nf, nr));
                char captured = board[target];
                if (captured != '.' && color_of(captured) != side) {
                    captures.push_back(Move::unpack(Move{square, target, piece, captured}.pack()));
                }
            }
        } else if (pt == 'B' || pt == 'R' || pt == 'Q') {
            auto append_sliding = [&](int df, int dr) {
                int nf = file + df;
                int nr = rank + dr;
                while (on_board(nf, nr)) {
                    uint8_t target = static_cast<uint8_t>(sq(nf, nr));
                    char captured = board[target];
                    if (captured != '.') {
                        if (color_of(captured) != side) {
                            captures.push_back(Move::unpack(Move{square, target, piece, captured}.pack()));
                        }
                        break;
                    }
                    nf += df;
                    nr += dr;
                }
            };

            if (pt == 'B' || pt == 'Q') {
                for (auto [df, dr] : BISHOP_DIRS) {
                    append_sliding(df, dr);
                }
            }
            if (pt == 'R' || pt == 'Q') {
                for (auto [df, dr] : ROOK_DIRS) {
                    append_sliding(df, dr);
                }
            }
        }
    }

    return captures;
}

static int quiescence_search(const State& state, int alpha, int beta, int64_t& nodes) {
    const int INF = std::numeric_limits<int>::max() / 4;
    const int MATE_SCORE = 100000;

    int stand_pat = relative_eval(state);
    if (stand_pat >= beta) {
        return stand_pat;
    }
    if (alpha < stand_pat) {
        alpha = stand_pat;
    }

    std::vector<Move> candidates;
    if (is_in_check(state)) {
        candidates = legal_moves(state);
    } else {
        candidates = gen_captures(state);
    }

    std::sort(candidates.begin(), candidates.end(), [](const Move& a, const Move& b) {
        int a_victim = piece_value(a.captured);
        int b_victim = piece_value(b.captured);
        if (a_victim != b_victim) return a_victim > b_victim;
        int a_attacker = piece_value(a.piece);
        int b_attacker = piece_value(b.piece);
        return a_attacker < b_attacker;
    });

    for (auto const& move : candidates) {
        State next = make_move(state, move);
        nodes += 1;
        int score = -quiescence_search(next, -beta, -alpha, nodes);
        if (score >= beta) {
            return score;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

static std::pair<int, std::optional<Move>> negamax(const State& state, int depth, int alpha, int beta, int64_t& nodes, TranspositionTable& tt, TimeState& time_state) {
    const int INF = std::numeric_limits<int>::max() / 4;
    const int MATE_SCORE = 100000;

    auto tt_hit = tt.probe(state.hash, depth, alpha, beta);
    if (tt_hit.has_value()) {
        return {tt_hit->score, Move::unpack(tt_hit->best_move)};
    }

    auto moves = legal_moves(state);
    if (moves.empty()) {
        if (is_in_check(state)) {
            return {-(MATE_SCORE + depth), std::nullopt};
        }
        return {0, std::nullopt};
    }

    if (depth == 0) {
        nodes += 1;
        return {quiescence_search(state, alpha, beta, nodes), std::nullopt};
    }

    int best_score = -INF;
    std::optional<Move> best_move;
    uint32_t tt_best = tt_hit.has_value() ? tt_hit->best_move : 0;
    auto ordered = order_moves(moves, tt_best);
    for (auto const& move : ordered) {
        if ((nodes & 2047) == 0 && should_stop(time_state)) {
            break;
        }
        State next = make_move(state, move);
        auto [score, _] = negamax(next, depth - 1, -beta, -alpha, nodes, tt, time_state);
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

    int flag = FLAG_EXACT;
    if (best_score <= alpha) flag = FLAG_UPPER;
    else if (best_score >= beta) flag = FLAG_LOWER;
    tt.store(state.hash, depth, best_score, flag, best_move.has_value() ? best_move->pack() : 0);

    return {best_score, best_move};
}

SearchResult search(const State& state, int depth, TranspositionTable& tt) {
    int64_t nodes = 0;
    auto [score, best_move] = negamax(state, depth, std::numeric_limits<int>::min() / 2, std::numeric_limits<int>::max() / 2, nodes, tt, dummy_time_state);
    SearchResult result;
    result.score = score;
    result.nodes = nodes;
    if (best_move.has_value()) {
        result.move = *best_move;
        result.has_move = true;
    }
    return result;
}

SearchResult search(const State& state, int depth, TranspositionTable& tt, TimeState& time_state) {
    time_state.nodes = 0;
    time_state.start_time = std::chrono::steady_clock::now();
    if (time_state.end_time == std::chrono::steady_clock::time_point()) {
        time_state.end_time = std::chrono::steady_clock::time_point::max();
    }
    auto [score, best_move] = negamax(state, depth, std::numeric_limits<int>::min() / 2, std::numeric_limits<int>::max() / 2, time_state.nodes, tt, time_state);
    SearchResult result;
    result.score = score;
    result.nodes = time_state.nodes;
    if (best_move.has_value()) {
        result.move = *best_move;
        result.has_move = true;
    }
    return result;
}

}
