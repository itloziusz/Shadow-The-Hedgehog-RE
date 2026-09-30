#pragma once
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>

namespace shadowpc {

enum class AssetType : std::uint8_t {
    Unknown=0, Texture, TextureDictionary, Mesh, Model, Material, Skeleton,
    Animation, MorphAnimation, WorldChunk, Collision, Audio, Script, Effect
};

enum class CompressionCodec : std::uint8_t { None=0, PRS=1, LZ4=2, Zstd=3, GDeflate=4 };
using AssetId = std::uint64_t;

struct AssetGuid {
    std::uint64_t lo{};
    std::uint64_t hi{};
    constexpr auto operator<=>(const AssetGuid&) const = default;
};

inline std::string canonical_asset_path(std::string_view in) {
    std::string out;
    out.reserve(in.size());
    bool slash = false;
    for (char ch : in) {
        char c = (ch == '\\') ? '/' : static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (c == '/') {
            if (slash) continue;
            slash = true;
        } else slash = false;
        out.push_back(c);
    }
    while (out.rfind("./", 0) == 0) out.erase(0, 2);
    while (!out.empty() && out.front() == '/') out.erase(out.begin());
    return out;
}

constexpr std::uint64_t fnv1a64(std::string_view s, std::uint64_t seed=14695981039346656037ull) {
    std::uint64_t h = seed;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ull; }
    return h;
}

inline AssetId make_asset_id(std::string_view path) {
    const auto canonical = canonical_asset_path(path);
    return fnv1a64(canonical);
}

inline AssetGuid make_asset_guid(std::string_view path) {
    const auto canonical = canonical_asset_path(path);
    return { fnv1a64(canonical), fnv1a64(canonical, 1099511628211ull ^ 0x9E3779B97F4A7C15ull) };
}

} // namespace shadowpc
