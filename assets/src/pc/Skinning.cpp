#include "shadow/pc/Skinning.hpp"

#include <cmath>
#include <unordered_map>
#include <utility>

namespace shadow::pc {
namespace {

constexpr float kQuantizeScale = 128.f;

bool finite_basis(const std::array<float, 12>& basis) {
    for (const float value : basis) {
        if (!std::isfinite(value)) {
            return false;
        }
    }
    return true;
}

std::array<float, 3> scale_add(const std::array<float, 3>& accumulated,
                               float weight,
                               const std::array<float, 3>& transformed) {
    return {accumulated[0] + weight * transformed[0], accumulated[1] + weight * transformed[1],
            accumulated[2] + weight * transformed[2]};
}

struct SortedSlot {
    float weight = 0.f;
    std::uint8_t index = 0;
};

void sort_slots(std::array<SortedSlot, 4>& slots) {
    // 0x8044F494. Three passes, each swapping a strictly greater later weight forward.
    for (int pass = 0; pass < 3; ++pass) {
        for (int later = pass + 1; later < 4; ++later) {
            if (slots[static_cast<std::size_t>(later)].weight > slots[static_cast<std::size_t>(pass)].weight) {
                std::swap(slots[static_cast<std::size_t>(later)], slots[static_cast<std::size_t>(pass)]);
            }
        }
    }
}

std::uint8_t quantize_weight(float weight) {
    // fmuls by the 128.0 at r2+6600, then fctiwz, then the low 8 bits (0x8044F5FC).
    const float scaled = weight * kQuantizeScale;
    const auto truncated = static_cast<long long>(std::trunc(scaled));
    return static_cast<std::uint8_t>(truncated & 0xFF);
}

void fix_weight_sum(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t count) {
    // 0x8044F62C. Add one to successive bytes until the low 8 bits of the sum are 128.
    int sum = 0;
    for (std::uint32_t slot = 0; slot < count; ++slot) {
        sum += bytes[offset + slot];
    }
    if ((sum & 0xFF) >= 0x80) {
        return;
    }
    for (std::uint32_t slot = 0; slot < count; ++slot) {
        bytes[offset + slot] = static_cast<std::uint8_t>(bytes[offset + slot] + 1);
        sum += 1;
        if ((sum & 0xFF) == 0x80) {
            return;
        }
    }
}

bool influences_match(const NativeMesh& mesh, const PreparedInfluences& prepared, std::string& problem) {
    if (prepared.vertex_count != mesh.positions.size()) {
        problem = "prepared vertex count does not match the mesh";
        return false;
    }
    if (prepared.max_weights < 1 || prepared.max_weights > 4) {
        problem = "max_weights is outside the blended range 1..4";
        return false;
    }
    if (prepared.bones.size() != prepared.vertex_count * prepared.max_weights) {
        problem = "prepared bone stream length is not vertex_count * max_weights";
        return false;
    }
    if (prepared.max_weights == 1) {
        return true;
    }
    if (prepared.weight_bytes.size() != prepared.bones.size()) {
        problem = "prepared weight stream length does not match the bone stream";
        return false;
    }
    return true;
}

void accumulate_vertex(const std::array<float, 3>& source,
                       bool directional,
                       const SkinPalette& palette,
                       const PreparedInfluences& prepared,
                       std::size_t vertex,
                       std::array<float, 3>& skinned,
                       std::vector<InfluenceTerm>* terms,
                       std::vector<std::string>& problems) {
    const std::uint32_t limit = prepared.max_weights;
    const std::size_t base = vertex * limit;
    std::array<float, 3> accumulated{};
    bool running = true;
    for (std::uint32_t slot = 0; slot < limit; ++slot) {
        const std::uint8_t bone = prepared.bones[base + slot];
        float weight = 1.f;
        if (limit != 1) {
            weight = static_cast<float>(prepared.weight_bytes[base + slot]) / kQuantizeScale;
        }
        const bool gate = slot == 0 || weight > 0.f;
        const bool apply = running && gate;
        if (slot > 0 && !(weight > 0.f)) {
            running = false;
        }
        InfluenceTerm term;
        term.bone = bone;
        term.weight = weight;
        term.applied = false;
        if (apply) {
            if (bone >= palette.matrices.size() || bone >= palette.written.size() || palette.written[bone] == 0) {
                problems.emplace_back("skin influence bone has no palette matrix");
            } else {
                const auto& matrix = palette.matrices[bone];
                term.transformed = directional ? transform_direction(source, matrix) : transform_point(source, matrix);
                term.contribution = limit == 1 ? term.transformed : scale_add({}, weight, term.transformed);
                accumulated = scale_add(accumulated, limit == 1 ? 1.f : weight, term.transformed);
                term.applied = true;
            }
        }
        if (terms != nullptr) {
            terms->push_back(term);
        }
    }
    skinned = accumulated;
}

}  // namespace

std::array<float, 12> affine_identity() {
    return {1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f};
}

std::array<float, 12> multiply_affine(const std::array<float, 12>& left, const std::array<float, 12>& right) {
    std::array<float, 12> out{};
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 3; ++column) {
            float value = left[static_cast<std::size_t>(row * 3)] * right[static_cast<std::size_t>(column)] +
                          left[static_cast<std::size_t>(row * 3 + 1)] * right[static_cast<std::size_t>(3 + column)] +
                          left[static_cast<std::size_t>(row * 3 + 2)] * right[static_cast<std::size_t>(6 + column)];
            if (row == 3) {
                value += right[static_cast<std::size_t>(9 + column)];
            }
            out[static_cast<std::size_t>(row * 3 + column)] = value;
        }
    }
    return out;
}

