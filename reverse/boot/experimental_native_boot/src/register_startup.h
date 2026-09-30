#pragma once

#include "boot_image.h"
#include "guest_types.h"

#include <array>

struct StartupState {
    std::array<GuestWord32, 32> gpr{};
    GuestAddress32 pc = NativeBootManifest::entry;
    GuestAddress32 lr = 0;
};

// Translated effect of 0x80003154 -> 0x800032B0 -> return at 0x80003158.
// Stops before the hardware helper. Does not read unmapped memory.
StartupState EnterRegisterStartup(const BootImage& image);
