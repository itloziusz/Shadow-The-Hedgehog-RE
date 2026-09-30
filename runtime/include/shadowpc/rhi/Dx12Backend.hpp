#pragma once
#include "shadowpc/rhi/Rhi.hpp"
#include <memory>
namespace shadowpc::rhi {
class Dx12Backend final : public IGraphicsBackend {
public:
    Dx12Backend(); ~Dx12Backend() override;
    Backend backend() const noexcept override { return Backend::Direct3D12; }
    std::string_view name() const noexcept override { return "Direct3D 12"; }
    BackendCaps caps() const noexcept override;
    BufferHandle create_buffer(const BufferDesc&) override;
    TextureHandle create_texture(const TextureDesc&) override;
    void destroy(BufferHandle) override;
    void destroy(TextureHandle) override;
    FenceValue upload(BufferHandle,std::uint64_t,std::span<const std::byte>) override;
    FenceValue upload(TextureHandle,std::uint32_t,std::span<const std::byte>) override;
    bool completed(FenceValue) const override;
    void begin_frame() override;
    void end_frame() override;
private: struct Impl; std::unique_ptr<Impl> impl_;
};
}
