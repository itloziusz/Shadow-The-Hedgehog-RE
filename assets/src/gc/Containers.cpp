#include "shadow/gc/Containers.hpp"

#include "shadow/BinaryReader.hpp"

#include <algorithm>

namespace shadow::gc {

AfsArchive parse_afs(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 8 || bytes[0] != 'A' || bytes[1] != 'F' || bytes[2] != 'S' || bytes[3] != 0) {
        throw ParseError("AFS magic is missing");
    }
    BinaryReader reader(bytes, "AFS");
    reader.skip(4);
    AfsArchive archive;
    archive.count = reader.u32le();
    const auto table = checked_mul(archive.count, 8, "AFS table");
    if (8 + table > bytes.size()) {
        throw ParseError("AFS table does not fit in the file");
    }
    archive.entries.resize(archive.count);
    for (AfsEntry& entry : archive.entries) {
        entry.offset = reader.u32le();
        entry.size = reader.u32le();
        if (entry.offset > bytes.size() || entry.size > bytes.size() - static_cast<std::size_t>(entry.offset)) {
            throw ParseError("AFS entry is outside the file");
        }
    }
    return archive;
}

AdxHeader parse_adx(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 20 || bytes[0] != 0x80 || bytes[1] != 0x00) {
        throw ParseError("ADX signature is missing");
    }
    BinaryReader reader(bytes, "ADX");
    reader.skip(2);
    AdxHeader header;
    header.copyright_offset = reader.u16be();
    header.encoding = reader.u8();
    header.block_size = reader.u8();
    header.sample_bit_depth = reader.u8();
    header.channel_count = reader.u8();
    header.sample_rate = reader.u32be();
    header.total_samples = reader.u32be();
    header.highpass_frequency = reader.u16be();
    if (header.copyright_offset >= bytes.size()) {
        throw ParseError("ADX copyright offset is outside the file");
    }
    std::size_t at = header.copyright_offset;
    // The sampled files point at ")CRI", one byte after the copyright mark.
    if (at > 0 && bytes[at] == ')' && at + 4 <= bytes.size()) {
        --at;
    }
    std::size_t end = at;
    while (end < bytes.size() && bytes[end] != 0 && end - at < 64) {
        ++end;
    }
    header.copyright.assign(reinterpret_cast<const char*>(bytes.data() + at), end - at);
    if (header.copyright.find("CRI") == std::string::npos) {
        throw ParseError("ADX copyright offset does not reach a CRI marker");
    }
    return header;
}

SofdecFile parse_sofdec(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 4 || bytes[0] != 0 || bytes[1] != 0 || bytes[2] != 1 || bytes[3] != 0xBA) {
        throw ParseError("Sofdec pack header is missing");
    }
    return SofdecFile{bytes.size()};
}

SetIdTable parse_setid(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 8) {
        throw ParseError("setid file is shorter than its header");
    }
    BinaryReader reader(bytes, "setid");
    const auto word0 = reader.u32le();
    const auto count = reader.u32le();
    if (word0 != 0) {
        throw ParseError("setid header word is not 0");
    }
    const auto body = checked_mul(count, 12, "setid records");
    if (body + 8 != bytes.size()) {
        throw ParseError("setid count does not fill the file");
    }
    SetIdTable table;
    table.records.resize(count);
    for (SetIdRecord& record : table.records) {
        record.word0 = reader.u32le();
        record.word1 = reader.u32le();
        record.word2 = reader.u32le();
    }
    return table;
}

bool looks_sized_blob(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 4) {
        return false;
    }
    const std::uint32_t declared = (static_cast<std::uint32_t>(bytes[0]) << 24) |
                                   (static_cast<std::uint32_t>(bytes[1]) << 16) |
                                   (static_cast<std::uint32_t>(bytes[2]) << 8) |
                                   static_cast<std::uint32_t>(bytes[3]);
    return declared == bytes.size();
}

SizedBlob parse_sized_blob(std::span<const std::uint8_t> bytes) {
    if (!looks_sized_blob(bytes)) {
        throw ParseError("sized blob length word does not match the file");
    }
    BinaryReader reader(bytes, "sized blob");
    SizedBlob blob;
    blob.declared_size = reader.u32be();
    const std::size_t words = std::min<std::size_t>(8, bytes.size() / 4);
    blob.header_words.reserve(words);
    blob.header_words.push_back(blob.declared_size);
    for (std::size_t i = 1; i < words; ++i) {
        blob.header_words.push_back(reader.u32be());
    }
    blob.bytes.assign(bytes.begin(), bytes.end());
    return blob;
}

bool looks_effect(std::span<const std::uint8_t> bytes) {
    return bytes.size() >= 8 && bytes[0] == 'E' && bytes[1] == 'F' && bytes[2] == 'F' && bytes[3] == 'D';
}

EffectFile parse_effect(std::span<const std::uint8_t> bytes) {
    if (!looks_effect(bytes)) {
        throw ParseError("effect magic EFFD is missing");
    }
    BinaryReader reader(bytes, "effect");
    reader.skip(4);
    EffectFile effect;
    effect.version = reader.u32le();
    effect.name = reader.rest_cstring();
    effect.bytes.assign(bytes.begin(), bytes.end());
    return effect;
}

MetricsText parse_metrics(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view kMagic = "METRICS1\n";
    if (bytes.size() < kMagic.size() ||
        std::string_view(reinterpret_cast<const char*>(bytes.data()), kMagic.size()) != kMagic) {
        throw ParseError("METRICS1 signature is missing");
    }
    MetricsText text;
    text.text.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return text;
}

CsdContainer parse_csd(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 4 || bytes[0] != 'C' || bytes[1] != 'P' || bytes[2] != 'A' || bytes[3] != 'F') {
        throw ParseError("CPAF magic is missing");
    }
    return CsdContainer{std::vector<std::uint8_t>(bytes.begin(), bytes.end())};
}

}  // namespace shadow::gc
