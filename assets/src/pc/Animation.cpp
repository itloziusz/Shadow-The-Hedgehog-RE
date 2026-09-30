#include "shadow/pc/Animation.hpp"

#include <cmath>
#include <optional>
#include <unordered_map>

namespace shadow::pc {
namespace {

constexpr float kTurnSteps = 65536.f;
constexpr float kFullTurn = 6.2831854820251465f;

void multiply_local(const float* local, const float* parent, float* out) {
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 3; ++column) {
            float value = local[row * 3 + 0] * parent[column] + local[row * 3 + 1] * parent[3 + column] +
                          local[row * 3 + 2] * parent[6 + column];
            if (row == 3) {
                value += parent[9 + column];
            }
            out[row * 3 + column] = value;
        }
    }
}

std::array<float, 12> identity_basis() {
    return {1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f};
}

void column_scale(std::array<float, 12>& basis, float sx, float sy, float sz) {
    for (int row = 0; row < 3; ++row) {
        basis[static_cast<std::size_t>(row * 3 + 0)] *= sx;
        basis[static_cast<std::size_t>(row * 3 + 1)] *= sy;
        basis[static_cast<std::size_t>(row * 3 + 2)] *= sz;
    }
}

void translate_local(std::array<float, 12>& basis, float x, float y, float z) {
    const float px = basis[0] * x + basis[1] * y + basis[2] * z + basis[9];
    const float py = basis[3] * x + basis[4] * y + basis[5] * z + basis[10];
    const float pz = basis[6] * x + basis[7] * y + basis[8] * z + basis[11];
    basis[9] = px;
    basis[10] = py;
    basis[11] = pz;
}

void multiply_axes(std::array<float, 12>& basis, const std::array<float, 12>& rotation) {
    float axes[9];
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            axes[row * 3 + column] = basis[static_cast<std::size_t>(row * 3 + 0)] * rotation[static_cast<std::size_t>(column)] +
                                     basis[static_cast<std::size_t>(row * 3 + 1)] * rotation[static_cast<std::size_t>(3 + column)] +
                                     basis[static_cast<std::size_t>(row * 3 + 2)] * rotation[static_cast<std::size_t>(6 + column)];
        }
    }
    for (int i = 0; i < 9; ++i) {
        basis[static_cast<std::size_t>(i)] = axes[i];
    }
}

std::array<float, 12> rotation_z(float radians) {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return {c, -s, 0.f, s, c, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f};
}

std::array<float, 12> rotation_y(float radians) {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return {c, 0.f, s, 0.f, 1.f, 0.f, -s, 0.f, c, 0.f, 0.f, 0.f};
}

std::array<float, 12> rotation_x(float radians) {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return {1.f, 0.f, 0.f, 0.f, c, -s, 0.f, s, c, 0.f, 0.f, 0.f};
}

// 0x80420B9C: trunc(component * 65536 / 2π), then the low 16 bits as a signed
// step around the circle. A truncated value of 0 skips that axis.
bool quantized_radians(float component, float& radians) {
    if (!std::isfinite(component)) {
        return false;
    }
    const float scaled = (component * kTurnSteps) / kFullTurn;
    if (scaled >= 2147483647.f || scaled <= -2147483648.f) {
        return false;
    }
    const int truncated = static_cast<int>(scaled);
    if (truncated == 0) {
        return false;
    }
    const auto low = static_cast<std::int16_t>(static_cast<std::uint16_t>(truncated));
    radians = static_cast<float>(low) * kFullTurn / kTurnSteps;
    return true;
}

void apply_rotation(std::array<float, 12>& basis, float x, float y, float z) {
    float radians = 0.f;
    if (quantized_radians(z, radians)) {
        multiply_axes(basis, rotation_z(radians));
    }
    if (quantized_radians(y, radians)) {
        multiply_axes(basis, rotation_y(radians));
    }
    if (quantized_radians(x, radians)) {
        multiply_axes(basis, rotation_x(radians));
    }
}

