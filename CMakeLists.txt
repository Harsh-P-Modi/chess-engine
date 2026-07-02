cmake_minimum_required(VERSION 3.10)
project(chess_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(chess_engine_v2
    src/main.cpp
    src/core/board.cpp
    src/core/zobrist.cpp
    src/movegen/movegen.cpp
    src/evaluation/evaluation.cpp
    src/search/search.cpp
    src/search/tt.cpp
    src/uci/uci.cpp
)

target_include_directories(chess_engine_v2 PRIVATE src)

target_compile_features(chess_engine_v2 PRIVATE cxx_std_17)
