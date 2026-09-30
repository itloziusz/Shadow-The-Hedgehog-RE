#include "shadowpc/core/Allocator.hpp"
#include "shadowpc/core/AssetRegistry.hpp"
#include "shadowpc/core/DependencyTracker.hpp"
#include "shadowpc/core/ResourceManager.hpp"
#include "shadowpc/core/RegistryBinary.hpp"
#include "shadowpc/core/VirtualArena.hpp"
#include "shadowpc/assets/NativeAssets.hpp"
#include "shadowpc/assets/AssetDecoder.hpp"
#include "shadowpc/assets/AssetPipeline.hpp"
#include "shadowpc/rhi/UploadScheduler.hpp"
#include "shadowpc/rhi/GpuResidency.hpp"
#include "shadowpc/streaming/PriorityEngine.hpp"
#include "shadowpc/streaming/WorldStreaming.hpp"
#include "shadowpc/rhi/NullBackend.hpp"
#include "shadowpc/streaming/Decompression.hpp"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>


namespace {
class TestTextureDecoder final : public shadowpc::IAssetDecoder {
public:
 shadowpc::AssetType type() const noexcept override { return shadowpc::AssetType::Texture; }
 shadowpc::DecodedAsset decode(shadowpc::AssetId,std::span<const std::byte> bytes) const override {
  shadowpc::NativeTexture t; t.debug_name="decoded_texture"; t.width=1;t.height=1;t.format=shadowpc::rhi::Format::RGBA8_UNorm;
  shadowpc::NativeTextureMip m; m.width=1;m.height=1;m.bytes.assign(bytes.begin(),bytes.end()); t.mips.push_back(std::move(m)); return t;
 }
};
class TestModelDecoder final : public shadowpc::IAssetDecoder {
public:
 shadowpc::AssetType type() const noexcept override { return shadowpc::AssetType::Model; }
 shadowpc::DecodedAsset decode(shadowpc::AssetId,std::span<const std::byte> bytes) const override {
  shadowpc::NativeMesh m; m.debug_name="decoded_model"; m.vertices.bytes.assign(bytes.begin(),bytes.end()); m.vertices.stride=1; return m;
 }
};
}