std::optional<std::size_t> node_at_file_offset(const NativeBon& bon, std::uint32_t offset) {
    if (offset < 48) {
        return std::nullopt;
    }
    const std::uint32_t relative = offset - 48;
    if (relative % 112u != 0) {
        return std::nullopt;
    }
    const std::uint32_t index = relative / 112u;
    if (index >= bon.nodes.size()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(index);
}

void walk_channels(const NativeBon& bon,
                   std::size_t index,
                   std::vector<std::uint8_t>& seen,
                   ChannelMap& out) {
    if (index >= bon.nodes.size()) {
        out.problems.emplace_back("BON channel walk left the node table");
        return;
    }
    if (seen[index] != 0) {
        out.problems.emplace_back("BON channel walk repeated a node");
        return;
    }
    seen[index] = 1;
    const NativeBonNode& node = bon.nodes[index];
    const std::uint16_t flags = node.halves[1];
    auto emit = [&](ChannelKind kind) {
        ChannelBinding binding;
        binding.chunk = out.bindings.size();
        binding.bone_id = node.bone_id;
        binding.node_index = index;
        binding.kind = kind;
        out.bindings.push_back(binding);
    };
    if ((flags & 0x0400u) != 0) {
        emit(ChannelKind::PreScaleX);
        emit(ChannelKind::PreScaleY);
        emit(ChannelKind::PreScaleZ);
    }
    if ((flags & 0x0007u) != 0) {
        if ((flags & 0x0001u) != 0) {
            emit(ChannelKind::TranslateX);
        }
        if ((flags & 0x0002u) != 0) {
            emit(ChannelKind::TranslateY);
        }
        if ((flags & 0x0004u) != 0) {
            emit(ChannelKind::TranslateZ);
        }
    }
    if ((flags & 0x0040u) == 0 && (flags & 0x0038u) != 0) {
        if ((flags & 0x0008u) != 0) {
            emit(ChannelKind::RotateX);
        }
        if ((flags & 0x0010u) != 0) {
            emit(ChannelKind::RotateY);
        }
        if ((flags & 0x0020u) != 0) {
            emit(ChannelKind::RotateZ);
        }
    }
    if ((flags & 0x0380u) != 0) {
        if ((flags & 0x0080u) != 0) {
            emit(ChannelKind::PostScaleX);
        }
        if ((flags & 0x0100u) != 0) {
            emit(ChannelKind::PostScaleY);
        }
        if ((flags & 0x0200u) != 0) {
            emit(ChannelKind::PostScaleZ);
        }
    }
    for (const std::uint32_t child : {node.child_a, node.child_b}) {
        if (child == 0) {
            continue;
        }
        const auto next = node_at_file_offset(bon, child);
        if (!next) {
            out.problems.emplace_back("BON child offset is not a node");
            continue;
        }
        walk_channels(bon, *next, seen, out);
    }
}

int component_index(ChannelKind kind) {
    switch (kind) {
        case ChannelKind::PreScaleX:
        case ChannelKind::TranslateX:
        case ChannelKind::RotateX:
        case ChannelKind::PostScaleX:
            return 0;
        case ChannelKind::PreScaleY:
        case ChannelKind::TranslateY:
        case ChannelKind::RotateY:
        case ChannelKind::PostScaleY:
            return 1;
        case ChannelKind::PreScaleZ:
        case ChannelKind::TranslateZ:
        case ChannelKind::RotateZ:
        case ChannelKind::PostScaleZ:
            return 2;
    }
    return 0;
}

bool is_pre_scale(ChannelKind kind) {
    return kind == ChannelKind::PreScaleX || kind == ChannelKind::PreScaleY || kind == ChannelKind::PreScaleZ;
}

bool is_translation(ChannelKind kind) {
    return kind == ChannelKind::TranslateX || kind == ChannelKind::TranslateY || kind == ChannelKind::TranslateZ;
}

bool is_rotation(ChannelKind kind) {
    return kind == ChannelKind::RotateX || kind == ChannelKind::RotateY || kind == ChannelKind::RotateZ;
}

}  // namespace

std::vector<std::array<float, 12>> compose_hierarchy(std::span<const std::array<float, 12>> local,
                                                     std::span<const std::int32_t> parents) {
    const std::size_t count = local.size() < parents.size() ? local.size() : parents.size();
    std::vector<std::array<float, 12>> global(count);
    std::vector<std::uint8_t> done(count, 0);
    for (std::size_t index = 0; index < count; ++index) {
        std::vector<std::size_t> chain;
        std::size_t cursor = index;
        while (done[cursor] == 0) {
            chain.push_back(cursor);
            done[cursor] = 2;
            const auto parent = parents[cursor];
            if (parent < 0) {
                break;
            }
            if (static_cast<std::size_t>(parent) >= count || done[static_cast<std::size_t>(parent)] == 2) {
                break;
            }
            cursor = static_cast<std::size_t>(parent);
        }
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            const std::size_t bone = *it;
            const auto parent = parents[bone];
            if (parent < 0 || static_cast<std::size_t>(parent) >= count || done[static_cast<std::size_t>(parent)] != 1) {
                global[bone] = local[bone];
            } else {
                multiply_local(local[bone].data(), global[static_cast<std::size_t>(parent)].data(), global[bone].data());
            }
            done[bone] = 1;
        }
        for (const std::size_t bone : chain) {
            if (done[bone] == 2) {
                done[bone] = 1;
            }
        }
    }
    return global;
}

