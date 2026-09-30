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
    // Every FPR's primary lane is this IEEE-754 binary64 value. FPSCR is its low 32 bits,
    // which is what mtfsf 255 of that register writes.
    std::uint64_t fpr_binary64 = 0;
    GuestWord32 fpscr = 0;
    // lfd/fmr replace PS0 only. Keep the exact binary32 source of PS1;
    // no floating conversion is needed until a consumer requires it.
    GuestWord32 paired_lane1_binary32 = 0;
    bool paired_temporary_survives = true;
};

// Requires the external 8-byte image at 0x805F1F30. Does not copy it into a
// register file and does not read the unmapped guest address.
HardwareSemantics InitializeNativeHardwareSemantics(const PreEntryOracle& oracle);

// Set only after the FPR image has been consumed. CRT zeroing checks it.
bool FprSemanticsConsumed();