std::array<float, 12> basis_from_skin(const std::array<float, 16>& block) {
    return {block[0], block[1],  block[2],  block[4],  block[5],  block[6],
            block[8], block[9], block[10], block[12], block[13], block[14]};
}

std::array<float, 3> transform_point(const std::array<float, 3>& point, const std::array<float, 12>& matrix) {
    return {point[0] * matrix[0] + point[1] * matrix[3] + point[2] * matrix[6] + matrix[9],
            point[0] * matrix[1] + point[1] * matrix[4] + point[2] * matrix[7] + matrix[10],
            point[0] * matrix[2] + point[1] * matrix[5] + point[2] * matrix[8] + matrix[11]};
}

std::array<float, 3> transform_direction(const std::array<float, 3>& direction, const std::array<float, 12>& matrix) {
    return {direction[0] * matrix[0] + direction[1] * matrix[3] + direction[2] * matrix[6],
            direction[0] * matrix[1] + direction[1] * matrix[4] + direction[2] * matrix[7],
            direction[0] * matrix[2] + direction[1] * matrix[5] + direction[2] * matrix[8]};
}

HierarchySlots hierarchy_slot_globals(const NativeModel& model, std::span<const std::array<float, 12>> frame_globals) {
    HierarchySlots slots;
    if (model.hierarchies.empty()) {
        slots.problems.emplace_back("model has no HAnim hierarchy");
        return slots;
    }
    if (frame_globals.size() != model.bones.size()) {
        slots.problems.emplace_back("frame global count does not match the frame list");
        return slots;
    }
    const NativeHierarchy& hierarchy = model.hierarchies.front();
    slots.matrices.assign(hierarchy.nodes.size(), affine_identity());
    std::unordered_map<std::int32_t, std::size_t> frame_by_id;
    for (std::size_t frame = 0; frame < model.bones.size(); ++frame) {
        const auto id = model.bones[frame].hanim_id;
        if (id < 0) {
            continue;
        }
        const auto inserted = frame_by_id.emplace(id, frame);
        if (!inserted.second) {
            slots.problems.emplace_back("HAnim id is duplicated on frames");
        }
    }
    for (std::size_t node = 0; node < hierarchy.nodes.size(); ++node) {
        const auto found = frame_by_id.find(hierarchy.nodes[node].id);
        if (found == frame_by_id.end()) {
            slots.problems.emplace_back("HAnim node has no frame for its skin slot");
            continue;
        }
        slots.matrices[node] = frame_globals[found->second];
    }
    return slots;
}

