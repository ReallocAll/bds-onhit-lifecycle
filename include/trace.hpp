#pragma once

#include <array>
#include <atomic>
#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace onhit {

enum class EventKind : std::uint32_t {
    ParseEnter = 1, ParseExit, ResetEnter, ResetExit, CleanupEnter, CleanupExit,
    DeleteWrapperEnter, DeleteWrapperExit
};

constexpr std::size_t kChildrenPerSnapshot = 8;
struct Event {
    std::uint64_t sequence = 0;
    std::uint64_t timestamp_ns = 0;
    std::uint64_t tid = 0;
    std::uintptr_t caller = 0;
    std::uintptr_t record = 0;
    std::uintptr_t args[3]{};
    std::uint64_t raw_rax = 0;
    std::uintptr_t vector_begin = 0;
    std::uintptr_t vector_end = 0;
    std::uintptr_t vector_capacity = 0;
    std::uintptr_t children[kChildrenPerSnapshot]{};
    std::uint32_t child_count = 0;
    std::uint32_t captured_children = 0;
    EventKind kind = EventKind::ParseEnter;
    bool vector_valid = false;
};

// Append-only, bounded recording. No allocations, locks or I/O in push().
// Readers may iterate only once all producers are quiescent.
template <std::size_t Capacity>
class Trace {
    struct Slot {
        std::atomic<bool> published{false};
        Event event{};
    };
    std::array<Slot, Capacity> slots_{};
    std::atomic<std::uint64_t> next_{0};
    std::atomic<std::uint64_t> dropped_{0};
    std::atomic<std::uint64_t> truncated_children_{0};

public:
    static_assert(Capacity > 0);

    void push(Event event) noexcept {
        const auto index = next_.fetch_add(1, std::memory_order_relaxed);
        if (index >= Capacity) {
            dropped_.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        event.sequence = index;
        if (event.child_count > event.captured_children) {
            truncated_children_.fetch_add(event.child_count - event.captured_children,
                                          std::memory_order_relaxed);
        }
        slots_[index].event = event;
        slots_[index].published.store(true, std::memory_order_release);
    }

    std::uint64_t attempted() const noexcept { return next_.load(std::memory_order_acquire); }
    std::uint64_t dropped() const noexcept { return dropped_.load(std::memory_order_acquire); }
    std::uint64_t truncated_children() const noexcept {
        return truncated_children_.load(std::memory_order_acquire);
    }

    template <typename Sink>
    void each_quiescent(Sink&& sink) const {
        const auto limit = std::min<std::uint64_t>(attempted(), Capacity);
        for (std::uint64_t i = 0; i < limit; ++i) {
            if (slots_[i].published.load(std::memory_order_acquire)) sink(slots_[i].event);
        }
    }
};

} // namespace onhit
