#include "shadow/BinaryReader.hpp"
#include "shadow/gc/Containers.hpp"
#include "shadow/gc/Fst.hpp"
#include "shadow/gc/Motion.hpp"
#include "shadow/gc/OneArchive.hpp"
#include "shadow/gc/RenderWare.hpp"
#include "shadow/pc/Animation.hpp"
#include "shadow/pc/Decode.hpp"
#include "shadow/pc/ResourceSystem.hpp"
#include "shadow/pc/Skinning.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

int failures = 0;

void require(bool condition, const char* text, int line) {
    if (!condition) {
        std::cerr << "FAIL " << line << ": " << text << '\n';
        ++failures;
    }
}

#define REQUIRE(cond) require(static_cast<bool>(cond), #cond, __LINE__)

std::filesystem::path content() { return SHADOW_CONTENT_DIR; }

std::vector<std::uint8_t> file_bytes(const std::filesystem::path& path) {
    return shadow::read_binary_file(path.string());
}

std::uint64_t fnv(const std::vector<std::array<float, 3>>& positions) {
    std::uint64_t hash = 14695981039346656037ull;
    const auto* raw = reinterpret_cast<const std::uint8_t*>(positions.data());
    const std::size_t bytes = positions.size() * sizeof(positions.front());
    for (std::size_t i = 0; i < bytes; ++i) {
        hash ^= raw[i];
        hash *= 1099511628211ull;
    }
    return hash;
}

std::array<float, 12> ref_mul(const std::array<float, 12>& left, const std::array<float, 12>& right) {
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

std::array<float, 3> ref_point(const std::array<float, 3>& point, const std::array<float, 12>& matrix) {
    return {point[0] * matrix[0] + point[1] * matrix[3] + point[2] * matrix[6] + matrix[9],
            point[0] * matrix[1] + point[1] * matrix[4] + point[2] * matrix[7] + matrix[10],
            point[0] * matrix[2] + point[1] * matrix[5] + point[2] * matrix[8] + matrix[11]};
}

std::array<float, 3> ref_direction(const std::array<float, 3>& direction, const std::array<float, 12>& matrix) {
    return {direction[0] * matrix[0] + direction[1] * matrix[3] + direction[2] * matrix[6],
            direction[0] * matrix[1] + direction[1] * matrix[4] + direction[2] * matrix[7],
            direction[0] * matrix[2] + direction[1] * matrix[5] + direction[2] * matrix[8]};
}

float basis_error(const std::array<float, 12>& basis) {
    const float identity[12] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
    float error = 0.f;
    for (int index = 0; index < 12; ++index) {
        error = std::max(error, std::fabs(basis[static_cast<std::size_t>(index)] - identity[index]));
    }
    return error;
}

bool near3(const std::array<float, 3>& left, const std::array<float, 3>& right, float epsilon) {
    return std::fabs(left[0] - right[0]) <= epsilon && std::fabs(left[1] - right[1]) <= epsilon &&
           std::fabs(left[2] - right[2]) <= epsilon;
}

std::array<float, 16> skin_block(float rx, float ry, float rz, float ux, float uy, float uz, float ax, float ay, float az,
                                 float px, float py, float pz) {
    return {rx, ry, rz, 0.f, ux, uy, uz, 0.f, ax, ay, az, 0.f, px, py, pz, 0.f};
}

void check_palette_branches() {
    shadow::pc::NativeSkin skin;
    skin.bone_count = 1;
    skin.used_count = 1;
    skin.used_bones = {0};
    skin.max_weights = 2;
    skin.bone_matrices = {skin_block(2.f, 0.f, 0.f, 0.f, 3.f, 0.f, 0.f, 0.f, 4.f, 5.f, 6.f, 7.f)};
    const auto skin_basis = shadow::pc::basis_from_skin(skin.bone_matrices[0]);
    const std::array<float, 12> hierarchy = {1, 0, 0, 0, 1, 0, 0, 0, 1, 10, 0, 0};
    const std::array<float, 12> bone_ltm = {0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 3};
    const std::array<float, 12> frame_ltm = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 8};
    const std::array<float, 12> extra = {1, 0, 0, 0, 1, 0, 0, 0, 1.f, 0, 1, 0};
    const std::array<std::array<float, 12>, 1> slots{hierarchy};
    const std::array<std::array<float, 12>, 1> ltms{bone_ltm};

    const auto both_clear = shadow::pc::build_skin_palette(skin, slots, ltms, frame_ltm, extra, 0, 0);
    REQUIRE(both_clear.problems.empty());
    REQUIRE(both_clear.branch == shadow::pc::PaletteBranch::SkinHierarchyChosen);
    REQUIRE(!both_clear.chosen_is_frame_ltm);
    const auto expect_clear = ref_mul(skin_basis, ref_mul(hierarchy, extra));
    float clear_error = 0.f;
    for (int index = 0; index < 12; ++index) {
        clear_error = std::max(clear_error, std::fabs(both_clear.matrices[0][static_cast<std::size_t>(index)] - expect_clear[static_cast<std::size_t>(index)]));
    }
    REQUIRE(clear_error < 1.0e-5f);

    const auto direct = shadow::pc::build_skin_palette(skin, slots, ltms, frame_ltm, extra, 0x4000u, 0);
    REQUIRE(direct.branch == shadow::pc::PaletteBranch::SkinHierarchy);
    const auto expect_direct = ref_mul(skin_basis, hierarchy);
    float direct_error = 0.f;
    for (int index = 0; index < 12; ++index) {
        direct_error = std::max(direct_error, std::fabs(direct.matrices[0][static_cast<std::size_t>(index)] - expect_direct[static_cast<std::size_t>(index)]));
    }
    REQUIRE(direct_error < 1.0e-5f);
    float branch_gap = 0.f;
    for (int index = 0; index < 12; ++index) {
        branch_gap = std::max(branch_gap, std::fabs(direct.matrices[0][static_cast<std::size_t>(index)] - both_clear.matrices[0][static_cast<std::size_t>(index)]));
    }
    REQUIRE(branch_gap > 0.5f);

    const auto bit2 = shadow::pc::build_skin_palette(skin, slots, ltms, frame_ltm, extra, 0x2u, 0);
    REQUIRE(bit2.branch == shadow::pc::PaletteBranch::SkinBoneThenChosen);
    const auto expect_bit2 = ref_mul(ref_mul(skin_basis, bone_ltm), extra);
    float bit2_error = 0.f;
    for (int index = 0; index < 12; ++index) {
        bit2_error = std::max(bit2_error, std::fabs(bit2.matrices[0][static_cast<std::size_t>(index)] - expect_bit2[static_cast<std::size_t>(index)]));
    }
    REQUIRE(bit2_error < 1.0e-5f);

    skin.max_weights = 1;
    const auto narrow = shadow::pc::build_skin_palette(skin, slots, ltms, frame_ltm, extra, 0x4000u, 0);
    REQUIRE(narrow.branch == shadow::pc::PaletteBranch::SkinHierarchyFrameExtra);
    const auto expect_narrow = ref_mul(skin_basis, ref_mul(hierarchy, ref_mul(frame_ltm, extra)));
    float narrow_error = 0.f;
    for (int index = 0; index < 12; ++index) {
        narrow_error = std::max(narrow_error, std::fabs(narrow.matrices[0][static_cast<std::size_t>(index)] - expect_narrow[static_cast<std::size_t>(index)]));
    }
    REQUIRE(narrow_error < 1.0e-5f);
}

