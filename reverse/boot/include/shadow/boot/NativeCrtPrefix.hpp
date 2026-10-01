#pragma once
#include "shadow/boot/NativeL2Prefix.hpp"
#include <optional>

namespace shadow::boot {
class NativeHandlerSlot {
public:
    explicit NativeHandlerSlot(std::optional<std::uint32_t> word) : word_(word) {}
    std::uint32_t Load(std::uint32_t address) const;
    void Store(std::uint32_t address,std::uint32_t word);
    const auto& Value() const { return word_; }
private:
    std::optional<std::uint32_t> word_;
};
struct NativeCrtInputs {
    NativeL2Inputs l2;
    // Opaque pre-entry word, not a callable pointer. Unknown stops at its load.
    std::optional<std::uint32_t> old_handler;
};
struct NativeCrtCheckpoint {
    NativeL2Checkpoint boot;
    std::optional<std::uint32_t> handler;
    std::optional<std::uint32_t> lowmem44;
};
struct BootWordEffect {
    std::uint32_t pc, address, value, readback;
};
struct NativeZeroRange {
    std::uint32_t address;
    std::vector<std::uint8_t> bytes;
};
struct NativeCrtRun {
    NativeL2Run prefix;
    std::vector<NativeCrtCheckpoint> checkpoints;
    std::vector<BootWordEffect> stores;
    std::vector<NativeZeroRange> zero_ranges;
};
// Exact integer projection of the two EE leaves, not a pending interrupt model.
std::uint32_t WithoutExternalInterrupts(std::uint32_t msr);
std::uint32_t RestoreExternalInterrupts(std::uint32_t live_msr, bool prior_enabled);
// STRONG closed-owner continuation from entry: selector1 registration, ordinary
// frame/logger effects, exact PAL identity-copy descriptors, three zero fills.
// Compiled text/descriptors have no external writers, asynchronous delivery or
// device observer. Unknown old-slot state stops at 803733B4. Otherwise stops
// before the first unprovided BI2-pointer read at 80003188. No retail claim.
NativeCrtRun RunImmutableNativeCrtPrefix(const BootImage&, const NativeCrtInputs&);
}
