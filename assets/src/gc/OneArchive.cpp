#include "shadow/gc/OneArchive.hpp"

#include "shadow/BinaryReader.hpp"
#include "shadow/gc/Prs.hpp"

#include <cstdlib>
#include <cstring>

namespace shadow::gc {
namespace {

// Exact bits loaded by 0x8004CD94 from r2-31224 (r2 = 0x805FA780).
constexpr std::uint32_t kVersionSplitBits = 0x3F170A3Du;

float version_split() {
    float value = 0.f;
    const auto bits = kVersionSplitBits;
    static_assert(sizeof(value) == sizeof(bits));
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

float parse_version(std::span<const std::uint8_t> field) {
    std::string text(reinterpret_cast<const char*>(field.data()), field.size());
    const auto end = text.find('\0');
    if (end != std::string::npos) {
        text.resize(end);
    }
    constexpr std::string_view kPrefix = "One Ver ";
    if (text.size() < kPrefix.size() || text.compare(0, kPrefix.size(), kPrefix) != 0) {
        throw ParseError("ONE header is not 'One Ver %f'");
    }
    const std::string number = text.substr(kPrefix.size());
    if (number.empty()) {
        throw ParseError("ONE version number is missing");
    }
    char* stop = nullptr;
    const float value = std::strtof(number.c_str(), &stop);
    if (stop == number.c_str() || *stop != '\0') {
        throw ParseError("ONE version number is not a float");
    }
    return value;
}

std::string read_name(std::span<const std::uint8_t> record, std::size_t field_bytes) {
    std::size_t n = 0;
    while (n < field_bytes && record[n] != 0) {
        ++n;
    }
    return std::string(reinterpret_cast<const char*>(record.data()), n);
}

}  // namespace

std::string uppercase_ascii(std::string_view text) {
    std::string out(text);
    for (char& ch : out) {
        const auto u = static_cast<unsigned char>(ch);
        if (u >= 'a' && u <= 'z') {
            ch = static_cast<char>(u - 32);
        }
    }
    return out;
}

const OneIndexEntry* OneArchive::find(std::string_view name) const {
    const std::string folded = uppercase_ascii(name);
    for (const OneIndexEntry& entry : entries) {
        if (uppercase_ascii(entry.name) == folded) {
            return &entry;
        }
    }
    return nullptr;
}

std::vector<std::uint8_t> OneArchive::load(const OneIndexEntry& entry) const {
    if (!file) {
        throw ParseError("ONE archive has no file buffer");
    }
    const std::uint64_t absolute = 12ull + static_cast<std::uint64_t>(entry.offset_from_view);
    if (absolute > file->size()) {
        throw ParseError("ONE entry offset is outside the file: " + entry.name);
    }
    const auto* begin = file->data() + static_cast<std::size_t>(absolute);
    const std::size_t available = file->size() - static_cast<std::size_t>(absolute);
    // Bit 0 is the only flag tested at 0x8004C7D0. Other bits are preserved and do not change the path.
    if ((entry.flags & 1u) != 0) {
        return prs_decompress(std::span<const std::uint8_t>(begin, available), entry.declared_size);
    }
    const std::uint64_t end = absolute + static_cast<std::uint64_t>(entry.declared_size);
    if (end < absolute || end > file->size()) {
        throw ParseError("raw ONE entry exceeds the file: " + entry.name);
    }
    return std::vector<std::uint8_t>(begin, begin + entry.declared_size);
}

OneArchive open_one(std::vector<std::uint8_t> file) {
    return open_one(std::make_shared<const std::vector<std::uint8_t>>(std::move(file)));
}

OneArchive open_one(std::shared_ptr<const std::vector<std::uint8_t>> file) {
    if (!file || file->size() < 0x40) {
        throw ParseError("ONE file is smaller than its header");
    }
    OneArchive archive;
    archive.file = std::move(file);
    const auto bytes = std::span<const std::uint8_t>(archive.file->data(), archive.file->size());
    BinaryReader reader(bytes, "ONE");
    archive.header_word0 = reader.u32le();
    archive.payload_size_field = reader.u32le();
    archive.library_id = reader.u32le();
    if (archive.header_word0 != 0) {
        throw ParseError("ONE word0 is not 0");
    }
    if (static_cast<std::uint64_t>(archive.payload_size_field) + 12ull != bytes.size()) {
        throw ParseError("ONE payload size field does not equal file size - 12");
    }
    const float version = parse_version(bytes.subspan(0x0C, 16));
    archive.version = version;
    archive.declared_count = reader.u32le_at(0x1C);

    const bool old_layout = version <= version_split();
    const std::size_t field = old_layout ? 0x20 : 0x2C;
    const std::uint64_t table_bytes =
        (static_cast<std::uint64_t>(archive.declared_count) + 2ull) * 0x38ull;
    if (0x40ull + table_bytes > bytes.size()) {
        throw ParseError("ONE directory does not fit in the file");
    }

    archive.entries.reserve(archive.declared_count);
    for (std::uint32_t index = 2; index < archive.declared_count + 2; ++index) {
        const std::size_t record_at = 0x40 + static_cast<std::size_t>(index) * 0x38;
        auto record = bytes.subspan(record_at, 0x38);
        OneIndexEntry entry;
        entry.index = index;
        entry.name = read_name(record, field);
        entry.declared_size = static_cast<std::uint32_t>(record[field] | (record[field + 1] << 8) |
                                                          (record[field + 2] << 16) | (record[field + 3] << 24));
        entry.offset_from_view = static_cast<std::uint32_t>(record[field + 4] | (record[field + 5] << 8) |
                                                             (record[field + 6] << 16) | (record[field + 7] << 24));
        entry.flags = static_cast<std::uint32_t>(record[field + 8] | (record[field + 9] << 8) |
                                                  (record[field + 10] << 16) | (record[field + 11] << 24));
        archive.entries.push_back(std::move(entry));
    }
    return archive;
}

}  // namespace shadow::gc
