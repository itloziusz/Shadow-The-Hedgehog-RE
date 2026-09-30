#include "shadowpc/assets/AssetPipeline.hpp"
namespace shadowpc {
void AssetPipeline::add_gpu_refs_for_plan(AssetId root){
    auto plan=deps_.build_load_order(registry_,root); if(!plan.ok()) return;
    for(auto id:plan.load_order){ auto h=resources_.find_handle(id); if(!h) continue; auto [it,inserted]=entries_.try_emplace(id,Entry{h,0,PipelineState::CpuLoading,{}}); if(!inserted && it->second.handle.generation()!=h.generation()) it->second=Entry{h,0,PipelineState::CpuLoading,{}}; ++it->second.refs; }
}
void AssetPipeline::release_gpu_refs_for_plan(AssetId root){
    auto plan=deps_.build_load_order(registry_,root); if(!plan.ok()) return;
    for(auto id:plan.load_order){ auto it=entries_.find(id); if(it==entries_.end()) continue; if(it->second.refs) --it->second.refs; if(!it->second.refs){ residency_.erase(id); entries_.erase(it); } }
}
ResourceHandle AssetPipeline::request(AssetId id, ResourcePriority p) {
    auto h=resources_.request(id,p); if(!h) return {}; add_gpu_refs_for_plan(id); return h;
}
void AssetPipeline::release(ResourceHandle h) {
    if(!h) return;
    AssetId root{};
    for(const auto& [id,e]:entries_) {
        if(e.handle.value==h.value){ root=id; break; }
    }
    resources_.release(h);
    if(root) release_gpu_refs_for_plan(root);
}
void AssetPipeline::tick(std::uint32_t upload_budget) {
    ++clock_; resources_.tick();
    for(auto& [id,e]:entries_){
        if(e.state!=PipelineState::CpuLoading || !resources_.is_ready(e.handle)) continue;
        e.state=PipelineState::Decoding;
        const auto* decoder=decoders_.find(e.handle.type());
        if(!decoder){ e.state=PipelineState::Failed; continue; }
        auto decoded=decoder->decode(id,resources_.bytes(e.handle));
        if(std::holds_alternative<std::monostate>(decoded)){ e.state=PipelineState::Failed; continue; }
        e.upload=uploads_.enqueue(std::move(decoded),e.handle.type()); e.state=PipelineState::Uploading;
    }
    uploads_.tick(upload_budget);
    for(auto& [id,e]:entries_){
        if(e.state!=PipelineState::Uploading) continue;
        auto result=uploads_.collect(e.upload); if(!result) continue;
        if(result->state==rhi::UploadState::Ready){ residency_.adopt(id,result->gpu,clock_); e.state=PipelineState::Ready; }
        else e.state=PipelineState::Failed;
    }
}
bool AssetPipeline::gpu_ready(ResourceHandle h) const { for(const auto& [id,e]:entries_){ (void)id; if(e.handle.value==h.value) return e.state==PipelineState::Ready; } return false; }
bool AssetPipeline::asset_gpu_ready(AssetId id) const { auto it=entries_.find(id); return it!=entries_.end()&&it->second.state==PipelineState::Ready; }
PipelineState AssetPipeline::state(ResourceHandle h) const { for(const auto& [id,e]:entries_){ (void)id; if(e.handle.value==h.value) return e.state; } return PipelineState::Failed; }
} // namespace shadowpc
