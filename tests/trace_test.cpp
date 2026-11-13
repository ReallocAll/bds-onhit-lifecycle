#include "trace.hpp"
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    onhit::Trace<128> trace;
    std::vector<std::thread> workers;
    for (int t = 0; t != 4; ++t) {
        workers.emplace_back([&, t] {
            for (int i = 0; i != 40; ++i) {
                onhit::Event e;
                e.record = static_cast<std::uintptr_t>(t + 1);
                e.child_count = 10;
                e.captured_children = 8;
                trace.push(e);
            }
        });
    }
    for (auto& worker : workers) worker.join();
    std::uint64_t count = 0;
    trace.each_quiescent([&](const onhit::Event& e) {
        assert(e.sequence == count++);
        assert(e.record >= 1 && e.record <= 4);
    });
    assert(count == 128);
    assert(trace.attempted() == 160);
    assert(trace.dropped() == 32);
    assert(trace.truncated_children() == 256);
    std::cout << "bounded concurrent trace checks passed\n";
}
