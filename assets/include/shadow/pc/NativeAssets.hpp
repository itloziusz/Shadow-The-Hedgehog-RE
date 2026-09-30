#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace shadow::pc {

// Host-endian assets. Counts, bones, vertices, textures and stage parts grow
// with the file. Nothing here is a GameCube arena, a 32-bit pointer, a
// 15-slot child table, or a GX 1024 texture ceiling.

struct NativeMaterial {
    std::array<std::uint8_t, 4> color{255, 255, 255, 255};
    float ambient = 1.f;
    float specular = 1.f;
    float diffuse = 1.f;
    std::uint32_t filter_addressing = 0;
    bool has_texture = false;
    std::string texture_name;
    std::string mask_name;
    bool has_matfx = false;
    std::uint32_t matfx_type = 0;
    std::vector<std::uint8_t> matfx;
};

struct NativeBinMesh {
    std::uint32_t material = 0;
    std::vector<std::uint32_t> indices;
};

struct NativeSkin {
    std::uint32_t bone_count = 0;
    std::uint32_t used_count = 0;
    std::uint32_t max_weights = 0;
    std::uint32_t header_pad = 0;
    std::vector<std::uint8_t> used_bones;
    // Four indices and four weights per vertex. The plugin always stores four
    // slots; max_weights is the file's declared used-slot count, not a PC cap.
    std::vector<std::uint8_t> indices;
    std::vector<float> weights;
    std::vector<std::array<float, 16>> bone_matrices;
    std::uint32_t trailer[3]{};
    std::vector<std::uint8_t> split_data;
};

// One 6-byte group from a motion chunk whose payload length is
// prefix_length + count * 6. time/value_* use the conversions at 0x80414B80.
struct NativeKeyframe {
    std::uint16_t header = 0;
    std::uint16_t component_b = 0;
    std::uint16_t component_c = 0;
    float time = 0.f;
    float value_b = 0.f;
    float value_c = 0.f;
};

struct NativeTrack {
    std::uint32_t size = 0;
    std::uint16_t group_count = 0;
    std::uint8_t prefix_length = 0;
    std::uint8_t link = 0;
    bool groups_split = false;
    std::vector<std::uint8_t> prefix;
    std::vector<NativeKeyframe> keys;
    std::vector<std::uint8_t> payload;
};

struct NativeMotion {
    std::string name;
    std::uint8_t version = 0;
    std::uint8_t endian_flag = 0;
    std::uint32_t word_4 = 0;
    std::uint32_t byte_size = 0;
    std::uint8_t flag_13 = 0;
    std::uint16_t word_16 = 0;
    std::uint16_t word_18 = 0;
    std::string inner_name;
    std::vector<NativeTrack> tracks;
    std::vector<std::uint8_t> extra;
};

struct NativeMotionPack {
    std::string name;
    std::uint16_t word_0 = 0;
    std::uint16_t motion_count = 0;
    std::uint32_t word_4 = 0;
    std::uint32_t word_8 = 0;
    std::uint32_t word_12 = 0;
    std::uint8_t endian_flag = 0;
    std::vector<NativeMotion> motions;
};

// BON node. child_a/child_b are file offsets of the next nodes in the channel
// walk (0x8041C30C), not skeleton parents. translation matches the DFF frame
// with the same bone_id. words_32 is the default rotation vector read by
// 0x80420B9C. words_48 is the default scale vector read after that rotation.
// halves[1] is the channel mask at node+6.
struct NativeBonNode {
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

struct NativeBon {
    std::string name;
    std::uint8_t version = 0;
    std::uint8_t endian_flag = 0;
    std::uint16_t header_2 = 0;
    std::uint32_t word_4 = 0;
    std::uint16_t header_8 = 0;
    std::uint16_t node_count = 0;
    float header_12 = 0.f;
    std::string skeleton_name;
    std::vector<NativeBonNode> nodes;
};

struct NativeMesh {
    std::uint32_t source_flags = 0;
    std::vector<std::array<float, 3>> positions;
    std::vector<std::array<float, 3>> normals;
    std::vector<std::array<float, 2>> uv;
    std::vector<std::uint8_t> prelit_rgba;
    std::vector<std::array<std::uint16_t, 3>> triangles;
    std::vector<std::uint16_t> triangle_extra;
    std::uint32_t bin_mesh_flags = 0;
    std::vector<NativeBinMesh> bin_meshes;
    std::vector<NativeMaterial> materials;
    std::optional<NativeSkin> skin;
};

struct NativeBone {
    std::int32_t parent = -1;
    float basis[12]{};
    std::uint32_t matrix_flags = 0;
    std::int32_t hanim_id = -1;
    std::uint32_t hanim_flags = 0;
};

struct NativeHierarchyNode {
    std::int32_t id = 0;
    std::int32_t index = 0;
    std::int32_t flags = 0;
};

struct NativeHierarchy {
    std::int32_t root_frame = -1;
    std::int32_t version = 0;
    std::int32_t node_id = 0;
    std::int32_t flags = 0;
    std::int32_t extra = 0;
    std::vector<NativeHierarchyNode> nodes;
};

struct NativeAtomic {
    std::uint32_t frame = 0;
    std::uint32_t mesh = 0;
    std::uint32_t flags = 0;
    std::uint32_t unused = 0;
};

struct NativeModel {
    std::string name;
    std::vector<NativeBone> bones;
    std::vector<NativeHierarchy> hierarchies;
    std::vector<NativeMesh> meshes;
    std::vector<NativeAtomic> atomics;
};

struct NativeTexture {
    std::string name;
    std::string mask;
    std::uint32_t platform_id = 0;
    std::uint32_t filter_addressing = 0;
    std::uint32_t raster_format = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint8_t depth = 0;
    std::uint8_t levels = 0;
    std::uint8_t kind = 0;
    std::uint8_t format_param = 0;
    std::uint32_t raster_tail = 0;
    std::vector<std::uint8_t> image;
};

struct NativeTextureDictionary {
    std::string name;
    std::uint32_t device_field = 0;
    std::vector<NativeTexture> textures;
};

struct NativeWorld {
    std::string name;
    float bound_a[3]{};
    float bound_b[3]{};
    std::uint32_t header_words[10]{};
    std::vector<NativeMaterial> materials;
    std::vector<std::pair<std::uint32_t, std::vector<std::uint8_t>>> sections;
};

struct NativeBlob {
    std::string kind;
    std::string name;
    std::vector<std::uint8_t> bytes;
    std::vector<std::string> notes;
};

struct Dependency {
    std::string kind;
    std::string archive;
    std::string entry;
    std::string name;
    bool resolved = false;
};

}  // namespace shadow::pc
