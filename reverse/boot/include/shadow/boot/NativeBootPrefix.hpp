#pragma once

#include "shadow/boot/FprSeedProjection.hpp"

#include <vector>

namespace shadow::boot {

// Explicit pre-entry observations/experiments, never retail defaults. GPR,
// PC, LR and subsequent stack/control values are produced from the DOL path.
struct NativeBootInputs {
    std::uint32_t msr;
    std::uint32_t hid0;
    std::uint32_t hid2;
    std::uint32_t cr;
    std::uint32_t xer;
    std::uint32_t ctr;
    std::uint32_t fpscr;
    std::array<FprLanes, 32> fpr;
    std::array<std::uint32_t, 8> gqr;
    std::uint32_t source_address;
    std::array<std::uint8_t, 16> source_bytes;
};

struct NativeBootCheckpoint {
    FprSeedState state;
    std::uint32_t ctr;
    std::uint32_t hid0;
    std::uint32_t hid2;
    std::array<std::uint32_t, 8> gqr;
    PairedSetupStackMemory paired_stack;
};

struct NativeBootRun {
    std::vector<NativeBootCheckpoint> checkpoints;
    PairedSetupStackMemory paired_stack;
    // Actual ordered bytes written by the cache-handler frame. Separate from
    // the earlier frame: untouched bytes are not claimed initialized.
    std::array<MemoryWrite32, 4> cache_stack_writes;
    std::array<PairedSetupStackMemory, 2> cache_stack;
};

// STRONG bounded semantic replacement, evidence: NATIVE_SYNC_COMPLETION_37.md.
// Owns all effects from 0x80003154; there is no caller-provided completion
// acknowledgment, injected mid-chain state or hardware callback. Ordinary
// private bytes and immutable compiled bodies replace local cache machinery.
// The checked path stops BEFORE the live L2CR call at 0x80372894. Declines
// exceptional MSR modes, cache-command inputs, ABE, locked-cache/DMA state,
// unvalidated HID bits, and unsupported FPR inputs. No timing/retail claim.
NativeBootRun RunImmutableNativeBootPrefix(const BootImage&, const NativeBootInputs&);

}  // namespace shadow::boot
