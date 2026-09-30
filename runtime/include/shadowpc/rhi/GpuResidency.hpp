#pragma once
#include "shadowpc/assets/NativeAssets.hpp"
#include "shadowpc/core/AssetTypes.hpp"
#include "shadowpc/rhi/Rhi.hpp"
#include <cstdint>
#include <unordered_map>

namespace shadowpc::rhi {
class GpuResidencyManager {
public:
    explicit GpuResidencyManager(IGraphicsBackend& backend):backend_(backend){}
    void adopt(AssetId id, shadowpc::GpuAsset asset, std::uint64_t touch);
    void touch(AssetId id,std::uint64_t stamp);
    bool contains(AssetId id) const noexcept;
    std::uint64_t resident_bytes() const noexcept { return bytes_; }
    void trim(std::uint64_t target_bytes);
    void erase(AssetId id);
private:
    struct Entry { shadowpc::GpuAsset asset; std::uint64_t touch{}; };
    void destroy(Entry&);
    IGraphicsBackend& backend_; std::unordered_map<AssetId,Entry> entries_; std::uint64_t bytes_{};
};
} // namespace shadowpc::rhi
