#include "shadow/pc/ResourceSystem.hpp"

#include "shadow/BinaryReader.hpp"
#include "shadow/gc/Containers.hpp"
#include "shadow/pc/Decode.hpp"

#include <algorithm>

namespace shadow::pc {
namespace {

std::string key_for(const LoadRequest& request) {
    if (request.entry.empty()) {
        return request.path;
    }
    return request.path + ":" + request.entry;
}

bool chunk_fits(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() < 12) {
        return false;
    }
    const std::uint32_t size = static_cast<std::uint32_t>(bytes[4] | (bytes[5] << 8) | (bytes[6] << 16) | (bytes[7] << 24));
    return static_cast<std::uint64_t>(size) + 12ull == bytes.size() || bytes.size() > 12;
}

std::vector<Dependency> texture_dependencies(const NativeModel& model, const std::string& archive,
                                             gc::OneArchive* one, std::unordered_map<std::string, NativeTextureDictionary>* dicts) {
    std::vector<Dependency> dependencies;
    for (const NativeMesh& mesh : model.meshes) {
        for (const NativeMaterial& material : mesh.materials) {
            if (!material.has_texture || material.texture_name.empty()) {
                continue;
            }
            Dependency edge;
            edge.kind = "texture";
            edge.archive = archive;
            edge.name = material.texture_name;
            if (one && dicts) {
                for (const auto& [entry_name, dictionary] : *dicts) {
                    for (const NativeTexture& texture : dictionary.textures) {
                        if (texture.name == material.texture_name) {
                            edge.entry = entry_name;
                            edge.resolved = true;
                            break;
                        }
                    }
                    if (edge.resolved) {
                        break;
                    }
                }
            }
            dependencies.push_back(std::move(edge));
        }
    }
    return dependencies;
}

}  // namespace

ResourceSystem::ResourceSystem(std::filesystem::path content_root) : root_(std::move(content_root)) {}

void ResourceSystem::load_fst(const std::filesystem::path& fst_file) {
    const auto bytes = shadow::read_binary_file(fst_file.string());
    fst_ = gc::open_fst(bytes);
}

void ResourceSystem::enqueue(LoadRequest request) { queue_.push_back(std::move(request)); }

void ResourceSystem::pump() {
    std::vector<LoadRequest> batch;
    batch.swap(queue_);
    for (const LoadRequest& request : batch) {
        load(request);
    }
}

ResourceSystem::CachedArchive& ResourceSystem::archive_for(const std::string& relative_path) {
    const auto found = archives_.find(relative_path);
    if (found != archives_.end()) {
        return found->second;
    }
    const auto absolute = root_ / relative_path;
    auto bytes = shadow::read_binary_file(absolute.string());
    CachedArchive cached;
    cached.archive = gc::open_one(std::move(bytes));
    for (const gc::OneIndexEntry& entry : cached.archive.entries) {
        auto payload = cached.archive.load(entry);
        if (payload.size() >= 12 && gc::root_chunk_id(payload) == gc::kRwTexDictionary) {
            cached.texture_dictionaries.emplace(entry.name, to_native_textures(gc::parse_tex_dictionary(payload), entry.name));
        }
    }
    auto inserted = archives_.emplace(relative_path, std::move(cached));
    return inserted.first->second;
}

