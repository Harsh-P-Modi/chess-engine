# Chess Engine Enhancement Plan

## Overview
Adding evaluation improvements, quiescence search, transposition tables, and iterative deepening to the existing alpha-beta search engine.

## Decisions
- Zobrist hashing: Use compile-time fixed seed `std::mt19937` with seed 7654321
- Transposition table: 16MB (~256K entries), depth stored as `uint8_t`
- Iterative deepening: Aspiration windows with soft/hard time bounds

## Implementation Details

### Zobrist Hashing
- 768 random numbers: 12 piece types × 64 squares = 768 values  
- Plus ~16 numbers for castling rights and en passant
- Hash XOR'd when making moves
- **Must save previous hash in State.copy() or history stack for unmake_move()**

### Transposition Table Entry
```cpp
struct TTEntry {
    uint64_t hash;        // 8 bytes
    int16_t score;        // 2 bytes
    uint8_t depth;        // 1 byte  
    uint8_t flag;         // 1 byte (0=exact, 1=upper, 2=lower)
    uint32_t best_move;   // 4 bytes (packed from/to piece captured promo flags)
    // Total: 16 bytes - verify with static_assert
};
```
// Pack Move to uint32_t: from(6) + to(6) + piece(4) + captured(4) + promo(4) + flags(3) = 27 bits

**CRITICAL NOTES:**
- **QS Check Evasions**: Must generate ALL legal evasion moves when in check, not just captures
- **Hash History**: Must save previous hash in move stack - cannot reliably "un-XOR" for en passant/castling
- **Node Polling**: Must check time every ~2048 nodes in search to avoid losing on time
- **Move Packing**: Use packed uint32_t representation (26 bits needed: from,to,piece,captured,promo,flags)

## Tasks

### Phase 1: Zobrist Hashing (`src/core/`)
1. Create `src/core/zobrist.hpp` - Declaration for hash functions
2. Create `src/core/zobrist.cpp` - Implementation with random 64-bit numbers
3. Modify `State` struct in `core/board.hpp` to include `uint64_t hash` field
4. Update `make_move()` in `movegen.cpp` to update Zobrist hash on each move
5. Add hash initialization in `initial_state()` and `fen_to_state()`

### Phase 2: Transposition Table (`src/search/`)
1. Create `src/search/tt.hpp` - TT entry struct, TT class
2. Create `src/search/tt.cpp` - TT storage (array of ~262K entries), probe/store methods
3. Integrate TT into search: probe at start, store before return

### Phase 3: Quiescence Search (`src/search/`)
1. Create `quiescence_search()` in search.cpp
2. Call quiescence when depth = 0 in negamax
3. Generate captures only - EXCEPT when in check (generate all legal evasions)
4. Add `gen_captures()` function or filter in search

### Phase 4: Move Ordering (`src/search/`)
1. Add MVV-LVA scoring for captures
2. Add killer move heuristic  
3. Track best move from TT for ordering
4. Pack Move into uint32_t (32 bits) for TTEntry = 16 bytes total: hash(8) + score(2) + depth(1) + flag(1) + move(4)

### Phase 5: Evaluation Improvements (`src/evaluation/`)
1. Create `src/evaluation/pst.hpp` - Piece-square table arrays
2. Add pawn structure detection (doubled, isolated pawns)
3. Add king safety (pawn shield) detection
4. Add tapered evaluation

### Phase 6: UCI Time Management (`src/uci/`)
1. Parse time controls from "go" command
2. Implement iterative deepening loop in `run_uci()`
3. Check time availability before each depth iteration
4. Poll time every ~2048 nodes inside search to abort on time pressure
5. Return best move when time expires

## Validation Steps
1. Build: `cmake --build build`
2. Verify: `static_assert(sizeof(TTEntry) == 16)` passes
3. Test: Basic positions (mate in 1, stalemate, material)
4. Test: QS with check evasions generates legal moves
5. Test: UCI time management doesn't lose on time in tactical positions