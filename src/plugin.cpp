#include <endstone/endstone.hpp>
#include <endstone/event/actor/actor_damage_event.h>
#include <endstone/level/dimension.h>
#include <endstone/command/command.h>
#include <endstone/command/command_sender.h>

#include "identity.hpp"
#include "hits.hpp"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>

#include <funchook.h>

namespace {
constexpr std::uintptr_t kCleanupEA = 0x7757840;
// No verified per-child allocation size is yet available from the exact factory.
// Deliberately exclude unverified bytes rather than reinterpret heap_v2 samples.
constexpr std::uint64_t kVerifiedObjectBytes = 0;
using CleanupFn = std::int64_t (*)(std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);

CleanupFn g_cleanup = nullptr;
funchook_t* g_hook = nullptr;
std::atomic<std::uint64_t> g_deleted{0};
std::atomic<std::uint64_t> g_bytes{0};

void reclaim_children(std::uintptr_t record) {
    if (!record) return;
    std::uintptr_t begin = 0, end = 0, capacity = 0;
    std::memcpy(&begin, reinterpret_cast<const void*>(record + 0x120), sizeof(begin));
    std::memcpy(&end, reinterpret_cast<const void*>(record + 0x128), sizeof(end));
    std::memcpy(&capacity, reinterpret_cast<const void*>(record + 0x130), sizeof(capacity));
    if (!begin || end < begin || capacity < end ||
        (begin % alignof(std::uintptr_t)) ||
        (end - begin) % sizeof(std::uintptr_t) ||
        (end - begin) / sizeof(std::uintptr_t) > 1024) return;

    const auto* children = reinterpret_cast<const std::uintptr_t*>(begin);
    const auto count = (end - begin) / sizeof(std::uintptr_t);
    for (std::size_t i = 0; i < count; ++i) {
        const auto child = children[i];
        if (!child) continue;
        // Preserve the tested within-record deduplication. This does not
        // establish unique ownership across distinct cleanup records.
        bool duplicate = false;
        for (std::size_t j = 0; j < i; ++j) {
            if (children[j] == child) { duplicate = true; break; }
        }
        if (duplicate) continue;
        const auto* vtable = *reinterpret_cast<const std::uintptr_t* const*>(child);
        using DeletingDestructor = void (*)(void*);
        auto destroy = reinterpret_cast<DeletingDestructor>(vtable[1]);
        destroy(reinterpret_cast<void*>(child));
        g_deleted.fetch_add(1, std::memory_order_relaxed);
        g_bytes.fetch_add(kVerifiedObjectBytes, std::memory_order_relaxed);
    }
}

std::int64_t cleanup_detour(std::uintptr_t record, std::uintptr_t a2,
                            std::uintptr_t a3, std::uintptr_t a4) {
    reclaim_children(record);
    return g_cleanup(record, a2, a3, a4);
}

bool install(std::uintptr_t base, std::string& error) {
    const auto address = base + kCleanupEA;
    if (!onhit::executable_address(address, 16)) {
        error = "Cleanup entry outside executable ELF segment";
        return false;
    }
    g_cleanup = reinterpret_cast<CleanupFn>(address);
    g_hook = funchook_create();
    if (!g_hook) { error = "funchook_create failed"; return false; }
    if (funchook_prepare(g_hook, reinterpret_cast<void**>(&g_cleanup),
                         reinterpret_cast<void*>(&cleanup_detour)) != FUNCHOOK_ERROR_SUCCESS ||
        funchook_install(g_hook, 0) != FUNCHOOK_ERROR_SUCCESS) {
        error = funchook_error_message(g_hook);
        funchook_destroy(g_hook);
        g_hook = nullptr;
        return false;
    }
    return true;
}

class Plugin : public endstone::Plugin {
public:
    void onLoad() override {
        const auto identity = onhit::inspect_executable();
        if (!identity.matched) {
            getLogger().error("OnHitFix inactive: {}", identity.error);
            return;
        }
        std::string error;
        if (!install(identity.load_bias, error)) {
            getLogger().error("OnHitFix Cleanup hook failed: {}", error);
            return;
        }
        active_ = true;
        getLogger().info("OnHitFix Cleanup hook installed");
    }

    void onEnable() override {
        if (active_) registerEvent(&Plugin::onActorDamage, *this);
    }

    void onActorDamage(endstone::ActorDamageEvent& event) {
        if (event.getDamageSource().getType() != "projectile") return;
        const auto location = event.getActor().getLocation();
        const auto& dimension = location.getDimension();
        std::string name;
        switch (dimension.getType()) {
            case endstone::Dimension::Type::Overworld: name = "OW"; break;
            case endstone::Dimension::Type::Nether: name = "NE"; break;
            case endstone::Dimension::Type::TheEnd: name = "END"; break;
            default: name = dimension.getName(); break;
        }
        hits_.record(name, onhit::chunk_start(location.getBlockX()),
                     onhit::chunk_start(location.getBlockZ()));
    }

    bool onCommand(endstone::CommandSender& sender, const endstone::Command&,
                   const std::vector<std::string>& args) override {
        if (args.size() != 1) {
            sender.sendErrorMessage("Usage: /ohf <status|hit>");
            return true;
        }
        if (args[0] == "status") {
            sender.sendMessage("Obj del: {}\nObj mem: {:.2f} MiB",
                               g_deleted.load(std::memory_order_relaxed),
                               g_bytes.load(std::memory_order_relaxed) / 1048576.0);
            return true;
        }
        if (args[0] == "hit") {
            std::string output = "Top 20 Chunks\n #  Dim   X      Z      Hits";
            auto entries = hits_.top(20);
            for (std::size_t i = 0; i < entries.size(); ++i) {
                const auto& e = entries[i];
                output += std::format("\n{:>2}  {:<4} {:>6} {:>6} {:>7}",
                                      i + 1, e.dim, e.x, e.z, e.count);
            }
            sender.sendMessage(output);
            return true;
        }
        sender.sendErrorMessage("Usage: /ohf <status|hit>");
        return true;
    }

    void onDisable() override {
        if (!g_hook) return;
        funchook_uninstall(g_hook, 0);
        funchook_destroy(g_hook);
        g_hook = nullptr;
        active_ = false;
    }

private:
    bool active_ = false;
    onhit::HitChunks<128> hits_;
};
} // namespace

ENDSTONE_PLUGIN("onhitfix", "0.1.0", Plugin) {
    description = "BDS 1.26.60.6 Linux preview OnHit Cleanup fix";
    authors = {"ReallocAll"};
    load = endstone::PluginLoadOrder::Startup;
    command("ohf").description("OnHitFix statistics").usages("/ohf <status|hit>")
        .permissions("onhitfix.admin");
    permission("onhitfix.admin").default_(endstone::PermissionDefault::Operator);
}
