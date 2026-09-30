#include "shadow/pc/Decode.hpp"

namespace shadow::pc {
namespace {

NativeMaterial to_material(const gc::Material& source) {
    NativeMaterial material;
    for (int i = 0; i < 4; ++i) {
        material.color[static_cast<std::size_t>(i)] = source.color[i];
    }
    material.ambient = source.ambient;
    material.specular = source.specular;
    material.diffuse = source.diffuse;
    material.filter_addressing = source.filter_addressing;
    material.has_texture = source.has_texture;
    material.texture_name = source.texture_name;
    material.mask_name = source.mask_name;
    material.has_matfx = source.has_matfx;
    material.matfx_type = source.matfx_type;
    material.matfx = source.matfx;
    return material;
}

}  // namespace

NativeModel to_native_model(const gc::Clump& clump, std::string name) {
    NativeModel model;
    model.name = std::move(name);
    model.bones.resize(clump.frames.size());
    for (std::size_t i = 0; i < clump.frames.size(); ++i) {
        const gc::Frame& frame = clump.frames[i];
        NativeBone& bone = model.bones[i];
        bone.parent = frame.parent;
        for (int value = 0; value < 12; ++value) {
            bone.basis[value] = frame.basis[value];
        }
        bone.matrix_flags = frame.matrix_flags;
        if (!frame.hanim.empty()) {
            bone.hanim_id = frame.hanim.front().node_id;
            bone.hanim_flags = static_cast<std::uint32_t>(frame.hanim.front().flags);
            if (frame.hanim.front().hierarchy) {
                NativeHierarchy hierarchy;
                hierarchy.root_frame = static_cast<std::int32_t>(i);
                hierarchy.version = frame.hanim.front().version;
                hierarchy.node_id = frame.hanim.front().node_id;
                hierarchy.flags = frame.hanim.front().flags;
                hierarchy.extra = frame.hanim.front().extra;
                hierarchy.nodes.reserve(frame.hanim.front().nodes.size());
                for (const gc::HAnimNode& node : frame.hanim.front().nodes) {
                    hierarchy.nodes.push_back(NativeHierarchyNode{node.id, node.index, node.flags});
                }
                model.hierarchies.push_back(std::move(hierarchy));
            }
        }
    }

    model.meshes.reserve(clump.geometries.size());
    for (const gc::Geometry& geometry : clump.geometries) {
        NativeMesh mesh;
        mesh.source_flags = geometry.flags;
        if (!geometry.morphs.empty() && geometry.morphs.front().has_positions) {
            mesh.positions = geometry.morphs.front().positions;
        }
        if (!geometry.morphs.empty() && geometry.morphs.front().has_normals) {
            mesh.normals = geometry.morphs.front().normals;
        }
        if (!geometry.uv_sets.empty()) {
            mesh.uv = geometry.uv_sets.front();
        }
        mesh.prelit_rgba = geometry.prelit_rgba;
        mesh.triangles.reserve(geometry.triangles.size());
        mesh.triangle_extra.reserve(geometry.triangles.size());
        for (const gc::Triangle& triangle : geometry.triangles) {
            mesh.triangles.push_back({triangle.v0, triangle.v1, triangle.v2});
            mesh.triangle_extra.push_back(triangle.extra);
        }
        mesh.bin_mesh_flags = geometry.bin_mesh_flags;
        for (const gc::BinMesh& bin : geometry.bin_meshes) {
            mesh.bin_meshes.push_back(NativeBinMesh{bin.material, bin.indices});
        }
        mesh.materials.reserve(geometry.materials.size());
        for (const gc::Material& material : geometry.materials) {
            mesh.materials.push_back(to_material(material));
        }
        if (geometry.skin && geometry.skin->recognized) {
            NativeSkin skin;
            skin.bone_count = geometry.skin->bone_count;
            skin.used_count = geometry.skin->used_count;
            skin.max_weights = geometry.skin->max_weights;
            skin.header_pad = geometry.skin->header_pad;
            skin.used_bones = geometry.skin->used_bones;
            skin.indices = geometry.skin->indices;
            skin.weights = geometry.skin->weights;
            skin.bone_matrices.resize(geometry.skin->bone_count);
            for (std::uint32_t bone = 0; bone < geometry.skin->bone_count; ++bone) {
                for (int value = 0; value < 16; ++value) {
                    skin.bone_matrices[bone][static_cast<std::size_t>(value)] =
                        geometry.skin->bone_matrices[static_cast<std::size_t>(bone) * 16u + static_cast<std::size_t>(value)];
                }
            }
            skin.trailer[0] = geometry.skin->trailer[0];
            skin.trailer[1] = geometry.skin->trailer[1];
            skin.trailer[2] = geometry.skin->trailer[2];
            skin.split_data = geometry.skin->split_data;
            mesh.skin = std::move(skin);
        }
        model.meshes.push_back(std::move(mesh));
    }

    for (const gc::Atomic& atomic : clump.atomics) {
        model.atomics.push_back(NativeAtomic{atomic.frame, atomic.geometry, atomic.flags, atomic.unused});
    }
    return model;
}

NativeTextureDictionary to_native_textures(const gc::TexDictionary& dictionary, std::string name) {
    NativeTextureDictionary out;
    out.name = std::move(name);
    out.device_field = dictionary.device_field;
    out.textures.reserve(dictionary.textures.size());
    for (const gc::Texture& source : dictionary.textures) {
        NativeTexture texture;
        texture.name = source.name;
        texture.mask = source.mask;
        texture.platform_id = source.platform_id;
        texture.filter_addressing = source.filter_addressing;
        texture.raster_format = source.raster_format;
        texture.width = source.width;
        texture.height = source.height;
        texture.depth = source.depth;
        texture.levels = source.levels;
        texture.kind = source.kind;
        texture.format_param = source.format_param;
        texture.raster_tail = source.raster_tail;
        texture.image = source.image;
        out.textures.push_back(std::move(texture));
    }
    return out;
}

NativeWorld to_native_world(const gc::World& world, std::string name) {
    NativeWorld out;
    out.name = std::move(name);
    for (int i = 0; i < 3; ++i) {
        out.bound_a[i] = world.bound_a[i];
        out.bound_b[i] = world.bound_b[i];
    }
    for (int i = 0; i < 10; ++i) {
        out.header_words[i] = world.header_words[i];
    }
    for (const gc::Material& material : world.materials) {
        out.materials.push_back(to_material(material));
    }
    for (const gc::RawChunk& section : world.sections) {
        out.sections.emplace_back(section.id, section.bytes);
    }
    return out;
}

namespace {

NativeTrack to_track(const gc::MotionChunk& chunk) {
    NativeTrack track;
    track.size = chunk.size;
    track.group_count = chunk.group_count;
    track.prefix_length = chunk.prefix_length;
    track.link = chunk.link;
    track.groups_split = chunk.groups_split;
    track.prefix = chunk.prefix;
    track.payload = chunk.payload;
    track.keys.reserve(chunk.keys.size());
    for (const gc::MotionKey& key : chunk.keys) {
        track.keys.push_back(NativeKeyframe{key.header, key.component_b, key.component_c, key.time, key.value_b, key.value_c});
    }
    return track;
}

NativeMotion to_motion(const gc::Motion& motion, std::string name) {
    NativeMotion out;
    out.name = std::move(name);
    out.version = motion.version;
    out.endian_flag = motion.endian_flag;
    out.word_4 = motion.word_4;
    out.byte_size = motion.byte_size;
    out.flag_13 = motion.flag_13;
    out.word_16 = motion.word_16;
    out.word_18 = motion.word_18;
    out.inner_name = motion.name;
    out.tracks.reserve(motion.chunks.size());
    for (const gc::MotionChunk& chunk : motion.chunks) {
        out.tracks.push_back(to_track(chunk));
    }
    return out;
}

}  // namespace

NativeBon to_native_bon(const gc::BonFile& bon, std::string name) {
    NativeBon out;
    out.name = std::move(name);
    out.version = bon.version;
    out.endian_flag = bon.endian_flag;
    out.header_2 = bon.header_2;
    out.word_4 = bon.word_4;
    out.header_8 = bon.header_8;
    out.node_count = bon.node_count;
    out.header_12 = bon.header_12;
    out.skeleton_name = bon.name;
    out.nodes.reserve(bon.nodes.size());
    for (const gc::BonNode& node : bon.nodes) {
        NativeBonNode copy;
        copy.bone_id = node.bone_id;
        for (int i = 0; i < 6; ++i) {
            copy.halves[i] = node.halves[i];
        }
        for (int i = 0; i < 4; ++i) {
            copy.translation[i] = node.translation[i];
            copy.words_32[i] = node.words_32[i];
            copy.words_48[i] = node.words_48[i];
        }
        for (int i = 0; i < 8; ++i) {
            copy.raw_64[i] = node.raw_64[i];
        }
        copy.child_a = node.child_a;
        copy.child_b = node.child_b;
        copy.name = node.name;
        out.nodes.push_back(std::move(copy));
    }
    return out;
}

NativeMotion to_native_motion(const gc::Motion& motion, std::string name) {
    return to_motion(motion, std::move(name));
}

NativeMotionPack to_native_motion_pack(const gc::MotionPack& pack, std::string name) {
    NativeMotionPack out;
    out.name = std::move(name);
    out.word_0 = pack.word_0;
    out.motion_count = pack.motion_count;
    out.word_4 = pack.word_4;
    out.word_8 = pack.word_8;
    out.word_12 = pack.word_12;
    out.endian_flag = pack.endian_flag;
    out.motions.reserve(pack.entries.size());
    for (const gc::MotionPackEntry& entry : pack.entries) {
        NativeMotion motion = to_motion(entry.motion, entry.name);
        motion.extra = entry.extra_bytes;
        out.motions.push_back(std::move(motion));
    }
    return out;
}

}  // namespace shadow::pc
