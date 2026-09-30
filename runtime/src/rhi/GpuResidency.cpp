#include "shadowpc/rhi/GpuResidency.hpp"
#include <algorithm>
#include <vector>
namespace shadowpc::rhi {
void GpuResidencyManager::destroy(Entry& e){ if(e.asset.vertex_buffer) backend_.destroy(e.asset.vertex_buffer); if(e.asset.index_buffer) backend_.destroy(e.asset.index_buffer); if(e.asset.texture) backend_.destroy(e.asset.texture); bytes_-=std::min(bytes_,e.asset.resident_bytes); }
void GpuResidencyManager::adopt(AssetId id, shadowpc::GpuAsset a,std::uint64_t t){ erase(id); bytes_+=a.resident_bytes; entries_[id]={a,t}; }
void GpuResidencyManager::touch(AssetId id,std::uint64_t s){ if(auto it=entries_.find(id);it!=entries_.end())it->second.touch=s; }
bool GpuResidencyManager::contains(AssetId id) const noexcept { return entries_.contains(id); }
void GpuResidencyManager::erase(AssetId id){ auto it=entries_.find(id); if(it==entries_.end())return; destroy(it->second); entries_.erase(it); }
void GpuResidencyManager::trim(std::uint64_t target){ std::vector<std::pair<AssetId,std::uint64_t>> order; order.reserve(entries_.size()); for(auto& [id,e]:entries_) order.emplace_back(id,e.touch); std::sort(order.begin(),order.end(),[](auto a,auto b){return a.second<b.second;}); for(auto [id,t]:order){(void)t;if(bytes_<=target)break;erase(id);} }
}
