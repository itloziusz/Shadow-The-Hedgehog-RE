#pragma once

#include "constructor_0.h"
#include <array>
#include <string>
#include <optional>

struct Constructor1Event {
    std::string_view kind;
    GuestAddress32 pc;
    GuestAddress32 address;
    unsigned size;
    // Hex bits or an explicit preserved caller token, never a fabricated value.
    std::string value;
};
struct Constructor1Inputs {
    GuestWord32 stack, cursor, return_pc, sda2, sda;
    std::uint64_t f31_primary;
    GuestWord32 f31_secondary_source, fpscr;
    GuestAddress32 current_context, physical_context, fpu_owner, fpu_handler;
    GuestWord32 context_flags;
    bool fpu_enabled;
    bool recoverable_exception;
    // The handler saves/restores these without interpreting their other bits.
    // Their values remain symbolic; no oracle register image is installed.
};
struct Constructor1Result {
    struct ExceptionSave {
        GuestWord32 r3, r4, r5, cr, lr, fault_pc;
        std::string_view ctr = "ENTRY_CTR", xer = "ENTRY_XER", msr = "ENTRY_MSR";
    };
    std::optional<FragmentState> fragment;
    std::optional<ExceptionSave> exception_save;
    std::array<GuestWord32, 9> vectors;
    std::array<GuestWord32, 2> angles;
    GuestAddress32 vector_address, angle_address, pc, cursor, next_constructor;
    GuestWord32 r1, lr, fpscr;
    std::uint64_t f31_primary, paired_spill;
    std::array<std::uint64_t, 3> fpr_primary; // f0/f1/f2; each secondary lane is identical
    GuestWord32 f31_secondary_source;
    GuestAddress32 context, fpu_owner;
    GuestWord32 context_flags;
    bool fpu_enabled;
    unsigned constructors_executed;
    std::vector<Constructor1Event> events;
};

// Bounded native semantics, not a CPU interpreter. Unsupported alternate OS
// contexts fail closed rather than pretending the lazy-FPU branches succeeded.
Constructor1Result RunConstructor1Body(const BootImage& image, const Constructor1Inputs& input);
Constructor1Result ExecuteSecondConstructor(const BootImage& image, const HardwareSemantics& hardware,
    const PreEntryOracle& pre, const Constructor0Result& previous, const Constructor1Oracle& oracle);
std::string Constructor1Hex(std::uint64_t value, unsigned size);
GuestWord32 Constructor1Radians(GuestWord32 degrees, GuestWord32 pi, GuestWord32 half_turn,
                               GuestWord32& fpscr);
