#!/usr/bin/env python3
"""
A small but complete chess engine.

Features:
- Full board representation with castling rights / en passant / promotion
- Fully legal move generation (filters out moves that leave your own king in check)
- Material-based static evaluation using standard piece values
- Minimax search with alpha-beta pruning + simple capture-first move ordering
- A simple text CLI so you can play against it

Run:
    python3 chess_engine.py            # engine searches 3 ply deep
    python3 chess_engine.py 4          # engine searches 4 ply deep
    python3 chess_engine.py 3 b        # you play Black, engine plays White

Move format: coordinate notation, e.g. "e2e4". Promotions: "e7e8q".
Type "quit" to exit, "moves" to list your legal moves.
"""

import sys
import math

# --------------------------------------------------------------------------
# Board helpers
#
# Squares are indexed 0-63 as  index = rank*8 + file
#   file 0..7 = a..h,  rank 0..7 = rank "1".."8"
# Pieces are single characters: PNBRQK = white, pnbrqk = black, '.' = empty
# --------------------------------------------------------------------------

FILES = "abcdefgh"

PIECE_VALUES = {"P": 100, "N": 320, "B": 330, "R": 500, "Q": 900, "K": 20000}

KNIGHT_OFFSETS = [(1, 2), (2, 1), (2, -1), (1, -2), (-1, -2), (-2, -1), (-2, 1), (-1, 2)]
KING_OFFSETS = [(1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1), (0, -1), (1, -1)]
BISHOP_DIRS = [(1, 1), (1, -1), (-1, 1), (-1, -1)]
ROOK_DIRS = [(1, 0), (-1, 0), (0, 1), (0, -1)]
QUEEN_DIRS = BISHOP_DIRS + ROOK_DIRS


def sq(file, rank):
    return rank * 8 + file


def file_of(s):
    return s % 8


def rank_of(s):
    return s // 8


def on_board(file, rank):
    return 0 <= file < 8 and 0 <= rank < 8


def alg(s):
    return FILES[file_of(s)] + str(rank_of(s) + 1)


def from_alg(a):
    f = FILES.index(a[0])
    r = int(a[1]) - 1
    return sq(f, r)


def is_white(p):
    return p.isupper()


def color_of(p):
    return "w" if p.isupper() else "b"


def opposite(color):
    return "b" if color == "w" else "w"


# --------------------------------------------------------------------------
# Game state
# --------------------------------------------------------------------------

class State:
    __slots__ = ("board", "side", "castling", "ep")

    def __init__(self, board, side, castling, ep):
        self.board = board
        self.side = side          # 'w' or 'b' : side to move
        self.castling = castling  # set of chars from 'KQkq' still available
        self.ep = ep              # en-passant target square, or None

    def copy(self):
        return State(self.board[:], self.side, set(self.castling), self.ep)


def fen_to_state(fen):
    """Parse a FEN string into a State (useful for tests / custom positions)."""
    parts = fen.split()
    rows = parts[0].split("/")
    board = ["."] * 64
    for i, row in enumerate(rows):
        rank = 7 - i
        f = 0
        for ch in row:
            if ch.isdigit():
                f += int(ch)
            else:
                board[sq(f, rank)] = ch
                f += 1
    side = parts[1] if len(parts) > 1 else "w"
    castling = set(parts[2]) if len(parts) > 2 and parts[2] != "-" else set()
    ep = from_alg(parts[3]) if len(parts) > 3 and parts[3] != "-" else None
    return State(board, side, castling, ep)


def initial_state():
    board = ["."] * 64
    back = ["R", "N", "B", "Q", "K", "B", "N", "R"]
    for f in range(8):
        board[sq(f, 0)] = back[f]
        board[sq(f, 1)] = "P"
        board[sq(f, 6)] = "p"
        board[sq(f, 7)] = back[f].lower()
    return State(board, "w", set("KQkq"), None)


# --------------------------------------------------------------------------
# Moves
# --------------------------------------------------------------------------

