#pragma once
#include "boot_image.h"
#include <optional>

struct CrtSemantics;
struct HardwareSemantics;
CrtSemantics InitializeNativeCrt(const BootImage&, const HardwareSemantics&);

// Channel0 SI type-status time, not the separate last-transfer array.
// One semantic value. Guest word addresses are trace metadata only.
// Only its CRT-produced initial generation is closed by this contract.
class SiTypeQueryTimestamp {
public:
    std::uint32_t ConsumeInitialLow() {
        if (!ticks_) throw BootError("SI type timestamp lacks its CRT producer");
        if (phase_ != Phase::BeforeLow) throw BootError("unproven SI timestamp read order/generation");
        phase_ = Phase::BeforeHigh;
        return static_cast<std::uint32_t>(*ticks_);
    }
    std::uint32_t ConsumeInitialHigh() {
        if (!ticks_) throw BootError("SI type timestamp lacks its CRT producer");
        if (phase_ != Phase::BeforeHigh) throw BootError("unproven SI timestamp read order/generation");
        phase_ = Phase::Consumed;
        return static_cast<std::uint32_t>(*ticks_ >> 32);
    }
private:
    friend CrtSemantics InitializeNativeCrt(const BootImage&, const HardwareSemantics&);
    std::optional<std::uint64_t> ticks_; // no implicit zero/unknown memory
    enum class Phase { BeforeLow, BeforeHigh, Consumed };
    Phase phase_ = Phase::BeforeLow; // proof-frontier bookkeeping, not a guest write
};

// Proven subfc/subfe value semantics, modulo 2^64. No clock is sampled here.
// This does not approve a host clock policy or any downstream deadline branch.
constexpr std::uint64_t SiTypeTimestampElapsed(std::uint64_t now, std::uint64_t stored) {
    return now - stored;
}
