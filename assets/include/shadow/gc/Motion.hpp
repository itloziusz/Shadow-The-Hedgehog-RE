#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace shadow::gc {

// Big-endian when endian_flag == 1, little-endian when it is 0.
// 0x80411A9C swaps the multi-byte fields only when this flag disagrees with
// the host. SH.BON / LTE.BON / RTE.BON are flag 1 and their node table is
// 48 + count * 112 bytes, so those files are big-endian.

struct BonNode {
    std::uint32_t bone_id = 0;
    std::uint16_t halves[6]{};
    float translation[4]{};
    float words_32[4]{};
    float words_48[4]{};
    std::uint8_t raw_64[8]{};
    std::uint32_t child_a = 0;
    std::uint32_t child_b = 0;
    std::string name;
};

struct BonFile {
    std::uint8_t version = 0;
    std::uint8_t endian_flag = 0;
    std::uint16_t header_2 = 0;
    std::uint32_t word_4 = 0;
    std::uint16_t header_8 = 0;
    std::uint16_t node_count = 0;
    float header_12 = 0.f;
    std::string name;
    std::vector<BonNode> nodes;
};

// Three big-endian (or little-endian) halfwords. 0x80414B80 reads this shape
// when bit 0x8000 of the first half is clear, and reads one extra half when
// that bit is set. On SHADOW.MTP the group is still 6 bytes when the bit is
// set, so the extra half is not present in those groups.
struct MotionKey {
    std::uint16_t header = 0;
    std::uint16_t component_b = 0;
    std::uint16_t component_c = 0;
    float time = 0.f;
    float value_b = 0.f;
    float value_c = 0.f;
};

struct MotionChunk {
    std::uint32_t size = 0;
    std::uint16_t group_count = 0;
    std::uint8_t prefix_length = 0;
    std::uint8_t link = 0;
    bool groups_split = false;
    std::vector<std::uint8_t> prefix;
    std::vector<MotionKey> keys;
    std::vector<std::uint8_t> payload;
};

struct Motion {
    std::uint8_t version = 0;
    std::uint8_t endian_flag = 0;
    std::uint32_t word_4 = 0;
    std::uint32_t byte_size = 0;
    std::uint8_t flag_13 = 0;
    std::uint16_t word_16 = 0;
    std::uint16_t word_18 = 0;
    std::string name;
    std::vector<MotionChunk> chunks;
};

struct MotionExtraRecord {
    std::uint16_t flags = 0;
    std::uint16_t stride = 0;
    std::vector<std::uint8_t> body;
};

struct MotionPackEntry {
    std::string name;
    Motion motion;
    std::vector<MotionExtraRecord> extra;
    // Bytes of the third pointer that were not a stride-terminated record list.
    std::vector<std::uint8_t> extra_tail;
    // The third pointer's bytes, in file order, including the terminator.
    std::vector<std::uint8_t> extra_bytes;
};

struct MotionPack {
    std::uint16_t word_0 = 0;
    std::uint16_t motion_count = 0;
    std::uint32_t word_4 = 0;
    std::uint32_t word_8 = 0;
    std::uint32_t word_12 = 0;
    std::uint8_t endian_flag = 0;
    std::vector<MotionPackEntry> entries;
};

// Conversions used by 0x80414B80. The constants are the doubles/floats at r2+5848 and r2+5840.
float decode_motion_u14(std::uint16_t bits);
float decode_motion_f16(std::uint16_t bits);

BonFile parse_bon(std::span<const std::uint8_t> bytes);
Motion parse_motion(std::span<const std::uint8_t> bytes);
MotionPack parse_motion_pack(std::span<const std::uint8_t> bytes);

bool looks_like_bon(std::span<const std::uint8_t> bytes);
bool looks_like_motion(std::span<const std::uint8_t> bytes);
bool looks_like_motion_pack(std::span<const std::uint8_t> bytes);

// Structural problems. An empty list means the checks below passed.
std::vector<std::string> bon_problems(const BonFile& bon);
std::vector<std::string> motion_problems(const Motion& motion);
std::vector<std::string> skin_problems(std::uint32_t bone_count,
                                       std::uint32_t vertex_count,
                                       std::span<const std::uint8_t> indices,
                                       std::span<const float> weights);

}  // namespace shadow::gc
