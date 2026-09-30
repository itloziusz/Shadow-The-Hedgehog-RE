#include "shadowpc/core/DependencyTracker.hpp"
#include <unordered_map>

namespace shadowpc {

DependencyPlan DependencyTracker::build_load_order(const AssetRegistry& registry, AssetId root) const {
    enum class Mark : std::uint8_t { None, Visiting, Done };
    std::unordered_map<AssetId, Mark> marks;
    DependencyPlan plan;

    auto visit = [&](auto&& self, AssetId id) -> bool {
        const auto mark = marks[id];
        if (mark == Mark::Done) return true;
        if (mark == Mark::Visiting) { plan.cycle.push_back(id); return false; }
        const auto* rec = registry.find(id);
        if (!rec) { plan.missing.push_back(id); return false; }
        marks[id] = Mark::Visiting;
        for (AssetId dep : rec->dependencies) {
            if (!self(self, dep) && !plan.cycle.empty()) { plan.cycle.push_back(id); return false; }
        }
        marks[id] = Mark::Done;
        plan.load_order.push_back(id);
        return true;
    };
    visit(visit, root);
    return plan;
}

} // namespace shadowpc
