#include "shadow/gc/RenderWare.hpp"

#include "shadow/BinaryReader.hpp"

namespace shadow::gc {
namespace {

std::uint32_t le32_at(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        throw ParseError("RenderWare chunk header is truncated");
    }
    return static_cast<std::uint32_t>(bytes[offset] | (bytes[offset + 1] << 8) |
                                       (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24));
}

struct ChunkView {
    std::uint32_t id = 0;
    std::uint32_t size = 0;
    std::uint32_t library_id = 0;
    std::span<const std::uint8_t> body;
};

class ChunkStream {
public:
    explicit ChunkStream(std::span<const std::uint8_t> data) : data_(data) {}

    bool done() const noexcept { return pos_ == data_.size(); }

    ChunkView read() {
        if (data_.size() - pos_ < 12) {
            throw ParseError("RenderWare chunk header is truncated");
        }
        const std::uint32_t id = le32_at(data_, pos_);
        const std::uint32_t size = le32_at(data_, pos_ + 4);
        const std::uint32_t library = le32_at(data_, pos_ + 8);
        if (static_cast<std::size_t>(size) > data_.size() - pos_ - 12) {
            throw ParseError("RenderWare chunk size exceeds its parent");
        }
        ChunkView view;
        view.id = id;
        view.size = size;
        view.library_id = library;
        view.body = data_.subspan(pos_ + 12, size);
        pos_ += 12 + static_cast<std::size_t>(size);
        return view;
    }

private:
    std::span<const std::uint8_t> data_;
    std::size_t pos_ = 0;
};

void expect(const ChunkView& chunk, std::uint32_t id, const char* what) {
    if (chunk.id != id) {
        throw ParseError(std::string(what) + " chunk id is " + std::to_string(chunk.id));
    }
}

std::string chunk_string(std::span<const std::uint8_t> body) {
    std::size_t n = 0;
    while (n < body.size() && body[n] != 0) {
        ++n;
    }
    return std::string(reinterpret_cast<const char*>(body.data()), n);
}

RawChunk raw_from(const ChunkView& chunk) {
    RawChunk raw;
    raw.id = chunk.id;
    raw.library_id = chunk.library_id;
    raw.bytes.assign(chunk.body.begin(), chunk.body.end());
    return raw;
}

std::vector<std::string> printable_strings(std::span<const std::uint8_t> bytes) {
    std::vector<std::string> found;
    std::string run;
    auto flush = [&]() {
        if (run.size() >= 4) {
            found.push_back(run);
        }
        run.clear();
    };
    for (std::uint8_t byte : bytes) {
        if (byte >= 32 && byte < 127) {
            run.push_back(static_cast<char>(byte));
        } else {
            flush();
        }
    }
    flush();
    return found;
}

HAnim parse_hanim(std::span<const std::uint8_t> body) {
    BinaryReader reader(body, "HAnim");
    HAnim anim;
    if (body.size() == 12) {
        anim.version = reader.i32le();
        anim.node_id = reader.i32le();
        anim.flags = reader.i32le();
        return anim;
    }
    if (body.size() < 20) {
        throw ParseError("HAnim chunk is neither 12 bytes nor a hierarchy");
    }
    anim.hierarchy = true;
    anim.version = reader.i32le();
    anim.node_id = reader.i32le();
    const auto node_count = reader.i32le();
    anim.flags = reader.i32le();
    anim.extra = reader.i32le();
    if (node_count < 0) {
        throw ParseError("HAnim node count is negative");
    }
    const auto bytes = checked_mul(static_cast<std::size_t>(node_count), 12, "HAnim nodes");
    if (bytes != body.size() - 20) {
        throw ParseError("HAnim node table does not fill the chunk");
    }
    anim.nodes.resize(static_cast<std::size_t>(node_count));
    for (HAnimNode& node : anim.nodes) {
        node.id = reader.i32le();
        node.index = reader.i32le();
        node.flags = reader.i32le();
    }
    return anim;
}

Material parse_material(const ChunkView& chunk) {
    expect(chunk, kRwMaterial, "material");
    ChunkStream stream(chunk.body);
    const ChunkView structured = stream.read();
    expect(structured, kRwStruct, "material struct");
    if (structured.body.size() != 0x1C) {
        throw ParseError("material struct is not 28 bytes");
    }
    BinaryReader reader(structured.body, "material struct");
    Material material;
    material.flags = reader.u32le();
    auto color = reader.read(4);
    for (int i = 0; i < 4; ++i) {
        material.color[i] = color[static_cast<std::size_t>(i)];
    }
    material.unused_word = reader.u32le();
    material.textured = reader.i32le();
    material.ambient = reader.f32le();
    material.specular = reader.f32le();
    material.diffuse = reader.f32le();

    while (!stream.done()) {
        const ChunkView child = stream.read();
        if (child.id == kRwTexture) {
            ChunkStream texture_stream(child.body);
            const ChunkView filter = texture_stream.read();
            expect(filter, kRwStruct, "texture struct");
            if (filter.body.size() < 4) {
                throw ParseError("texture filter struct is shorter than 4 bytes");
            }
            material.filter_addressing = le32_at(filter.body, 0);
            const ChunkView name = texture_stream.read();
            expect(name, kRwString, "texture name");
            material.texture_name = chunk_string(name.body);
            const ChunkView mask = texture_stream.read();
            expect(mask, kRwString, "texture mask");
            material.mask_name = chunk_string(mask.body);
            while (!texture_stream.done()) {
                const ChunkView extra = texture_stream.read();
                if (extra.id == kRwExtension) {
                    ChunkStream extension(extra.body);
                    while (!extension.done()) {
                        material.unknown.push_back(raw_from(extension.read()));
                    }
                } else {
                    material.unknown.push_back(raw_from(extra));
                }
            }
            material.has_texture = true;
        } else if (child.id == kRwExtension) {
            ChunkStream extension(child.body);
            while (!extension.done()) {
                const ChunkView plugin = extension.read();
                if (plugin.id == kRwMatFx) {
                    material.has_matfx = true;
                    material.matfx.assign(plugin.body.begin(), plugin.body.end());
                    if (plugin.body.size() >= 4) {
                        material.matfx_type = le32_at(plugin.body, 0);
                    }
                } else {
                    material.unknown.push_back(raw_from(plugin));
                }
            }
        } else {
            throw ParseError("unexpected chunk inside a material");
        }
    }
    return material;
}

std::vector<Material> parse_matlist(const ChunkView& chunk) {
    expect(chunk, kRwMatList, "material list");
    ChunkStream stream(chunk.body);
    const ChunkView structured = stream.read();
    expect(structured, kRwStruct, "material list struct");
    BinaryReader reader(structured.body, "material list");
    const auto count = reader.i32le();
    if (count < 0) {
        throw ParseError("material count is negative");
    }
    const auto index_bytes = checked_mul(static_cast<std::size_t>(count), 4, "material indices");
    if (index_bytes + 4 != structured.body.size()) {
        throw ParseError("material index table does not match the struct size");
    }
    std::vector<std::int32_t> indices(static_cast<std::size_t>(count));
    for (std::int32_t& index : indices) {
        index = reader.i32le();
    }
    std::vector<Material> materials;
    materials.reserve(indices.size());
    for (std::int32_t index : indices) {
        if (index == -1) {
            materials.push_back(parse_material(stream.read()));
        } else if (index >= 0 && static_cast<std::size_t>(index) < materials.size()) {
            materials.push_back(materials[static_cast<std::size_t>(index)]);
        } else {
            throw ParseError("material index is out of range");
        }
    }
    if (!stream.done()) {
        throw ParseError("material list has trailing bytes");
    }
    return materials;
}

Skin parse_skin(std::span<const std::uint8_t> body, std::uint32_t vertex_count) {
    BinaryReader reader(body, "skin");
    Skin skin;
    skin.bone_count = reader.u8();
    skin.used_count = reader.u8();
    skin.max_weights = reader.u8();
    skin.header_pad = reader.u8();
    const auto used = reader.read(skin.used_count);
    skin.used_bones.assign(used.begin(), used.end());
    const auto index_bytes = checked_mul(vertex_count, 4, "skin indices");
    const auto index_raw = reader.read(index_bytes);
    skin.indices.assign(index_raw.begin(), index_raw.end());
    const auto weight_count = checked_mul(vertex_count, 4, "skin weights");
    skin.weights.resize(weight_count);
    for (float& weight : skin.weights) {
        weight = reader.f32le();
    }
    const auto matrix_floats = checked_mul(skin.bone_count, 16, "skin matrices");
    skin.bone_matrices.resize(matrix_floats);
    for (float& value : skin.bone_matrices) {
        value = reader.f32le();
    }
    skin.trailer[0] = reader.u32le();
    skin.trailer[1] = reader.u32le();
    skin.trailer[2] = reader.u32le();
    if (skin.trailer[1] > 0) {
        const auto tail = checked_add(skin.bone_count, checked_mul(skin.trailer[1], 2, "skin split"), "skin split");
        const auto bytes = checked_add(tail, checked_mul(skin.trailer[2], 2, "skin split"), "skin split");
        const auto raw = reader.read(bytes);
        skin.split_data.assign(raw.begin(), raw.end());
    }
    if (!reader.exhausted()) {
        throw ParseError("skin chunk has bytes the reader at 0x8044D7A4 does not consume");
    }
    skin.recognized = true;
    return skin;
}

void parse_geometry_extension(Geometry& geometry, const ChunkView& chunk) {
    expect(chunk, kRwExtension, "geometry extension");
    ChunkStream stream(chunk.body);
    while (!stream.done()) {
        const ChunkView plugin = stream.read();
        if (plugin.id == kRwBinMesh) {
            BinaryReader reader(plugin.body, "binmesh");
            geometry.bin_mesh_flags = reader.u32le();
            const auto mesh_count = reader.u32le();
            const auto total = reader.u32le();
            if (static_cast<std::uint64_t>(mesh_count) > plugin.body.size() / 8u) {
                throw ParseError("binmesh count does not fit in the chunk");
            }
            std::uint64_t sum = 0;
            geometry.bin_meshes.reserve(mesh_count);
            for (std::uint32_t mesh_index = 0; mesh_index < mesh_count; ++mesh_index) {
                BinMesh mesh;
                const auto index_count = reader.u32le();
                mesh.material = reader.u32le();
                sum += index_count;
                mesh.indices.resize(index_count);
                for (std::uint32_t& index : mesh.indices) {
                    index = reader.u32le();
                }
                geometry.bin_meshes.push_back(std::move(mesh));
            }
            if (sum != total || !reader.exhausted()) {
                throw ParseError("binmesh index table does not match its header");
            }
        } else if (plugin.id == kRwSkin) {
            if (geometry.native) {
                geometry.unknown.push_back(raw_from(plugin));
            } else {
                geometry.skin = parse_skin(plugin.body, geometry.vertex_count);
            }
        } else if (plugin.id == kRwUserData) {
            UserData data;
            data.bytes.assign(plugin.body.begin(), plugin.body.end());
            data.strings = printable_strings(plugin.body);
            geometry.user_data.push_back(std::move(data));
        } else {
            geometry.unknown.push_back(raw_from(plugin));
        }
    }
}

bool fill_geometry_arrays(Geometry& geometry, std::span<const std::uint8_t> body, bool with_surface) {
    BinaryReader reader(body, "geometry");
    geometry.flags = reader.u32le();
    const auto triangle_count = reader.u32le();
    const auto vertex_count = reader.u32le();
    const auto morph_count = reader.u32le();
    geometry.vertex_count = vertex_count;
    geometry.native = (geometry.flags & 0x01000000u) != 0;
    geometry.surface_in_struct = with_surface;
    if (with_surface) {
        if (reader.tell() + 12 > body.size()) {
            return false;
        }
        geometry.struct_ambient = reader.f32le();
        geometry.struct_specular = reader.f32le();
        geometry.struct_diffuse = reader.f32le();
    }
    if (geometry.native) {
        geometry.native_body.assign(body.begin() + static_cast<std::ptrdiff_t>(reader.tell()), body.end());
        return true;
    }
    std::uint32_t sets = (geometry.flags >> 16) & 0xFFu;
    if (sets == 0) {
        if ((geometry.flags & 0x80u) != 0) {
            sets = 2;
        } else if ((geometry.flags & 0x04u) != 0) {
            sets = 1;
        }
    }
    geometry.texcoord_sets = sets;
    try {
        if ((geometry.flags & 0x08u) != 0) {
            const auto bytes = checked_mul(vertex_count, 4, "prelit");
            auto raw = reader.read(bytes);
            geometry.prelit_rgba.assign(raw.begin(), raw.end());
        }
        geometry.uv_sets.resize(sets);
        for (std::uint32_t set = 0; set < sets; ++set) {
            const auto bytes = checked_mul(vertex_count, 8, "uv");
            if (reader.size() - reader.tell() < bytes) {
                return false;
            }
            geometry.uv_sets[set].resize(vertex_count);
            for (auto& uv : geometry.uv_sets[set]) {
                uv = {reader.f32le(), reader.f32le()};
            }
        }
        if (reader.size() - reader.tell() < checked_mul(triangle_count, 8, "triangles")) {
            return false;
        }
        geometry.triangles.resize(triangle_count);
        for (Triangle& triangle : geometry.triangles) {
            triangle.v0 = reader.u16le();
            triangle.v1 = reader.u16le();
            triangle.v2 = reader.u16le();
            triangle.extra = reader.u16le();
            if (triangle.v0 >= vertex_count || triangle.v1 >= vertex_count || triangle.v2 >= vertex_count) {
                return false;
            }
        }
        geometry.morphs.resize(morph_count);
        for (MorphTarget& morph : geometry.morphs) {
            for (float& value : morph.sphere) {
                value = reader.f32le();
            }
            morph.has_positions = reader.i32le();
            morph.has_normals = reader.i32le();
            if (morph.has_positions) {
                if (reader.size() - reader.tell() < checked_mul(vertex_count, 12, "positions")) {
                    return false;
                }
                morph.positions.resize(vertex_count);
                for (auto& position : morph.positions) {
                    position = {reader.f32le(), reader.f32le(), reader.f32le()};
                }
            }
            if (morph.has_normals) {
                if (reader.size() - reader.tell() < checked_mul(vertex_count, 12, "normals")) {
                    return false;
                }
                morph.normals.resize(vertex_count);
                for (auto& normal : morph.normals) {
                    normal = {reader.f32le(), reader.f32le(), reader.f32le()};
                }
            }
        }
    } catch (const ParseError&) {
        return false;
    }
    return reader.exhausted();
}

Geometry parse_geometry(const ChunkView& chunk) {
    expect(chunk, kRwGeometry, "geometry");
    ChunkStream stream(chunk.body);
    const ChunkView structured = stream.read();
    expect(structured, kRwStruct, "geometry struct");
    Geometry geometry;
    if (!fill_geometry_arrays(geometry, structured.body, false)) {
        geometry = Geometry{};
        if (!fill_geometry_arrays(geometry, structured.body, true)) {
            throw ParseError("geometry struct does not match a known layout");
        }
    }
    while (!stream.done()) {
        const ChunkView child = stream.read();
        if (child.id == kRwMatList) {
            geometry.materials = parse_matlist(child);
        } else if (child.id == kRwExtension) {
            parse_geometry_extension(geometry, child);
        } else {
            throw ParseError("unexpected chunk inside a geometry");
        }
    }
    return geometry;
}

std::vector<Frame> parse_frames(const ChunkView& chunk) {
    expect(chunk, kRwFrameList, "frame list");
    ChunkStream stream(chunk.body);
    const ChunkView structured = stream.read();
    expect(structured, kRwStruct, "frame struct");
    BinaryReader reader(structured.body, "frames");
    const auto count = reader.u32le();
    const auto body_bytes = checked_add(4, checked_mul(count, 56, "frames"), "frames");
    if (body_bytes != structured.body.size()) {
        throw ParseError("frame struct size does not match the frame count");
    }
    std::vector<Frame> frames(count);
    for (Frame& frame : frames) {
        for (float& value : frame.basis) {
            value = reader.f32le();
        }
        frame.parent = reader.i32le();
        frame.matrix_flags = reader.u32le();
    }
    for (Frame& frame : frames) {
        if (stream.done()) {
            throw ParseError("frame list is missing an extension");
        }
        const ChunkView extension = stream.read();
        expect(extension, kRwExtension, "frame extension");
        ChunkStream plugins(extension.body);
        while (!plugins.done()) {
            const ChunkView plugin = plugins.read();
            if (plugin.id == kRwHAnim) {
                frame.hanim.push_back(parse_hanim(plugin.body));
            } else {
                frame.unknown.push_back(raw_from(plugin));
            }
        }
    }
    if (!stream.done()) {
        throw ParseError("frame list has trailing chunks");
    }
    return frames;
}

Atomic parse_atomic(const ChunkView& chunk) {
    expect(chunk, kRwAtomic, "atomic");
    ChunkStream stream(chunk.body);
    const ChunkView structured = stream.read();
    expect(structured, kRwStruct, "atomic struct");
    if (structured.body.size() != 16) {
        throw ParseError("atomic struct is not 16 bytes");
    }
    BinaryReader reader(structured.body, "atomic");
    Atomic atomic;
    atomic.frame = reader.u32le();
    atomic.geometry = reader.u32le();
    atomic.flags = reader.u32le();
    atomic.unused = reader.u32le();
    while (!stream.done()) {
        const ChunkView child = stream.read();
        if (child.id != kRwExtension) {
            throw ParseError("unexpected chunk inside an atomic");
        }
        ChunkStream plugins(child.body);
        while (!plugins.done()) {
            atomic.unknown.push_back(raw_from(plugins.read()));
        }
    }
    return atomic;
}

Texture parse_native_texture(const ChunkView& chunk) {
    expect(chunk, kRwTexNative, "native texture");
    ChunkStream stream(chunk.body);
    const ChunkView structured = stream.read();
    expect(structured, kRwStruct, "native texture struct");
    const ChunkView extension = stream.read();
    expect(extension, kRwExtension, "native texture extension");
    if (!stream.done()) {
        throw ParseError("native texture has trailing chunks");
    }
    Texture texture;
    texture.raw_struct.assign(structured.body.begin(), structured.body.end());
    if (structured.body.size() < 104) {
        return texture;
    }
    // The fields below are big-endian inside an otherwise little-endian stream.
    auto be32 = [&](std::size_t offset) {
        const auto b = structured.body.subspan(offset, 4);
        return (static_cast<std::uint32_t>(b[0]) << 24) | (static_cast<std::uint32_t>(b[1]) << 16) |
               (static_cast<std::uint32_t>(b[2]) << 8) | static_cast<std::uint32_t>(b[3]);
    };
    texture.platform_id = be32(0);
    texture.filter_addressing = be32(4);
    texture.unknown_08 = be32(8);
    texture.unknown_0c = be32(12);
    texture.unknown_10 = be32(16);
    texture.unknown_14 = be32(20);
    texture.name = chunk_string(structured.body.subspan(24, 32));
    texture.mask = chunk_string(structured.body.subspan(56, 32));
    texture.raster_format = be32(88);
    texture.width = static_cast<std::uint16_t>((structured.body[92] << 8) | structured.body[93]);
    texture.height = static_cast<std::uint16_t>((structured.body[94] << 8) | structured.body[95]);
    texture.depth = structured.body[96];
    texture.levels = structured.body[97];
    texture.kind = structured.body[98];
    texture.format_param = structured.body[99];
    texture.raster_tail = be32(100);
    if (texture.width == 0 || texture.height == 0) {
        throw ParseError("native texture " + texture.name + " has no width or height");
    }
    texture.raster_recognized = true;
    texture.image.assign(structured.body.begin() + 104, structured.body.end());
    return texture;
}

World parse_world_chunk(const ChunkView& chunk) {
    expect(chunk, kRwWorld, "world");
    ChunkStream stream(chunk.body);
    const ChunkView structured = stream.read();
    expect(structured, kRwStruct, "world struct");
    if (structured.body.size() != 64) {
        throw ParseError("world struct is not 64 bytes");
    }
    BinaryReader reader(structured.body, "world");
    World world;
    for (std::uint32_t& word : world.header_words) {
        word = reader.u32le();
    }
    for (float& value : world.bound_a) {
        value = reader.f32le();
    }
    for (float& value : world.bound_b) {
        value = reader.f32le();
    }
    if (stream.done()) {
        throw ParseError("world is missing its material list");
    }
    world.materials = parse_matlist(stream.read());
    while (!stream.done()) {
        world.sections.push_back(raw_from(stream.read()));
    }
    return world;
}

}  // namespace

