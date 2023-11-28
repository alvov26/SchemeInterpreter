#include <iostream>
#include "scheme/scheme.h"

int main() {
    std::string line;
    Interpreter interpreter;
    while (getline(std::cin, line)) {
        try {
            std::cout << interpreter.Run(line) << std::endl;
        } catch (RuntimeError& e) {
            std::cerr << e.what() << std::endl;
        } catch (NameError& e) {
            std::cerr << e.what() << std::endl;
        }
    }
    return 0;
}