class Move:
    __slots__ = ("frm", "to", "piece", "captured", "promo", "is_castle", "is_ep", "is_double")

    def __init__(self, frm, to, piece, captured=None, promo=None,
                 is_castle=None, is_ep=False, is_double=False):
        self.frm = frm
        self.to = to
        self.piece = piece          # the moving piece, e.g. 'P' or 'n'
        self.captured = captured    # captured piece char, or None
        self.promo = promo          # 'Q'/'R'/'B'/'N' (always uppercase), or None
        self.is_castle = is_castle  # 'K' (kingside) / 'Q' (queenside) / None
        self.is_ep = is_ep
        self.is_double = is_double

    def uci(self):
        s = alg(self.frm) + alg(self.to)
        if self.promo:
            s += self.promo.lower()
        return s

    def __repr__(self):
        return self.uci()

    def __eq__(self, other):
        return (self.frm, self.to, self.promo) == (other.frm, other.to, other.promo)


# --------------------------------------------------------------------------
# Attack detection (used for check / castling-through-check tests)
# --------------------------------------------------------------------------

def attacked_by(board, target, by_color):
    """True if `target` square is attacked by any piece of `by_color`."""
    tf, tr = file_of(target), rank_of(target)

    # Pawns: a pawn of by_color attacking `target` sits one rank "behind" it.
    pr = tr - 1 if by_color == "w" else tr + 1
    for df in (-1, 1):
        f = tf + df
        if on_board(f, pr):
            p = board[sq(f, pr)]
            if p != "." and color_of(p) == by_color and p.upper() == "P":
                return True

    for df, dr in KNIGHT_OFFSETS:
        f, r = tf + df, tr + dr
        if on_board(f, r):
            p = board[sq(f, r)]
            if p != "." and color_of(p) == by_color and p.upper() == "N":
                return True

    for df, dr in KING_OFFSETS:
        f, r = tf + df, tr + dr
        if on_board(f, r):
            p = board[sq(f, r)]
            if p != "." and color_of(p) == by_color and p.upper() == "K":
                return True

    for df, dr in BISHOP_DIRS:
        f, r = tf + df, tr + dr
        while on_board(f, r):
            p = board[sq(f, r)]
            if p != ".":
                if color_of(p) == by_color and p.upper() in ("B", "Q"):
                    return True
                break
            f += df
            r += dr

    for df, dr in ROOK_DIRS:
        f, r = tf + df, tr + dr
        while on_board(f, r):
            p = board[sq(f, r)]
            if p != ".":
                if color_of(p) == by_color and p.upper() in ("R", "Q"):
                    return True
                break
            f += df
            r += dr

    return False


def find_king(board, color):
    k = "K" if color == "w" else "k"
    for i, p in enumerate(board):
        if p == k:
            return i
    return -1


def is_in_check(state):
    kpos = find_king(state.board, state.side)
    return attacked_by(state.board, kpos, opposite(state.side))


# --------------------------------------------------------------------------
# Pseudo-legal move generation
# --------------------------------------------------------------------------

def gen_pawn_moves(state, s, f, r, moves):
    board = state.board
    p = board[s]
    side = state.side
    direction = 1 if side == "w" else -1
    start_rank = 1 if side == "w" else 6
    promo_rank = 7 if side == "w" else 0

    one_r = r + direction
    if on_board(f, one_r):
        t = sq(f, one_r)
        if board[t] == ".":
            if one_r == promo_rank:
                for promo in "QRBN":
                    moves.append(Move(s, t, p, promo=promo))
            else:
                moves.append(Move(s, t, p))
            if r == start_rank:
                two_r = r + 2 * direction
                t2 = sq(f, two_r)
                if board[t2] == ".":
                    moves.append(Move(s, t2, p, is_double=True))

    for df in (-1, 1):
        nf, nr = f + df, r + direction
        if on_board(nf, nr):
            t = sq(nf, nr)
            tp = board[t]
            if tp != "." and color_of(tp) != side:
                if nr == promo_rank:
                    for promo in "QRBN":
                        moves.append(Move(s, t, p, captured=tp, promo=promo))
                else:
                    moves.append(Move(s, t, p, captured=tp))
            elif state.ep is not None and t == state.ep:
                cap = "p" if side == "w" else "P"
                moves.append(Move(s, t, p, captured=cap, is_ep=True))


