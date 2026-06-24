

## Project Structure


- `CMakeLists.txt` - Project build configuration.
- `src/` - C++ source tree.
  - `core/` - Board representation, state, and helper utilities.
  - `movegen/` - Move generation, legality checks, and move parsing.
  - `evaluation/` - Static material evaluation.
  - `search/` - Alpha-beta search engine.
  - `uci/` - UCI interface implementation.
- `build/` - Compiled binaries and build artifacts (ignored by Git).

## Features

- Full Chess rules support
  - castling
  - en passant
  - promotions
  - legal move generation
- Material-based static evaluation
- Negamax search with alpha-beta pruning
- UCI-compatible interface for GUI integration
- CLI mode for direct play

## Build Instructions

Requires a C++17 compiler. Example using `g++` on Windows with MSYS2/MinGW:

```sh
cd "c:\Users\harsh\Desktop\chess engine"
g++ -std=c++17 -I src src/main.cpp src/core/board.cpp src/movegen/movegen.cpp src/evaluation/evaluation.cpp src/search/search.cpp src/uci/uci.cpp -o build/chess_engine.exe
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

- The engine currently uses a simple evaluation and search strategy.
- It is intended for learning and basic play rather than competitive performance.
- Future improvements can include TT, quiescence, move ordering, and improved heuristics.
