#include "runtime.hpp"
#include <atomic>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
void require(bool ok) { if (!ok) throw std::runtime_error("Test assertion failed"); }
template<class F> void rejects(F f) {
    bool rejected = false;
    try { f(); } catch (const std::runtime_error&) { rejected = true; }
    require(rejected);
}
int main(int argc, char** argv) {
    try {
        require(argc == 10);
        Runtime r;
        rejects([&]{ r.process({1}); });
        rejects([&]{ r.load(std::filesystem::path(argv[1]).parent_path()/"not-a-plugin.missing"); });
        rejects([&]{ r.process({1}); });
        r.load(argv[1]); require(r.process({1,2,3}) == std::vector<float>({1,2,3}));
        r.load(argv[2]); require(r.process({1,2,3}) == std::vector<float>({2,4,6}));
        for (int i=3; i<argc; ++i) {
            rejects([&]{ r.load(argv[i]); });
            require(r.process({2})[0] == 4); // Failed candidate preserves B.
        }
        rejects([&]{ r.process({}); });
        rejects([&]{ r.process({std::numeric_limits<float>::quiet_NaN()}); });
        rejects([&]{ r.process({std::numeric_limits<float>::max()}); });
        require(r.process({2})[0] == 4);
        std::atomic<bool> ok{true};
        std::thread worker([&]{
            try { for (int i=0;i<300;++i) {
                auto v=r.process({1,2,3});
                if (v != std::vector<float>({1,2,3}) && v != std::vector<float>({2,4,6})) ok=false;
            }} catch (...) { ok=false; }
        });
        for (int i=0;i<100;++i) r.load(argv[1+i%2]);
        worker.join(); require(ok);
        r.release(); r.release();
        rejects([&]{ r.process({1}); });
        r.load(argv[1]); require(r.process({3})[0] == 3);
        std::cout << "PASS: ABI, rollback, input validation, repeated/concurrent switching, release/reload\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
