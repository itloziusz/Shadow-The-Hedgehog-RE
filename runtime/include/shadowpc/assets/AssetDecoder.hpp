#pragma once
#include "shadowpc/assets/NativeAssets.hpp"
#include <memory>
#include <span>
#include <unordered_map>
#include <variant>

namespace shadowpc {
using DecodedAsset = std::variant<std::monostate, NativeTexture, NativeMesh, NativeMaterial, NativeSkeleton, NativeAnimation, NativeWorldChunk>;

class IAssetDecoder {
public:
    virtual ~IAssetDecoder() = default;
    virtual AssetType type() const noexcept = 0;
    virtual DecodedAsset decode(AssetId id, std::span<const std::byte> bytes) const = 0;
};

class AssetDecoderRegistry {
public:
    void register_decoder(std::unique_ptr<IAssetDecoder> decoder);
    const IAssetDecoder* find(AssetType type) const noexcept;
private:
    std::unordered_map<AssetType,std::unique_ptr<IAssetDecoder>> decoders_;
};
} // namespace shadowpc
