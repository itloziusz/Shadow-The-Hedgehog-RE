#pragma once

#include "shadow/pc/NativeAssets.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace shadow::pc {

// Twelve floats: right, up, at, position. Row-vector product from 0x8047FC08
// and the point transform at 0x804834A8. The flag word at matrix +12 is not
// stored here; the vertex loops do not read it.

std::array<float, 12> affine_identity();

// dst = left * right. 0x8047FC08 (r3 = r4 * r5) and the same product inside
// 0x80451DBC.
std::array<float, 12> multiply_affine(const std::array<float, 12>& left, const std::array<float, 12>& right);

// Drop the flag/pad word after each xyz of a 16-float skin block.
std::array<float, 12> basis_from_skin(const std::array<float, 16>& block);

// 0x804834A8. Translation is included.
std::array<float, 3> transform_point(const std::array<float, 3>& point, const std::array<float, 12>& matrix);

// 0x80483554. The same 3x3, translation omitted, no renormalize.
std::array<float, 3> transform_direction(const std::array<float, 3>& direction, const std::array<float, 12>& matrix);

// One matrix per HAnim node, in node order. Slot i is the frame global whose
// HAnim id equals node i's id. The hierarchy frame is included. 0x8044E4C4
// indexes the hierarchy matrix array by the used-bone byte, which is this slot.
struct HierarchySlots {
    std::vector<std::array<float, 12>> matrices;
    std::vector<std::string> problems;
};

HierarchySlots hierarchy_slot_globals(const NativeModel& model, std::span<const std::array<float, 12>> frame_globals);

// Branch of 0x8044E394. The word at the hierarchy object +0 selects it.
// Bit 0x2 and bit 0x4000 are the tests at 0x8044E3BC and 0x8044E48C.
// max_weights and the word at skin +44 select the extra matrix
// (0x8044E3C8, 0x8044E498, 0x8044E534). skin +44 is 0 after the 76-byte
// zero fill at 0x8044CE9C. The matrix at 0x805E428C is BSS; callers pass it
// as `extra`. It is not a constant in the DOL.
enum class PaletteBranch : std::uint8_t {
    SkinBoneThenChosen,                 // bit 0x2: (skin * boneLtm) * chosen
    SkinHierarchy,                      // bit 0x4000 and (max_weights > 1 or word44 == 3)
    SkinHierarchyFrameExtra,            // bit 0x4000, max_weights <= 1, word44 != 3
    SkinHierarchyChosen,                // both bits clear: skin * (hierarchy * chosen)
};

struct SkinPalette {
    std::vector<std::array<float, 12>> matrices;
    std::vector<std::uint8_t> written;
    PaletteBranch branch = PaletteBranch::SkinHierarchyChosen;
    bool chosen_is_frame_ltm = false;
    std::vector<std::string> problems;
};

// `hierarchy_slots[bone]` is the matrix array at hierarchy+8.
// `bone_ltms[bone]` is the frame matrix 0x804882E8 returns on the bit-0x2 path.
// `frame_ltm` is the incoming r5 of 0x8044E394. `extra` is the matrix at 0x805E428C.
SkinPalette build_skin_palette(const NativeSkin& skin,
                               std::span<const std::array<float, 12>> hierarchy_slots,
                               std::span<const std::array<float, 12>> bone_ltms,
                               const std::array<float, 12>& frame_ltm,
                               const std::array<float, 12>& extra,
                               std::uint32_t hierarchy_flags,
                               std::uint32_t skin_word_44);

// 0x8044F494 sorts the four file slots by weight, descending, stable on ties.
// 0x8044F5C8 then, when max_weights > 1, stores trunc(weight * 128) as a byte
// (GQR5 is 0x07040000: unsigned 8-bit, scale 7, so the blend divides by 128)
// and adds 1 to earlier bytes until the low 8 bits of the sum equal 128.
// max_weights == 1 keeps the first sorted index and does not quantize.
// max_weights outside 1..4 is the dispatcher fall-through at 0x8044E660: no blend.
struct PreparedInfluences {
    std::uint32_t max_weights = 0;
    std::size_t vertex_count = 0;
    std::vector<std::uint8_t> bones;
    std::vector<std::uint8_t> weight_bytes;
    std::vector<std::string> problems;
};

PreparedInfluences prepare_influences(const NativeSkin& skin, std::size_t vertex_count);

struct SkinnedMesh {
    std::vector<std::array<float, 3>> positions;
    std::vector<std::array<float, 3>> normals;
    std::vector<std::string> problems;
};

// 0x8044E684, 0x80451FB8, 0x804522A4, 0x80452668.
// out = w0*(v*M0) + w1*(v*M1) + ... A later weight is applied only when it is
// > 0 and every earlier weight after the first was > 0. The first weight is
// always applied. max_weights == 1 does not multiply by a weight (0x804834A8).
// Normals use transform_direction and the same weights. They are not renormalized.
SkinnedMesh skin_vertices(const NativeMesh& mesh, const SkinPalette& palette, const PreparedInfluences& prepared);

struct InfluenceTerm {
    std::uint8_t bone = 0;
    float weight = 0.f;
    bool applied = false;
    std::array<float, 3> transformed{};
    std::array<float, 3> contribution{};
};

struct VertexRecord {
    std::array<float, 3> source_position{};
    std::array<float, 3> skinned_position{};
    std::array<float, 3> source_normal{};
    std::array<float, 3> skinned_normal{};
    bool has_normal = false;
    std::vector<InfluenceTerm> terms;
    std::vector<std::string> problems;
};

VertexRecord skin_vertex(const NativeMesh& mesh,
                         std::size_t vertex,
                         const SkinPalette& palette,
                         const PreparedInfluences& prepared);

}  // namespace shadow::pc
