#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace shadow::gc {

// GameCube FST (sys/fst.bin). Entries are big-endian.
// A directory stores parent index and next-index. A file stores the disc offset and length.
// Paths are std::string. There is no 32-byte or 256-byte path buffer.
struct FstNode {
    bool is_directory = false;
    std::string name;
    std::string path;
    std::uint32_t parent = 0;
    std::uint32_t disc_offset = 0;
    std::uint32_t size_or_next = 0;
};

struct Fst {
    std::vector<FstNode> nodes;
    const FstNode* find(std::string_view path) const;
};

Fst open_fst(std::span<const std::uint8_t> bytes);

}  // namespace shadow::gc
