#pragma once

#include "shadow/gc/Fst.hpp"
#include "shadow/gc/OneArchive.hpp"
#include "shadow/pc/NativeAssets.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace shadow::pc {

// 64-bit handle. The low half is a slot in a growable table. The high half is
// a generation so a recycled slot cannot be used through an old handle.
// This is not a GameCube address.
struct ResourceHandle {
    std::uint64_t value = 0;

    static ResourceHandle make(std::uint32_t slot, std::uint32_t generation) {
        return ResourceHandle{(static_cast<std::uint64_t>(generation) << 32) | slot};
    }
    std::uint32_t slot() const { return static_cast<std::uint32_t>(value); }
    std::uint32_t generation() const { return static_cast<std::uint32_t>(value >> 32); }
    bool valid() const { return generation() != 0; }
};

struct LoadRequest {
    std::string path;
    std::string entry;
};

using Asset = std::variant<NativeModel, NativeTextureDictionary, NativeWorld, NativeBon, NativeMotionPack, NativeMotion, NativeBlob>;

// PC resource cache.
//
// Removed, because they are runtime limits rather than file or gameplay rules:
//   - three simultaneous DVD reads (0x800453E0, table at 0x805742A0)
//   - 32-byte DMA alignment of read buffers (0x800454A4)
//   - 15 child-pending flags on LandALoadManager (this+0xC4)
//   - fixed 0x3C in-memory ONE rows as the only storage
//   - guest 32-bit pointers
// Stage parts on disc reach _98.one, so a 15-slot pending array cannot be the
// format maximum. The queue here has no slot ceiling.
class ResourceSystem {
public:
    explicit ResourceSystem(std::filesystem::path content_root);

    void load_fst(const std::filesystem::path& fst_file);
    const gc::Fst* fst() const { return fst_ ? &*fst_ : nullptr; }

    // Unlimited. Exposed so tests can show it is not the original 3.
    static constexpr std::size_t queue_limit = std::numeric_limits<std::size_t>::max();

    void enqueue(LoadRequest request);
    std::size_t queued() const { return queue_.size(); }
    void pump();

    ResourceHandle load(const LoadRequest& request);
    void release(ResourceHandle handle);

    const Asset* get(ResourceHandle handle) const;
    const NativeModel* model(ResourceHandle handle) const;
    const NativeTextureDictionary* textures(ResourceHandle handle) const;
    const NativeWorld* world(ResourceHandle handle) const;
    const NativeBon* bon(ResourceHandle handle) const;
    const NativeMotionPack* motion_pack(ResourceHandle handle) const;
    const NativeMotion* motion(ResourceHandle handle) const;
    const std::vector<Dependency>& dependencies(ResourceHandle handle) const;

    // Every file under a stage directory. Not clipped to 15.
    std::vector<std::string> stage_files(const std::string& stage_directory) const;

private:
    struct Slot {
        std::uint32_t generation = 1;
        bool live = false;
        std::vector<Dependency> dependencies;
        Asset asset;
    };

    struct CachedArchive {
        gc::OneArchive archive;
        std::unordered_map<std::string, NativeTextureDictionary> texture_dictionaries;
    };

    std::filesystem::path root_;
    std::vector<Slot> slots_;
    std::vector<std::uint32_t> free_slots_;
    std::vector<LoadRequest> queue_;
    std::optional<gc::Fst> fst_;
    std::unordered_map<std::string, CachedArchive> archives_;

    CachedArchive& archive_for(const std::string& relative_path);
    Asset decode_bytes(std::string name, const std::vector<std::uint8_t>& bytes) const;
    ResourceHandle adopt(std::string key, Asset asset, std::vector<Dependency> dependencies);
    Slot* slot_for(ResourceHandle handle);
    const Slot* slot_for(ResourceHandle handle) const;
};

}  // namespace shadow::pc
