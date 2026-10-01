#pragma once

#include "shadow/boot/NativeBootPrefix.hpp"

namespace shadow::boot {

// Entry experiment/observation, not an IPL default or a completion input.
struct NativeL2Inputs {
    NativeBootInputs entry;
    std::uint32_t l2cr;
};

// Bounded native stack storage for this semantic section. Unknown bytes may
// be displayed with a validity mask but may never satisfy a load.
struct L2StackBytes {
    static constexpr std::uint32_t base = 0x8060C570u;
    std::array<std::uint8_t, 0x90> bytes{};
    std::array<bool, 0x90> valid{};
    void StoreBE32(std::uint32_t address, std::uint32_t value);
    std::uint32_t LoadBE32(std::uint32_t address) const;
};

enum class L2EffectKind { Read, Write, InvalidateCompleted, StoreCompletion };
struct L2Effect {
    L2EffectKind kind;
    std::uint32_t pc;
    std::uint32_t value;
};
struct NativeL2Checkpoint {
    NativeBootCheckpoint native;
    std::uint32_t l2cr;
    L2StackBytes stack;
};
struct NativeL2Run {
    NativeBootRun prefix;
    std::vector<NativeL2Checkpoint> checkpoints;
    std::vector<MemoryWrite32> ordered_stack_writes;
    std::vector<L2Effect> effects;
};

// PROVEN bit/edge projection for three exact PAL branch sites. This does not
// declare a busy read complete. Unknown branch addresses decline.
std::uint32_t L2BranchSuccessor(std::uint32_t branch_pc, std::uint32_t live_l2cr);

// STRONG bounded semantic collapse; see NATIVE_L2_COMPLETION_38.md. Calls the
// unchanged checkpoint-37 runner from entry. Private RAM is authoritative,
// immutable native code has no L2-resident payload, and there is no external
// observer/queue or inherited dirty cache. Only E/CE/DO/WT input bits are
// admitted; pending I/IP, test support and reserved states decline. This is
// not a physical cache/timing/retail implementation. Stops before 80372904
// (handler installation), without supplying an old handler-slot value.
NativeL2Run RunImmutableNativeL2Prefix(const BootImage&, const NativeL2Inputs&);

}  // namespace shadow::boot