std::uint32_t root_chunk_id(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 4) {
        throw ParseError("asset is too small to contain a chunk id");
    }
    return le32_at(bytes, 0);
}

Clump parse_clump(std::span<const std::uint8_t> bytes) {
    ChunkStream outer(bytes);
    const ChunkView root = outer.read();
    expect(root, kRwClump, "clump");
    if (!outer.done()) {
        throw ParseError("bytes follow the clump chunk");
    }
    ChunkStream stream(root.body);
    const ChunkView structured = stream.read();
    expect(structured, kRwStruct, "clump struct");
    if (structured.body.size() != 12) {
        throw ParseError("clump struct is not 12 bytes");
    }
    BinaryReader reader(structured.body, "clump");
    Clump clump;
    clump.num_atomics = reader.i32le();
    clump.num_lights = reader.i32le();
    clump.num_cameras = reader.i32le();
    if (clump.num_atomics < 0 || clump.num_lights < 0 || clump.num_cameras < 0) {
        throw ParseError("clump count is negative");
    }
    clump.frames = parse_frames(stream.read());
    const ChunkView geometries = stream.read();
    expect(geometries, kRwGeometryList, "geometry list");
    ChunkStream geometry_stream(geometries.body);
    const ChunkView geometry_struct = geometry_stream.read();
    expect(geometry_struct, kRwStruct, "geometry list struct");
    if (geometry_struct.body.size() != 4) {
        throw ParseError("geometry list struct is not 4 bytes");
    }
    const auto geometry_count = le32_at(geometry_struct.body, 0);
    clump.geometries.reserve(geometry_count);
    for (std::uint32_t i = 0; i < geometry_count; ++i) {
        clump.geometries.push_back(parse_geometry(geometry_stream.read()));
    }
    if (!geometry_stream.done()) {
        throw ParseError("geometry list has trailing chunks");
    }
    clump.atomics.reserve(static_cast<std::size_t>(clump.num_atomics));
    for (std::int32_t i = 0; i < clump.num_atomics; ++i) {
        clump.atomics.push_back(parse_atomic(stream.read()));
    }
    for (std::int32_t i = 0; i < clump.num_lights; ++i) {
        if (stream.done()) {
            throw ParseError("clump is missing a light chunk");
        }
        clump.lights.push_back(raw_from(stream.read()));
    }
    for (std::int32_t i = 0; i < clump.num_cameras; ++i) {
        if (stream.done()) {
            throw ParseError("clump is missing a camera chunk");
        }
        clump.cameras.push_back(raw_from(stream.read()));
    }
    while (!stream.done()) {
        const ChunkView extra = stream.read();
        if (extra.id == kRwExtension) {
            ChunkStream plugins(extra.body);
            while (!plugins.done()) {
                clump.unknown.push_back(raw_from(plugins.read()));
            }
        } else {
            clump.unknown.push_back(raw_from(extra));
        }
    }
    for (const Frame& frame : clump.frames) {
        if (frame.parent < -1 || frame.parent >= static_cast<std::int32_t>(clump.frames.size())) {
            throw ParseError("frame parent is outside the frame list");
        }
    }
    for (const Atomic& atomic : clump.atomics) {
        if (atomic.frame >= clump.frames.size() || atomic.geometry >= clump.geometries.size()) {
            throw ParseError("atomic references a missing frame or geometry");
        }
    }
    return clump;
}

