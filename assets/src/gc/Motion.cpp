#include "shadow/gc/Motion.hpp"

#include "shadow/BinaryReader.hpp"

#include <cmath>
#include <cstring>

namespace shadow::gc {
namespace {

struct Words {
    bool big = true;

    std::uint16_t u16(std::span<const std::uint8_t> bytes, std::size_t offset) const {
        if (offset > bytes.size() || bytes.size() - offset < 2) {
            throw ParseError("motion field is truncated");
        }
        const auto a = bytes[offset];
        const auto b = bytes[offset + 1];
        if (big) {
            return static_cast<std::uint16_t>((a << 8) | b);
        }
        return static_cast<std::uint16_t>(a | (b << 8));
    }

    std::uint32_t u32(std::span<const std::uint8_t> bytes, std::size_t offset) const {
        if (offset > bytes.size() || bytes.size() - offset < 4) {
            throw ParseError("motion field is truncated");
        }
        const auto a = bytes[offset];
        const auto b = bytes[offset + 1];
        const auto c = bytes[offset + 2];
        const auto d = bytes[offset + 3];
        if (big) {
            return (static_cast<std::uint32_t>(a) << 24) | (static_cast<std::uint32_t>(b) << 16) |
                   (static_cast<std::uint32_t>(c) << 8) | d;
        }
        return static_cast<std::uint32_t>(a) | (static_cast<std::uint32_t>(b) << 8) |
               (static_cast<std::uint32_t>(c) << 16) | (static_cast<std::uint32_t>(d) << 24);
    }

