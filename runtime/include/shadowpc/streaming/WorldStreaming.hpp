#pragma once
#include "shadowpc/core/ResourceManager.hpp"
#include "shadowpc/streaming/PriorityEngine.hpp"
#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace shadowpc {
struct Vec3 { float x{},y{},z{}; };
struct Aabb { Vec3 min{},max{}; };
using WorldCellId=std::uint64_t;
struct WorldCell {
    WorldCellId id{}; Aabb bounds{}; std::vector<AssetId> assets;
    float preload_radius{120.0f}; float unload_radius{170.0f}; bool mission_critical{};
};
struct StreamingView { Vec3 position{}; Vec3 forward{0,0,1}; float speed{}; };
class WorldStreamer {
public:
    WorldStreamer(ResourceManager& resources):resources_(resources){}
    void set_cells(std::vector<WorldCell> cells){cells_=std::move(cells);}
    void update(const StreamingView& view);
    std::size_t resident_cell_count() const noexcept { return resident_.size(); }
private:
    static float distance_to_aabb(Vec3 p,const Aabb& b);
    static Vec3 center(const Aabb& b){return {(b.min.x+b.max.x)*0.5f,(b.min.y+b.max.y)*0.5f,(b.min.z+b.max.z)*0.5f};}
    ResourceManager& resources_;
    StreamingPriorityEngine priority_;
    std::vector<WorldCell> cells_;
    std::unordered_map<WorldCellId,std::vector<ResourceHandle>> resident_;
};
}