Asset ResourceSystem::decode_bytes(std::string name, const std::vector<std::uint8_t>& bytes) const {
    if (bytes.size() >= 12 && chunk_fits(bytes)) {
        const std::uint32_t id = gc::root_chunk_id(bytes);
        if (id == gc::kRwClump) {
            return to_native_model(gc::parse_clump(bytes), std::move(name));
        }
        if (id == gc::kRwTexDictionary) {
            return to_native_textures(gc::parse_tex_dictionary(bytes), std::move(name));
        }
        if (id == gc::kRwWorld || id == gc::kRwRegion) {
            const gc::Region region = gc::parse_world_or_region(bytes);
            const std::string world_name = region.name.empty() ? name : region.name;
            return to_native_world(region.world, world_name);
        }
        if (id == gc::kRwUvAnimDict) {
            const auto dictionary = gc::parse_uvanim_dictionary(bytes);
            NativeBlob blob;
            blob.kind = "uvanim";
            blob.name = std::move(name);
            blob.bytes.assign(bytes.begin(), bytes.end());
            blob.notes.push_back("animations=" + std::to_string(dictionary.count));
            return blob;
        }
        if (id == gc::kRwDeltaMorph) {
            NativeBlob blob;
            blob.kind = "deltamorph";
            blob.name = std::move(name);
            blob.bytes.assign(bytes.begin(), bytes.end());
            return blob;
        }
    }
    if (gc::looks_effect(bytes)) {
        const auto effect = gc::parse_effect(bytes);
        NativeBlob blob;
        blob.kind = "effect";
        blob.name = effect.name.empty() ? name : effect.name;
        blob.bytes = effect.bytes;
        blob.notes.push_back("version=" + std::to_string(effect.version));
        return blob;
    }
    if (bytes.size() >= 9 && std::string_view(reinterpret_cast<const char*>(bytes.data()), 9) == "METRICS1\n") {
        NativeBlob blob;
        blob.kind = "metrics";
        blob.name = std::move(name);
        blob.bytes.assign(bytes.begin(), bytes.end());
        return blob;
    }
    if (bytes.size() >= 4 && bytes[0] == 'C' && bytes[1] == 'P' && bytes[2] == 'A' && bytes[3] == 'F') {
        NativeBlob blob;
        blob.kind = "csd";
        blob.name = std::move(name);
        blob.bytes.assign(bytes.begin(), bytes.end());
        return blob;
    }
    if (bytes.size() >= 4 && bytes[0] == 'A' && bytes[1] == 'F' && bytes[2] == 'S' && bytes[3] == 0) {
        const auto archive = gc::parse_afs(bytes);
        NativeBlob blob;
        blob.kind = "afs";
        blob.name = std::move(name);
        blob.notes.push_back("count=" + std::to_string(archive.count));
        return blob;
    }
    if (bytes.size() >= 4 && bytes[0] == 0x80 && bytes[1] == 0x00) {
        const auto header = gc::parse_adx(bytes);
        NativeBlob blob;
        blob.kind = "adx";
        blob.name = std::move(name);
        blob.notes.push_back("rate=" + std::to_string(header.sample_rate));
        blob.notes.push_back("channels=" + std::to_string(header.channel_count));
        return blob;
    }
    if (bytes.size() >= 4 && bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 1 && bytes[3] == 0xBA) {
        NativeBlob blob;
        blob.kind = "sofdec";
        blob.name = std::move(name);
        blob.notes.push_back("bytes=" + std::to_string(bytes.size()));
        return blob;
    }
    if (bytes.size() >= 8) {
        try {
            const auto table = gc::parse_setid(bytes);
            NativeBlob blob;
            blob.kind = "setid";
            blob.name = std::move(name);
            blob.notes.push_back("records=" + std::to_string(table.records.size()));
            return blob;
        } catch (const ParseError&) {
        }
    }
    if (gc::looks_sized_blob(bytes)) {
        const auto sized = gc::parse_sized_blob(bytes);
        NativeBlob blob;
        blob.kind = "sized-blob";
        blob.name = std::move(name);
        blob.bytes = sized.bytes;
        blob.notes.push_back("declared=" + std::to_string(sized.declared_size));
        return blob;
    }
    if (gc::looks_like_motion_pack(bytes)) {
        try {
            return to_native_motion_pack(gc::parse_motion_pack(bytes), std::move(name));
        } catch (const ParseError&) {
        }
    }
    if (gc::looks_like_bon(bytes)) {
        try {
            return to_native_bon(gc::parse_bon(bytes), std::move(name));
        } catch (const ParseError&) {
        }
    }
    if (gc::looks_like_motion(bytes)) {
        try {
            return to_native_motion(gc::parse_motion(bytes), std::move(name));
        } catch (const ParseError&) {
        }
    }
    NativeBlob blob;
    blob.kind = "raw";
    blob.name = std::move(name);
    blob.bytes.assign(bytes.begin(), bytes.end());
    return blob;
}

ResourceHandle ResourceSystem::adopt(std::string key, Asset asset, std::vector<Dependency> dependencies) {
    std::uint32_t index = 0;
    if (!free_slots_.empty()) {
        index = free_slots_.back();
        free_slots_.pop_back();
    } else {
        index = static_cast<std::uint32_t>(slots_.size());
        slots_.push_back(Slot{});
    }
    Slot& slot = slots_[index];
    if (slot.generation == 0) {
        slot.generation = 1;
    }
    slot.live = true;
    slot.asset = std::move(asset);
    slot.dependencies = std::move(dependencies);
    (void)key;
    return ResourceHandle::make(index, slot.generation);
}