std::vector<std::array<float, 12>> bind_globals(const NativeModel& model) {
    std::vector<std::array<float, 12>> local(model.bones.size());
    std::vector<std::int32_t> parents(model.bones.size());
    for (std::size_t index = 0; index < model.bones.size(); ++index) {
        for (int value = 0; value < 12; ++value) {
            local[index][static_cast<std::size_t>(value)] = model.bones[index].basis[value];
        }
        parents[index] = model.bones[index].parent;
    }
    return compose_hierarchy(local, parents);
}

float sample_channel(std::span<const NativeKeyframe> keys, float clock) {
    if (keys.empty() || !std::isfinite(clock)) {
        return 0.f;
    }
    const NativeKeyframe* current = &keys.front();
    for (const NativeKeyframe& key : keys) {
        if (clock > key.time - kMotionCompareEpsilon) {
            current = &key;
        }
    }
    if (current->value_b == 0.f) {
        return current->value_c;
    }
    return current->value_b * (clock - current->time) + current->value_c;
}

ChannelMap map_skeleton_channels(const NativeBon& bon) {
    ChannelMap out;
    if (bon.nodes.empty()) {
        out.problems.emplace_back("BON has no nodes to map");
        return out;
    }
    std::vector<std::uint8_t> seen(bon.nodes.size(), 0);
    walk_channels(bon, 0, seen, out);
    return out;
}

AnimatedLocals animate_locals(const NativeBon& bon, const NativeMotion& motion, const ChannelMap& map, float clock) {
    AnimatedLocals pose;
    pose.local.assign(bon.nodes.size(), identity_basis());
    std::vector<std::array<float, 3>> pre(bon.nodes.size(), {1.f, 1.f, 1.f});
    std::vector<std::array<float, 3>> translation(bon.nodes.size());
    std::vector<std::array<float, 3>> rotation(bon.nodes.size());
    std::vector<std::array<float, 3>> post(bon.nodes.size(), {1.f, 1.f, 1.f});
    std::vector<std::uint8_t> pre_on(bon.nodes.size(), 0);
    std::vector<std::uint8_t> translation_on(bon.nodes.size(), 0);
    std::vector<std::uint8_t> rotation_on(bon.nodes.size(), 0);
    std::vector<std::uint8_t> post_on(bon.nodes.size(), 0);
    for (std::size_t index = 0; index < bon.nodes.size(); ++index) {
        const NativeBonNode& node = bon.nodes[index];
        const std::uint16_t flags = node.halves[1];
        for (int axis = 0; axis < 3; ++axis) {
            translation[index][static_cast<std::size_t>(axis)] = node.translation[axis];
            rotation[index][static_cast<std::size_t>(axis)] = node.words_32[axis];
            post[index][static_cast<std::size_t>(axis)] = node.words_48[axis];
        }
        pre_on[index] = (flags & 0x0400u) != 0 ? 1 : 0;
        translation_on[index] = (flags & 0x0007u) != 0 ? 1 : 0;
        rotation_on[index] = ((flags & 0x0040u) == 0 && (flags & 0x0038u) != 0) ? 1 : 0;
        post_on[index] = (flags & 0x0380u) != 0 ? 1 : 0;
    }
    for (const ChannelBinding& binding : map.bindings) {
        if (binding.node_index >= bon.nodes.size() || binding.chunk >= motion.tracks.size()) {
            continue;
        }
        const NativeTrack& track = motion.tracks[binding.chunk];
        if (!track.groups_split) {
            ++pose.unsplit_channels;
            continue;
        }
        const float sample = sample_channel(track.keys, clock);
        const int axis = component_index(binding.kind);
        if (is_pre_scale(binding.kind)) {
            pre[binding.node_index][static_cast<std::size_t>(axis)] = sample;
        } else if (is_translation(binding.kind)) {
            translation[binding.node_index][static_cast<std::size_t>(axis)] = sample;
        } else if (is_rotation(binding.kind)) {
            rotation[binding.node_index][static_cast<std::size_t>(axis)] = sample;
        } else {
            post[binding.node_index][static_cast<std::size_t>(axis)] = sample;
        }
    }
    for (std::size_t index = 0; index < bon.nodes.size(); ++index) {
        std::array<float, 12> basis = identity_basis();
        if (pre_on[index] != 0) {
            const auto& scale = pre[index];
            if (scale[0] != 1.f || scale[1] != 1.f || scale[2] != 1.f) {
                column_scale(basis, scale[0], scale[1], scale[2]);
            }
        }
        if (translation_on[index] != 0) {
            translate_local(basis, translation[index][0], translation[index][1], translation[index][2]);
        }
        if (rotation_on[index] != 0) {
            apply_rotation(basis, rotation[index][0], rotation[index][1], rotation[index][2]);
        }
        if (post_on[index] != 0) {
            const auto& scale = post[index];
            if (scale[0] != 1.f || scale[1] != 1.f || scale[2] != 1.f) {
                column_scale(basis, scale[0], scale[1], scale[2]);
            }
        }
        pose.local[index] = basis;
    }
    return pose;
}

