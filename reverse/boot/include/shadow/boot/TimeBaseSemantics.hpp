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
    // Explicit research phase. Legacy experiments observe it; produced-work
    // experiments derive it from original apploader and native prefix inputs.
    // Neither supplies a native event owner or physical elapsed-time proof.
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
    // Effects have executed but this source block has not retired its work.
    // The terminal debugger discards it; an uninterrupted successor must carry it.
    std::uint32_t unretired_work{};
};
// Research-only projection of the proven GDB SingleStep source contract:
// one source-cycle per debugger step. No retail/free-run latency claim, no
// live clock, interpreter, callback execution or host-timing substitution.
// Ordinary boot STILL stops before80379628. A matching research result at
//80373AC4 is explicitly not a validated connected frontier.
ClockResearchRun ProjectClockSingleStep(const BootImage&, const ClockResearchInputs&);
// Separate source-conditioned continuous-block experiment. Inputs remain
// labelled research until a native epoch/work/event owner passes chain parity.
// Getter observations do not retire work; the stable reads share one cycle.
ClockResearchRun ProjectClockContinuousResearch(const BootImage&, const ClockResearchInputs&);
}
