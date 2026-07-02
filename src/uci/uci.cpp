#include "uci/uci.hpp"
#include "core/board.hpp"
#include "movegen/movegen.hpp"
#include "search/search.hpp"
#include "search/tt.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>

namespace chess {
namespace uci {

static std::vector<std::string> split_words(const std::string& line) {
    std::istringstream iss(line);
    return std::vector<std::string>{std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>()};
}

static std::optional<State> parse_position(const std::vector<std::string>& tokens) {
    State state = initial_state();
    size_t index = 1;
    if (index >= tokens.size()) {
        return std::nullopt;
    }

    if (tokens[index] == "startpos") {
        state = initial_state();
        ++index;
    } else if (tokens[index] == "fen") {
        if (index + 6 > tokens.size()) {
            return std::nullopt;
        }
        std::string fen;
        for (size_t i = 0; i < 6; ++i) {
            if (i > 0) {
                fen.push_back(' ');
            }
            fen += tokens[index + 1 + i];
        }
        state = fen_to_state(fen);
        index += 7;
    }

    if (index < tokens.size() && tokens[index] == "moves") {
        ++index;
        for (; index < tokens.size(); ++index) {
            auto move = find_move(state, tokens[index]);
            if (!move.has_value()) {
                return std::nullopt;
            }
            state = make_move(state, *move);
        }
    }
    return state;
}

static void parse_go(const std::vector<std::string>& tokens, int& depth, int64_t& wtime, int64_t& btime, int64_t& winc, int64_t& binc, int64_t& movetime) {
    depth = 64;
    wtime = 0;
    btime = 0;
    winc = 0;
    binc = 0;
    movetime = 0;
    for (size_t i = 1; i + 1 < tokens.size(); ++i) {
        if (tokens[i] == "depth") {
            depth = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "wtime") {
            wtime = std::stoll(tokens[i + 1]);
        } else if (tokens[i] == "btime") {
            btime = std::stoll(tokens[i + 1]);
        } else if (tokens[i] == "winc") {
            winc = std::stoll(tokens[i + 1]);
        } else if (tokens[i] == "binc") {
            binc = std::stoll(tokens[i + 1]);
        } else if (tokens[i] == "movetime") {
            movetime = std::stoll(tokens[i + 1]);
        }
    }
}

int run_uci() {
    std::ofstream log("uci.log");
    State position = initial_state();
    int search_depth = 3;
    std::string line;

    while (std::getline(std::cin, line)) {
        log<<"Received: " << line << std::endl;
        if (line.empty()) {
            continue;
        }
        auto tokens = split_words(line);
        if (tokens.empty()) {
            continue;
        }

        const std::string& command = tokens[0];
        if (command == "uci") {
            std::cout << "id name ChessEngine" << std::endl;
            std::cout << "id author PortedByCopilot" << std::endl;
            std::cout << "uciok" << std::endl;
        } else if (command == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (command == "ucinewgame") {
            position = initial_state();
        } else if (command == "position") {
            auto maybe = parse_position(tokens);
            if (maybe.has_value()) {
                position = *maybe;
            }
        } else if (command == "go") {
            int depth = 64;
            int64_t wtime = 0, btime = 0, winc = 0, binc = 0, movetime = 0;
            parse_go(tokens, depth, wtime, btime, winc, binc, movetime);

            TranspositionTable tt(16);
            TimeState time_state;
            time_state.start_time = std::chrono::steady_clock::now();
            time_state.end_time = std::chrono::steady_clock::time_point::max();
            time_state.max_nodes = 0;

            if (movetime > 0) {
                time_state.max_nodes = 100000000;
                time_state.end_time = time_state.start_time + std::chrono::milliseconds(movetime);
            } else if (position.side == Color::White && wtime > 0) {
                time_state.end_time = time_state.start_time + std::chrono::milliseconds(wtime / 20);
                time_state.max_nodes = 100000000;
            } else if (position.side == Color::Black && btime > 0) {
                time_state.end_time = time_state.start_time + std::chrono::milliseconds(btime / 20);
                time_state.max_nodes = 100000000;
            }

            SearchResult best_result;
            bool found = false;
            for (int d = 1; d <= depth; ++d) {
                if (should_stop(time_state)) {
                    break;
                }
                auto result = search(position, d, tt, time_state);
                if (result.has_move) {
                    best_result = result;
                    found = true;
                }
            }

            if (found && best_result.has_move) {
                std::cout << "bestmove " << best_result.move.uci() << std::endl;
            } else {
                std::cout << "bestmove 0000" << std::endl;
            }
        } else if (command == "quit") {
            return 0;
        } else if (command == "setoption") {
            // no options supported yet.
        }
    }
    return 0;
}

} // namespace uci
} // namespace chess
