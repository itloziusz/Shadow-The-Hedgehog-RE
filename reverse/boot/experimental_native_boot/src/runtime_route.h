#pragma once

#include "boot_image.h"
#include "low_memory_semantics.h"

// 0x80370BF0 plus the proven route through 0x80370E68 to the constructor call.
// The runtime body and the constructors are not executed.
struct RuntimeRoute {
    GuestAddress32 os_entry_pc = 0;
    GuestWord32 os_marker_physical = 0;
    GuestWord32 os_marker_ready = 0;
    GuestAddress32 arena_low = 0;
    GuestAddress32 arena_high = 0;
    GuestWord32 link_800030e4 = 1;
    GuestWord32 link_800030e6 = 1;
    GuestWord32 init_flag_byte = 1;
    GuestAddress32 pc = 0;
    GuestAddress32 constructor_walker = 0;
    GuestAddress32 constructor_table = 0;
    int constructor_count = -1;
    GuestAddress32 first_constructor = 0;
    bool constructors_executed = true;
    bool guest_ram_allocated = true;
    bool os_body_executed = true;
};

RuntimeRoute ResolveRuntimeRoute(const BootImage& image, const LowMemorySemantics& low_memory);
