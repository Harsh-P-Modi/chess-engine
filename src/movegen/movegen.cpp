#include "movegen.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace chess {

static Move make_move_entry(uint8_t from, uint8_t to, char piece, char captured = '.', char promo = '.', bool is_castle = false, bool is_ep = false, bool is_double = false) {
    Move move;
    move.from = from;
    move.to = to;
    move.piece = piece;
    move.captured = captured;
    move.promo = promo;
    move.is_castle = is_castle;
    move.is_ep = is_ep;
    move.is_double = is_double;
    return move;
}

static void gen_pawn_moves(const State& state, uint8_t square, int file, int rank, std::vector<Move>& moves) {
    char piece = state.board[square];
    Color side = state.side;
    int direction = side == Color::White ? 1 : -1;
    int start_rank = side == Color::White ? 1 : 6;
    int promo_rank = side == Color::White ? 7 : 0;

    int one_rank = rank + direction;
    if (on_board(file, one_rank)) {
        uint8_t target = static_cast<uint8_t>(sq(file, one_rank));
        if (state.board[target] == '.') {
            if (one_rank == promo_rank) {
                for (char promo : {'Q', 'R', 'B', 'N'}) {
                    moves.push_back(make_move_entry(square, target, piece, '.', promo));
                }
            } else {
                moves.push_back(make_move_entry(square, target, piece));
            }
            if (rank == start_rank) {
                int two_rank = rank + 2 * direction;
                if (on_board(file, two_rank)) {
                    uint8_t target2 = static_cast<uint8_t>(sq(file, two_rank));
                    if (state.board[target2] == '.') {
                        moves.push_back(make_move_entry(square, target2, piece, '.', '.', false, false, true));
                    }
                }
            }
        }
    }

    for (int df : {-1, 1}) {
        int capture_file = file + df;
        int capture_rank = rank + direction;
        if (!on_board(capture_file, capture_rank)) {
            continue;
        }
        uint8_t target = static_cast<uint8_t>(sq(capture_file, capture_rank));
        char captured = state.board[target];
        if (captured != '.' && color_of(captured) != side) {
            if (capture_rank == promo_rank) {
                for (char promo : {'Q', 'R', 'B', 'N'}) {
                    moves.push_back(make_move_entry(square, target, piece, captured, promo));
                }
            } else {
                moves.push_back(make_move_entry(square, target, piece, captured));
            }
        } else if (state.ep.has_value() && target == *state.ep) {
            char cap = side == Color::White ? 'p' : 'P';
            moves.push_back(make_move_entry(square, target, piece, cap, '.', false, true));
        }
    }
}

static void gen_castle_moves(const State& state, std::vector<Move>& moves) {
    const Board& board = state.board;
    Color side = state.side;
    Color opp = opposite(side);
    if (side == Color::White) {
        if ((state.castling & CASTLE_K) && board[5] == '.' && board[6] == '.' && board[7] == 'R' && board[4] == 'K') {
            if (!attacked_by(board, 4, opp) && !attacked_by(board, 5, opp) && !attacked_by(board, 6, opp)) {
                moves.push_back(make_move_entry(4, 6, 'K', '.', '.', true));
            }
        }
        if ((state.castling & CASTLE_Q) && board[1] == '.' && board[2] == '.' && board[3] == '.' && board[0] == 'R' && board[4] == 'K') {
            if (!attacked_by(board, 4, opp) && !attacked_by(board, 3, opp) && !attacked_by(board, 2, opp)) {
                moves.push_back(make_move_entry(4, 2, 'K', '.', '.', true));
            }
        }
    } else {
        if ((state.castling & CASTLE_k) && board[61] == '.' && board[62] == '.' && board[63] == 'r' && board[60] == 'k') {
            if (!attacked_by(board, 60, opp) && !attacked_by(board, 61, opp) && !attacked_by(board, 62, opp)) {
                moves.push_back(make_move_entry(60, 62, 'k', '.', '.', true));
            }
        }
        if ((state.castling & CASTLE_q) && board[57] == '.' && board[58] == '.' && board[59] == '.' && board[56] == 'r' && board[60] == 'k') {
            if (!attacked_by(board, 60, opp) && !attacked_by(board, 59, opp) && !attacked_by(board, 58, opp)) {
                moves.push_back(make_move_entry(60, 58, 'k', '.', '.', true));
            }
        }
    }
}