TexDictionary parse_tex_dictionary(std::span<const std::uint8_t> bytes) {
    ChunkStream outer(bytes);
    const ChunkView root = outer.read();
    expect(root, kRwTexDictionary, "texture dictionary");
    if (!outer.done()) {
        throw ParseError("bytes follow the texture dictionary");
    }
    ChunkStream stream(root.body);
    const ChunkView structured = stream.read();
    expect(structured, kRwStruct, "texture dictionary struct");
    if (structured.body.size() < 4) {
        throw ParseError("texture dictionary struct is shorter than 4 bytes");
    }
    TexDictionary dictionary;
    dictionary.count_field = static_cast<std::uint16_t>(structured.body[0] | (structured.body[1] << 8));
    dictionary.device_field = static_cast<std::uint16_t>(structured.body[2] | (structured.body[3] << 8));
    dictionary.textures.reserve(dictionary.count_field);
    for (std::uint16_t i = 0; i < dictionary.count_field; ++i) {
        dictionary.textures.push_back(parse_native_texture(stream.read()));
    }
    while (!stream.done()) {
        const ChunkView extra = stream.read();
        if (extra.id != kRwExtension) {
            throw ParseError("unexpected chunk in a texture dictionary");
        }
        if (!extra.body.empty()) {
            throw ParseError("texture dictionary extension is not empty");
        }
    }
    if (dictionary.textures.size() != dictionary.count_field) {
        throw ParseError("texture count does not match the textures read");
    }
    return dictionary;
}

