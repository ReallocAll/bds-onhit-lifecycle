#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace onhit {

// Block position of the start of a 16x16 chunk, including negative positions.
constexpr int chunk_start(int block) {
    return (block / 16 - (block % 16 < 0 ? 1 : 0)) * 16;
}

// Fixed-capacity Space-Saving sketch: counts/ranking are approximate after
// capacity exhaustion. No event history or unbounded map is retained.
template <std::size_t Capacity>
class HitChunks {
public:
    struct Entry {
        std::string dim;
        int x = 0, z = 0;
        std::uint64_t count = 0;
    };

    void record(std::string_view dimension, int x, int z) {
        for (std::size_t i = 0; i < used_; ++i) {
            auto& e = slots_[i];
            if (e.dim == dimension && e.x == x && e.z == z) {
                ++e.count;
                return;
            }
        }
        if (used_ < Capacity) {
            slots_[used_++] = {std::string(dimension), x, z, 1};
            return;
        }
        auto lowest = std::min_element(slots_.begin(), slots_.end(),
            [](const Entry& a, const Entry& b) { return a.count < b.count; });
        *lowest = {std::string(dimension), x, z, lowest->count + 1};
    }

    [[nodiscard]] std::vector<Entry> top(std::size_t limit) const {
        std::vector<Entry> entries(slots_.begin(), slots_.begin() + used_);
        std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
            if (a.count != b.count) return a.count > b.count;
            if (a.dim != b.dim) return a.dim < b.dim;
            if (a.x != b.x) return a.x < b.x;
            return a.z < b.z;
        });
        if (entries.size() > limit) entries.resize(limit);
        return entries;
    }
    [[nodiscard]] std::size_t size() const { return used_; }

private:
    std::array<Entry, Capacity> slots_{};
    std::size_t used_ = 0;
};
} // namespace onhit
