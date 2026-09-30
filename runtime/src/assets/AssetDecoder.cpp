#include "shadowpc/assets/AssetDecoder.hpp"
#include <stdexcept>
namespace shadowpc {
void AssetDecoderRegistry::register_decoder(std::unique_ptr<IAssetDecoder> decoder) {
    if (!decoder) throw std::invalid_argument("null asset decoder");
    decoders_[decoder->type()] = std::move(decoder);
}
const IAssetDecoder* AssetDecoderRegistry::find(AssetType type) const noexcept {
    const auto it=decoders_.find(type); return it==decoders_.end()?nullptr:it->second.get();
}
}
