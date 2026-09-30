#include "shadowpc/rhi/NullBackend.hpp"
#include <algorithm>
#include <cstring>

namespace shadowpc::rhi {
BufferHandle NullBackend::create_buffer(const BufferDesc& d) { auto id=next_++; buffers_[id].resize(static_cast<std::size_t>(d.bytes)); return {id}; }
TextureHandle NullBackend::create_texture(const TextureDesc& d) { auto id=next_++; textures_[id]=d; return {id}; }
void NullBackend::destroy(BufferHandle h) { buffers_.erase(h.value); }
void NullBackend::destroy(TextureHandle h) { textures_.erase(h.value); }
FenceValue NullBackend::upload(BufferHandle h,std::uint64_t off,std::span<const std::byte> src) {
    auto& b=buffers_.at(h.value); if (off+src.size()>b.size()) b.resize(static_cast<std::size_t>(off+src.size())); std::copy(src.begin(),src.end(),b.begin()+static_cast<std::size_t>(off)); return {next_++};
}
FenceValue NullBackend::upload(TextureHandle,std::uint32_t,std::span<const std::byte>) { return {next_++}; }
}
