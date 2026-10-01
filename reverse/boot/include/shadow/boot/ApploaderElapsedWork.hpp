#pragma once

#include "shadow/boot/BootFoundation.hpp"

#include <cstdint>
#include <vector>

namespace shadow::boot {

// Source-conditioned research work only. These typed premises describe the
// pinned fresh GC HLE SDK path in audit42. They are not captured counters,
// post-states, a native timing source or evidence that callbacks/events ran.
struct ApploaderSdkInitialization {
    enum class MemoryOwner { Unknown, FreshClearNoRestoreOrForeignWrites };
    enum class StepPolicy { Unknown, GcBootSingleStepSource42 };
    MemoryOwner memory_owner = MemoryOwner::Unknown;
    StepPolicy step_policy = StepPolicy::Unknown;
    std::uint32_t physical_ram_bytes = 0;
    std::uint32_t console_type = 0;
    std::uint32_t si_poll = 0;
    std::uint32_t syscall_handler_word = 0;
    std::uint32_t report_stub_word = 0;
    bool original_disc_without_patches = false;
    bool no_additional_guest_paths_or_exceptions = false;
};

struct ApploaderElapsedWork {
    std::uint64_t entry_steps = 0;
    std::uint64_t init_steps = 0;
    std::vector<std::uint64_t> main_steps;
    std::uint64_t close_steps = 0;
    std::uint64_t total_steps = 0;
    std::uint32_t callback_count = 0;
    std::uint32_t return_thunk_bytes = 0;
    std::uint64_t return_thunk_steps = 0;
    std::uint64_t footprint_steps = 0;
    std::uint64_t each_bounds_check_steps = 0;
    std::uint64_t bss_fill_steps = 0;
    std::uint64_t bss_flush_steps = 0;
    std::uint64_t state6_steps = 0;
};

// Explicitly select the pinned original GC source profile for a conditional
// research run. This does not inspect or certify a live memory/event owner;
// source/profile correspondence remains a caller proof obligation.
ApploaderSdkInitialization FreshGcHleApploaderSourceInitialization();

// Exact original bytes are gates. boot_header is the complete1088-byte
// boot.bin; bi2 is8192 bytes; dol_header is exactly the first256 DOL bytes.
// image must be built from that PAL DOL through LoadValidatedFixture; its
// whole-file SHA identity remains the caller's separate fixture obligation.
// This function reduces the19 admitted callback paths to finite arithmetic.
// It executes no PPC, guest stores, devices, events, host clock or callbacks.
// Physical/native event ownership remains unresolved; this is not a boot
// frontier promotion and must not be installed as a production clock.
ApploaderElapsedWork DeriveApploaderElapsedWork(
    const BootImage& image,
    const std::vector<std::uint8_t>& apploader,
    const std::vector<std::uint8_t>& boot_header,
    const std::vector<std::uint8_t>& bi2,
    const std::vector<std::uint8_t>& dol_header,
    const ApploaderSdkInitialization& initialization);

}  // namespace shadow::boot