ResourceHandle ResourceSystem::load(const LoadRequest& request) {
    const auto absolute = root_ / request.path;
    std::vector<Dependency> dependencies;
    Asset asset;
    if (!request.entry.empty()) {
        CachedArchive& cached = archive_for(request.path);
        const gc::OneIndexEntry* entry = cached.archive.find(request.entry);
        if (!entry) {
            throw ParseError("ONE entry was not found: " + request.entry);
        }
        const auto payload = cached.archive.load(*entry);
        asset = decode_bytes(entry->name, payload);
        if (const auto* model = std::get_if<NativeModel>(&asset)) {
            dependencies = texture_dependencies(*model, request.path, &cached.archive, &cached.texture_dictionaries);
        }
    } else {
        const auto bytes = shadow::read_binary_file(absolute.string());
        if (bytes.size() > 16 && std::string_view(reinterpret_cast<const char*>(bytes.data() + 0x0C), 8) == "One Ver ") {
            const gc::OneArchive archive = gc::open_one(bytes);
            NativeBlob blob;
            blob.kind = "one";
            blob.name = request.path;
            blob.notes.push_back("count=" + std::to_string(archive.declared_count));
            blob.notes.push_back("version=" + std::to_string(archive.version));
            asset = std::move(blob);
        } else {
            asset = decode_bytes(request.path, bytes);
        }
    }
    return adopt(key_for(request), std::move(asset), std::move(dependencies));
}

void ResourceSystem::release(ResourceHandle handle) {
    Slot* slot = slot_for(handle);
    if (!slot) {
        return;
    }
    slot->live = false;
    slot->dependencies.clear();
    slot->asset = NativeBlob{};
    slot->generation += 1;
    if (slot->generation == 0) {
        slot->generation = 1;
    }
    free_slots_.push_back(handle.slot());
}

ResourceSystem::Slot* ResourceSystem::slot_for(ResourceHandle handle) {
    if (!handle.valid() || handle.slot() >= slots_.size()) {
        return nullptr;
    }
    Slot& slot = slots_[handle.slot()];
    if (!slot.live || slot.generation != handle.generation()) {
        return nullptr;
    }
    return &slot;
}

const ResourceSystem::Slot* ResourceSystem::slot_for(ResourceHandle handle) const {
    if (!handle.valid() || handle.slot() >= slots_.size()) {
        return nullptr;
    }
    const Slot& slot = slots_[handle.slot()];
    if (!slot.live || slot.generation != handle.generation()) {
        return nullptr;
    }
    return &slot;
}

const Asset* ResourceSystem::get(ResourceHandle handle) const {
    const Slot* slot = slot_for(handle);
    return slot ? &slot->asset : nullptr;
}

const NativeModel* ResourceSystem::model(ResourceHandle handle) const {
    const Asset* asset = get(handle);
    return asset ? std::get_if<NativeModel>(asset) : nullptr;
}

const NativeTextureDictionary* ResourceSystem::textures(ResourceHandle handle) const {
    const Asset* asset = get(handle);
    return asset ? std::get_if<NativeTextureDictionary>(asset) : nullptr;
}

const NativeWorld* ResourceSystem::world(ResourceHandle handle) const {
    const Asset* asset = get(handle);
    return asset ? std::get_if<NativeWorld>(asset) : nullptr;
}

const NativeBon* ResourceSystem::bon(ResourceHandle handle) const {
    const Asset* asset = get(handle);
    return asset ? std::get_if<NativeBon>(asset) : nullptr;
}

const NativeMotionPack* ResourceSystem::motion_pack(ResourceHandle handle) const {
    const Asset* asset = get(handle);
    return asset ? std::get_if<NativeMotionPack>(asset) : nullptr;
}

const NativeMotion* ResourceSystem::motion(ResourceHandle handle) const {
    const Asset* asset = get(handle);
    return asset ? std::get_if<NativeMotion>(asset) : nullptr;
}

const std::vector<Dependency>& ResourceSystem::dependencies(ResourceHandle handle) const {
    static const std::vector<Dependency> kEmpty;
    const Slot* slot = slot_for(handle);
    return slot ? slot->dependencies : kEmpty;
}

std::vector<std::string> ResourceSystem::stage_files(const std::string& stage_directory) const {
    const auto directory = root_ / stage_directory;
    std::vector<std::string> paths;
    if (!std::filesystem::exists(directory)) {
        throw ParseError("stage directory is missing: " + stage_directory);
    }
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto relative = std::filesystem::relative(entry.path(), root_);
        std::string text = relative.generic_string();
        paths.push_back(std::move(text));
    }
    std::sort(paths.begin(), paths.end());
    return paths;
}

}  // namespace shadow::pc
