#include "shadow/BinaryReader.hpp"

#include <fstream>

namespace shadow {

std::vector<std::uint8_t> read_binary_file_impl(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw ParseError("cannot open " + path);
    }
    in.seekg(0, std::ios::end);
    const auto end = in.tellg();
    if (end < 0) {
        throw ParseError("cannot size " + path);
    }
    in.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
    if (!bytes.empty()) {
        in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!in) {
            throw ParseError("short read " + path);
        }
    }
    return bytes;
}

}  // namespace shadow
