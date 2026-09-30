#pragma once
#include "shadowpc/rhi/Rhi.hpp"
#include <memory>
namespace shadowpc::rhi {
class VulkanBackend final : public IGraphicsBackend {
public:
    VulkanBackend(); ~VulkanBackend() override;
    Backend backend() const noexcept override { return Backend::Vulkan; }
    std::string_view name() const noexcept override { return "Vulkan"; }
    BackendCaps caps() const noexcept override;
    BufferHandle create_buffer(const BufferDesc&) override;
    TextureHandle create_texture(const TextureDesc&) override;
    void destroy(BufferHandle) override; void destroy(TextureHandle) override;
    FenceValue upload(BufferHandle,std::uint64_t,std::span<const std::byte>) override;
    FenceValue upload(TextureHandle,std::uint32_t,std::span<const std::byte>) override;
    bool completed(FenceValue) const override; void begin_frame() override; void end_frame() override;
private: struct Impl; std::unique_ptr<Impl> impl_;
};
}