std::vector<std::array<float, 12>> pose_globals(const NativeModel& model,
                                                const NativeBon& bon,
                                                const AnimatedLocals& locals) {
    std::unordered_map<std::int32_t, std::size_t> node_by_id;
    for (std::size_t index = 0; index < bon.nodes.size() && index < locals.local.size(); ++index) {
        node_by_id.emplace(static_cast<std::int32_t>(bon.nodes[index].bone_id), index);
    }
    const std::int32_t hierarchy_frame = model.hierarchies.empty() ? -1 : model.hierarchies.front().root_frame;
    std::vector<std::array<float, 12>> local(model.bones.size());
    std::vector<std::int32_t> parents(model.bones.size());
    for (std::size_t index = 0; index < model.bones.size(); ++index) {
        parents[index] = model.bones[index].parent;
        const auto found = node_by_id.find(model.bones[index].hanim_id);
        const bool hierarchy_container = hierarchy_frame >= 0 && static_cast<std::size_t>(hierarchy_frame) == index;
        if (!hierarchy_container && model.bones[index].hanim_id >= 0 && found != node_by_id.end()) {
            local[index] = locals.local[found->second];
        } else {
            for (int value = 0; value < 12; ++value) {
                local[index][static_cast<std::size_t>(value)] = model.bones[index].basis[value];
            }
        }
    }
    return compose_hierarchy(local, parents);
}

std::vector<std::string> hierarchy_problems(const NativeModel& model) {
    std::vector<std::string> problems;
    if (model.hierarchies.empty()) {
        problems.emplace_back("model has no HAnim hierarchy");
        return problems;
    }
    const NativeHierarchy& hierarchy = model.hierarchies.front();
    std::unordered_map<std::int32_t, int> id_uses;
    for (std::size_t index = 0; index < model.bones.size(); ++index) {
        if (hierarchy.root_frame >= 0 && static_cast<std::size_t>(hierarchy.root_frame) == index) {
            continue;
        }
        const auto id = model.bones[index].hanim_id;
        if (id < 0) {
            continue;
        }
        id_uses[id] += 1;
    }
    for (const auto& [id, count] : id_uses) {
        if (count != 1) {
            problems.emplace_back("HAnim id is duplicated on frames");
            break;
        }
    }
    for (const NativeHierarchyNode& node : hierarchy.nodes) {
        if (node.id == 0) {
            continue;
        }
        if (id_uses.find(node.id) == id_uses.end()) {
            problems.emplace_back("HAnim hierarchy node has no frame");
            break;
        }
    }
    std::vector<std::uint8_t> seen(hierarchy.nodes.size(), 0);
    for (const NativeHierarchyNode& node : hierarchy.nodes) {
        if (node.index < 0 || static_cast<std::size_t>(node.index) >= hierarchy.nodes.size()) {
            problems.emplace_back("HAnim node index is outside the hierarchy");
            break;
        }
        if (seen[static_cast<std::size_t>(node.index)] != 0) {
            problems.emplace_back("HAnim node index is duplicated");
            break;
        }
        seen[static_cast<std::size_t>(node.index)] = 1;
    }
    for (std::size_t index = 0; index < model.bones.size(); ++index) {
        const auto parent = model.bones[index].parent;
        if (parent < -1 || (parent >= 0 && static_cast<std::size_t>(parent) >= model.bones.size())) {
            problems.emplace_back("frame parent is outside the frame list");
            break;
        }
    }
    return problems;
}

}  // namespace shadow::pc