def gen_castle_moves(state, moves):
    board = state.board
    side = state.side
    opp = opposite(side)
    if side == "w":
        if ("K" in state.castling and board[5] == "." and board[6] == "."
                and board[7] == "R" and board[4] == "K"):
            if not any(attacked_by(board, x, opp) for x in (4, 5, 6)):
                moves.append(Move(4, 6, "K", is_castle="K"))
        if ("Q" in state.castling and board[1] == "." and board[2] == "."
                and board[3] == "." and board[0] == "R" and board[4] == "K"):
            if not any(attacked_by(board, x, opp) for x in (4, 3, 2)):
                moves.append(Move(4, 2, "K", is_castle="Q"))
    else:
        if ("k" in state.castling and board[61] == "." and board[62] == "."
                and board[63] == "r" and board[60] == "k"):
            if not any(attacked_by(board, x, opp) for x in (60, 61, 62)):
                moves.append(Move(60, 62, "k", is_castle="K"))
        if ("q" in state.castling and board[57] == "." and board[58] == "."
                and board[59] == "." and board[56] == "r" and board[60] == "k"):
            if not any(attacked_by(board, x, opp) for x in (60, 59, 58)):
                moves.append(Move(60, 58, "k", is_castle="Q"))


def gen_pseudo_moves(state):
    board = state.board
    side = state.side
    moves = []
    for s in range(64):
        p = board[s]
        if p == "." or color_of(p) != side:
            continue
        f, r = file_of(s), rank_of(s)
        pt = p.upper()

        if pt == "P":
            gen_pawn_moves(state, s, f, r, moves)

        elif pt == "N":
            for df, dr in KNIGHT_OFFSETS:
                nf, nr = f + df, r + dr
                if on_board(nf, nr):
                    t = sq(nf, nr)
                    tp = board[t]
                    if tp == "." or color_of(tp) != side:
                        moves.append(Move(s, t, p, captured=(tp if tp != "." else None)))

        elif pt == "K":
            for df, dr in KING_OFFSETS:
                nf, nr = f + df, r + dr
                if on_board(nf, nr):
                    t = sq(nf, nr)
                    tp = board[t]
                    if tp == "." or color_of(tp) != side:
                        moves.append(Move(s, t, p, captured=(tp if tp != "." else None)))

        elif pt in ("B", "R", "Q"):
            dirs = BISHOP_DIRS if pt == "B" else ROOK_DIRS if pt == "R" else QUEEN_DIRS
            for df, dr in dirs:
                nf, nr = f + df, r + dr
                while on_board(nf, nr):
                    t = sq(nf, nr)
                    tp = board[t]
                    if tp == ".":
                        moves.append(Move(s, t, p))
                    else:
                        if color_of(tp) != side:
                            moves.append(Move(s, t, p, captured=tp))
                        break
                    nf += df
                    nr += dr

    gen_castle_moves(state, moves)
    return moves


# --------------------------------------------------------------------------
# Make move / legal move filtering
# --------------------------------------------------------------------------

def make_move(state, move):
    ns = state.copy()
    board = ns.board
    side = state.side
    p = move.piece

    board[move.frm] = "."

    if move.is_ep:
        cap_sq = move.to + (-8 if side == "w" else 8)
        board[cap_sq] = "."

    placed = (move.promo if side == "w" else move.promo.lower()) if move.promo else p
    board[move.to] = placed

    if move.is_castle:
        if side == "w":
            if move.is_castle == "K":
                board[7] = "."
                board[5] = "R"
            else:
                board[0] = "."
                board[3] = "R"
        else:
            if move.is_castle == "K":
                board[63] = "."
                board[61] = "r"
            else:
                board[56] = "."
                board[59] = "r"

    if p.upper() == "K":
        if side == "w":
            ns.castling.discard("K")
            ns.castling.discard("Q")
        else:
            ns.castling.discard("k")
            ns.castling.discard("q")

    for corner, right in ((0, "Q"), (7, "K"), (56, "q"), (63, "k")):
        if move.frm == corner or move.to == corner:
            ns.castling.discard(right)

    ns.ep = (move.frm + move.to) // 2 if move.is_double else None
    ns.side = opposite(side)
    return ns


def legal_moves(state):
    legal = []
    for m in gen_pseudo_moves(state):
        ns = make_move(state, m)
        kpos = find_king(ns.board, state.side)
        if not attacked_by(ns.board, kpos, opposite(state.side)):
            legal.append(m)
    return legal


# --------------------------------------------------------------------------
# Evaluation (material only, using the standard piece values)
# --------------------------------------------------------------------------

