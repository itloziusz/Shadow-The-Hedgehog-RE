#pragma once
#include "shadowpc/core/AssetRegistry.hpp"
#include "shadowpc/core/DependencyTracker.hpp"
#include "shadowpc/core/ResourceHandle.hpp"
#include "shadowpc/streaming/AsyncIo.hpp"
#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

namespace shadowpc {

enum class ResourceState : std::uint8_t { Unloaded, Requested, Reading, Decompressing, Creating, Uploading, Ready, Cached, Failed, Evicted };

struct ResourceStats {
    std::uint64_t cpu_bytes{};
    std::uint64_t gpu_bytes{};
    std::uint32_t ready{};
    std::uint32_t cached{};
    std::uint32_t failed{};
};

class ResourceManager {
public:
    ResourceManager(const AssetRegistry& registry, IAsyncIoScheduler& io);
    ResourceHandle request(AssetId id, ResourcePriority priority=ResourcePriority::Normal);
    void add_ref(ResourceHandle h);
    void release(ResourceHandle h);
    void tick(std::uint64_t io_collect_budget=64);
    bool is_ready(ResourceHandle h) const;
    ResourceHandle find_handle(AssetId id) const noexcept;
    std::span<const std::byte> bytes(ResourceHandle h) const;
    void trim_cache(std::uint64_t target_cpu_bytes);
    ResourceStats stats() const;

private:
    struct Slot {
        AssetId asset{};
        AssetType type{AssetType::Unknown};
        ResourceState state{ResourceState::Unloaded};
        std::uint32_t generation{1};
        std::uint32_t refs{};
        std::uint64_t last_touch{};
        ResourcePriority priority{ResourcePriority::Normal};
        IoTicket ticket{};
        std::vector<std::byte> stored;
        std::vector<std::byte> decoded;
    };
    ResourceHandle ensure_slot(AssetId, ResourcePriority);
    Slot* validate(ResourceHandle);
    const Slot* validate(ResourceHandle) const;

    const AssetRegistry& registry_;
    IAsyncIoScheduler& io_;
    DependencyTracker deps_;
    std::vector<Slot> slots_;
    std::vector<std::uint32_t> free_;
    std::unordered_map<AssetId,std::uint32_t> by_asset_;
    std::uint64_t clock_{};
};

} // namespace shadowpc
