#pragma once
#include "shadowpc/core/AssetTypes.hpp"
#include <cstdint>
#include <span>
#include <vector>
namespace shadowpc {
struct MipRequest { AssetId texture{}; std::uint16_t most_detailed_mip{}; std::uint16_t mip_count{}; float priority{}; };
class ITextureResidencyManager {
public:
    virtual ~ITextureResidencyManager()=default;
    virtual void submit(std::span<const MipRequest>)=0;
    virtual void tick(std::uint64_t upload_budget_bytes)=0;
};
}