SkinPalette build_skin_palette(const NativeSkin& skin,
                               std::span<const std::array<float, 12>> hierarchy_slots,
                               std::span<const std::array<float, 12>> bone_ltms,
                               const std::array<float, 12>& frame_ltm,
                               const std::array<float, 12>& extra,
                               std::uint32_t hierarchy_flags,
                               std::uint32_t skin_word_44) {
    SkinPalette palette;
    palette.matrices.assign(skin.bone_count, affine_identity());
    palette.written.assign(skin.bone_count, 0);
    const bool bit2 = (hierarchy_flags & 0x2u) != 0;
    const bool bit4000 = (hierarchy_flags & 0x4000u) != 0;
    const bool chosen_is_frame = skin.max_weights == 1 || skin_word_44 == 3;
    palette.chosen_is_frame_ltm = chosen_is_frame;
    const std::array<float, 12>& chosen = chosen_is_frame ? frame_ltm : extra;

    if (bit2) {
        palette.branch = PaletteBranch::SkinBoneThenChosen;
    } else if (bit4000 && (skin.max_weights > 1 || skin_word_44 == 3)) {
        palette.branch = PaletteBranch::SkinHierarchy;
    } else if (bit4000) {
        palette.branch = PaletteBranch::SkinHierarchyFrameExtra;
    } else {
        palette.branch = PaletteBranch::SkinHierarchyChosen;
    }

    if (skin.bone_matrices.size() != skin.bone_count) {
        palette.problems.emplace_back("skin matrix count does not match bone_count");
        return palette;
    }
    if (hierarchy_slots.size() < skin.bone_count) {
        palette.problems.emplace_back("hierarchy matrix array is shorter than bone_count");
        return palette;
    }
    if (palette.branch == PaletteBranch::SkinBoneThenChosen && bone_ltms.size() < skin.bone_count) {
        palette.problems.emplace_back("per-bone LTM array is shorter than bone_count");
        return palette;
    }
    if (skin.used_bones.size() != skin.used_count) {
        palette.problems.emplace_back("used-bone list length does not match used_count");
        return palette;
    }

    const std::array<float, 12> frame_times_extra = multiply_affine(frame_ltm, extra);
    for (const std::uint8_t bone_id : skin.used_bones) {
        if (bone_id >= skin.bone_count) {
            palette.problems.emplace_back("used-bone id is outside bone_count");
            continue;
        }
        const auto skin_basis = basis_from_skin(skin.bone_matrices[bone_id]);
        const auto& hierarchy = hierarchy_slots[bone_id];
        std::array<float, 12> combined = affine_identity();
        switch (palette.branch) {
        case PaletteBranch::SkinBoneThenChosen:
            combined = multiply_affine(multiply_affine(skin_basis, bone_ltms[bone_id]), chosen);
            break;
        case PaletteBranch::SkinHierarchy:
            combined = multiply_affine(skin_basis, hierarchy);
            break;
        case PaletteBranch::SkinHierarchyFrameExtra:
            combined = multiply_affine(skin_basis, multiply_affine(hierarchy, frame_times_extra));
            break;
        case PaletteBranch::SkinHierarchyChosen:
            combined = multiply_affine(skin_basis, multiply_affine(hierarchy, chosen));
            break;
        }
        if (!finite_basis(combined)) {
            palette.problems.emplace_back("palette matrix is not finite");
        }
        palette.matrices[bone_id] = combined;
        palette.written[bone_id] = 1;
    }
    return palette;
}

