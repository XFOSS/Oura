#pragma once
#include "interpreter.hpp"
#include <iostream>
#include <string>

namespace ouro {

inline void repl() {
    Interpreter interp;
    std::string line;
    std::cout << "OuroLang REPL (type 'exit' to quit)\n";
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        if (line == "exit") break;
        try {
            interp.run(line);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}

} // namespace ouro
