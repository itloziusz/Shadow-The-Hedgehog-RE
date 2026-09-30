#pragma once
#include "shadowpc/core/AssetTypes.hpp"
#include <cstdint>

namespace shadowpc {

struct ResourceHandle {
    std::uint64_t value{};

    static constexpr ResourceHandle make(std::uint32_t index, std::uint32_t generation, AssetType type) {
        return ResourceHandle{
            (std::uint64_t(index)) |
            ((std::uint64_t(generation & 0x00FFFFFFu)) << 32) |
            ((std::uint64_t(static_cast<std::uint8_t>(type))) << 56)
        };
    }

    constexpr std::uint32_t index() const { return std::uint32_t(value); }
    constexpr std::uint32_t generation() const { return std::uint32_t((value >> 32) & 0x00FFFFFFu); }
    constexpr AssetType type() const { return AssetType((value >> 56) & 0xFFu); }
    constexpr explicit operator bool() const { return value != 0; }
    constexpr auto operator<=>(const ResourceHandle&) const = default;
};

} // namespace shadowpc
