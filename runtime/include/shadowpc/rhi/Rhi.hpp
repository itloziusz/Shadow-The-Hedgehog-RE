#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace shadowpc::rhi {

enum class Backend : std::uint8_t { Null, Direct3D12, Vulkan };
enum class QueueType : std::uint8_t { Graphics, Compute, Copy };
enum class MemoryClass : std::uint8_t { DeviceLocal, Upload, Readback };
enum class Format : std::uint16_t { Unknown=0, RGBA8_UNorm, RGBA8_sRGB, BC1, BC3, BC4, BC5, BC7, D32_Float };

enum BufferUsage : std::uint32_t { BufferVertex=1u<<0, BufferIndex=1u<<1, BufferConstant=1u<<2, BufferStorage=1u<<3, BufferCopyDst=1u<<4 };
enum TextureUsage : std::uint32_t { TextureSampled=1u<<0, TextureRenderTarget=1u<<1, TextureDepth=1u<<2, TextureStorage=1u<<3, TextureCopyDst=1u<<4 };

struct BufferHandle { std::uint64_t value{}; constexpr explicit operator bool() const { return value!=0; } };
struct TextureHandle { std::uint64_t value{}; constexpr explicit operator bool() const { return value!=0; } };
struct PipelineHandle { std::uint64_t value{}; };
struct FenceValue { std::uint64_t value{}; };

struct BufferDesc { std::uint64_t bytes{}; std::uint32_t usage{}; MemoryClass memory{MemoryClass::DeviceLocal}; };
struct TextureDesc { std::uint32_t width{},height{},depth{1},mip_count{1},array_layers{1}; Format format{Format::Unknown}; std::uint32_t usage{}; };
struct BackendCaps { bool async_compute{}; bool dedicated_copy_queue{}; bool sparse_resources{}; bool mesh_shaders{}; bool sampler_feedback{}; };

class IGraphicsBackend {
public:
    virtual ~IGraphicsBackend()=default;
    virtual Backend backend() const noexcept=0;
    virtual std::string_view name() const noexcept=0;
    virtual BackendCaps caps() const noexcept=0;
    virtual BufferHandle create_buffer(const BufferDesc&)=0;
    virtual TextureHandle create_texture(const TextureDesc&)=0;
    virtual void destroy(BufferHandle)=0;
    virtual void destroy(TextureHandle)=0;
    virtual FenceValue upload(BufferHandle,std::uint64_t,std::span<const std::byte>)=0;
    virtual FenceValue upload(TextureHandle,std::uint32_t,std::span<const std::byte>)=0;
    virtual bool completed(FenceValue) const=0;
    virtual void begin_frame()=0;
    virtual void end_frame()=0;
};

} // namespace shadowpc::rhi
