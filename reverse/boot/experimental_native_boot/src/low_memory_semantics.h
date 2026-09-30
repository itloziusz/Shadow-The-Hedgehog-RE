#pragma once

#include "boot_image.h"
#include "crt_semantics.h"
#include "golden_trace.h"

// Semantic result of 0x80003170 through 0x80003260. No low-memory page is allocated.
struct LowMemorySemantics {
    GuestWord32 boot_word = 0xFFFFFFFFu;
    GuestWord32 debug_flag = 0xFFFFFFFFu;
    GuestWord32 argument_offset = 0xFFFFFFFFu;
    GuestWord32 country_code = 0xFFFFFFFFu;
    GuestWord32 bi2_word_0x24 = 0xFFFFFFFFu;
    GuestAddress32 arena_low_observed = 0;
    GuestAddress32 arena_high = 0;
    GuestAddress32 bi2_observed = 0;
    bool debug_transfer = true;
    bool argument_relocation = true;
    GuestWord32 r14 = 1;
    GuestWord32 r15 = 1;
    GuestWord32 link_800030e4 = 1;
    GuestWord32 link_800030e6 = 1;
    GuestAddress32 pc = 0;
};

LowMemorySemantics ResolveLowMemoryPrelude(const BootImage& image, const CrtSemantics& crt,
                                           const PreEntryOracle& oracle);
