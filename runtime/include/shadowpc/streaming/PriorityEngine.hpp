#pragma once
#include "shadowpc/core/AssetTypes.hpp"
#include "shadowpc/streaming/AsyncIo.hpp"
#include <array>
#include <cstdint>

namespace shadowpc {
struct StreamingObservation {
    AssetId asset{};
    std::array<float,3> camera_to_asset{};
    std::array<float,3> camera_forward{0,0,1};
    std::array<float,3> player_velocity{};
    float bounding_radius{1.0f};
    bool inside_frustum{};
    bool mission_critical{};
};
struct StreamingScore {
    float value{};
    ResourcePriority priority{ResourcePriority::Normal};
};
class StreamingPriorityEngine {
public:
    StreamingScore score(const StreamingObservation&) const noexcept;
};
} // namespace shadowpc