void check_blend_rules() {
    shadow::pc::NativeMesh mesh;
    mesh.positions = {{1.f, 2.f, 3.f}};
    mesh.normals = {{0.f, 1.f, 0.f}};
    shadow::pc::SkinPalette palette;
    palette.matrices.assign(2, shadow::pc::affine_identity());
    palette.matrices[0][9] = 9.f;
    palette.written = {1, 1};
    shadow::pc::PreparedInfluences prepared;
    prepared.max_weights = 2;
    prepared.vertex_count = 1;
    prepared.bones = {0, 1};
    prepared.weight_bytes = {128, 0};
    const auto skinned = shadow::pc::skin_vertices(mesh, palette, prepared);
    REQUIRE(skinned.problems.empty());
    REQUIRE(near3(skinned.positions[0], {10.f, 2.f, 3.f}, 1.0e-5f));
    REQUIRE(near3(skinned.normals[0], {0.f, 1.f, 0.f}, 1.0e-5f));
    const auto again = shadow::pc::skin_vertices(mesh, palette, prepared);
    REQUIRE(again.positions == skinned.positions);
    REQUIRE(again.normals == skinned.normals);

    prepared.weight_bytes = {128, 128};
    palette.matrices[1] = shadow::pc::affine_identity();
    const auto doubled = shadow::pc::skin_vertices(mesh, palette, prepared);
    REQUIRE(near3(doubled.normals[0], {0.f, 2.f, 0.f}, 1.0e-5f));
    const float length = std::sqrt(doubled.normals[0][0] * doubled.normals[0][0] + doubled.normals[0][1] * doubled.normals[0][1] +
                                   doubled.normals[0][2] * doubled.normals[0][2]);
    REQUIRE(std::fabs(length - 2.f) < 1.0e-5f);

    shadow::pc::NativeSkin sortable;
    sortable.bone_count = 4;
    sortable.used_count = 2;
    sortable.used_bones = {1, 2};
    sortable.max_weights = 2;
    sortable.indices = {1, 2, 0, 0};
    sortable.weights = {0.2f, 0.8f, 0.f, 0.f};
    sortable.bone_matrices.assign(4, skin_block(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0));
    const auto sorted = shadow::pc::prepare_influences(sortable, 1);
    REQUIRE(sorted.problems.empty());
    REQUIRE(sorted.bones[0] == 2);
    REQUIRE(sorted.bones[1] == 1);
    REQUIRE(std::trunc(0.8f * 128.f) == 102.f);
    REQUIRE(std::trunc(0.2f * 128.f) == 25.f);
    REQUIRE(sorted.weight_bytes[0] == 103);
    REQUIRE(sorted.weight_bytes[1] == 25);

    prepared.max_weights = 4;
    prepared.bones = {0, 1, 2, 3};
    prepared.weight_bytes = {64, 0, 64, 0};
    palette.matrices.assign(4, shadow::pc::affine_identity());
    palette.written.assign(4, 1);
    mesh.positions = {{1.f, 0.f, 0.f}};
    mesh.normals.clear();
    const auto skipped = shadow::pc::skin_vertex(mesh, 0, palette, prepared);
    REQUIRE(skipped.problems.empty());
    REQUIRE(skipped.terms.size() == 4);
    REQUIRE(skipped.terms[0].applied);
    REQUIRE(!skipped.terms[1].applied);
    REQUIRE(!skipped.terms[2].applied);
    REQUIRE(near3(skipped.skinned_position, {0.5f, 0.f, 0.f}, 1.0e-5f));
}

