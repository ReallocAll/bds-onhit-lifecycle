#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace onhit {

constexpr char kExpectedSha256[] = "ea16b541a448e07c337848d2c5f37321b706554b5216dd942f3f11e7a8ed2bad";
constexpr char kExpectedBuildId[] = "52576b75b6af5a09a6bde582be804e29657dbbf3";

struct Identity {
    bool matched = false;
    std::uintptr_t load_bias = 0;
    std::string sha256;
    std::string build_id;
    std::string error;
};

// Inspect the actual running executable, never a user-supplied file path.
Identity inspect_executable();

// Read-only range check against the executable's PT_LOAD executable segments.
bool executable_address(std::uintptr_t address, std::size_t length);

} // namespace onhit
