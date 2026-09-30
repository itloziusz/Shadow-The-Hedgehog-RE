#pragma once
#include "shadowpc/assets/AssetDecoder.hpp"
#include "shadowpc/core/AssetRegistry.hpp"
#include "shadowpc/core/DependencyTracker.hpp"
#include "shadowpc/core/ResourceManager.hpp"
#include "shadowpc/rhi/GpuResidency.hpp"
#include "shadowpc/rhi/UploadScheduler.hpp"
#include <cstdint>
#include <unordered_map>

namespace shadowpc {
enum class PipelineState : std::uint8_t { CpuLoading, Decoding, Uploading, Ready, Failed };

class AssetPipeline {
public:
    AssetPipeline(const AssetRegistry& registry, ResourceManager& resources, const AssetDecoderRegistry& decoders,
                  rhi::UploadScheduler& uploads, rhi::GpuResidencyManager& residency)
        : registry_(registry),resources_(resources),decoders_(decoders),uploads_(uploads),residency_(residency) {}
    ResourceHandle request(AssetId id, ResourcePriority priority=ResourcePriority::Normal);
    void release(ResourceHandle handle);
    void tick(std::uint32_t upload_budget=8);
    bool gpu_ready(ResourceHandle handle) const;
    bool asset_gpu_ready(AssetId id) const;
    PipelineState state(ResourceHandle handle) const;
private:
    struct Entry {
        ResourceHandle handle{};
        std::uint32_t refs{};
        PipelineState state{PipelineState::CpuLoading};
        rhi::UploadTicket upload{};
    };
    void add_gpu_refs_for_plan(AssetId root);
    void release_gpu_refs_for_plan(AssetId root);
    const AssetRegistry& registry_;
    ResourceManager& resources_;
    const AssetDecoderRegistry& decoders_;
    rhi::UploadScheduler& uploads_;
    rhi::GpuResidencyManager& residency_;
    DependencyTracker deps_;
    std::unordered_map<AssetId,Entry> entries_;
    std::uint64_t clock_{};
};
} // namespace shadowpc