int main(){
 using namespace shadowpc;
 const auto id=make_asset_id("CHARACTER\\SHADOW.DFF");
 assert(id==make_asset_id("character/shadow.dff"));
 auto h=ResourceHandle::make(42,7,AssetType::Model);assert(h.index()==42&&h.generation()==7&&h.type()==AssetType::Model);
 LinearArena arena(1024);assert(arena.allocate(32,32));assert(reinterpret_cast<std::uintptr_t>(arena.allocate(17,64))%64==0);arena.reset();
 SlabPool<int> pool(2);auto* pi=pool.create(7);assert(pi&&*pi==7&&pool.live_count()==1);pool.destroy(pi);assert(pool.live_count()==0);
 const std::byte prs_raw[]={std::byte{0x17},std::byte{'A'},std::byte{'B'},std::byte{'C'},std::byte{0},std::byte{0}};auto prs=decompress(CompressionCodec::PRS,prs_raw,3);assert(prs.ok&&prs.bytes.size()==3&&char(prs.bytes[2])=='C');
 const char* path="shadowpc_v13_test.bin";{std::ofstream f(path,std::ios::binary);f.write("ABCDEFGH",8);}
 AssetRegistry reg; AssetRecord tex;tex.canonical_path="tex/a";tex.type=AssetType::Texture;tex.container_path=path;tex.stored_size=4;tex.uncompressed_size=4;assert(reg.add(tex));
 AssetRecord mdl;mdl.canonical_path="mdl/a";mdl.type=AssetType::Model;mdl.container_path=path;mdl.container_offset=4;mdl.stored_size=4;mdl.uncompressed_size=4;mdl.dependencies={make_asset_id("tex/a")};assert(reg.add(mdl));
 DependencyTracker dt;auto p=dt.build_load_order(reg,make_asset_id("mdl/a"));assert(p.ok()&&p.load_order.size()==2&&p.load_order.back()==make_asset_id("mdl/a"));
 const char* regpath="shadowpc_v13_registry.bin";assert(save_registry_binary(reg,regpath).ok);AssetRegistry reg2;assert(load_registry_binary(reg2,regpath).ok);assert(reg2.size()==2&&reg2.find_by_path("MDL\\A"));
 ThreadedIoScheduler io(2);ResourceManager rm(reg,io);auto mh=rm.request(make_asset_id("mdl/a"),ResourcePriority::Critical);assert(mh);
 for(int i=0;i<200&&!rm.is_ready(mh);++i){rm.tick();std::this_thread::sleep_for(std::chrono::milliseconds(2));}
 assert(rm.is_ready(mh));auto bytes=rm.bytes(mh);assert(bytes.size()==4&&char(bytes[0])=='E');const auto old_generation=mh.generation();rm.release(mh);rm.trim_cache(0);assert(!rm.is_ready(mh));auto mh2=rm.request(make_asset_id("mdl/a"),ResourcePriority::High);assert(mh2&&mh2.generation()!=old_generation);for(int i=0;i<200&&!rm.is_ready(mh2);++i){rm.tick();std::this_thread::sleep_for(std::chrono::milliseconds(2));}assert(rm.is_ready(mh2));rm.release(mh2);
 VirtualArena va(8*1024*1024); auto* vp=va.allocate(4096,4096); assert(vp&&reinterpret_cast<std::uintptr_t>(vp)%4096==0); assert(va.committed_bytes()>=4096); va.reset();
 StreamingPriorityEngine pe; auto critical=pe.score({make_asset_id("world/near"),{0,0,3},{0,0,1},{0,0,20},1,true,true}); auto low=pe.score({make_asset_id("world/far"),{0,0,1000},{0,0,1},{0,0,0},1,false,false}); assert(critical.priority==ResourcePriority::Critical&&critical.value>low.value);
 WorldStreamer ws(rm); WorldCell wc; wc.id=1; wc.bounds={{-5,-5,20},{5,5,30}}; wc.assets={make_asset_id("tex/a")}; wc.preload_radius=10; wc.unload_radius=50; ws.set_cells({wc}); ws.update({{0,0,0},{0,0,1},30}); assert(ws.resident_cell_count()==1);
 rhi::NullBackend rhi;auto bh=rhi.create_buffer({64,rhi::BufferCopyDst,rhi::MemoryClass::DeviceLocal});assert(bh);rhi.destroy(bh);
 NativeMesh nm; nm.debug_name="testmesh"; nm.vertices.bytes.resize(96,std::byte{0x2a}); nm.indices.bytes.resize(12,std::byte{0x01}); nm.index_count=6;
 rhi::UploadScheduler uploader(rhi); auto ut=uploader.enqueue(DecodedAsset{nm},AssetType::Mesh); assert(ut); uploader.tick(); auto ur=uploader.collect(ut); assert(ur&&ur->state==rhi::UploadState::Ready&&ur->gpu.resident_bytes==108);
 rhi::GpuResidencyManager residency(rhi); residency.adopt(make_asset_id("mesh/test"),ur->gpu,1); assert(residency.contains(make_asset_id("mesh/test"))&&residency.resident_bytes()==108); residency.trim(0); assert(!residency.contains(make_asset_id("mesh/test"))&&residency.resident_bytes()==0);
 AssetDecoderRegistry decoders; decoders.register_decoder(std::make_unique<TestTextureDecoder>()); decoders.register_decoder(std::make_unique<TestModelDecoder>());
 rhi::UploadScheduler pipeline_uploads(rhi); rhi::GpuResidencyManager pipeline_residency(rhi); AssetPipeline pipeline(reg,rm,decoders,pipeline_uploads,pipeline_residency);
 auto ph=pipeline.request(make_asset_id("mdl/a"),ResourcePriority::Critical); assert(ph);
 for(int i=0;i<200&&!pipeline.gpu_ready(ph);++i){pipeline.tick();std::this_thread::sleep_for(std::chrono::milliseconds(2));}
 assert(pipeline.gpu_ready(ph)); assert(pipeline.asset_gpu_ready(make_asset_id("tex/a"))); assert(pipeline_residency.resident_bytes()>=8);
 pipeline.release(ph); assert(!pipeline_residency.contains(make_asset_id("mdl/a"))&&!pipeline_residency.contains(make_asset_id("tex/a")));
 std::remove(path); std::remove(regpath);
 std::cout<<"ShadowPC v1.4 core tests: PASS\n";
}