    float f32(std::span<const std::uint8_t> bytes, std::size_t offset) const {
        const auto bits = u32(bytes, offset);
        float value = 0.f;
        static_assert(sizeof(value) == sizeof(bits));
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }
};

Words words_from_flag(std::uint8_t flag) {
    if (flag == 1) {
        return Words{true};
    }
    if (flag == 0) {
        return Words{false};
    }
    throw ParseError("motion endian flag is not 0 or 1");
}

std::string field_name(std::span<const std::uint8_t> bytes, std::size_t offset, std::size_t width) {
    if (offset > bytes.size() || bytes.size() - offset < width) {
        throw ParseError("name field is truncated");
    }
    std::size_t n = 0;
    while (n < width && bytes[offset + n] != 0) {
        ++n;
    }
    return std::string(reinterpret_cast<const char*>(bytes.data() + offset), n);
}

void read_node(const Words& words, std::span<const std::uint8_t> bytes, std::size_t offset, BonNode& node) {
    if (offset > bytes.size() || bytes.size() - offset < 112) {
        throw ParseError("BON node is truncated");
    }
    const auto node_bytes = bytes.subspan(offset, 112);
    node.bone_id = words.u32(node_bytes, 0);
    for (int i = 0; i < 6; ++i) {
        node.halves[i] = words.u16(node_bytes, static_cast<std::size_t>(4 + i * 2));
    }
    for (int i = 0; i < 4; ++i) {
        node.translation[i] = words.f32(node_bytes, static_cast<std::size_t>(16 + i * 4));
        node.words_32[i] = words.f32(node_bytes, static_cast<std::size_t>(32 + i * 4));
        node.words_48[i] = words.f32(node_bytes, static_cast<std::size_t>(48 + i * 4));
    }
    std::memcpy(node.raw_64, node_bytes.data() + 64, 8);
    node.child_a = words.u32(node_bytes, 72);
    node.child_b = words.u32(node_bytes, 76);
    node.name = field_name(node_bytes, 80, 32);
}

MotionChunk read_chunk(const Words& words, std::span<const std::uint8_t> object, std::size_t pos) {
    MotionChunk chunk;
    chunk.size = words.u32(object, pos);
    chunk.group_count = words.u16(object, pos + 4);
    chunk.prefix_length = object[pos + 6];
    chunk.link = object[pos + 7];
    if (chunk.size < 8 || pos > object.size() || object.size() - pos < chunk.size) {
        throw ParseError("motion chunk size leaves the object");
    }
    const auto payload = object.subspan(pos + 8, chunk.size - 8);
    chunk.payload.assign(payload.begin(), payload.end());
    const std::size_t grouped = static_cast<std::size_t>(chunk.prefix_length) +
                                static_cast<std::size_t>(chunk.group_count) * 6u;
    if (grouped == payload.size() && chunk.prefix_length <= payload.size()) {
        chunk.groups_split = true;
        chunk.prefix.assign(payload.begin(), payload.begin() + chunk.prefix_length);
        chunk.keys.resize(chunk.group_count);
        std::size_t cursor = chunk.prefix_length;
        for (MotionKey& key : chunk.keys) {
            key.header = words.u16(payload, cursor);
            key.component_b = words.u16(payload, cursor + 2);
            key.component_c = words.u16(payload, cursor + 4);
            key.time = decode_motion_u14(key.header);
            key.value_b = decode_motion_f16(key.component_b);
            key.value_c = decode_motion_f16(key.component_c);
            cursor += 6;
        }
    }
    return chunk;
}

Motion parse_motion_at(std::span<const std::uint8_t> bytes, bool require_exact_file) {
    if (bytes.size() < 52) {
        throw ParseError("motion object is shorter than its header");
    }
    if (bytes[0] < 3) {
        throw ParseError("motion version byte is below 3");
    }
    const Words words = words_from_flag(bytes[1]);
    Motion motion;
    motion.version = bytes[0];
    motion.endian_flag = bytes[1];
    motion.word_4 = words.u32(bytes, 4);
    motion.byte_size = words.u32(bytes, 8);
    motion.flag_13 = bytes[13];
    motion.word_16 = words.u16(bytes, 16);
    motion.word_18 = words.u16(bytes, 18);
    motion.name = field_name(bytes, 20, 32);
    if (motion.byte_size < 52 || motion.byte_size > bytes.size()) {
        throw ParseError("motion size field leaves the buffer");
    }
    if (require_exact_file && motion.byte_size != bytes.size()) {
        throw ParseError("standalone motion size does not equal the file");
    }
    const auto object = bytes.subspan(0, motion.byte_size);
    std::size_t pos = 52;
    while (pos < object.size()) {
        if (object.size() - pos < 8) {
            throw ParseError("motion chunk header is truncated");
        }
        MotionChunk chunk = read_chunk(words, object, pos);
        const auto link = chunk.link;
        pos += chunk.size;
        motion.chunks.push_back(std::move(chunk));
        if (link == 0) {
            break;
        }
    }
    if (pos != object.size()) {
        throw ParseError("motion chunks do not fill the size field");
    }
    return motion;
}

std::vector<MotionExtraRecord> read_extra(const Words& words, std::span<const std::uint8_t> bytes,
                                          std::vector<std::uint8_t>& tail) {
    std::vector<MotionExtraRecord> records;
    std::size_t pos = 0;
    while (pos + 4 <= bytes.size()) {
        const auto flags = words.u16(bytes, pos);
        const auto stride = words.u16(bytes, pos + 2);
        const auto signed_stride = static_cast<std::int16_t>(stride);
        if (signed_stride == -1) {
            if (pos + 4 < bytes.size()) {
                tail.assign(bytes.begin() + static_cast<std::ptrdiff_t>(pos + 4), bytes.end());
            }
            return records;
        }
        if (stride < 4 || pos > bytes.size() || bytes.size() - pos < stride) {
            tail.assign(bytes.begin() + static_cast<std::ptrdiff_t>(pos), bytes.end());
            return records;
        }
        MotionExtraRecord record;
        record.flags = flags;
        record.stride = stride;
        record.body.assign(bytes.begin() + static_cast<std::ptrdiff_t>(pos + 4),
                           bytes.begin() + static_cast<std::ptrdiff_t>(pos + stride));
        records.push_back(std::move(record));
        pos += stride;
    }
    if (pos < bytes.size()) {
        tail.assign(bytes.begin() + static_cast<std::ptrdiff_t>(pos), bytes.end());
    }
    return records;
}

}  // namespace

float decode_motion_u14(std::uint16_t bits) {
    const std::uint32_t low = 0x80000000u | (bits & 0x3FFFu);
    const std::uint64_t pattern = (static_cast<std::uint64_t>(0x43300000u) << 32) | low;
    const std::uint64_t magic_bits = (static_cast<std::uint64_t>(0x43300000u) << 32) | 0x80000000u;
    double value = 0.0;
    double magic = 0.0;
    static_assert(sizeof(value) == sizeof(pattern));
    std::memcpy(&value, &pattern, sizeof(value));
    std::memcpy(&magic, &magic_bits, sizeof(magic));
    return static_cast<float>(value - magic);
}

float decode_motion_f16(std::uint16_t bits) {
    if (bits == 0) {
        return 0.f;
    }
    const std::uint32_t magnitude = static_cast<std::uint32_t>(bits & 0x7FFFu) << 13;
    std::uint32_t word = magnitude + 0x38000000u;
    if ((bits & 0x8000u) != 0) {
        word |= 0x80000000u;
    }
    float value = 0.f;
    std::memcpy(&value, &word, sizeof(value));
    return value;
}

bool looks_like_bon(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 48 || bytes[0] < 4 || (bytes[1] != 0 && bytes[1] != 1)) {
        return false;
    }
    const Words words = words_from_flag(bytes[1]);
    const auto count = words.u16(bytes, 10);
    const auto span = 48u + static_cast<std::uint32_t>(count) * 112u;
    return span == bytes.size();
}

