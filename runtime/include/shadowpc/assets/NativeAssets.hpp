#pragma once
#include "shadowpc/core/AssetTypes.hpp"
#include "shadowpc/rhi/Rhi.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace shadowpc {

struct NativeTextureMip {
    std::uint32_t width{}, height{};
    std::vector<std::byte> bytes;
};
struct NativeTexture {
    std::string debug_name;
    rhi::Format format{rhi::Format::Unknown};
    std::uint32_t width{}, height{}, array_layers{1};
    bool srgb{};
    std::vector<NativeTextureMip> mips;
};
struct NativeMeshBuffer {
    std::vector<std::byte> bytes;
    std::uint32_t stride{};
};
struct NativeMesh {
    std::string debug_name;
    NativeMeshBuffer vertices;
    NativeMeshBuffer indices;
    std::uint32_t index_count{};
    std::vector<AssetId> materials;
};
struct NativeMaterial {
    std::string debug_name;
    std::array<float,4> base_color{1,1,1,1};
    std::vector<AssetId> textures;
    std::uint32_t feature_mask{};
};
struct NativeSkeleton { std::string debug_name; std::uint32_t joint_count{}; };
struct NativeAnimation { std::string debug_name; std::uint32_t duration_ticks{}; std::uint32_t channel_count{}; };
struct NativeWorldChunk { std::string debug_name; std::vector<AssetId> dependencies; };

struct GpuAsset {
    AssetType type{AssetType::Unknown};
    rhi::BufferHandle vertex_buffer{};
    rhi::BufferHandle index_buffer{};
    rhi::TextureHandle texture{};
    rhi::FenceValue ready_fence{};
    std::uint64_t resident_bytes{};
};

} // namespace shadowpc
