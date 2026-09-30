#include "shadow/gc/Fst.hpp"

#include "shadow/BinaryReader.hpp"

namespace shadow::gc {

const FstNode* Fst::find(std::string_view path) const {
    for (const FstNode& node : nodes) {
        if (node.path == path) {
            return &node;
        }
    }
    return nullptr;
}

Fst open_fst(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 12) {
        throw ParseError("FST is smaller than one entry");
    }
    BinaryReader reader(bytes, "FST");
    const std::uint32_t root_word = reader.u32be();
    const std::uint32_t root_param0 = reader.u32be();
    const std::uint32_t root_next = reader.u32be();
    if ((root_word >> 24) != 1u) {
        throw ParseError("FST root is not a directory");
    }
    if (root_next == 0 || root_next > (bytes.size() / 12)) {
        throw ParseError("FST entry count does not fit");
    }
    const std::size_t count = root_next;
    const std::size_t string_base = count * 12;
    if (string_base > bytes.size()) {
        throw ParseError("FST string table is outside the file");
    }

    Fst fst;
    fst.nodes.resize(count);
    fst.nodes[0].is_directory = true;
    fst.nodes[0].parent = root_param0;
    fst.nodes[0].size_or_next = root_next;
    fst.nodes[0].name = std::string();
    fst.nodes[0].path = std::string();

    for (std::size_t i = 1; i < count; ++i) {
        const std::size_t at = i * 12;
        const std::uint32_t word = (static_cast<std::uint32_t>(bytes[at]) << 24) |
                                   (static_cast<std::uint32_t>(bytes[at + 1]) << 16) |
                                   (static_cast<std::uint32_t>(bytes[at + 2]) << 8) |
                                   static_cast<std::uint32_t>(bytes[at + 3]);
        const std::uint32_t param0 = (static_cast<std::uint32_t>(bytes[at + 4]) << 24) |
                                     (static_cast<std::uint32_t>(bytes[at + 5]) << 16) |
                                     (static_cast<std::uint32_t>(bytes[at + 6]) << 8) |
                                     static_cast<std::uint32_t>(bytes[at + 7]);
        const std::uint32_t param1 = (static_cast<std::uint32_t>(bytes[at + 8]) << 24) |
                                     (static_cast<std::uint32_t>(bytes[at + 9]) << 16) |
                                     (static_cast<std::uint32_t>(bytes[at + 10]) << 8) |
                                     static_cast<std::uint32_t>(bytes[at + 11]);
        const std::uint32_t name_offset = word & 0x00FFFFFFu;
        const std::size_t name_at = string_base + static_cast<std::size_t>(name_offset);
        if (name_at >= bytes.size()) {
            throw ParseError("FST name offset is outside the string table");
        }
        std::size_t name_end = name_at;
        while (name_end < bytes.size() && bytes[name_end] != 0) {
            ++name_end;
        }
        if (name_end == bytes.size()) {
            throw ParseError("FST name is not terminated");
        }
        FstNode& node = fst.nodes[i];
        node.is_directory = (word >> 24) == 1u;
        node.name.assign(reinterpret_cast<const char*>(bytes.data() + name_at), name_end - name_at);
        if (node.is_directory) {
            node.parent = param0;
            node.size_or_next = param1;
            if (param1 > count) {
                throw ParseError("FST directory next-index is out of range");
            }
        } else {
            node.disc_offset = param0;
            node.size_or_next = param1;
        }
    }

    // Direct children of a directory occupy [index+1, next). Nested directories
    // jump forward by their own next-index. This is the FST tree, not a flat cap.
    const auto assign_paths = [&](auto&& self, std::size_t dir) -> void {
        std::size_t cursor = dir + 1;
        const std::size_t end = fst.nodes[dir].size_or_next;
        while (cursor < end) {
            FstNode& child = fst.nodes[cursor];
            child.parent = static_cast<std::uint32_t>(dir);
            if (fst.nodes[dir].path.empty()) {
                child.path = child.name;
            } else {
                child.path = fst.nodes[dir].path + "/" + child.name;
            }
            if (child.is_directory) {
                self(self, cursor);
                cursor = child.size_or_next;
            } else {
                ++cursor;
            }
        }
    };
    assign_paths(assign_paths, 0);
    return fst;
}

}  // namespace shadow::gc
