#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace shadow::gc {

// On-disk ONE directory record ("One Ver 0.50" / "One Ver 0.60").
//
// Traced from main.dol:
//   0x8004CAC0 opens a buffer and points the view at file+12
//   0x8004CD38 sscanf's "One Ver %f" (string at 0x804ACCB4)
//   version <= the float at r2-31224 (bits 0x3F170A3D, about 0.59) uses the
//   three little-endian words at entry+0x20; newer files use entry+0x2C
//   0x80043D34 byte-swaps those words for the big-endian CPU
//   the word at +0x30 is an offset from the view (file+12) and becomes a pointer
//   0x8004CA50 indexes records as view+0x34 + index*56, and rejects index < 2
//   0x8004C770 tests flag bit 0: set means PRS (0x80043FE0), clear means a raw copy
//
// The two leading records are part of the file. They are not a 15-slot or
// 32-byte-aligned runtime table. Entry count is the u32 at file+0x1C.
struct OneIndexEntry {
    std::uint32_t index = 0;
    std::string name;
    std::uint32_t declared_size = 0;
    std::uint32_t offset_from_view = 0;
    std::uint32_t flags = 0;
};

struct OneArchive {
    float version = 0.f;
    std::uint32_t library_id = 0;
    std::uint32_t header_word0 = 0;
    std::uint32_t payload_size_field = 0;
    std::uint32_t declared_count = 0;
    // Kept so a single entry can be inflated later. Offsets are 64-bit sizes
    // of this buffer, not GameCube pointers.
    std::shared_ptr<const std::vector<std::uint8_t>> file;
    std::vector<OneIndexEntry> entries;

    const OneIndexEntry* find(std::string_view name) const;
    std::vector<std::uint8_t> load(const OneIndexEntry& entry) const;
};

OneArchive open_one(std::shared_ptr<const std::vector<std::uint8_t>> file);
OneArchive open_one(std::vector<std::uint8_t> file);

// Original name fold at 0x8004C938: ASCII a-z becomes A-Z. Other bytes are kept.
// The on-disk name field is 32 bytes (ver 0.50) or 44 bytes (ver 0.60). That
// width is the record layout. The returned std::string is not stored in a 44-byte buffer.
std::string uppercase_ascii(std::string_view text);

}  // namespace shadow::gc