std::array<float, 3> check_character_skin(const char* label,
                                         const shadow::pc::NativeModel& model,
                                         const std::vector<std::array<float, 12>>& frame_globals,
                                         bool print_vertex) {
    REQUIRE(!model.meshes.empty());
    REQUIRE(model.meshes[0].skin.has_value());
    REQUIRE(!model.hierarchies.empty());
    const auto& mesh = model.meshes[0];
    const auto& skin = *mesh.skin;
    REQUIRE(model.hierarchies[0].flags == 0);
    REQUIRE(!mesh.positions.empty());
    REQUIRE(mesh.normals.size() == mesh.positions.size());
    const auto slots = shadow::pc::hierarchy_slot_globals(model, frame_globals);
    REQUIRE(slots.problems.empty());
    REQUIRE(slots.matrices.size() == skin.bone_count);
    const auto extra = shadow::pc::affine_identity();
    const auto palette = shadow::pc::build_skin_palette(skin, slots.matrices, slots.matrices, extra, extra,
                                                        static_cast<std::uint32_t>(model.hierarchies[0].flags), 0);
    REQUIRE(palette.problems.empty());
    REQUIRE(palette.branch == shadow::pc::PaletteBranch::SkinHierarchyChosen);
    const auto prepared = shadow::pc::prepare_influences(skin, mesh.positions.size());
    REQUIRE(prepared.problems.empty());
    REQUIRE(prepared.max_weights == skin.max_weights);
    int byte_sum_failures = 0;
    if (prepared.max_weights > 1) {
        for (std::size_t vertex = 0; vertex < prepared.vertex_count; ++vertex) {
            int sum = 0;
            for (std::uint32_t slot = 0; slot < prepared.max_weights; ++slot) {
                const auto bone = prepared.bones[vertex * prepared.max_weights + slot];
                REQUIRE(bone < skin.bone_count);
                sum += prepared.weight_bytes[vertex * prepared.max_weights + slot];
            }
            if (sum != 128) {
                ++byte_sum_failures;
            }
        }
    }
    REQUIRE(byte_sum_failures == 0);
    const auto skinned = shadow::pc::skin_vertices(mesh, palette, prepared);
    REQUIRE(skinned.problems.empty());
    const auto skinned_again = shadow::pc::skin_vertices(mesh, palette, prepared);
    REQUIRE(skinned.positions == skinned_again.positions);
    REQUIRE(skinned.normals == skinned_again.normals);
    for (const auto& position : skinned.positions) {
        REQUIRE(std::isfinite(position[0]) && std::isfinite(position[1]) && std::isfinite(position[2]));
    }
    for (const auto& normal : skinned.normals) {
        REQUIRE(std::isfinite(normal[0]) && std::isfinite(normal[1]) && std::isfinite(normal[2]));
    }
    for (const auto& matrix : palette.matrices) {
        for (const float value : matrix) {
            REQUIRE(std::isfinite(value));
        }
    }

    std::array<float, 3> expected{};
    bool running = true;
    for (std::uint32_t slot = 0; slot < prepared.max_weights; ++slot) {
        const auto bone = prepared.bones[slot];
        const float weight = prepared.max_weights == 1 ? 1.f : static_cast<float>(prepared.weight_bytes[slot]) / 128.f;
        if (slot > 0 && !(weight > 0.f)) {
            running = false;
        }
        if (running && (slot == 0 || weight > 0.f)) {
            const auto transformed = ref_point(mesh.positions[0], palette.matrices[bone]);
            const float scale = prepared.max_weights == 1 ? 1.f : weight;
            expected[0] += scale * transformed[0];
            expected[1] += scale * transformed[1];
            expected[2] += scale * transformed[2];
        }
    }
    REQUIRE(near3(skinned.positions[0], expected, 1.0e-4f));
    std::array<float, 3> expected_normal{};
    running = true;
    for (std::uint32_t slot = 0; slot < prepared.max_weights; ++slot) {
        const auto bone = prepared.bones[slot];
        const float weight = prepared.max_weights == 1 ? 1.f : static_cast<float>(prepared.weight_bytes[slot]) / 128.f;
        if (slot > 0 && !(weight > 0.f)) {
            running = false;
        }
        if (running && (slot == 0 || weight > 0.f)) {
            const auto transformed = ref_direction(mesh.normals[0], palette.matrices[bone]);
            const float scale = prepared.max_weights == 1 ? 1.f : weight;
            expected_normal[0] += scale * transformed[0];
            expected_normal[1] += scale * transformed[1];
            expected_normal[2] += scale * transformed[2];
        }
    }
    REQUIRE(near3(skinned.normals[0], expected_normal, 1.0e-4f));
    REQUIRE(std::isfinite(expected_normal[0]) && std::isfinite(expected_normal[1]) && std::isfinite(expected_normal[2]));

    shadow::pc::SkinPalette identity_palette;
    identity_palette.matrices.assign(skin.bone_count, shadow::pc::affine_identity());
    identity_palette.written.assign(skin.bone_count, 1);
    const auto identity_record = shadow::pc::skin_vertex(mesh, 0, identity_palette, prepared);
    REQUIRE(identity_record.problems.empty());
    REQUIRE(near3(identity_record.skinned_position, mesh.positions[0], 1.0e-4f));

    if (print_vertex) {
        const auto record = shadow::pc::skin_vertex(mesh, 0, palette, prepared);
        std::cout << "SKIN_V0 " << label << " bones";
        for (const auto& term : record.terms) {
            std::cout << ' ' << static_cast<int>(term.bone) << ':' << term.weight << (term.applied ? "" : "(skip)");
        }
        std::cout << " src " << record.source_position[0] << ' ' << record.source_position[1] << ' ' << record.source_position[2]
                  << " out " << record.skinned_position[0] << ' ' << record.skinned_position[1] << ' ' << record.skinned_position[2] << '\n';
    }
    return skinned.positions[0];
}

}  // namespace

