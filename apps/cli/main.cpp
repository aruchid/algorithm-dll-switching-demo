#include "runtime.hpp"
#include <iostream>
int main(int argc, char** argv) {
    if (argc != 3) { std::cerr << "Usage: algorithm_demo <plugin-A> <plugin-B>\n"; return 2; }
    try {
        Runtime runtime;
        for (int i=1; i<=2; ++i) {
            runtime.load(argv[i]);
            std::cout << (i == 1 ? "A:" : "B:");
            for (float v : runtime.process({1,2,3})) std::cout << ' ' << v;
            std::cout << '\n';
        }
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
