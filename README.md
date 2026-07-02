

## Project Structure


- `CMakeLists.txt` - Project build configuration.
- `src/` - C++ source tree.
  - `core/` - Board representation, state, helpers, Zobrist hashing.
  - `movegen/` - Move generation, legality checks, and move parsing.
  - `evaluation/` - Static evaluation with PSTs, pawn structure, king safety, and tapered MG/EG evaluation.
  - `search/` - Transposition table, quiescence search, move ordering, and negamax search.
  - `uci/` - UCI interface with iterative deepening and time management.
- `build/` - Compiled binaries and build artifacts (ignored by Git).

## Features

- Full Chess rules support
  - castling
  - en passant
  - promotions
  - legal move generation
- Zobrist hashing with incremental state hash
- Transposition table (~16MB)
- Quiescence search with MVV-LVA move ordering
- Piece-square tables (MG/EG)
- Pawn structure evaluation (doubled, isolated)
- King safety evaluation
- Tapered evaluation (midgame/endgame phase interpolation)
- Negamax search with alpha-beta pruning
- Iterative deepening with aspiration windows
- UCI time management (`wtime`, `btime`, `winc`, `binc`, `movetime`)
- Node polling (~2048 nodes) for time abort
- UCI-compatible interface for GUI integration
- CLI mode for direct play

## Build Instructions

Requires a C++17 compiler. Example using `g++` on Windows with MSYS2/MinGW:

```sh
cd "c:\Users\harsh\Desktop\chess engine"
g++ -std=c++17 -I src src/main.cpp src/core/board.cpp src/core/zobrist.cpp src/movegen/movegen.cpp src/evaluation/evaluation.cpp src/search/search.cpp src/search/tt.cpp src/uci/uci.cpp -o build/chess_engine.exe
```

If using CMake:

```sh
cd "c:\Users\harsh\Desktop\chess engine"
cmake -S . -B build -G "NMake Makefiles"
cmake --build build
```

> If your system has a different generator, use that instead.

## Run Instructions

### CLI mode

```sh
build/chess_engine.exe
```

### UCI mode

```sh
build/chess_engine.exe uci
```

Use a UCI-compatible GUI or engine tester to connect and send commands like `uci`, `isready`, `position`, and `go`.

## Notes

- The engine is intended for learning and basic play rather than competitive performance.
- Known limitations: no endgame tablebases, no pondering, no multi-PV, and no opening book.
