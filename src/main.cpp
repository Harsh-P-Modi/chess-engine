#include <iostream>
#include <string>

#include "core/board.hpp"
#include "movegen/movegen.hpp"
#include "search/search.hpp"
#include "uci/uci.hpp"

namespace chess {

void run_cli(int depth, Color human_color) {
    State state = initial_state();
    std::cout << "Simple chess engine -- material eval + alpha-beta search." << std::endl;
    std::cout << "You play " << (human_color == Color::White ? "White" : "Black") << ". Search depth: " << depth << " ply." << std::endl;
    std::cout << "Enter moves like e2e4 (promotion: e7e8q). quit to exit, moves to list legal moves." << std::endl;

    while (true) {
        print_board(state);
        auto moves = legal_moves(state);
        if (moves.empty()) {
            if (is_in_check(state)) {
                std::cout << "Checkmate! " << (state.side == Color::White ? "Black" : "White") << " wins." << std::endl;
            } else {
                std::cout << "Stalemate -- draw." << std::endl;
            }
            break;
        }

        if (state.side == human_color) {
            std::cout << "Your move (" << (state.side == Color::White ? 'w' : 'b') << " to play): ";
            std::string input;
            if (!std::getline(std::cin, input)) {
                break;
            }
            if (input.empty()) {
                continue;
            }
            if (input == "quit" || input == "exit") {
                break;
            }
            if (input == "moves") {
                for (size_t i = 0; i < moves.size(); ++i) {
                    std::cout << moves[i].uci();
                    if (i + 1 < moves.size()) {
                        std::cout << ", ";
                    }
                }
                std::cout << std::endl;
                continue;
            }
            auto move = parse_move_input(state, input);
            if (!move.has_value()) {
                std::cout << "Illegal or unrecognized move -- try again (e.g. e2e4)." << std::endl;
                continue;
            }
            state = make_move(state, *move);
        } else {
            std::cout << "Engine is thinking..." << std::endl;
            auto result = search(state, depth);
            if (!result.has_move) {
                break;
            }
            std::cout << "Engine plays: " << result.move.uci() << "   (eval " << (result.score / 100.0) << ", " << result.nodes << " leaves searched)" << std::endl;
            state = make_move(state, result.move);
        }
    }
}

} // namespace chess

int main(int argc, char** argv) {
    bool uci_mode = false;
    int depth = 3;
    chess::Color human_color = chess::Color::White;

    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 == "uci" || arg1 == "--uci") {
            uci_mode = true;
        } else {
            try {
                depth = std::stoi(arg1);
            } catch (...) {
                if (arg1 == "b" || arg1 == "black") {
                    human_color = chess::Color::Black;
                }
            }
        }
        if (argc > 2) {
            std::string arg2 = argv[2];
            if (arg2 == "b" || arg2 == "black") {
                human_color = chess::Color::Black;
            }
        }
    }

    if (uci_mode) {
        return chess::uci::run_uci();
    }

    chess::run_cli(depth, human_color);
    return 0;
}
