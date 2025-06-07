#pragma once
#include "interpreter.h"
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

#ifdef __cplusplus
extern "C" {
#endif
void ouro_repl();
#ifdef __cplusplus
}
#endif

} // namespace ouro
