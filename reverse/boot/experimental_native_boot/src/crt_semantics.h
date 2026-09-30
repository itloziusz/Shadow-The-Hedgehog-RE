#pragma once

#include "boot_image.h"
#include "hardware_semantics.h"
#include "si_type_query_timestamp.h"

// Native CRT result. Identity copies are not performed. Zero ranges are a
// semantic fact, not a write into a guest RAM image.
struct CrtSemantics {
    SiTypeQueryTimestamp si_type_query_channel0;
    GuestAddress32 pc = 0;
    GuestWord32 r0 = 0;
    GuestWord32 r1 = 0;
    GuestWord32 stack_word_at_r1 = 0;
    GuestWord32 stack_word_at_r1_plus_4 = 0;
    int identity_copies = 0;
    int zero_ranges = 0;
    bool copies_executed = true;
    bool guest_image_zeroed = true;
};

CrtSemantics InitializeNativeCrt(const BootImage& image, const HardwareSemantics& hardware);
