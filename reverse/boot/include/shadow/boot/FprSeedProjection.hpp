#pragma once

#include "shadow/boot/BootFoundation.hpp"

#include <array>
#include <cstdint>

namespace shadow::boot {

// Raw two-lane evidence representation, matched to the HLE oracle's binary64
// exports. Physical Gekko internal format is not claimed. No host arithmetic.
struct FprLanes {
    std::uint64_t ps0 = 0;
    std::uint64_t ps1 = 0;
};

// Explicit input, not a boot default. Standalone bounded projection; the
// NativeBootPrefix runner supplies it from the checked immutable entry path.
// Other ICFI/sync profiles remain unresolved. Source bytes must be live reads.
struct FprSeedState {
    HardwareCallPrefix machine;
    std::uint32_t cr = 0;
    std::uint32_t xer = 0;
    std::uint32_t fpscr = 0;
    std::array<FprLanes, 32> fpr{};
};

struct FprSeedInputs {
    FprSeedState entry;
    std::uint32_t live_hid2 = 0;
    std::uint32_t live_gqr0 = 0;
    std::uint32_t source_address = 0;
    std::array<std::uint8_t, 16> source_bytes{};
};

// Pre-instruction checkpoints at CF0, CFC, D00, D7C, D80, D84, DFC,
// then the return target. Every executed instruction is DOL-fingerprinted.
struct FprSeedPrediction {
    std::array<FprSeedState, 8> checkpoints;
};

// Supported: supervisor FP enable, awake big-endian nontracing execution,
// uninterruptible/nontrapping FP mode and valid incoming FPSCR,
// paired-enabled HID2, GQR0=0, normal or signed-zero single inputs and finite
// double input. Other paths decline; no unknown lane is replaced by zero.
FprSeedPrediction PredictFprSeed(const BootImage&, const FprSeedInputs&);

}  // namespace shadow::boot
