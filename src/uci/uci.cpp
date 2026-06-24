#include "uci/uci.hpp"
#include "core/board.hpp"
#include "movegen/movegen.hpp"
#include "search/search.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

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

int run_uci() {
    State position = initial_state();
    int search_depth = 3;
    std::string line;

    while (std::getline(std::cin, line)) {
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
            int depth = search_depth;
            for (size_t i = 1; i + 1 < tokens.size(); ++i) {
                if (tokens[i] == "depth") {
                    depth = std::stoi(tokens[i + 1]);
                }
            }
            auto result = search(position, depth);
            if (result.has_move) {
                std::cout << "bestmove " << result.move.uci() << std::endl;
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
