#include "shadowpc/rhi/Dx12Backend.hpp"
#if !defined(_WIN32)
#error "Dx12Backend.cpp must only be built on Windows"
#endif
#define NOMINMAX
#include <windows.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <algorithm>
#include <cstring>
#include <deque>
#include <stdexcept>
#include <unordered_map>
using Microsoft::WRL::ComPtr;
namespace shadowpc::rhi {
namespace {
void check(HRESULT hr,const char* msg){if(FAILED(hr))throw std::runtime_error(msg);}
DXGI_FORMAT dxgi(Format f){switch(f){case Format::RGBA8_UNorm:return DXGI_FORMAT_R8G8B8A8_UNORM;case Format::RGBA8_sRGB:return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;case Format::BC1:return DXGI_FORMAT_BC1_UNORM;case Format::BC3:return DXGI_FORMAT_BC3_UNORM;case Format::BC4:return DXGI_FORMAT_BC4_UNORM;case Format::BC5:return DXGI_FORMAT_BC5_UNORM;case Format::BC7:return DXGI_FORMAT_BC7_UNORM;case Format::D32_Float:return DXGI_FORMAT_D32_FLOAT;default:return DXGI_FORMAT_UNKNOWN;}}
std::uint32_t tight_row_bytes(Format f,std::uint32_t w){switch(f){case Format::BC1:case Format::BC4:return std::max(1u,(w+3)/4)*8;case Format::BC3:case Format::BC5:case Format::BC7:return std::max(1u,(w+3)/4)*16;default:return w*4;}}
std::uint32_t row_count(Format f,std::uint32_t h){switch(f){case Format::BC1:case Format::BC3:case Format::BC4:case Format::BC5:case Format::BC7:return std::max(1u,(h+3)/4);default:return h;}}
}
struct Dx12Backend::Impl {
    struct BufferRec{ComPtr<ID3D12Resource> resource;BufferDesc desc;};
    struct TextureRec{ComPtr<ID3D12Resource> resource;TextureDesc desc;};
    struct Pending{std::uint64_t fence{};ComPtr<ID3D12Resource> staging;ComPtr<ID3D12CommandAllocator> allocator;};
    ComPtr<IDXGIFactory6> factory; ComPtr<ID3D12Device> device; ComPtr<ID3D12CommandQueue> graphics,copy;
    ComPtr<ID3D12Fence> fence; HANDLE fence_event{}; std::uint64_t next_fence{1},next_handle{1};
    std::unordered_map<std::uint64_t,BufferRec> buffers; std::unordered_map<std::uint64_t,TextureRec> textures; std::deque<Pending> pending;
    ~Impl(){if(fence_event)CloseHandle(fence_event);}
    FenceValue submit(ID3D12GraphicsCommandList* list,ComPtr<ID3D12CommandAllocator> allocator,ComPtr<ID3D12Resource> staging){
        check(list->Close(),"DX12 copy list close failed"); ID3D12CommandList* lists[]={list}; copy->ExecuteCommandLists(1,lists); const auto v=next_fence++; check(copy->Signal(fence.Get(),v),"DX12 copy fence signal failed"); pending.push_back({v,std::move(staging),std::move(allocator)}); return {v};
    }
    void collect(){const auto done=fence->GetCompletedValue();while(!pending.empty()&&pending.front().fence<=done)pending.pop_front();}
};
Dx12Backend::Dx12Backend():impl_(std::make_unique<Impl>()){
 check(CreateDXGIFactory2(0,IID_PPV_ARGS(&impl_->factory)),"CreateDXGIFactory2 failed");
 ComPtr<IDXGIAdapter1> adapter; for(UINT i=0;impl_->factory->EnumAdapters1(i,&adapter)!=DXGI_ERROR_NOT_FOUND;++i){DXGI_ADAPTER_DESC1 d{};adapter->GetDesc1(&d);if(d.Flags&DXGI_ADAPTER_FLAG_SOFTWARE){adapter.Reset();continue;}if(SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_0,__uuidof(ID3D12Device),nullptr)))break;adapter.Reset();}
 if(!adapter)throw std::runtime_error("No DX12 hardware adapter"); check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&impl_->device)),"D3D12CreateDevice failed");
 D3D12_COMMAND_QUEUE_DESC q{};q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;check(impl_->device->CreateCommandQueue(&q,IID_PPV_ARGS(&impl_->graphics)),"graphics queue failed");q.Type=D3D12_COMMAND_LIST_TYPE_COPY;check(impl_->device->CreateCommandQueue(&q,IID_PPV_ARGS(&impl_->copy)),"copy queue failed");
 check(impl_->device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&impl_->fence)),"fence create failed");impl_->fence_event=CreateEvent(nullptr,FALSE,FALSE,nullptr);if(!impl_->fence_event)throw std::runtime_error("CreateEvent failed");
}
Dx12Backend::~Dx12Backend()=default;
BackendCaps Dx12Backend::caps()const noexcept{return {.async_compute=true,.dedicated_copy_queue=true,.sparse_resources=true,.mesh_shaders=false,.sampler_feedback=true};}
BufferHandle Dx12Backend::create_buffer(const BufferDesc& d){
 D3D12_HEAP_PROPERTIES hp{};hp.Type=d.memory==MemoryClass::Upload?D3D12_HEAP_TYPE_UPLOAD:d.memory==MemoryClass::Readback?D3D12_HEAP_TYPE_READBACK:D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=std::max<std::uint64_t>(1,d.bytes);rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;ComPtr<ID3D12Resource> r;auto state=hp.Type==D3D12_HEAP_TYPE_UPLOAD?D3D12_RESOURCE_STATE_GENERIC_READ:hp.Type==D3D12_HEAP_TYPE_READBACK?D3D12_RESOURCE_STATE_COPY_DEST:D3D12_RESOURCE_STATE_COMMON;check(impl_->device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,state,nullptr,IID_PPV_ARGS(&r)),"buffer create failed");auto id=impl_->next_handle++;impl_->buffers[id]={std::move(r),d};return{id};
}
TextureHandle Dx12Backend::create_texture(const TextureDesc& d){
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC rd{};rd.Dimension=d.depth>1?D3D12_RESOURCE_DIMENSION_TEXTURE3D:D3D12_RESOURCE_DIMENSION_TEXTURE2D;rd.Width=d.width;rd.Height=d.height;rd.DepthOrArraySize=static_cast<UINT16>(d.depth>1?d.depth:d.array_layers);rd.MipLevels=static_cast<UINT16>(d.mip_count);rd.Format=dxgi(d.format);rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_UNKNOWN;rd.Flags=(d.usage&TextureRenderTarget)?D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET:(d.usage&TextureDepth)?D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL:D3D12_RESOURCE_FLAG_NONE;ComPtr<ID3D12Resource> r;check(impl_->device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&r)),"texture create failed");auto id=impl_->next_handle++;impl_->textures[id]={std::move(r),d};return{id};
}
void Dx12Backend::destroy(BufferHandle h){impl_->buffers.erase(h.value);} void Dx12Backend::destroy(TextureHandle h){impl_->textures.erase(h.value);}
FenceValue Dx12Backend::upload(BufferHandle h,std::uint64_t off,std::span<const std::byte> src){
 auto it=impl_->buffers.find(h.value);if(it==impl_->buffers.end())throw std::runtime_error("invalid DX12 buffer handle");auto& dst=it->second;if(off+src.size()>dst.desc.bytes)throw std::runtime_error("DX12 buffer upload overflow");
 if(dst.desc.memory==MemoryClass::Upload){void* p{};D3D12_RANGE no_read{0,0};check(dst.resource->Map(0,&no_read,&p),"upload buffer map failed");std::memcpy(static_cast<std::byte*>(p)+off,src.data(),src.size());dst.resource->Unmap(0,nullptr);return{};}
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_UPLOAD;D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=std::max<std::size_t>(1,src.size());rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;ComPtr<ID3D12Resource> staging;check(impl_->device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&staging)),"staging buffer create failed");void* p{};D3D12_RANGE no_read{0,0};check(staging->Map(0,&no_read,&p),"staging map failed");std::memcpy(p,src.data(),src.size());staging->Unmap(0,nullptr);
 ComPtr<ID3D12CommandAllocator> a;ComPtr<ID3D12GraphicsCommandList> l;check(impl_->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY,IID_PPV_ARGS(&a)),"copy allocator failed");check(impl_->device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_COPY,a.Get(),nullptr,IID_PPV_ARGS(&l)),"copy list failed");l->CopyBufferRegion(dst.resource.Get(),off,staging.Get(),0,src.size());return impl_->submit(l.Get(),std::move(a),std::move(staging));
}
FenceValue Dx12Backend::upload(TextureHandle h,std::uint32_t mip,std::span<const std::byte> src){
 auto it=impl_->textures.find(h.value);if(it==impl_->textures.end())throw std::runtime_error("invalid DX12 texture handle");auto& dst=it->second;if(mip>=dst.desc.mip_count)throw std::runtime_error("DX12 mip out of range");
 const UINT subresource=mip;D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT rows{};UINT64 row_bytes{},total{};auto rd=dst.resource->GetDesc();impl_->device->GetCopyableFootprints(&rd,subresource,1,0,&fp,&rows,&row_bytes,&total);D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_UPLOAD;D3D12_RESOURCE_DESC br{};br.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;br.Width=std::max<UINT64>(1,total);br.Height=1;br.DepthOrArraySize=1;br.MipLevels=1;br.SampleDesc.Count=1;br.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;ComPtr<ID3D12Resource> staging;check(impl_->device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&br,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&staging)),"texture staging create failed");
 void* mapped{};D3D12_RANGE nr{0,0};check(staging->Map(0,&nr,&mapped),"texture staging map failed");const auto mw=std::max(1u,dst.desc.width>>mip),mh=std::max(1u,dst.desc.height>>mip);const auto tight=tight_row_bytes(dst.desc.format,mw);const auto nrows=row_count(dst.desc.format,mh);if(src.size()<std::uint64_t(tight)*nrows){staging->Unmap(0,nullptr);throw std::runtime_error("texture mip payload too small");}for(std::uint32_t y=0;y<nrows;++y)std::memcpy(static_cast<std::byte*>(mapped)+fp.Offset+std::uint64_t(y)*fp.Footprint.RowPitch,src.data()+std::uint64_t(y)*tight,tight);staging->Unmap(0,nullptr);
 ComPtr<ID3D12CommandAllocator> a;ComPtr<ID3D12GraphicsCommandList> l;check(impl_->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY,IID_PPV_ARGS(&a)),"texture copy allocator failed");check(impl_->device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_COPY,a.Get(),nullptr,IID_PPV_ARGS(&l)),"texture copy list failed");D3D12_TEXTURE_COPY_LOCATION s{};s.pResource=staging.Get();s.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;s.PlacedFootprint=fp;D3D12_TEXTURE_COPY_LOCATION d{};d.pResource=dst.resource.Get();d.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;d.SubresourceIndex=subresource;l->CopyTextureRegion(&d,0,0,0,&s,nullptr);D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=dst.resource.Get();b.Transition.Subresource=subresource;b.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_DEST;b.Transition.StateAfter=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;l->ResourceBarrier(1,&b);return impl_->submit(l.Get(),std::move(a),std::move(staging));
}
bool Dx12Backend::completed(FenceValue f)const{return !f.value||impl_->fence->GetCompletedValue()>=f.value;} void Dx12Backend::begin_frame(){impl_->collect();} void Dx12Backend::end_frame(){}
}
