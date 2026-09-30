#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace shadow::gc {

// Little-endian RenderWare chunk stream used by the shipped ONE payloads and
// loose TXD/DFF/BSP files. Chunk headers are LE (id, size, library id).
// Generic structs (frames, geometry, materials) are LE.
// GameCube native texture headers inside rwID 0x15 are big-endian.
// Those endians were established by nested sizes and by values that only make
// sense in one endian (geometry flags 0x00010037, texture filter 0x00001101).

constexpr std::uint32_t kRwStruct = 0x01;
constexpr std::uint32_t kRwString = 0x02;
constexpr std::uint32_t kRwExtension = 0x03;
constexpr std::uint32_t kRwTexture = 0x06;
constexpr std::uint32_t kRwMaterial = 0x07;
constexpr std::uint32_t kRwMatList = 0x08;
constexpr std::uint32_t kRwWorld = 0x0B;
constexpr std::uint32_t kRwFrameList = 0x0E;
constexpr std::uint32_t kRwGeometry = 0x0F;
constexpr std::uint32_t kRwClump = 0x10;
constexpr std::uint32_t kRwAtomic = 0x14;
constexpr std::uint32_t kRwTexNative = 0x15;
constexpr std::uint32_t kRwTexDictionary = 0x16;
constexpr std::uint32_t kRwGeometryList = 0x1A;
constexpr std::uint32_t kRwAnim = 0x1B;
constexpr std::uint32_t kRwDeltaMorph = 0x1E;
constexpr std::uint32_t kRwRegion = 0x29;
constexpr std::uint32_t kRwUvAnimDict = 0x2B;
constexpr std::uint32_t kRwSkin = 0x116;
constexpr std::uint32_t kRwHAnim = 0x11E;
constexpr std::uint32_t kRwUserData = 0x11F;
constexpr std::uint32_t kRwMatFx = 0x120;
constexpr std::uint32_t kRwBinMesh = 0x50E;

struct RawChunk {
    std::uint32_t id = 0;
    std::uint32_t library_id = 0;
    std::vector<std::uint8_t> bytes;
};

struct HAnimNode {
    std::int32_t id = 0;
    std::int32_t index = 0;
    std::int32_t flags = 0;
};

// 12-byte form is one node id on a frame. The longer form is the hierarchy
// on the root frame: 20-byte header plus 12-byte nodes. Byte fit was checked
// on SHADOW_BODY.DFF (30 nodes, chunk size 0x17C).
struct HAnim {
    std::int32_t version = 0;
    std::int32_t node_id = 0;
    std::int32_t flags = 0;
    std::int32_t extra = 0;
    bool hierarchy = false;
    std::vector<HAnimNode> nodes;
};

struct Frame {
    // Twelve little-endian floats. The root frame of a clump is the identity
    // basis (1,0,0, 0,1,0, 0,0,1, 0,0,0), so the storage order is right, up, at, position.
    float basis[12]{};
    std::int32_t parent = 0;
    std::uint32_t matrix_flags = 0;
    std::vector<HAnim> hanim;
    std::vector<RawChunk> unknown;
};

struct Material {
    std::uint32_t flags = 0;
    std::uint8_t color[4]{};
    std::uint32_t unused_word = 0;
    std::int32_t textured = 0;
    float ambient = 0.f;
    float specular = 0.f;
    float diffuse = 0.f;
    bool has_texture = false;
    std::uint32_t filter_addressing = 0;
    std::string texture_name;
    std::string mask_name;
    bool has_matfx = false;
    std::uint32_t matfx_type = 0;
    std::vector<std::uint8_t> matfx;
    std::vector<RawChunk> unknown;
};

struct Triangle {
    std::uint16_t v0 = 0;
    std::uint16_t v1 = 0;
    std::uint16_t v2 = 0;
    // Fourth u16 of the 8-byte triangle record. It is not a material index
    // (values exceed the material count). Purpose is unresolved.
    std::uint16_t extra = 0;
};

struct MorphTarget {
    float sphere[4]{};
    std::int32_t has_positions = 0;
    std::int32_t has_normals = 0;
    std::vector<std::array<float, 3>> positions;
    std::vector<std::array<float, 3>> normals;
};

struct BinMesh {
    std::uint32_t material = 0;
    std::vector<std::uint32_t> indices;
};

