#pragma once
#include "shadow/boot/NativeCrtPrefix.hpp"
#include <array>

namespace shadow::boot {
struct NativeBi2Inputs {
    NativeCrtInputs crt;
    // Explicit measured pre-entry mapping, never a presumed loader address.
    std::optional<std::uint32_t> pointer;
    std::vector<std::uint8_t> loaded_bi2;
};
struct NativeBi2Checkpoint {
    NativeCrtCheckpoint boot;
    std::vector<std::uint8_t> bi2;
    std::array<std::uint32_t,4> sda;
    std::optional<std::uint32_t> arena_high, lowmem48;
};
struct BootByteEffect { std::uint32_t pc, address; std::uint8_t value, readback; };
struct NativeBi2Run {
    NativeCrtRun prefix;
    std::vector<NativeBi2Checkpoint> checkpoints;
    std::vector<BootWordEffect> stores;
    std::vector<BootByteEffect> byte_stores;
    // Current storage; prefix.zero_ranges records the earlier completed fills.
    std::vector<NativeZeroRange> globals;
    std::vector<std::uint8_t> bi2;
};
// STRONG bounded closed-owner continuation: two live BI2 pointer reads, debug4
// byte leaf, owned relocation array, metadata, first OS guard/frame, clock frame
// and EE leaf. Stops before first time-base read 80379628. Unknown pointer/blob,
// zero-pointer fallback and debug2/3 context transfer stop at their consumers.
NativeBi2Run RunImmutableNativeBi2Prefix(const BootImage&, const NativeBi2Inputs&);
}