int main() {
    using shadow::pc::LoadRequest;
    using shadow::pc::ResourceSystem;

    const auto eye_bytes_archive = file_bytes(content() / "character/shadow.one");
    auto shadow_one = shadow::gc::open_one(eye_bytes_archive);
    REQUIRE(shadow_one.version > 0.59f);
    REQUIRE(shadow_one.declared_count == 28);
    REQUIRE(shadow_one.entries.size() == 28);
    REQUIRE(shadow_one.find("shadow_eye_l.dff") != nullptr);
    for (const auto& entry : shadow_one.entries) {
        const auto payload = shadow_one.load(entry);
        REQUIRE(payload.size() == entry.declared_size);
    }

    const auto* eye_entry = shadow_one.find("SHADOW_EYE_L.DFF");
    REQUIRE(eye_entry != nullptr);
    const auto eye_payload = shadow_one.load(*eye_entry);
    const auto eye = shadow::gc::parse_clump(eye_payload);
    REQUIRE(eye.frames.size() == 2);
    REQUIRE(eye.frames[0].parent == -1);
    REQUIRE(eye.geometries.size() == 1);
    REQUIRE(eye.geometries[0].triangles.size() == 67);
    REQUIRE(eye.geometries[0].morphs.size() == 1);
    REQUIRE(eye.geometries[0].morphs[0].positions.size() == 45);
    REQUIRE(eye.geometries[0].bin_meshes.size() == 1);
    REQUIRE(eye.geometries[0].bin_meshes[0].indices.size() == 91);
    REQUIRE(eye.geometries[0].materials.size() == 1);
    REQUIRE(eye.geometries[0].materials[0].texture_name == "pl_bw03n");
    REQUIRE(eye.atomics.size() == 1);
    const auto eye_hash = fnv(eye.geometries[0].morphs[0].positions);
    std::cout << "EYE_FNV " << std::hex << eye_hash << std::dec << '\n';

    const auto* body_entry = shadow_one.find("SHADOW_BODY.DFF");
    REQUIRE(body_entry != nullptr);
    const auto body = shadow::gc::parse_clump(shadow_one.load(*body_entry));
    REQUIRE(body.frames.size() == 31);
    REQUIRE(body.geometries.size() == 1);
    REQUIRE(body.geometries[0].morphs[0].positions.size() == 1382);
    REQUIRE(body.geometries[0].bin_meshes.size() == 4);
    std::size_t body_indices = 0;
    for (const auto& mesh : body.geometries[0].bin_meshes) {
        body_indices += mesh.indices.size();
    }
    REQUIRE(body_indices == 0xA8F);
    REQUIRE(body.geometries[0].skin.has_value());
    REQUIRE(body.geometries[0].skin->recognized);
    REQUIRE(body.geometries[0].skin->bone_count == 30);
    REQUIRE(body.geometries[0].skin->used_count == 26);
    std::size_t hierarchies = 0;
    std::size_t hierarchy_nodes = 0;
    for (const auto& frame : body.frames) {
        for (const auto& anim : frame.hanim) {
            if (anim.hierarchy) {
                ++hierarchies;
                hierarchy_nodes = anim.nodes.size();
            }
        }
    }
    REQUIRE(hierarchies == 1);
    REQUIRE(hierarchy_nodes == 30);

    const auto* txd_entry = shadow_one.find("SHADOW.TXD");
    REQUIRE(txd_entry != nullptr);
    const auto txd = shadow::gc::parse_tex_dictionary(shadow_one.load(*txd_entry));
    REQUIRE(txd.count_field == 7);
    REQUIRE(txd.textures.size() == 7);
    const unsigned expected_edge[] = {32, 128, 128, 32, 64, 64, 32};
    for (std::size_t i = 0; i < 7; ++i) {
        REQUIRE(txd.textures[i].width == expected_edge[i]);
        REQUIRE(txd.textures[i].height == expected_edge[i]);
        REQUIRE(txd.textures[i].platform_id == 6);
        REQUIRE(txd.textures[i].image.size() + 104 == txd.textures[i].raw_struct.size());
    }

    bool saw_uncompressed = false;
    std::size_t one_files = 0;
    std::size_t version_50 = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(content())) {
        if (!entry.is_regular_file() || entry.path().extension() != ".one") {
            continue;
        }
        auto archive = shadow::gc::open_one(file_bytes(entry.path()));
        ++one_files;
        if (archive.version <= 0.59f) {
            ++version_50;
        }
        REQUIRE(archive.entries.size() == archive.declared_count);
        if (!saw_uncompressed) {
            for (const auto& item : archive.entries) {
                if ((item.flags & 1u) == 0 && item.declared_size > 0) {
                    const auto raw = archive.load(item);
                    REQUIRE(raw.size() == item.declared_size);
                    saw_uncompressed = true;
                    std::cout << "RAW_ENTRY " << entry.path().filename().string() << " " << item.name << '\n';
                    break;
                }
            }
        }
    }
    REQUIRE(one_files == 1244);
    REQUIRE(version_50 == 13);
    REQUIRE(saw_uncompressed);

    auto debug_models = shadow::gc::open_one(file_bytes(content() / "DebugModels.one"));
    REQUIRE(debug_models.version <= 0.59f);
    REQUIRE(debug_models.entries.size() == 5);
    const auto cube = shadow::gc::parse_clump(debug_models.load(*debug_models.find("CUBE_MODEL.DFF")));
    REQUIRE(!cube.geometries.empty());
    REQUIRE(!cube.geometries[0].morphs.empty());
    REQUIRE(!cube.geometries[0].morphs[0].positions.empty());

    bool truncated = false;
    try {
        std::vector<std::uint8_t> broken(32, 0);
        shadow::gc::open_one(std::move(broken));
    } catch (const shadow::ParseError&) {
        truncated = true;
    }
    REQUIRE(truncated);

    const auto number = shadow::gc::parse_tex_dictionary(file_bytes(content() / "number.txd"));
    REQUIRE(number.textures.size() == 1);
    REQUIRE(number.textures[0].width > 0);
    REQUIRE(number.textures[0].height > 0);

    const auto fst = shadow::gc::open_fst(file_bytes(SHADOW_FST_PATH));
    const auto* fst_shadow = fst.find("character/shadow.one");
    REQUIRE(fst_shadow != nullptr);
    REQUIRE(!fst_shadow->is_directory);
    REQUIRE(fst_shadow->size_or_next == eye_bytes_archive.size());

    const auto afs = shadow::gc::parse_afs(file_bytes(content() / "PRS_VOICE_E.afs"));
    REQUIRE(afs.count == afs.entries.size());
    REQUIRE(afs.count > 3);

    const auto adx = shadow::gc::parse_adx(file_bytes(content() / "sng_stg0100.adx"));
    REQUIRE(adx.sample_rate == 48000);
    REQUIRE(adx.channel_count == 2);
    REQUIRE(adx.copyright.find("CRI") != std::string::npos);

    const auto setid = shadow::gc::parse_setid(file_bytes(content() / "setid.bin"));
    REQUIRE(setid.records.size() == 302);

    const auto sofdec = shadow::gc::parse_sofdec(file_bytes(content() / "E1001.sfd"));
    REQUIRE(sofdec.size > 1000);

    ResourceSystem system(content());
    system.load_fst(SHADOW_FST_PATH);
    REQUIRE(ResourceSystem::queue_limit > 3);
    system.enqueue(LoadRequest{"character/shadow.one", "SHADOW_EYE_L.DFF"});
    system.enqueue(LoadRequest{"character/shadow.one", "SHADOW.TXD"});
    system.enqueue(LoadRequest{"stg0100/stg0100_01.one", "STG0100_COLI_01.BSP"});
    system.enqueue(LoadRequest{"stg0100/stg0100_01.one", "STG0100_D_01.RG1"});
    system.enqueue(LoadRequest{"number.txd", ""});
    system.enqueue(LoadRequest{"setid.bin", ""});
    REQUIRE(system.queued() == 6);
    system.pump();
    REQUIRE(system.queued() == 0);

    const auto eye_handle = system.load(LoadRequest{"character/shadow.one", "SHADOW_EYE_L.DFF"});
    const auto* eye_model = system.model(eye_handle);
    REQUIRE(eye_model != nullptr);
    REQUIRE(eye_model->meshes.size() == 1);
    REQUIRE(eye_model->meshes[0].positions.size() == 45);
    REQUIRE(eye_model->meshes[0].triangles.size() == 67);
    REQUIRE(eye_model->bones.size() == 2);
    bool texture_linked = false;
    for (const auto& edge : system.dependencies(eye_handle)) {
        if (edge.name == "pl_bw03n" && edge.resolved && edge.entry == "SHADOW.TXD") {
            texture_linked = true;
        }
    }
    REQUIRE(texture_linked);
    system.release(eye_handle);
    REQUIRE(system.model(eye_handle) == nullptr);

    const auto world_handle = system.load(LoadRequest{"stg0100/stg0100_01.one", "STG0100_COLI_01.BSP"});
    const auto* world = system.world(world_handle);
    REQUIRE(world != nullptr);
    REQUIRE(world->materials.size() == 4);
    bool saw_sector = false;
    for (const auto& section : world->sections) {
        if (section.first == 0x09) {
            saw_sector = true;
            REQUIRE(!section.second.empty());
        }
    }
    REQUIRE(saw_sector);
    REQUIRE(world->bound_a[0] != 0.f || world->bound_b[0] != 0.f);

    const auto region_handle = system.load(LoadRequest{"stg0100/stg0100_01.one", "STG0100_D_01.RG1"});
    const auto* region = system.world(region_handle);
    REQUIRE(region != nullptr);
    REQUIRE(region->name == "stg0100_d_01");

    const auto stage = system.stage_files("stg0403");
    REQUIRE(stage.size() > 15);
    bool saw_98 = false;
    for (const auto& path : stage) {
        if (path.find("stg0403_98.one") != std::string::npos) {
            saw_98 = true;
        }
    }
    REQUIRE(saw_98);

    const auto& body_skin = *body.geometries[0].skin;
    REQUIRE(body_skin.max_weights == 2);
    REQUIRE(body_skin.header_pad == 0);
    REQUIRE(body_skin.indices.size() == 1382u * 4u);
    REQUIRE(body_skin.weights.size() == 1382u * 4u);
    REQUIRE(body_skin.bone_matrices.size() == 30u * 16u);
    REQUIRE(body_skin.indices[0] == 4);
    REQUIRE(body_skin.indices[1] == 5);
    REQUIRE(body_skin.indices[2] == 0);
    REQUIRE(body_skin.indices[3] == 0);
    REQUIRE(std::fabs(body_skin.weights[0] - 0.8898069858551025f) < 1.0e-6f);
    REQUIRE(std::fabs(body_skin.weights[1] - 0.11019299924373627f) < 1.0e-6f);
    REQUIRE(body_skin.trailer[0] == 0 && body_skin.trailer[1] == 0 && body_skin.trailer[2] == 0);
    REQUIRE(body_skin.split_data.empty());
    REQUIRE(shadow::gc::skin_problems(body_skin.bone_count, body.geometries[0].vertex_count, body_skin.indices, body_skin.weights).empty());

    const auto body_model = shadow::pc::to_native_model(body, "SHADOW_BODY.DFF");
    REQUIRE(shadow::pc::hierarchy_problems(body_model).empty());
    REQUIRE(body_model.hierarchies.size() == 1);
    REQUIRE(body_model.hierarchies[0].nodes.size() == 30);
    for (std::size_t i = 0; i < 30; ++i) {
        REQUIRE(body_model.hierarchies[0].nodes[i].index == static_cast<std::int32_t>(i));
    }
    const auto globals = shadow::pc::bind_globals(body_model);
    check_palette_branches();
    check_blend_rules();
    const auto body_bind_v0 = check_character_skin("SHADOW_BODY bind", body_model, globals, true);
    {
        const auto skin0 = shadow::pc::basis_from_skin(body_model.meshes[0].skin->bone_matrices[0]);
        const auto slots = shadow::pc::hierarchy_slot_globals(body_model, globals);
        REQUIRE(slots.problems.empty());
        const auto either = ref_mul(skin0, slots.matrices[0]);
        const auto other = ref_mul(slots.matrices[0], skin0);
        REQUIRE(basis_error(either) > 1.f);
        REQUIRE(basis_error(other) > 1.f);
        const auto prepared = shadow::pc::prepare_influences(*body_model.meshes[0].skin, body_model.meshes[0].positions.size());
        REQUIRE(prepared.bones[0] == 4);
        REQUIRE(prepared.bones[1] == 5);
        REQUIRE(std::trunc(body_model.meshes[0].skin->weights[0] * 128.f) == 113.f);
        REQUIRE(std::trunc(body_model.meshes[0].skin->weights[1] * 128.f) == 14.f);
        REQUIRE(prepared.weight_bytes[0] == 114);
        REQUIRE(prepared.weight_bytes[1] == 14);
    }
    REQUIRE(globals.size() == 31);
    REQUIRE(std::fabs(globals[2][10] - 5.15f) < 1.0e-4f);
    REQUIRE(std::fabs(globals[15][10] - 6.5f) < 1.0e-3f);

    const auto bon = shadow::gc::parse_bon(shadow_one.load(*shadow_one.find("SH.BON")));
    REQUIRE(bon.endian_flag == 1);
    REQUIRE(bon.name == "sh");
    REQUIRE(bon.node_count == 30);
    REQUIRE(std::fabs(bon.header_12 - 5.15f) < 1.0e-4f);
    REQUIRE(shadow::gc::bon_problems(bon).empty());
    std::size_t matched_bones = 0;
    for (const auto& node : bon.nodes) {
        const shadow::pc::NativeBone* frame = nullptr;
        for (std::size_t index = 0; index < body_model.bones.size(); ++index) {
            if (static_cast<std::size_t>(body_model.hierarchies[0].root_frame) == index) {
                continue;
            }
            if (body_model.bones[index].hanim_id == static_cast<std::int32_t>(node.bone_id)) {
                frame = &body_model.bones[index];
            }
        }
        if (node.bone_id == 0) {
            REQUIRE(frame == nullptr);
            continue;
        }
        REQUIRE(frame != nullptr);
        REQUIRE(std::fabs(frame->basis[9] - node.translation[0]) < 1.0e-4f);
        REQUIRE(std::fabs(frame->basis[10] - node.translation[1]) < 1.0e-4f);
        REQUIRE(std::fabs(frame->basis[11] - node.translation[2]) < 1.0e-4f);
        ++matched_bones;
    }
    REQUIRE(matched_bones == 29);
    const auto lte = shadow::gc::parse_bon(shadow_one.load(*shadow_one.find("LTE.BON")));
    const auto rte = shadow::gc::parse_bon(shadow_one.load(*shadow_one.find("RTE.BON")));
    REQUIRE(lte.name == "lte" && lte.node_count == 7);
    REQUIRE(rte.name == "rte" && rte.node_count == 7);
    REQUIRE(shadow::gc::bon_problems(lte).empty());
    REQUIRE(shadow::gc::bon_problems(rte).empty());

    const auto pack = shadow::gc::parse_motion_pack(shadow_one.load(*shadow_one.find("SHADOW.MTP")));
    REQUIRE(pack.motion_count == 158);
    REQUIRE(pack.entries.size() == 158);
    std::size_t unsplit = 0;
    std::size_t sh_motions = 0;
    std::size_t hand_motions = 0;
    const shadow::gc::Motion* sh_cb = nullptr;
    for (const auto& entry : pack.entries) {
        REQUIRE(shadow::gc::motion_problems(entry.motion).empty());
        if (entry.name == "sh_CB") {
            sh_cb = &entry.motion;
        }
        for (const auto& chunk : entry.motion.chunks) {
            if (!chunk.groups_split) {
                ++unsplit;
            }
        }
        if (entry.motion.name == "sh") {
            ++sh_motions;
            REQUIRE(entry.motion.chunks.size() == 357);
        } else if (entry.motion.name == "lte" || entry.motion.name == "rte") {
            ++hand_motions;
            REQUIRE(entry.motion.chunks.size() == 81);
        }
    }
    REQUIRE(sh_motions == 122);
    REQUIRE(hand_motions == 36);
    REQUIRE(unsplit == 49);
    REQUIRE(sh_cb != nullptr);
    REQUIRE(sh_cb->name == "sh");
    REQUIRE(sh_cb->word_18 == 240);
    std::vector<std::string> motion_names;
    motion_names.reserve(pack.entries.size());
    for (const auto& entry : pack.entries) {
        motion_names.push_back(entry.name);
    }
    std::sort(motion_names.begin(), motion_names.end());
    REQUIRE(std::adjacent_find(motion_names.begin(), motion_names.end()) == motion_names.end());
    bool saw_constant = false;
    for (const auto& chunk : sh_cb->chunks) {
        if (chunk.groups_split && chunk.keys.size() == 1 && chunk.keys[0].value_b == 0.f && chunk.keys[0].value_c == 0.f) {
            shadow::pc::NativeKeyframe key{chunk.keys[0].header, chunk.keys[0].component_b, chunk.keys[0].component_c,
                                           chunk.keys[0].time, chunk.keys[0].value_b, chunk.keys[0].value_c};
            REQUIRE(shadow::pc::sample_channel(std::span<const shadow::pc::NativeKeyframe>(&key, 1), 0.f) == 0.f);
            saw_constant = true;
            break;
        }
    }
    REQUIRE(saw_constant);
    shadow::pc::NativeKeyframe slope{0, 0, 0, 0.f, 2.f, 1.f};
    REQUIRE(std::fabs(shadow::pc::sample_channel(std::span<const shadow::pc::NativeKeyframe>(&slope, 1), 0.5f) - 2.f) < 1.0e-6f);

    const auto native_sh = shadow::pc::to_native_bon(bon, "SH.BON");
    const auto native_lte = shadow::pc::to_native_bon(lte, "LTE.BON");
    const auto native_rte = shadow::pc::to_native_bon(rte, "RTE.BON");
    const auto shadow_motions = shadow::pc::to_native_motion_pack(pack, "SHADOW.MTP");
    const shadow::pc::ChannelMap sh_map = shadow::pc::map_skeleton_channels(native_sh);
    const shadow::pc::ChannelMap lte_map = shadow::pc::map_skeleton_channels(native_lte);
    const shadow::pc::ChannelMap rte_map = shadow::pc::map_skeleton_channels(native_rte);
    REQUIRE(sh_map.problems.empty());
    REQUIRE(lte_map.problems.empty());
    REQUIRE(rte_map.problems.empty());
    REQUIRE(sh_map.bindings.size() == 357);
    REQUIRE(lte_map.bindings.size() == 81);
    REQUIRE(rte_map.bindings.size() == 81);
    REQUIRE(sh_map.bindings[0].bone_id == 0);
    REQUIRE(sh_map.bindings[0].kind == shadow::pc::ChannelKind::TranslateX);
    REQUIRE(sh_map.bindings[1].kind == shadow::pc::ChannelKind::TranslateY);
    REQUIRE(sh_map.bindings[2].kind == shadow::pc::ChannelKind::TranslateZ);
    REQUIRE(sh_map.bindings[3].kind == shadow::pc::ChannelKind::RotateX);
    REQUIRE(sh_map.bindings[6].kind == shadow::pc::ChannelKind::PostScaleX);
    REQUIRE(sh_map.bindings[9].bone_id == 1);
    REQUIRE(sh_map.bindings[9].kind == shadow::pc::ChannelKind::PreScaleX);
    std::vector<std::uint8_t> sh_ids_seen(native_sh.nodes.size(), 0);
    for (const auto& binding : sh_map.bindings) {
        REQUIRE(binding.chunk < 357);
        REQUIRE(binding.node_index < native_sh.nodes.size());
        REQUIRE(native_sh.nodes[binding.node_index].bone_id == binding.bone_id);
        sh_ids_seen[binding.node_index] = 1;
    }
    for (const auto seen : sh_ids_seen) {
        REQUIRE(seen == 1);
    }
    const shadow::pc::NativeMotion* sh_run = nullptr;
    const shadow::pc::NativeMotion* sh_idle = nullptr;
    std::size_t mapped_motions = 0;
    std::size_t unsplit_channels = 0;
    for (const auto& motion : shadow_motions.motions) {
        const shadow::pc::ChannelMap* map = nullptr;
        const shadow::pc::NativeBon* skeleton = nullptr;
        if (motion.inner_name == native_sh.skeleton_name) {
            map = &sh_map;
            skeleton = &native_sh;
        } else if (motion.inner_name == native_lte.skeleton_name) {
            map = &lte_map;
            skeleton = &native_lte;
        } else if (motion.inner_name == native_rte.skeleton_name) {
            map = &rte_map;
            skeleton = &native_rte;
        }
        REQUIRE(map != nullptr);
        REQUIRE(motion.tracks.size() == map->bindings.size());
        for (const auto& binding : map->bindings) {
            bool found = false;
            for (const auto& node : skeleton->nodes) {
                if (node.bone_id == binding.bone_id) {
                    found = true;
                }
            }
            REQUIRE(found);
        }
        const auto posed = shadow::pc::animate_locals(*skeleton, motion, *map, 0.f);
        const auto posed_again = shadow::pc::animate_locals(*skeleton, motion, *map, 0.f);
        REQUIRE(posed.local.size() == skeleton->nodes.size());
        REQUIRE(posed.local == posed_again.local);
        for (const auto& basis : posed.local) {
            for (const float value : basis) {
                REQUIRE(std::isfinite(value));
            }
        }
        unsplit_channels += posed.unsplit_channels;
        if (motion.name == "sh_run") {
            sh_run = &motion;
        }
        if (motion.name == "sh_idle") {
            sh_idle = &motion;
        }
        ++mapped_motions;
    }
    REQUIRE(mapped_motions == 158);
    REQUIRE(unsplit_channels == 49);
    REQUIRE(sh_run != nullptr);
    REQUIRE(sh_idle != nullptr);
    const auto idle_at_zero = shadow::pc::animate_locals(native_sh, *sh_idle, sh_map, 0.f);
    std::size_t root_node = 0;
    for (std::size_t index = 0; index < native_sh.nodes.size(); ++index) {
        if (native_sh.nodes[index].bone_id == 1) {
            root_node = index;
        }
    }
    float sampled_ty = 0.f;
    for (const auto& binding : sh_map.bindings) {
        if (binding.bone_id == 1 && binding.kind == shadow::pc::ChannelKind::TranslateY) {
            sampled_ty = shadow::pc::sample_channel(sh_idle->tracks[binding.chunk].keys, 0.f);
        }
    }
    REQUIRE(std::fabs(idle_at_zero.local[root_node][0] - 1.f) < 1.0e-5f);
    REQUIRE(std::fabs(idle_at_zero.local[root_node][4] - 1.f) < 1.0e-5f);
    REQUIRE(std::fabs(idle_at_zero.local[root_node][8] - 1.f) < 1.0e-5f);
    REQUIRE(std::fabs(idle_at_zero.local[root_node][10] - sampled_ty) < 1.0e-5f);
    REQUIRE(std::fabs(sampled_ty - 4.48828125f) < 1.0e-4f);
    const auto idle_globals = shadow::pc::pose_globals(body_model, native_sh, idle_at_zero);
    const auto body_idle_v0 = check_character_skin("SHADOW_BODY idle0", body_model, idle_globals, true);
    const float idle_delta = std::fabs(body_idle_v0[0] - body_bind_v0[0]) + std::fabs(body_idle_v0[1] - body_bind_v0[1]) +
                             std::fabs(body_idle_v0[2] - body_bind_v0[2]);
    REQUIRE(idle_delta > 1.0e-3f);
    const auto run_pose = shadow::pc::animate_locals(native_sh, *sh_run, sh_map, 0.5f);
    const auto idle_pose = shadow::pc::animate_locals(native_sh, *sh_idle, sh_map, 0.5f);
    REQUIRE(run_pose.local != idle_pose.local);
    const auto run_globals = shadow::pc::pose_globals(body_model, native_sh, run_pose);
    const auto run_globals_again = shadow::pc::pose_globals(body_model, native_sh, run_pose);
    REQUIRE(run_globals.size() == body_model.bones.size());
    REQUIRE(run_globals == run_globals_again);
    for (const auto& basis : run_globals) {
        for (const float value : basis) {
            REQUIRE(std::isfinite(value));
        }
    }
    bool hand_id_in_body = false;
    for (const auto& binding : lte_map.bindings) {
        for (const auto& frame : body_model.bones) {
            if (frame.hanim_id == static_cast<std::int32_t>(binding.bone_id)) {
                hand_id_in_body = true;
            }
        }
    }
    REQUIRE(!hand_id_in_body);
    const auto* hand_entry = shadow_one.find("SHADOW_HAND_L.DFF");
    REQUIRE(hand_entry != nullptr);
    const auto hand = shadow::gc::parse_clump(shadow_one.load(*hand_entry));
    const auto hand_model = shadow::pc::to_native_model(hand, "SHADOW_HAND_L.DFF");
    const auto hand_bind = shadow::pc::bind_globals(hand_model);
    check_character_skin("SHADOW_HAND_L bind", hand_model, hand_bind, false);
    REQUIRE(hand_model.meshes[0].skin->indices[0] == 0);
    REQUIRE(std::fabs(hand_model.meshes[0].skin->weights[0] - 1.f) < 1.0e-5f);
    std::size_t hand_matches = 0;
    for (const auto& node : native_lte.nodes) {
        if (node.bone_id == 0) {
            continue;
        }
        for (const auto& frame : hand_model.bones) {
            if (frame.hanim_id == static_cast<std::int32_t>(node.bone_id)) {
                ++hand_matches;
            }
        }
    }
    const shadow::pc::NativeMotion* lte_motion = nullptr;
    for (const auto& motion : shadow_motions.motions) {
        if (motion.inner_name == "lte") {
            lte_motion = &motion;
            break;
        }
    }
    REQUIRE(lte_motion != nullptr);
    const auto lte_pose = shadow::pc::animate_locals(native_lte, *lte_motion, lte_map, 0.25f);
    REQUIRE(hand_matches == native_lte.nodes.size());
    REQUIRE(hand_model.bones.size() == 8);
    const auto* right_hand_entry = shadow_one.find("SHADOW_HAND_R.DFF");
    REQUIRE(right_hand_entry != nullptr);
    const auto right_hand_model = shadow::pc::to_native_model(
        shadow::gc::parse_clump(shadow_one.load(*right_hand_entry)), "SHADOW_HAND_R.DFF");
    check_character_skin("SHADOW_HAND_R bind", right_hand_model, shadow::pc::bind_globals(right_hand_model), false);
    std::size_t right_matches = 0;
    for (const auto& node : native_rte.nodes) {
        for (const auto& frame : right_hand_model.bones) {
            if (frame.hanim_id == static_cast<std::int32_t>(node.bone_id)) {
                ++right_matches;
            }
        }
    }
    REQUIRE(right_matches == native_rte.nodes.size());
    if (!hand_model.bones.empty() && hand_matches > 0) {
        const auto hand_globals = shadow::pc::pose_globals(hand_model, native_lte, lte_pose);
        check_character_skin("SHADOW_HAND_L lte", hand_model, hand_globals, false);
        REQUIRE(hand_globals.size() == hand_model.bones.size());
        for (const auto& basis : hand_globals) {
            for (const float value : basis) {
                REQUIRE(std::isfinite(value));
            }
        }
    }

    const auto amy_one = shadow::gc::open_one(file_bytes(content() / "character/amy.one"));
    const auto amy = shadow::gc::parse_clump(amy_one.load(*amy_one.find("AMY.DFF")));
    REQUIRE(amy.frames.size() == 22);
    REQUIRE(amy.geometries[0].skin.has_value());
    REQUIRE(amy.geometries[0].skin->bone_count == 21);
    REQUIRE(amy.geometries[0].skin->max_weights == 2);
    REQUIRE(amy.geometries[0].vertex_count == 930);
    REQUIRE(shadow::gc::skin_problems(amy.geometries[0].skin->bone_count, amy.geometries[0].vertex_count,
                                      amy.geometries[0].skin->indices, amy.geometries[0].skin->weights)
                .empty());
    const auto amy_model = shadow::pc::to_native_model(amy, "AMY.DFF");
    REQUIRE(shadow::pc::hierarchy_problems(amy_model).empty());
    check_character_skin("AMY bind", amy_model, shadow::pc::bind_globals(amy_model), false);
    const auto amy_bon = shadow::pc::to_native_bon(shadow::gc::parse_bon(amy_one.load(*amy_one.find("AMY.BON"))), "AMY.BON");
    const auto amy_pack = shadow::pc::to_native_motion_pack(shadow::gc::parse_motion_pack(amy_one.load(*amy_one.find("AMY.MTP"))), "AMY.MTP");
    const auto amy_map = shadow::pc::map_skeleton_channels(amy_bon);
    REQUIRE(amy_map.problems.empty());
    REQUIRE(amy_map.bindings.size() == 249);
    std::size_t amy_matched = 0;
    for (const auto& motion : amy_pack.motions) {
        if (motion.inner_name != amy_bon.skeleton_name) {
            continue;
        }
        REQUIRE(motion.tracks.size() == amy_map.bindings.size());
        const auto posed = shadow::pc::animate_locals(amy_bon, motion, amy_map, 0.f);
        for (const auto& basis : posed.local) {
            for (const float value : basis) {
                REQUIRE(std::isfinite(value));
            }
        }
        ++amy_matched;
    }
    REQUIRE(amy_matched == amy_pack.motions.size());
    REQUIRE(amy_matched > 0);

    const auto bee_one = shadow::gc::open_one(file_bytes(content() / "character/bee.one"));
    const auto bee = shadow::gc::parse_clump(bee_one.load(*bee_one.find("BEE.DFF")));
    REQUIRE(bee.geometries[0].skin.has_value());
    REQUIRE(bee.geometries[0].skin->bone_count == 20);
    REQUIRE(bee.geometries[0].skin->max_weights == 4);
    REQUIRE(shadow::gc::skin_problems(bee.geometries[0].skin->bone_count, bee.geometries[0].vertex_count,
                                      bee.geometries[0].skin->indices, bee.geometries[0].skin->weights)
                .empty());
    const auto bee_bon = shadow::pc::to_native_bon(shadow::gc::parse_bon(bee_one.load(*bee_one.find("BEE.BON"))), "BEE.BON");
    const auto wing_bon = shadow::pc::to_native_bon(shadow::gc::parse_bon(bee_one.load(*bee_one.find("BEEWING.BON"))), "BEEWING.BON");
    const auto bee_pack = shadow::pc::to_native_motion_pack(shadow::gc::parse_motion_pack(bee_one.load(*bee_one.find("BEE.MTP"))), "BEE.MTP");
    const auto bee_map = shadow::pc::map_skeleton_channels(bee_bon);
    const auto wing_map = shadow::pc::map_skeleton_channels(wing_bon);
    REQUIRE(bee_map.problems.empty());
    REQUIRE(wing_map.problems.empty());
    REQUIRE(bee_map.bindings.size() == 237);
    REQUIRE(wing_map.bindings.size() == 45);
    std::size_t bee_body_motions = 0;
    std::size_t bee_wing_motions = 0;
    for (const auto& motion : bee_pack.motions) {
        const shadow::pc::ChannelMap* map = nullptr;
        const shadow::pc::NativeBon* skeleton = nullptr;
        if (motion.inner_name == bee_bon.skeleton_name) {
            map = &bee_map;
            skeleton = &bee_bon;
            ++bee_body_motions;
        } else if (motion.inner_name == wing_bon.skeleton_name) {
            map = &wing_map;
            skeleton = &wing_bon;
            ++bee_wing_motions;
        }
        REQUIRE(map != nullptr);
        REQUIRE(motion.tracks.size() == map->bindings.size());
        const auto posed = shadow::pc::animate_locals(*skeleton, motion, *map, 0.2f);
        for (const auto& basis : posed.local) {
            for (const float value : basis) {
                REQUIRE(std::isfinite(value));
            }
        }
    }
    REQUIRE(bee_body_motions + bee_wing_motions == bee_pack.motions.size());
    REQUIRE(bee_body_motions > 0);

    const auto bee_model = shadow::pc::to_native_model(bee, "BEE.DFF");
    check_character_skin("BEE bind", bee_model, shadow::pc::bind_globals(bee_model), false);
    REQUIRE(bee_model.meshes[0].skin->max_weights == 4);
    REQUIRE(bee.geometries[0].skin->weights.size() == bee.geometries[0].vertex_count * 4u);
    int significant[4] = {};
    for (std::uint32_t vertex = 0; vertex < bee.geometries[0].vertex_count; ++vertex) {
        const float* weight = bee.geometries[0].skin->weights.data() + static_cast<std::size_t>(vertex) * 4u;
        for (int slot = 0; slot < 4; ++slot) {
            if (weight[slot] > 0.01f) {
                ++significant[slot];
            }
        }
    }
    REQUIRE(significant[0] == static_cast<int>(bee.geometries[0].vertex_count));
    REQUIRE(significant[1] > 0);
    REQUIRE(significant[2] == 0);
    REQUIRE(significant[3] == 0);

    std::vector<std::uint8_t> bad_indices = {40, 0, 0, 0};
    std::vector<float> bad_weights = {1.f, 0.f, 0.f, 0.f};
    REQUIRE(!shadow::gc::skin_problems(30, 1, bad_indices, bad_weights).empty());
    bad_weights[0] = 0.25f;
    bad_indices[0] = 1;
    REQUIRE(!shadow::gc::skin_problems(30, 1, bad_indices, bad_weights).empty());

    auto broken_bon = bon;
    broken_bon.nodes[0].child_a = 0x00FFFFFFu;
    REQUIRE(!shadow::gc::bon_problems(broken_bon).empty());

    bool bad_motion = false;
    try {
        std::vector<std::uint8_t> broken_motion(64, 0);
        broken_motion[0] = 4;
        broken_motion[1] = 1;
        broken_motion[11] = 64;
        shadow::gc::parse_motion(broken_motion);
    } catch (const shadow::ParseError&) {
        bad_motion = true;
    }
    REQUIRE(bad_motion);

    shadow::gc::Motion nan_motion;
    shadow::gc::MotionChunk nan_chunk;
    nan_chunk.groups_split = true;
    nan_chunk.group_count = 1;
    nan_chunk.keys.push_back(shadow::gc::MotionKey{});
    nan_chunk.keys[0].time = std::numeric_limits<float>::quiet_NaN();
    nan_motion.chunks.push_back(nan_chunk);
    REQUIRE(!shadow::gc::motion_problems(nan_motion).empty());

    shadow::pc::NativeModel missing;
    missing.bones.resize(1);
    missing.bones[0].parent = -1;
    missing.bones[0].hanim_id = -1;
    missing.hierarchies.resize(1);
    missing.hierarchies[0].root_frame = -1;
    missing.hierarchies[0].nodes.push_back(shadow::pc::NativeHierarchyNode{7, 0, 0});
    REQUIRE(!shadow::pc::hierarchy_problems(missing).empty());
    missing.hierarchies[0].nodes.push_back(shadow::pc::NativeHierarchyNode{8, 0, 0});
    REQUIRE(!shadow::pc::hierarchy_problems(missing).empty());

    const auto bon_handle = system.load(LoadRequest{"character/shadow.one", "SH.BON"});
    const auto* native_bon = system.bon(bon_handle);
    REQUIRE(native_bon != nullptr);
    REQUIRE(native_bon->nodes.size() == 30);
    const auto pack_handle = system.load(LoadRequest{"character/shadow.one", "SHADOW.MTP"});
    const auto* native_pack = system.motion_pack(pack_handle);
    REQUIRE(native_pack != nullptr);
    REQUIRE(native_pack->motions.size() == 158);

    const auto dat = shadow::gc::open_one(file_bytes(content() / "stg0100/stg0100_dat.one"));
    const auto* ptp = dat.find("PATH.PTP");
    REQUIRE(ptp != nullptr);
    const auto ptp_bytes = dat.load(*ptp);
    REQUIRE(ptp_bytes.size() == 0x2374);
    REQUIRE(shadow::gc::looks_sized_blob(ptp_bytes));

    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "asset_tests passed\n";
    return 0;
}
