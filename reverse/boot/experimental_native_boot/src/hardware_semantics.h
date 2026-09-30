#pragma once

#include "golden_trace.h"
#include "guest_types.h"

// Semantic result of 0x80003400. There is no MSR, HID, GQR, or cache object.
struct HardwareSemantics {
    GuestAddress32 pc = 0;
    GuestAddress32 lr = 0;
    GuestWord32 r1 = 0;
    GuestWord32 r2 = 0;
    GuestWord32 r13 = 0;
    // Every FPR's primary lane is this IEEE-754 binary64 value. mtfsf 0xFF
    // copies its low word but derives the FEX/VX summary bits.
    std::uint64_t fpr_binary64 = 0;
    GuestWord32 fpscr = 0;
    // lfd/fmr replace PS0 only. This stores PS1's binary32 source bits for
    // ordinary finite values; exceptional encodings are declined below.
    GuestWord32 paired_lane1_binary32 = 0;
    bool paired_temporary_survives = true;
};

// Requires the external 8-byte image at 0x805F1F30. Does not copy it into a
// register file and does not read the unmapped guest address.
HardwareSemantics InitializeNativeHardwareSemantics(const PreEntryOracle& oracle);

// Gekko mtfsf FM=0xFF semantics for the defined FPSCR status/enable bits.
// This arithmetic helper alone does not establish original startup state.
GuestWord32 DeriveGekkoFPSCR(std::uint64_t source_fpr_bits);

// Set only after the FPR image has been consumed. CRT zeroing checks it.
bool FprSemanticsConsumed();