def evaluate(state):
    """Static material evaluation from White's perspective."""
    score = 0
    for p in state.board:
        if p == ".":
            continue
        v = PIECE_VALUES[p.upper()]
        score += v if is_white(p) else -v
    return score


def relative_eval(state):
    e = evaluate(state)
    return e if state.side == "w" else -e


# --------------------------------------------------------------------------
# Search: minimax with alpha-beta pruning (negamax form)
# --------------------------------------------------------------------------

INF = math.inf
MATE_SCORE = 100000


def order_moves(moves):
    # Simple MVV ordering: try captures of the most valuable pieces first.
    def key(m):
        if m.captured:
            return -PIECE_VALUES[m.captured.upper()]
        return 0
    return sorted(moves, key=key)


def negamax(state, depth, alpha, beta, nodes):
    moves = legal_moves(state)

    if not moves:
        if is_in_check(state):
            # Being checkmated now is bad; prefer to delay it (or, for the
            # side delivering mate, prefer the fastest mate).
            return -(MATE_SCORE + depth), None
        return 0, None  # stalemate

    if depth == 0:
        nodes[0] += 1
        return relative_eval(state), None

    best_score = -INF
    best_move = None
    for m in order_moves(moves):
        ns = make_move(state, m)
        score, _ = negamax(ns, depth - 1, -beta, -alpha, nodes)
        score = -score
        if score > best_score:
            best_score = score
            best_move = m
        alpha = max(alpha, score)
        if alpha >= beta:
            break  # beta cutoff (alpha-beta pruning)

    return best_score, best_move


def search(state, depth):
    nodes = [0]
    score, move = negamax(state, depth, -INF, INF, nodes)
    return move, score, nodes[0]


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------

UNICODE_PIECES = {
    "P": "\u2659", "N": "\u2658", "B": "\u2657", "R": "\u2656", "Q": "\u2655", "K": "\u2654",
    "p": "\u265F", "n": "\u265E", "b": "\u265D", "r": "\u265C", "q": "\u265B", "k": "\u265A",
}


def print_board(state):
    print()
    for r in range(7, -1, -1):
        row = []
        for f in range(8):
            p = state.board[sq(f, r)]
            row.append(UNICODE_PIECES.get(p, "."))
        print(r + 1, " ".join(row))
    print("  a b c d e f g h")
    print()


def parse_move_input(state, s):
    s = s.strip().lower()
    if len(s) < 4:
        return None
    try:
        frm = from_alg(s[0:2])
        to = from_alg(s[2:4])
    except (ValueError, IndexError):
        return None
    promo = s[4].upper() if len(s) > 4 else None
    for m in legal_moves(state):
        if m.frm == frm and m.to == to:
            if m.promo:
                if (promo and m.promo == promo) or (not promo and m.promo == "Q"):
                    return m
            else:
                return m
    return None


def main():
    depth = 3
    human_color = "w"
    args = sys.argv[1:]
    if args:
        try:
            depth = int(args[0])
        except ValueError:
            pass
    if len(args) > 1 and args[1].lower() == "b":
        human_color = "b"

    state = initial_state()
    print("Simple chess engine -- material eval + alpha-beta search.")
    print(f"You play {'White' if human_color == 'w' else 'Black'}. Search depth: {depth} ply.")
    print('Enter moves like "e2e4" (promotion: "e7e8q"). "quit" to exit, "moves" to list legal moves.')

    while True:
        print_board(state)
        moves = legal_moves(state)
        if not moves:
            if is_in_check(state):
                winner = "Black" if state.side == "w" else "White"
                print(f"Checkmate! {winner} wins.")
            else:
                print("Stalemate -- draw.")
            break

        if state.side == human_color:
            s = input(f"Your move ({state.side} to play): ").strip()
            if s.lower() in ("quit", "exit"):
                break
            if s.lower() == "moves":
                print(", ".join(m.uci() for m in moves))
                continue
            m = parse_move_input(state, s)
            if m is None:
                print("Illegal or unrecognized move -- try again (e.g. e2e4).")
                continue
            state = make_move(state, m)
        else:
            print("Engine is thinking...")
            move, score, nodes = search(state, depth)
            print(f"Engine plays: {move.uci()}   (eval {score/100:+.2f}, {nodes} leaves searched)")
            state = make_move(state, move)


if __name__ == "__main__":
    main()