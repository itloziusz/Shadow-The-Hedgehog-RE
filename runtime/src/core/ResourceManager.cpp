#include "shadowpc/core/ResourceManager.hpp"
#include "shadowpc/streaming/Decompression.hpp"
#include <algorithm>

namespace shadowpc {
ResourceManager::ResourceManager(const AssetRegistry& r,IAsyncIoScheduler& io):registry_(r),io_(io){}

ResourceHandle ResourceManager::ensure_slot(AssetId id, ResourcePriority p) {
    if (auto it=by_asset_.find(id); it!=by_asset_.end()) {
        auto& s=slots_[it->second]; s.priority=std::max(s.priority,p); s.last_touch=++clock_;
        return ResourceHandle::make(it->second,s.generation,s.type);
    }
    const auto* rec=registry_.find(id); if(!rec) return {};
    std::uint32_t idx;
    if(!free_.empty()){
        idx=free_.back(); free_.pop_back();
        const auto next_generation = (slots_[idx].generation + 1u) & 0x00FFFFFFu;
        slots_[idx]=Slot{};
        slots_[idx].generation = next_generation ? next_generation : 1u;
    } else { idx=static_cast<std::uint32_t>(slots_.size()); slots_.push_back({}); }
    auto& s=slots_[idx]; s.asset=id; s.type=rec->type; s.state=ResourceState::Requested; s.priority=p; s.last_touch=++clock_;
    by_asset_[id]=idx; return ResourceHandle::make(idx,s.generation,s.type);
}

ResourceHandle ResourceManager::request(AssetId id, ResourcePriority p) {
    auto plan=deps_.build_load_order(registry_,id); if(!plan.ok()) return {};
    ResourceHandle root{};
    for(auto dep:plan.load_order){ auto h=ensure_slot(dep,p); if(dep==id)root=h; if(h) ++slots_[h.index()].refs; }
    return root;
}
void ResourceManager::add_ref(ResourceHandle h){ if(auto*s=validate(h)){++s->refs;s->last_touch=++clock_;} }
void ResourceManager::release(ResourceHandle h){
    auto* root = validate(h);
    if (!root) return;
    const AssetId root_id = root->asset;
    auto plan = deps_.build_load_order(registry_, root_id);
    if (!plan.ok()) return;
    for (AssetId id : plan.load_order) {
        auto it = by_asset_.find(id);
        if (it == by_asset_.end()) continue;
        auto& s = slots_[it->second];
        if (s.refs) --s.refs;
        s.last_touch = ++clock_;
        if (!s.refs && s.state == ResourceState::Ready) s.state = ResourceState::Cached;
    }
}

void ResourceManager::tick(std::uint64_t budget){
    ++clock_;
    for(auto& s:slots_){
        if(s.state!=ResourceState::Requested)continue;
        const auto* rec=registry_.find(s.asset); if(!rec){s.state=ResourceState::Failed;continue;}
        s.ticket=io_.submit(IoRequest{rec->container_path,rec->container_offset,rec->stored_size,s.priority}); s.state=ResourceState::Reading;
    }
    for(auto& s:slots_){
        if(!budget) break;
        if(s.state!=ResourceState::Reading) continue;
        auto r=io_.try_collect(s.ticket); if(!r)continue; --budget;
        if(!r->ok){s.state=ResourceState::Failed;continue;}
        s.stored=std::move(r->bytes); s.state=ResourceState::Decompressing;
    }
    for(auto& s:slots_){
        if(s.state!=ResourceState::Decompressing)continue;
        const auto* rec=registry_.find(s.asset); auto r=decompress(rec->codec,s.stored,static_cast<std::size_t>(rec->uncompressed_size));
        s.stored.clear(); s.stored.shrink_to_fit();
        if(!r.ok){s.state=ResourceState::Failed;continue;}
        s.decoded=std::move(r.bytes); s.state=s.refs?ResourceState::Ready:ResourceState::Cached; s.last_touch=++clock_;
    }
}

ResourceManager::Slot* ResourceManager::validate(ResourceHandle h){ if(!h||h.index()>=slots_.size())return nullptr;auto&s=slots_[h.index()];return s.generation==h.generation()&&s.type==h.type()?&s:nullptr; }
const ResourceManager::Slot* ResourceManager::validate(ResourceHandle h)const{ if(!h||h.index()>=slots_.size())return nullptr;const auto&s=slots_[h.index()];return s.generation==h.generation()&&s.type==h.type()?&s:nullptr; }
ResourceHandle ResourceManager::find_handle(AssetId id) const noexcept { auto it=by_asset_.find(id); if(it==by_asset_.end()) return {}; const auto& s=slots_[it->second]; return ResourceHandle::make(it->second,s.generation,s.type); }
bool ResourceManager::is_ready(ResourceHandle h)const{
    const auto* root = validate(h);
    if (!root) return false;
    auto plan = deps_.build_load_order(registry_, root->asset);
    if (!plan.ok()) return false;
    for (AssetId id : plan.load_order) {
        auto it = by_asset_.find(id);
        if (it == by_asset_.end()) return false;
        const auto state = slots_[it->second].state;
        if (state != ResourceState::Ready && state != ResourceState::Cached) return false;
    }
    return true;
}
std::span<const std::byte> ResourceManager::bytes(ResourceHandle h)const{auto*s=validate(h);return s?std::span<const std::byte>(s->decoded):std::span<const std::byte>{};}
void ResourceManager::trim_cache(std::uint64_t target){
    auto st=stats(); if(st.cpu_bytes<=target)return;
    std::vector<std::uint32_t> c; for(std::uint32_t i=0;i<slots_.size();++i)if(slots_[i].state==ResourceState::Cached&&slots_[i].refs==0)c.push_back(i);
    std::sort(c.begin(),c.end(),[this](auto a,auto b){return slots_[a].last_touch<slots_[b].last_touch;});
    std::uint64_t bytes=st.cpu_bytes;
    for(auto i:c){if(bytes<=target)break;auto&s=slots_[i];bytes-=s.decoded.size();s.decoded.clear();s.decoded.shrink_to_fit();s.state=ResourceState::Evicted;by_asset_.erase(s.asset);++s.generation;if(!s.generation)s.generation=1;free_.push_back(i);}
}
ResourceStats ResourceManager::stats()const{ResourceStats r;for(auto&s:slots_){r.cpu_bytes+=s.decoded.size()+s.stored.size();r.ready+=s.state==ResourceState::Ready;r.cached+=s.state==ResourceState::Cached;r.failed+=s.state==ResourceState::Failed;}return r;}
}