PreparedInfluences prepare_influences(const NativeSkin& skin, std::size_t vertex_count) {
    PreparedInfluences prepared;
    prepared.max_weights = skin.max_weights;
    prepared.vertex_count = vertex_count;
    if (skin.indices.size() != vertex_count * 4u || skin.weights.size() != vertex_count * 4u) {
        prepared.problems.emplace_back("skin index or weight stream is not four slots per vertex");
        return prepared;
    }
    if (skin.max_weights < 1 || skin.max_weights > 4) {
        prepared.problems.emplace_back("max_weights is outside 1..4, so 0x8044E5A4 does not blend");
        return prepared;
    }
    const std::uint32_t kept = skin.max_weights;
    prepared.bones.resize(vertex_count * kept);
    if (kept > 1) {
        prepared.weight_bytes.resize(vertex_count * kept);
    }
    for (std::size_t vertex = 0; vertex < vertex_count; ++vertex) {
        std::array<SortedSlot, 4> slots{};
        for (int slot = 0; slot < 4; ++slot) {
            const std::size_t index = vertex * 4u + static_cast<std::size_t>(slot);
            slots[static_cast<std::size_t>(slot)].weight = skin.weights[index];
            slots[static_cast<std::size_t>(slot)].index = skin.indices[index];
            if (!std::isfinite(skin.weights[index])) {
                prepared.problems.emplace_back("skin weight is not finite");
            }
        }
        sort_slots(slots);
        const std::size_t base = vertex * kept;
        for (std::uint32_t slot = 0; slot < kept; ++slot) {
            prepared.bones[base + slot] = slots[slot].index;
        }
        if (kept == 1) {
            continue;
        }
        for (std::uint32_t slot = 0; slot < kept; ++slot) {
            const float weight = slots[slot].weight;
            prepared.weight_bytes[base + slot] = std::isfinite(weight) ? quantize_weight(weight) : 0;
        }
        fix_weight_sum(prepared.weight_bytes, base, kept);
    }
    return prepared;
}

SkinnedMesh skin_vertices(const NativeMesh& mesh, const SkinPalette& palette, const PreparedInfluences& prepared) {
    SkinnedMesh skinned;
    skinned.problems = prepared.problems;
    skinned.problems.insert(skinned.problems.end(), palette.problems.begin(), palette.problems.end());
    skinned.positions.resize(mesh.positions.size());
    const bool normals = !mesh.normals.empty();
    if (normals && mesh.normals.size() != mesh.positions.size()) {
        skinned.problems.emplace_back("normal count does not match position count");
    }
    if (normals) {
        skinned.normals.resize(mesh.positions.size());
    }
    std::string mismatch;
    if (!influences_match(mesh, prepared, mismatch)) {
        skinned.problems.push_back(mismatch);
        skinned.positions = mesh.positions;
        skinned.normals = mesh.normals;
        return skinned;
    }
    for (std::size_t vertex = 0; vertex < mesh.positions.size(); ++vertex) {
        accumulate_vertex(mesh.positions[vertex], false, palette, prepared, vertex, skinned.positions[vertex], nullptr,
                          skinned.problems);
        if (normals && mesh.normals.size() == mesh.positions.size()) {
            accumulate_vertex(mesh.normals[vertex], true, palette, prepared, vertex, skinned.normals[vertex], nullptr,
                              skinned.problems);
        }
    }
    return skinned;
}

VertexRecord skin_vertex(const NativeMesh& mesh,
                         std::size_t vertex,
                         const SkinPalette& palette,
                         const PreparedInfluences& prepared) {
    VertexRecord record;
    record.problems = prepared.problems;
    record.problems.insert(record.problems.end(), palette.problems.begin(), palette.problems.end());
    if (vertex >= mesh.positions.size()) {
        record.problems.emplace_back("vertex index is outside the mesh");
        return record;
    }
    std::string mismatch;
    if (!influences_match(mesh, prepared, mismatch)) {
        record.problems.push_back(mismatch);
        return record;
    }
    record.source_position = mesh.positions[vertex];
    accumulate_vertex(record.source_position, false, palette, prepared, vertex, record.skinned_position, &record.terms,
                      record.problems);
    if (vertex < mesh.normals.size()) {
        record.has_normal = true;
        record.source_normal = mesh.normals[vertex];
        accumulate_vertex(record.source_normal, true, palette, prepared, vertex, record.skinned_normal, nullptr,
                          record.problems);
    }
    return record;
}

}  // namespace shadow::pc
