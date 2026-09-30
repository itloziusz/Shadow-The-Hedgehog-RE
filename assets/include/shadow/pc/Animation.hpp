#pragma once

#include "shadow/pc/NativeAssets.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace shadow::pc {

// Compare epsilon loaded from r2+5856 by 0x80415170 / 0x80414B80.
inline constexpr float kMotionCompareEpsilon = 9.999999747378752e-05f;

// Global bind matrices, one per DFF frame, row-vector product local * parent.
// The 12 floats are right, up, at, position.
std::vector<std::array<float, 12>> bind_globals(const NativeModel& model);

// Same product as bind_globals. `local` is one basis per frame, in frame order.
std::vector<std::array<float, 12>> compose_hierarchy(std::span<const std::array<float, 12>> local,
                                                     std::span<const std::int32_t> parents);

// 0x80415768: value_c when value_b is 0, otherwise value_b * (clock - time) + value_c.
// The key is the last one for which clock > time - epsilon. If none, the first key.
float sample_channel(std::span<const NativeKeyframe> keys, float clock);

// One MTP chunk, in the order 0x8041C30C consumes them while walking BON children.
// halves[1] (node+6) selects the groups. Child offsets are file offsets, child A then child B.
enum class ChannelKind : std::uint8_t {
    PreScaleX,
    PreScaleY,
    PreScaleZ,
    TranslateX,
    TranslateY,
    TranslateZ,
    RotateX,
    RotateY,
    RotateZ,
    PostScaleX,
    PostScaleY,
    PostScaleZ,
};

struct ChannelBinding {
    std::size_t chunk = 0;
    std::uint32_t bone_id = 0;
    std::size_t node_index = 0;
    ChannelKind kind = ChannelKind::TranslateX;
};

struct ChannelMap {
    std::vector<ChannelBinding> bindings;
    std::vector<std::string> problems;
};

// The map is a property of the BON. Every motion whose inner name selects this
// BON consumes its chunks in this order.
ChannelMap map_skeleton_channels(const NativeBon& bon);

struct AnimatedLocals {
    // One basis per BON node, same 12-float layout as a DFF frame.
    std::vector<std::array<float, 12>> local;
    std::size_t unsplit_channels = 0;
};

// Samples `motion` through `map` and builds each node's local matrix.
// Unsplit chunks leave that component at the BON default.
// The clock is not wrapped: no loop instruction was found in the sampler.
AnimatedLocals animate_locals(const NativeBon& bon, const NativeMotion& motion, const ChannelMap& map, float clock);

// Frames whose HAnim id matches a BON node use that node's animated local.
// Every other frame keeps its bind basis. Parents stay the DFF parent indices.
std::vector<std::array<float, 12>> pose_globals(const NativeModel& model,
                                                const NativeBon& bon,
                                                const AnimatedLocals& locals);

std::vector<std::string> hierarchy_problems(const NativeModel& model);

}  // namespace shadow::pc