// Skin plugin 0x116, CPU path 0x8044D7A4 (native geometry bit 0x01000000 clear).
// The header word is swapped, then split into bytes. Indices are four file-order
// bytes per vertex: 0x8044E684 and 0x8044DB00 take influence i from byte i.
// Weights are four little-endian floats. Matrices are bone_count contiguous
// 16-float blocks (right, up, at, position). 0x8044CC4C then reads three u32s;
// the middle one gates an extra raw tail.
struct Skin {
    bool recognized = false;
    std::uint32_t bone_count = 0;
    std::uint32_t used_count = 0;
    std::uint32_t max_weights = 0;
    std::uint32_t header_pad = 0;
    std::vector<std::uint8_t> used_bones;
    std::vector<std::uint8_t> indices;
    std::vector<float> weights;
    std::vector<float> bone_matrices;
    std::uint32_t trailer[3]{};
    std::vector<std::uint8_t> split_data;
};

struct UserData {
    std::vector<std::uint8_t> bytes;
    std::vector<std::string> strings;
};

struct Geometry {
    std::uint32_t flags = 0;
    std::uint32_t vertex_count = 0;
    std::uint32_t texcoord_sets = 0;
    bool native = false;
    bool surface_in_struct = false;
    float struct_ambient = 0.f;
    float struct_specular = 0.f;
    float struct_diffuse = 0.f;
    std::vector<std::uint8_t> prelit_rgba;
    std::vector<std::vector<std::array<float, 2>>> uv_sets;
    std::vector<Triangle> triangles;
    std::vector<MorphTarget> morphs;
    std::vector<Material> materials;
    std::uint32_t bin_mesh_flags = 0;
    std::vector<BinMesh> bin_meshes;
    std::optional<Skin> skin;
    std::vector<UserData> user_data;
    std::vector<RawChunk> unknown;
    std::vector<std::uint8_t> native_body;
};

struct Atomic {
    std::uint32_t frame = 0;
    std::uint32_t geometry = 0;
    std::uint32_t flags = 0;
    std::uint32_t unused = 0;
    std::vector<RawChunk> unknown;
};

struct Clump {
    std::int32_t num_atomics = 0;
    std::int32_t num_lights = 0;
    std::int32_t num_cameras = 0;
    std::vector<Frame> frames;
    std::vector<Geometry> geometries;
    std::vector<Atomic> atomics;
    std::vector<RawChunk> lights;
    std::vector<RawChunk> cameras;
    std::vector<RawChunk> unknown;
};

struct Texture {
    std::uint32_t platform_id = 0;
    std::uint32_t filter_addressing = 0;
    std::uint32_t unknown_08 = 0;
    std::uint32_t unknown_0c = 0;
    std::uint32_t unknown_10 = 0;
    std::uint32_t unknown_14 = 0;
    std::string name;
    std::string mask;
    bool raster_recognized = false;
    std::uint32_t raster_format = 0;
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::uint8_t depth = 0;
    std::uint8_t levels = 0;
    std::uint8_t kind = 0;
    std::uint8_t format_param = 0;
    std::uint32_t raster_tail = 0;
    // Bytes after the 16-byte raster header. No GameCube 1024-wide ceiling is applied.
    // The image stays in file order; a tile decode is not claimed.
    std::vector<std::uint8_t> image;
    std::vector<std::uint8_t> raw_struct;
};

struct TexDictionary {
    std::uint16_t count_field = 0;
    // Second u16 of the 4-byte dictionary struct. Observed value is 3. Purpose unresolved.
    std::uint16_t device_field = 0;
    std::vector<Texture> textures;
};

struct World {
    std::uint32_t header_words[10]{};
    // Six floats at the end of the 64-byte world struct. They are two vec3 bounds.
    // Which one is min versus max is not renamed: the file order is kept.
    float bound_a[3]{};
    float bound_b[3]{};
    std::vector<Material> materials;
    // Chunks after the material list (observed id 0x09 and its extension).
    // Their internal vertex layout was not traced, so the bytes stay intact.
    std::vector<RawChunk> sections;
};

struct Region {
    bool has_prelude = false;
    // Every sampled .RG1 ends with an empty chunk id 0x2A. Its purpose is unresolved.
    bool has_end_marker = false;
    std::uint32_t prelude[4]{};
    std::string name;
    World world;
};

struct UvAnimDictionary {
    std::uint32_t count = 0;
    std::vector<RawChunk> animations;
};

struct DeltaMorph {
    std::uint32_t library_id = 0;
    std::vector<std::uint8_t> payload;
};

std::uint32_t root_chunk_id(std::span<const std::uint8_t> bytes);

Clump parse_clump(std::span<const std::uint8_t> bytes);
TexDictionary parse_tex_dictionary(std::span<const std::uint8_t> bytes);
Region parse_world_or_region(std::span<const std::uint8_t> bytes);
UvAnimDictionary parse_uvanim_dictionary(std::span<const std::uint8_t> bytes);
DeltaMorph parse_delta_morph(std::span<const std::uint8_t> bytes);

}  // namespace shadow::gc
