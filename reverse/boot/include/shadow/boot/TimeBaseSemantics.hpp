#pragma once
#include "shadow/boot/NativeBi2Prefix.hpp"

namespace shadow::boot {
// PROVEN private-reference arithmetic; these inputs describe the producer,
// not architectural TB outputs. This is not a host or native runtime clock.
struct CycleTimeBaseEpoch {
    std::uint32_t rtc_seconds;
    std::uint32_t cpu_hz;
    std::uint64_t source_cycles;
};
std::uint64_t CounterTimeBaseAt(std::uint64_t epoch,std::uint64_t epoch_cycles,std::uint64_t cycles);
std::uint64_t TimeBaseAt(const CycleTimeBaseEpoch&, std::uint64_t cycles);
std::uint32_t CompareSignedHigh(std::uint32_t first, std::uint32_t second,
                                std::uint32_t cr, std::uint32_t xer);
struct ClockWordSum { std::uint32_t high, low, xer_after_low, xer_after_high; };
ClockWordSum AddClockOffset(std::uint64_t time_base, std::uint64_t offset, std::uint32_t xer);

struct ClockResearchInputs {
    NativeBi2Inputs entry;
    CycleTimeBaseEpoch epoch;
    // Optional original-entry producer rebase; explicit research input only.
    // Captures must prove low/high/commit controls before any guest instruction.
    std::optional<std::uint64_t> rebased_epoch;
    // Observed upstream cycle count at checkpoint40; NOT derived by the
    // native prefix. Consequently this projection cannot advance boot.
    std::uint64_t frontier_cycles;
    std::uint64_t offset;
    std::uint64_t cached_tb;
    std::optional<std::uint32_t> exceptions; // unknown/pending declines
};
struct ClockResearchCheckpoint {
    NativeCrtCheckpoint boot;
    std::uint64_t cycles, cached_tb;
    std::array<std::uint32_t,2> clock_global; // high, low; same CRT owner
};
struct ClockResearchRun {
    NativeBi2Run prefix;
    std::vector<ClockResearchCheckpoint> checkpoints;
    std::vector<BootWordEffect> stores;
    std::vector<NativeZeroRange> globals;
};
// Research-only projection of the proven GDB SingleStep source contract:
// one source-cycle per debugger step. No retail/free-run latency claim, no
// live clock, interpreter, callback execution or host-timing substitution.
// Ordinary boot STILL stops before80379628. A matching research result at
//80373AC4 is explicitly not a validated connected frontier.
ClockResearchRun ProjectClockSingleStep(const BootImage&, const ClockResearchInputs&);
}
