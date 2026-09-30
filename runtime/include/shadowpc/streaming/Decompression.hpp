#pragma once
#include "shadowpc/core/AssetTypes.hpp"
#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace shadowpc {

struct DecompressionResult {
    bool ok{};
    std::vector<std::byte> bytes;
    std::string error;
};

DecompressionResult decompress(CompressionCodec codec, std::span<const std::byte> src,
                               std::size_t expected_size = 0);

} // namespace shadowpc
