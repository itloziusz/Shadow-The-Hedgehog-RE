#pragma once
#include "shadowpc/rhi/Rhi.hpp"
#include <unordered_map>
#include <vector>

namespace shadowpc::rhi {

class NullBackend final : public IGraphicsBackend {
public:
    Backend backend() const noexcept override { return Backend::Null; }
    std::string_view name() const noexcept override { return "NullRHI"; }
    BackendCaps caps() const noexcept override { return {}; }
    BufferHandle create_buffer(const BufferDesc&) override;
    TextureHandle create_texture(const TextureDesc&) override;
    void destroy(BufferHandle) override;
    void destroy(TextureHandle) override;
    FenceValue upload(BufferHandle,std::uint64_t,std::span<const std::byte>) override;
    FenceValue upload(TextureHandle,std::uint32_t,std::span<const std::byte>) override;
    bool completed(FenceValue) const override { return true; }
    void begin_frame() override {}
    void end_frame() override {}
private:
    std::uint64_t next_{1};
    std::unordered_map<std::uint64_t,std::vector<std::byte>> buffers_;
    std::unordered_map<std::uint64_t,TextureDesc> textures_;
};

} // namespace shadowpc::rhi
