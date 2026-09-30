#include "shadowpc/core/AssetRegistry.hpp"

namespace shadowpc {

bool AssetRegistry::add(AssetRecord record) {
    record.canonical_path = canonical_asset_path(record.canonical_path);
    if (!record.id) record.id = make_asset_id(record.canonical_path);
    if (record.guid.lo == 0 && record.guid.hi == 0) record.guid = make_asset_guid(record.canonical_path);
    if (by_id_.contains(record.id) || by_path_.contains(record.canonical_path)) return false;
    const auto index = records_.size();
    by_id_[record.id] = index;
    by_path_[record.canonical_path] = record.id;
    records_.push_back(std::move(record));
    return true;
}

const AssetRecord* AssetRegistry::find(AssetId id) const {
    const auto it = by_id_.find(id);
    return it == by_id_.end() ? nullptr : &records_[it->second];
}

const AssetRecord* AssetRegistry::find_by_path(std::string_view path) const {
    const auto canonical = canonical_asset_path(path);
    const auto it = by_path_.find(canonical);
    return it == by_path_.end() ? nullptr : find(it->second);
}

} // namespace shadowpc
