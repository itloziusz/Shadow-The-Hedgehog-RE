#pragma once
#include "shadowpc/core/AssetRegistry.hpp"
#include <string>
#include <vector>

namespace shadowpc {

struct DependencyPlan {
    std::vector<AssetId> load_order;
    std::vector<AssetId> missing;
    std::vector<AssetId> cycle;
    bool ok() const noexcept { return missing.empty() && cycle.empty(); }
};

class DependencyTracker {
public:
    DependencyPlan build_load_order(const AssetRegistry& registry, AssetId root) const;
};

} // namespace shadowpc
