#include "shadowpc/rhi/UploadScheduler.hpp"
#include <algorithm>
#include <stdexcept>
namespace shadowpc::rhi {
UploadTicket UploadScheduler::enqueue(shadowpc::DecodedAsset asset, AssetType type){
    UploadTicket t{next_++}; jobs_.emplace(t.value,Job{t,type,std::move(asset),{}}); queue_.push_back(t.value); return t;
}
void UploadScheduler::submit(Job& j){
    try {
        if(auto* tex=std::get_if<shadowpc::NativeTexture>(&j.asset)){
            TextureDesc d{tex->width,tex->height,1,static_cast<std::uint32_t>(tex->mips.size()),tex->array_layers,tex->format,TextureSampled|TextureCopyDst};
            j.result.gpu.texture=backend_.create_texture(d); j.result.gpu.type=j.type;
            FenceValue last{}; std::uint64_t bytes=0;
            for(std::uint32_t mip=0;mip<tex->mips.size();++mip){ last=backend_.upload(j.result.gpu.texture,mip,tex->mips[mip].bytes); bytes+=tex->mips[mip].bytes.size(); }
            j.result.gpu.ready_fence=last; j.result.gpu.resident_bytes=bytes;
        } else if(auto* mesh=std::get_if<shadowpc::NativeMesh>(&j.asset)){
            j.result.gpu.type=j.type;
            j.result.gpu.vertex_buffer=backend_.create_buffer({mesh->vertices.bytes.size(),BufferVertex|BufferCopyDst,MemoryClass::DeviceLocal});
            auto f1=backend_.upload(j.result.gpu.vertex_buffer,0,mesh->vertices.bytes);
            if(!mesh->indices.bytes.empty()){
                j.result.gpu.index_buffer=backend_.create_buffer({mesh->indices.bytes.size(),BufferIndex|BufferCopyDst,MemoryClass::DeviceLocal});
                j.result.gpu.ready_fence=backend_.upload(j.result.gpu.index_buffer,0,mesh->indices.bytes);
            } else j.result.gpu.ready_fence=f1;
            j.result.gpu.resident_bytes=mesh->vertices.bytes.size()+mesh->indices.bytes.size();
        } else {
            // CPU-only metadata assets become immediately resident without GPU storage.
            j.result.gpu.type=j.type; j.result.gpu.ready_fence={};
        }
        j.result.state=UploadState::Submitted;
    } catch(...) { j.result.state=UploadState::Failed; }
}
void UploadScheduler::tick(std::uint32_t submit_budget){
    while(submit_budget-- && !queue_.empty()) { auto id=queue_.front(); queue_.pop_front(); auto it=jobs_.find(id); if(it!=jobs_.end()) submit(it->second); }
    for(auto& [id,j]:jobs_){ (void)id; if(j.result.state==UploadState::Submitted && (!j.result.gpu.ready_fence.value || backend_.completed(j.result.gpu.ready_fence))) j.result.state=UploadState::Ready; }
}
std::optional<UploadResult> UploadScheduler::query(UploadTicket t) const { auto it=jobs_.find(t.value); if(it==jobs_.end()) return std::nullopt; return it->second.result; }
std::optional<UploadResult> UploadScheduler::collect(UploadTicket t){ auto it=jobs_.find(t.value); if(it==jobs_.end()|| (it->second.result.state!=UploadState::Ready&&it->second.result.state!=UploadState::Failed)) return std::nullopt; auto r=it->second.result; jobs_.erase(it); return r; }
}
