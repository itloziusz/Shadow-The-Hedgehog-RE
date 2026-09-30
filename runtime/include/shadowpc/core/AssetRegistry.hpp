#pragma once
#include "shadowpc/core/AssetTypes.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace shadowpc {

struct AssetRecord {
    AssetId id{};
    AssetGuid guid{};
    AssetType type{AssetType::Unknown};
    CompressionCodec codec{CompressionCodec::None};
    std::string canonical_path;
    std::string container_path;
    std::uint64_t container_offset{};
    std::uint64_t stored_size{};
    std::uint64_t uncompressed_size{};
    std::uint64_t content_hash{};
    std::vector<AssetId> dependencies;
};

class AssetRegistry {
public:
    bool add(AssetRecord record);
    const AssetRecord* find(AssetId id) const;
    const AssetRecord* find_by_path(std::string_view path) const;
    std::size_t size() const noexcept { return records_.size(); }
    const std::vector<AssetRecord>& records() const noexcept { return records_; }

private:
    std::vector<AssetRecord> records_;
    std::unordered_map<AssetId, std::size_t> by_id_;
    std::unordered_map<std::string, AssetId> by_path_;
};

} // namespace shadowpc
