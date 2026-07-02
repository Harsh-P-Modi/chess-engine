#include "core/zobrist.hpp"
#include "core/board.hpp"
#include "core/constants.hpp"

#include <random>

namespace chess {

namespace {
    constexpr std::mt19937_64::result_type FIXED_SEED = 7654321ULL;
    std::mt19937_64 rng(FIXED_SEED);

    enum PieceIndex : int {
        P, N, B, R, Q, K,
        p, n, b, r, q, k,
        PIECE_COUNT
    };

    inline int piece_index(char piece) {
        switch (piece) {
            case 'P': return P;
            case 'N': return N;
            case 'B': return B;
            case 'R': return R;
            case 'Q': return Q;
            case 'K': return K;
            case 'p': return p;
            case 'n': return n;
            case 'b': return b;
            case 'r': return r;
            case 'q': return q;
            case 'k': return k;
            default: return -1;
        }
    }

    uint64_t zobrist_piece[PIECE_COUNT][64];
    uint64_t zobrist_castle[16];
    uint64_t zobrist_ep[8];
    uint64_t zobrist_side;
    bool initialized = false;

    void init_zobrist() {
        if (initialized) return;
        for (int p = 0; p < PIECE_COUNT; ++p) {
            for (int s = 0; s < 64; ++s) {
                zobrist_piece[p][s] = rng();
            }
        }
        for (int i = 0; i < 16; ++i) {
            zobrist_castle[i] = rng();
        }
        for (int f = 0; f < 8; ++f) {
            zobrist_ep[f] = rng();
        }
        zobrist_side = rng();
        initialized = true;
    }
}

void ensure_zobrist_initialized() {
    if (!initialized) {
        init_zobrist();
    }
}

uint64_t compute_hash(const State& state) {
    ensure_zobrist_initialized();
    uint64_t h = 0;

    for (int sq = 0; sq < 64; ++sq) {
        char piece = state.board[sq];
        if (piece != '.') {
            int idx = piece_index(piece);
            if (idx >= 0) {
                h ^= zobrist_piece[idx][sq];
            }
        }
    }

    h ^= zobrist_castle[state.castling & 15];

    if (state.ep.has_value()) {
        int ep_file = static_cast<int>(file_of(*state.ep));
        h ^= zobrist_ep[ep_file];
    }

    if (state.side == Color::Black) {
        h ^= zobrist_side;
    }

    return h;
}

uint64_t compute_hash(const State& state, const Move& move) {
    ensure_zobrist_initialized();
    uint64_t h = state.hash;

    int from_idx = piece_index(move.piece);
    if (from_idx >= 0) {
        h ^= zobrist_piece[from_idx][move.from];
    }

    char placed = move.promo != '.' ? (state.side == Color::White ? move.promo : static_cast<char>(std::tolower(move.promo))) : move.piece;
    int to_idx = piece_index(placed);
    if (to_idx >= 0) {
        h ^= zobrist_piece[to_idx][move.to];
    }

    if (move.captured != '.') {
        int cap_idx = piece_index(move.captured);
        if (cap_idx >= 0) {
            h ^= zobrist_piece[cap_idx][move.to];
        }
    }

    if (move.is_ep) {
        int cap_square = static_cast<int>(move.to) + (state.side == Color::White ? -8 : 8);
        char cap_piece = state.side == Color::White ? 'p' : 'P';
        int cap_idx = piece_index(cap_piece);
        if (cap_idx >= 0) {
            h ^= zobrist_piece[cap_idx][cap_square];
        }
    }

    h ^= zobrist_castle[state.castling & 15];

    uint8_t new_castling = state.castling;
    if (std::toupper(move.piece) == 'K') {
        if (state.side == Color::White) {
            new_castling &= static_cast<uint8_t>(~(CASTLE_K | CASTLE_Q));
        } else {
            new_castling &= static_cast<uint8_t>(~(CASTLE_k | CASTLE_q));
        }
    }
    if (move.from == 0 || move.to == 0) new_castling &= static_cast<uint8_t>(~CASTLE_Q);
    if (move.from == 7 || move.to == 7) new_castling &= static_cast<uint8_t>(~CASTLE_K);
    if (move.from == 56 || move.to == 56) new_castling &= static_cast<uint8_t>(~CASTLE_q);
    if (move.from == 63 || move.to == 63) new_castling &= static_cast<uint8_t>(~CASTLE_k);

    h ^= zobrist_castle[new_castling & 15];

    if (state.ep.has_value()) {
        int old_ep_file = static_cast<int>(file_of(*state.ep));
        h ^= zobrist_ep[old_ep_file];
    }

    if (move.is_double) {
        int new_ep_file = static_cast<int>(file_of(move.from));
        h ^= zobrist_ep[new_ep_file];
    }

    h ^= zobrist_side;

    return h;
}

}