Region parse_world_or_region(std::span<const std::uint8_t> bytes) {
    ChunkStream stream(bytes);
    const ChunkView first = stream.read();
    Region region;
    if (first.id == kRwRegion) {
        if (first.body.size() < 16) {
            throw ParseError("region prelude is shorter than 16 bytes");
        }
        region.has_prelude = true;
        for (int i = 0; i < 4; ++i) {
            region.prelude[i] = le32_at(first.body, static_cast<std::size_t>(i) * 4);
        }
        region.name = chunk_string(first.body.subspan(16));
        region.world = parse_world_chunk(stream.read());
    } else if (first.id == kRwWorld) {
        region.world = parse_world_chunk(first);
    } else {
        throw ParseError("asset is neither a world nor a region");
    }
    if (!stream.done()) {
        const ChunkView tail = stream.read();
        if (tail.id != 0x2A || tail.size != 0 || !stream.done()) {
            throw ParseError("bytes follow the world");
        }
        region.has_end_marker = true;
    }
    return region;
}

UvAnimDictionary parse_uvanim_dictionary(std::span<const std::uint8_t> bytes) {
    ChunkStream outer(bytes);
    const ChunkView root = outer.read();
    expect(root, kRwUvAnimDict, "uvanim dictionary");
    if (!outer.done()) {
        throw ParseError("bytes follow the uvanim dictionary");
    }
    ChunkStream stream(root.body);
    const ChunkView structured = stream.read();
    expect(structured, kRwStruct, "uvanim struct");
    if (structured.body.size() < 4) {
        throw ParseError("uvanim struct is shorter than 4 bytes");
    }
    UvAnimDictionary dictionary;
    dictionary.count = le32_at(structured.body, 0);
    while (!stream.done()) {
        const ChunkView anim = stream.read();
        if (anim.id != kRwAnim) {
            throw ParseError("uvanim dictionary contains a non-animation chunk");
        }
        dictionary.animations.push_back(raw_from(anim));
    }
    if (dictionary.animations.size() != dictionary.count) {
        throw ParseError("uvanim count does not match the animation chunks");
    }
    return dictionary;
}

DeltaMorph parse_delta_morph(std::span<const std::uint8_t> bytes) {
    ChunkStream outer(bytes);
    const ChunkView root = outer.read();
    expect(root, kRwDeltaMorph, "delta morph");
    if (!outer.done()) {
        throw ParseError("bytes follow the delta morph chunk");
    }
    DeltaMorph morph;
    morph.library_id = root.library_id;
    morph.payload.assign(root.body.begin(), root.body.end());
    return morph;
}

}  // namespace shadow::gc
