#pragma once

#include <cstdint>

namespace shadow::boot {

// PROVEN static control flow: PAL GUPP8P 0x800511E0..0x80051218.
// The caller must have completed 0x800510C0's preceding initialization.
// These hooks represent unresolved game/OS state, not replacement behavior.
class ApplicationLoopHooks {
public:
    virtual ~ApplicationLoopHooks() = default;
    virtual std::uint32_t ExitFlag() = 0; // u32 at 0x80576DBC
    virtual std::uint32_t DispatchEvent(std::uint32_t event, std::uint32_t argument) = 0;
    virtual void PostEventService() = 0; // call at 0x800511F0 to 0x8032D444
};

// Runs only the recurring and normal-exit portion of 0x800510C0.
// DispatchEvent represents 0x80051978, including its still-unlifted
// prefilter, callback dispatch, and game frame step.
void RunApplicationRecurringPhase(ApplicationLoopHooks& hooks);

} // namespace shadow::boot
