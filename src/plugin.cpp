#include <endstone/endstone.hpp>
#include <endstone/event/actor/actor_damage_event.h>

#include "identity.hpp"
#include "trace.hpp"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>

#ifdef ONHIT_ENABLE_EXPERIMENTAL_HOOKS
#include <funchook.h>
#endif

namespace {
constexpr std::uintptr_t kParseEA = 0x7143660;
constexpr std::uintptr_t kResetEA = 0x7143370;
constexpr std::uintptr_t kCleanupEA = 0x7757840;
constexpr std::size_t kTraceCapacity = 32768;

// Fixed allocation before any hook executes. Never I/O on the hooked path.
onhit::Trace<kTraceCapacity> g_trace;

#ifdef ONHIT_ENABLE_EXPERIMENTAL_HOOKS
using ParseFn = std::uint64_t (*)(std::uintptr_t, std::uintptr_t);
using ResetFn = std::int64_t (*)(std::uintptr_t);
using CleanupFn = std::int64_t (*)(std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
ParseFn g_parse = nullptr;
ResetFn g_reset = nullptr;
CleanupFn g_cleanup = nullptr;
funchook_t* g_funchook = nullptr;
bool g_installed = false;
bool g_fix_candidate = false;
bool g_fix_cleanup = false;
std::atomic<std::uint64_t> g_fix_reclaimed{0};
std::atomic<std::uint64_t> g_fix_nonempty{0};

// Isolated, opt-in repair hypothesis: parse or cleanup may abandon the
// raw-pointer vector. Use the child's BDS deleting
// destructor (Itanium vtable slot 1), not the plugin's C++ delete operator.
// This is intentionally limited to the parse boundary; no generic ownership
// tracker, allocator interception, or assumptions about other containers.
void reclaim_children(std::uintptr_t record) {
    if (!g_fix_candidate || !record) return;
    std::uintptr_t begin = 0, end = 0, capacity = 0;
    std::memcpy(&begin, reinterpret_cast<const void*>(record + 0x120), sizeof(begin));
    std::memcpy(&end, reinterpret_cast<const void*>(record + 0x128), sizeof(end));
    std::memcpy(&capacity, reinterpret_cast<const void*>(record + 0x130), sizeof(capacity));
    if (!begin || end < begin || capacity < end ||
        (begin % alignof(std::uintptr_t)) ||
        (end - begin) % sizeof(std::uintptr_t) ||
        (end - begin) / sizeof(std::uintptr_t) > 1024) return;
    auto* children = reinterpret_cast<const std::uintptr_t*>(begin);
    const auto count = (end - begin) / sizeof(std::uintptr_t);
    if (count) g_fix_nonempty.fetch_add(1, std::memory_order_relaxed);
    for (std::size_t i = 0; i < count; ++i) {
        const auto child = children[i];
        if (!child) continue;
        // An identical raw pointer in this vector must not be deleted twice.
        bool duplicate = false;
        for (std::size_t j = 0; j < i; ++j) {
            if (children[j] == child) { duplicate = true; break; }
        }
        if (duplicate) continue;
        const auto* vtable = *reinterpret_cast<const std::uintptr_t* const*>(child);
        using DeletingDestructor = void (*)(void*);
        auto destroy = reinterpret_cast<DeletingDestructor>(vtable[1]);
        destroy(reinterpret_cast<void*>(child));
        g_fix_reclaimed.fetch_add(1, std::memory_order_relaxed);
    }
}

std::uint64_t timestamp() noexcept {
    timespec time{};
    clock_gettime(CLOCK_MONOTONIC, &time);
    return std::uint64_t(time.tv_sec) * 1000000000ULL + std::uint64_t(time.tv_nsec);
}

// This code is compiled only with ONHIT_ENABLE_EXPERIMENTAL_HOOKS.
// The BDS pointer layout is unverified at runtime; use solely in isolated BDS.
void emit(onhit::EventKind kind, std::uintptr_t record, std::uintptr_t a2,
          std::uintptr_t a3, std::uintptr_t a4, std::uint64_t rax,
          std::uintptr_t caller) noexcept {
    const int prior_errno = errno;
    onhit::Event e{};
    e.kind = kind;
    e.timestamp_ns = timestamp();
    e.tid = static_cast<std::uint64_t>(syscall(SYS_gettid));
    e.caller = caller;
    e.record = record;
    e.args[0] = a2;
    e.args[1] = a3;
    e.args[2] = a4;
    e.raw_rax = rax;
    // Scalar-only instrumentation until record lifetime, layout and synchronization
    // are established against the exact target ELF. Never dereference an inferred
    // record or child vector on an untrusted hook path.
    g_trace.push(e);
    errno = prior_errno;
}

// These signatures are hypotheses from a bounded static ABI export, not proven contracts.
// Detours remain *potentially throwing*: never suppress a BDS exception.
std::uint64_t parse_detour(std::uintptr_t record, std::uintptr_t context) {
    const auto caller = reinterpret_cast<std::uintptr_t>(__builtin_return_address(0));
    emit(onhit::EventKind::ParseEnter, record, context, 0, 0, 0, caller);
    if (g_fix_candidate && !g_fix_cleanup) reclaim_children(record);
    const auto raw = g_parse(record, context);
    emit(onhit::EventKind::ParseExit, record, context, 0, 0, raw, caller);
    return raw;
}

std::int64_t reset_detour(std::uintptr_t record) {
    const auto caller = reinterpret_cast<std::uintptr_t>(__builtin_return_address(0));
    emit(onhit::EventKind::ResetEnter, record, 0, 0, 0, 0, caller);
    const auto raw = g_reset(record);
    emit(onhit::EventKind::ResetExit, record, 0, 0, 0, static_cast<std::uint64_t>(raw), caller);
    return raw;
}

std::int64_t cleanup_detour(std::uintptr_t record, std::uintptr_t a2,
                            std::uintptr_t a3, std::uintptr_t a4) {
    const auto caller = reinterpret_cast<std::uintptr_t>(__builtin_return_address(0));
    emit(onhit::EventKind::CleanupEnter, record, a2, a3, a4, 0, caller);
    if (g_fix_cleanup) reclaim_children(record);
    const auto raw = g_cleanup(record, a2, a3, a4);
    emit(onhit::EventKind::CleanupExit, record, a2, a3, a4,
         static_cast<std::uint64_t>(raw), caller);
    return raw;
}

bool install(std::uintptr_t base, bool full_lifecycle, std::string& error) {
    const auto parse = base + kParseEA;
    const auto reset = base + kResetEA;
    const auto cleanup = base + kCleanupEA;
    if (!onhit::executable_address(parse, 16) ||
        (full_lifecycle && (!onhit::executable_address(reset, 16) ||
                            !onhit::executable_address(cleanup, 16)))) {
        error = "requested hook entrypoint lies outside an executable ELF segment";
        return false;
    }
    // Parse-only remains the baseline. Both additional signatures are
    // hypotheses and may only be exercised by explicitly isolated CI.
    g_parse = reinterpret_cast<ParseFn>(parse);
    if (full_lifecycle) {
        g_reset = reinterpret_cast<ResetFn>(reset);
        g_cleanup = reinterpret_cast<CleanupFn>(cleanup);
    }
    g_funchook = funchook_create();
    if (!g_funchook) { error = "funchook_create failed"; return false; }
    if (funchook_prepare(g_funchook, reinterpret_cast<void**>(&g_parse),
                         reinterpret_cast<void*>(&parse_detour)) != FUNCHOOK_ERROR_SUCCESS ||
        (full_lifecycle &&
         (funchook_prepare(g_funchook, reinterpret_cast<void**>(&g_reset),
                           reinterpret_cast<void*>(&reset_detour)) != FUNCHOOK_ERROR_SUCCESS ||
          funchook_prepare(g_funchook, reinterpret_cast<void**>(&g_cleanup),
                           reinterpret_cast<void*>(&cleanup_detour)) != FUNCHOOK_ERROR_SUCCESS)) ||
        funchook_install(g_funchook, 0) != FUNCHOOK_ERROR_SUCCESS) {
        error = funchook_error_message(g_funchook);
        funchook_destroy(g_funchook);
        g_funchook = nullptr;
        return false;
    }
    g_installed = true;
    return true;
}

void uninstall() noexcept {
    if (!g_funchook) return;
    if (g_installed) funchook_uninstall(g_funchook, 0);
    funchook_destroy(g_funchook);
    g_funchook = nullptr;
    g_installed = false;
}
#endif

// ENDSTONE_PLUGIN generates a subclass implementing getDescription().
class Plugin : public endstone::Plugin {
public:
    void onLoad() override {
        const auto identity = onhit::inspect_executable();
        if (!identity.matched) {
            getLogger().warning("OnHit diagnostics inactive: {} (SHA-256={})",
                                identity.error, identity.sha256);
            return;
        }
        identity_verified_ = true;
        getLogger().info("Exact BDS image verified; load bias=0x{:x}", identity.load_bias);
#ifndef ONHIT_ENABLE_EXPERIMENTAL_HOOKS
        getLogger().warning("Diagnostics passive only; hooks were excluded at build time.");
#else
        const char* consent = std::getenv("BDS_ONHIT_ISOLATED_TEST");
        if (!consent || std::string_view(consent) != "I_CONFIRM_ISOLATED_BDS") {
            getLogger().warning("Detours inactive: isolated-test environment opt-in absent.");
            return;
        }
        const char* scope = std::getenv("BDS_ONHIT_HOOK_SCOPE");
        const bool full = scope && std::string_view(scope) == "all";
        std::string error;
        if (!install(identity.load_bias, full, error)) {
            getLogger().error("Diagnostic detour installation failed: {}", error);
            return;
        }
        active_ = true;
        const char* fix = std::getenv("BDS_ONHIT_FIX_CANDIDATE");
        g_fix_cleanup = full && fix &&
            std::string_view(fix) == "CLEANUP_RECLAIM";
        g_fix_candidate = full && fix &&
            (std::string_view(fix) == "PARSE_RECLAIM" || g_fix_cleanup);
        if (g_fix_candidate) {
            getLogger().warning("ONHIT_FIX_CANDIDATE_ENABLED {}_reclaim",
                                g_fix_cleanup ? "cleanup" : "parse");
        }
        getLogger().warning(
            "EXPERIMENTAL OnHit {} hook active in isolated BDS. Never use on production.",
            full ? "lifecycle" : "parse-only");
#endif
    }

    // Independent gameplay oracle: verifies an actual projectile damaged the
    // Survival SimulatedPlayer. Uses Endstone's public event API, not a detour.
    void onEnable() override {
        const char* consent = std::getenv("BDS_ONHIT_CONTACT_PROBE");
        if (!identity_verified_ || !consent ||
            std::string_view(consent) != "I_CONFIRM_ISOLATED_BDS") return;
        registerEvent(&Plugin::onActorDamage, *this);
        getLogger().info("ONHIT_CONTACT_PROBE_READY");
    }

    void onActorDamage(endstone::ActorDamageEvent& event) {
        if (event.getActor().getName() != "OnHitTarget") return;
        const auto& source = event.getDamageSource();
        const auto* direct = source.getDamagingActor();
        getLogger().info("ONHIT_CONTACT receiver={} source={} direct={} damage={}",
                         event.getActor().getName(), source.getType(),
                         direct ? direct->getType() : "none", event.getDamage());
    }

    void onDisable() override {
#ifdef ONHIT_ENABLE_EXPERIMENTAL_HOOKS
        if (!active_) return;
        uninstall();
        active_ = false;
        getLogger().info("ONHIT_FIX_CANDIDATE_RESULT enabled={} site={} nonempty={} reclaimed={}",
                         g_fix_candidate, g_fix_cleanup ? "cleanup" : "parse",
                         g_fix_nonempty.load(std::memory_order_relaxed),
                         g_fix_reclaimed.load(std::memory_order_relaxed));
        const auto path = getDataFolder() / "trace.tsv";
        std::error_code ignored;
        std::filesystem::create_directories(path.parent_path(), ignored);
        std::ofstream out(path, std::ios::trunc);
        if (!out) { getLogger().error("Could not open trace file: {}", path.string()); return; }
        out << "#diagnostic_only=1;target_sha256=" << onhit::kExpectedSha256 << '\n';
        out << "#fix_candidate=" << g_fix_candidate << ";site="
            << (g_fix_cleanup ? "cleanup" : "parse") << ";nonempty="
            << g_fix_nonempty.load(std::memory_order_relaxed) << ";reclaimed="
            << g_fix_reclaimed.load(std::memory_order_relaxed) << '\n';
        out << "#attempted=" << g_trace.attempted() << ";dropped=" << g_trace.dropped()
            << ";truncated_child_pointers=" << g_trace.truncated_children() << '\n';
        out << "sequence\tkind\tmonotonic_ns\ttid\tcaller\trecord\ta2\ta3\ta4\trax\tbegin\tend\tcapacity\tcount\tvalid\tchildren_first8\n";
        g_trace.each_quiescent([&](const onhit::Event& e) {
            out << e.sequence << '\t' << static_cast<unsigned>(e.kind) << '\t'
                << e.timestamp_ns << '\t' << e.tid << '\t'
                << e.caller << '\t' << e.record << '\t' << e.args[0] << '\t'
                << e.args[1] << '\t' << e.args[2] << '\t' << e.raw_rax << '\t'
                << e.vector_begin << '\t' << e.vector_end << '\t'
                << e.vector_capacity << '\t' << e.child_count << '\t'
                << e.vector_valid << '\t';
            for (std::uint32_t i = 0; i < e.captured_children; ++i) {
                if (i) out << ',';
                out << e.children[i];
            }
            out << '\n';
        });
        getLogger().info("Experimental OnHit trace written: {} (dropped={})",
                         path.string(), g_trace.dropped());
#endif
    }

private:
    bool identity_verified_ = false;
#ifdef ONHIT_ENABLE_EXPERIMENTAL_HOOKS
    bool active_ = false;
#endif
};
} // namespace

ENDSTONE_PLUGIN("onhit_lifecycle", "0.1.0", Plugin) {
    description = "Experimental, evidence-gated BDS OnHit lifecycle diagnostics";
    authors = {"ReallocAll"};
    load = endstone::PluginLoadOrder::Startup;
}