std::vector<Move> gen_pseudo_moves(const State& state) {
    std::vector<Move> moves;
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
            gen_pawn_moves(state, square, file, rank, moves);
        } else if (pt == 'N') {
            for (auto [df, dr] : KNIGHT_OFFSETS) {
                int nf = file + df;
                int nr = rank + dr;
                if (!on_board(nf, nr)) {
                    continue;
                }
                uint8_t target = static_cast<uint8_t>(sq(nf, nr));
                char captured = board[target];
                if (captured == '.' || color_of(captured) != side) {
                    moves.push_back(make_move_entry(square, target, piece, captured == '.' ? '.' : captured));
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
                if (captured == '.' || color_of(captured) != side) {
                    moves.push_back(make_move_entry(square, target, piece, captured == '.' ? '.' : captured));
                }
            }
        } else if (pt == 'B' || pt == 'R' || pt == 'Q') {
            auto append_sliding = [&](int df, int dr) {
                int nf = file + df;
                int nr = rank + dr;
                while (on_board(nf, nr)) {
                    uint8_t target = static_cast<uint8_t>(sq(nf, nr));
                    char captured = board[target];
                    if (captured == '.') {
                        moves.push_back(make_move_entry(square, target, piece));
                    } else {
                        if (color_of(captured) != side) {
                            moves.push_back(make_move_entry(square, target, piece, captured));
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

    gen_castle_moves(state, moves);
    return moves;
}

State make_move(const State& state, const Move& move) {
    State next = state.copy();
    Board& board = next.board;
    Color side = state.side;

    board[move.from] = '.';
    if (move.is_ep) {
        int cap_square = static_cast<int>(move.to) + (side == Color::White ? -8 : 8);
        board[cap_square] = '.';
    }

    char placed = move.promo != '.' ? (side == Color::White ? move.promo : static_cast<char>(std::tolower(move.promo))) : move.piece;
    board[move.to] = placed;

    if (move.is_castle) {
        if (side == Color::White) {
            if (move.to == 6) {
                board[7] = '.';
                board[5] = 'R';
            } else if (move.to == 2) {
                board[0] = '.';
                board[3] = 'R';
            }
        } else {
            if (move.to == 62) {
                board[63] = '.';
                board[61] = 'r';
            } else if (move.to == 58) {
                board[56] = '.';
                board[59] = 'r';
            }
        }
    }

    if (std::toupper(move.piece) == 'K') {
        if (side == Color::White) {
            next.castling &= static_cast<uint8_t>(~(CASTLE_K | CASTLE_Q));
        } else {
            next.castling &= static_cast<uint8_t>(~(CASTLE_k | CASTLE_q));
        }
    }

    if (move.from == 0 || move.to == 0) {
        next.castling &= static_cast<uint8_t>(~CASTLE_Q);
    }
    if (move.from == 7 || move.to == 7) {
        next.castling &= static_cast<uint8_t>(~CASTLE_K);
    }
    if (move.from == 56 || move.to == 56) {
        next.castling &= static_cast<uint8_t>(~CASTLE_q);
    }
    if (move.from == 63 || move.to == 63) {
        next.castling &= static_cast<uint8_t>(~CASTLE_k);
    }

    if (move.is_double) {
        next.ep = static_cast<uint8_t>((move.from + move.to) / 2);
    } else {
        next.ep.reset();
    }

    next.side = opposite(side);
    return next;
}

std::vector<Move> legal_moves(const State& state) {
    std::vector<Move> legal;
    for (auto const& move : gen_pseudo_moves(state)) {
        State next = make_move(state, move);
        int king_pos = find_king(next.board, state.side);
        if (king_pos >= 0 && !attacked_by(next.board, static_cast<uint8_t>(king_pos), opposite(state.side))) {
            legal.push_back(move);
        }
    }
    return legal;
}

std::optional<Move> find_move(const State& state, std::string_view uci) {
    if (uci.size() < 4) {
        return std::nullopt;
    }
    for (auto const& move : legal_moves(state)) {
        if (move.uci() == uci) {
            return move;
        }
    }
    return std::nullopt;
}

std::optional<Move> parse_move_input(const State& state, std::string_view text) {
    std::string lower;
    lower.reserve(text.size());
    for (char ch : text) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    if (lower.size() < 4) {
        return std::nullopt;
    }
    std::string from_str(lower.substr(0, 2));
    std::string to_str(lower.substr(2, 2));
    char promo = '.';
    if (lower.size() > 4) {
        promo = static_cast<char>(std::toupper(lower[4]));
    }
    std::string uci_string = from_str + to_str;
    if (promo != '.') {
        uci_string.push_back(static_cast<char>(std::tolower(promo)));
    }

    for (auto const& move : legal_moves(state)) {
        if (move.from == from_algebraic(from_str) && move.to == from_algebraic(to_str)) {
            if (move.promo != '.') {
                if (promo == '.' && move.promo == 'Q') {
                    return move;
                }
                if (promo != '.' && move.promo == promo) {
                    return move;
                }
            } else {
                return move;
            }
        }
    }
    return std::nullopt;
}

} // namespace chess