bool looks_like_motion(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 52 || bytes[0] < 3 || (bytes[1] != 0 && bytes[1] != 1)) {
        return false;
    }
    const Words words = words_from_flag(bytes[1]);
    return words.u32(bytes, 8) == bytes.size();
}

bool looks_like_motion_pack(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 20 || (bytes[5] != 0 && bytes[5] != 1)) {
        return false;
    }
    const Words words = words_from_flag(bytes[5]);
    const auto count = words.u16(bytes, 2);
    const auto table = words.u32(bytes, 16);
    const auto bytes_needed = static_cast<std::uint64_t>(table) + static_cast<std::uint64_t>(count) * 12u;
    return table >= 20 && bytes_needed <= bytes.size() && count > 0;
}

BonFile parse_bon(std::span<const std::uint8_t> bytes) {
    if (!looks_like_bon(bytes)) {
        throw ParseError("BON size does not match 48 + node_count * 112");
    }
    const Words words = words_from_flag(bytes[1]);
    BonFile bon;
    bon.version = bytes[0];
    bon.endian_flag = bytes[1];
    bon.header_2 = words.u16(bytes, 2);
    bon.word_4 = words.u32(bytes, 4);
    bon.header_8 = words.u16(bytes, 8);
    bon.node_count = words.u16(bytes, 10);
    bon.header_12 = words.f32(bytes, 12);
    bon.name = field_name(bytes, 16, 32);
    bon.nodes.resize(bon.node_count);
    for (std::uint16_t i = 0; i < bon.node_count; ++i) {
        read_node(words, bytes, 48u + static_cast<std::size_t>(i) * 112u, bon.nodes[i]);
    }
    return bon;
}

Motion parse_motion(std::span<const std::uint8_t> bytes) {
    return parse_motion_at(bytes, true);
}

MotionPack parse_motion_pack(std::span<const std::uint8_t> bytes) {
    if (!looks_like_motion_pack(bytes)) {
        throw ParseError("motion pack directory does not fit");
    }
    const Words words = words_from_flag(bytes[5]);
    MotionPack pack;
    pack.word_0 = words.u16(bytes, 0);
    pack.motion_count = words.u16(bytes, 2);
    pack.word_4 = words.u32(bytes, 4);
    pack.word_8 = words.u32(bytes, 8);
    pack.word_12 = words.u32(bytes, 12);
    pack.endian_flag = bytes[5];
    const auto table = words.u32(bytes, 16);
    pack.entries.resize(pack.motion_count);
    for (std::uint16_t i = 0; i < pack.motion_count; ++i) {
        const std::size_t entry = static_cast<std::size_t>(table) + static_cast<std::size_t>(i) * 12u;
        const auto name_at = words.u32(bytes, entry);
        const auto data_at = words.u32(bytes, entry + 4);
        const auto extra_at = words.u32(bytes, entry + 8);
        if (name_at >= bytes.size() || data_at >= bytes.size() || (extra_at != 0 && extra_at >= bytes.size())) {
            throw ParseError("motion pack offset leaves the file");
        }
        MotionPackEntry& out = pack.entries[i];
        out.name = field_name(bytes, name_at, bytes.size() - name_at);
        out.motion = parse_motion_at(bytes.subspan(data_at), false);
        if (extra_at != 0) {
            std::size_t extra_end = bytes.size();
            for (std::uint16_t other = 0; other < pack.motion_count; ++other) {
                const std::size_t at = static_cast<std::size_t>(table) + static_cast<std::size_t>(other) * 12u;
                for (int word = 0; word < 3; ++word) {
                    const auto pointer = words.u32(bytes, at + static_cast<std::size_t>(word) * 4u);
                    if (pointer > extra_at && pointer < extra_end) {
                        extra_end = pointer;
                    }
                }
            }
            const auto extra_span = bytes.subspan(extra_at, extra_end - extra_at);
            out.extra_bytes.assign(extra_span.begin(), extra_span.end());
            out.extra = read_extra(words, extra_span, out.extra_tail);
        }
    }
    return pack;
}

