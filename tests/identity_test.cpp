#include "identity.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
    const auto identity = onhit::inspect_executable();
    // The standalone test executable is *not* the 265,500,280-byte BDS image.
    assert(!identity.matched);
    assert(identity.sha256.size() == 64);
    assert(identity.build_id.size() == 40);
    assert(identity.error.find("does not match") != std::string::npos);
    assert(onhit::executable_address(reinterpret_cast<std::uintptr_t>(&main), 1));
    assert(!onhit::executable_address(0, 16));
    std::cout << "identity negative-gate and executable-range checks passed\n";
}