std::vector<std::string> bon_problems(const BonFile& bon) {
    std::vector<std::string> problems;
    if (bon.nodes.size() != bon.node_count) {
        problems.emplace_back("BON node table does not match the count");
    }
    std::vector<std::uint32_t> seen;
    seen.reserve(bon.nodes.size());
    for (const BonNode& node : bon.nodes) {
        for (const std::uint32_t child : {node.child_a, node.child_b}) {
            if (child == 0) {
                continue;
            }
            const bool aligned = child >= 48 && ((child - 48) % 112u) == 0;
            const auto index = aligned ? (child - 48) / 112u : 0;
            if (!aligned || index >= bon.nodes.size()) {
                problems.emplace_back("BON child offset is outside the node table: " + node.name);
            }
        }
        if (!node.name.empty()) {
            for (const std::uint32_t id : seen) {
                if (id == node.bone_id) {
                    problems.emplace_back("BON bone id is duplicated: " + node.name);
                }
            }
            seen.push_back(node.bone_id);
        }
    }
    return problems;
}

std::vector<std::string> motion_problems(const Motion& motion) {
    std::vector<std::string> problems;
    if (motion.chunks.empty()) {
        problems.emplace_back("motion has no chunks");
    }
    for (const MotionChunk& chunk : motion.chunks) {
        if (!chunk.groups_split) {
            continue;
        }
        if (chunk.keys.size() != chunk.group_count) {
            problems.emplace_back("motion group count does not match the decoded keys");
        }
        for (const MotionKey& key : chunk.keys) {
            if (!std::isfinite(key.time) || !std::isfinite(key.value_b) || !std::isfinite(key.value_c)) {
                problems.emplace_back("motion key is not finite");
                break;
            }
        }
    }
    return problems;
}

std::vector<std::string> skin_problems(std::uint32_t bone_count, std::uint32_t vertex_count,
                                       std::span<const std::uint8_t> indices, std::span<const float> weights) {
    std::vector<std::string> problems;
    const auto index_count = static_cast<std::uint64_t>(vertex_count) * 4u;
    if (indices.size() != index_count || weights.size() != index_count) {
        problems.emplace_back("skin index or weight count is not 4 per vertex");
        return problems;
    }
    for (std::uint32_t vertex = 0; vertex < vertex_count; ++vertex) {
        float sum = 0.f;
        for (int slot = 0; slot < 4; ++slot) {
            const std::size_t at = static_cast<std::size_t>(vertex) * 4u + static_cast<std::size_t>(slot);
            const float weight = weights[at];
            if (!std::isfinite(weight) || weight < -1.0e-4f || weight > 1.0001f) {
                problems.emplace_back("skin weight is outside 0..1");
                return problems;
            }
            if (indices[at] >= bone_count && weight > 1.0e-6f) {
                problems.emplace_back("skin bone index is outside the skeleton");
                return problems;
            }
            sum += weight;
        }
        if (std::fabs(sum - 1.f) > 1.0e-3f) {
            problems.emplace_back("skin weight sum is outside the expected range");
            return problems;
        }
    }
    return problems;
}

}  // namespace shadow::gc
