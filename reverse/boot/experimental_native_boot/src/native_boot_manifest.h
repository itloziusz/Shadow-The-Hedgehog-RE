#pragma once

#include "guest_types.h"

#include <array>
#include <cstddef>

// Typed identity of the one PAL revision this laboratory accepts.
// Fixture loading may read main.dol to check these fields. The fields themselves
// are build data, not a generic DOL loader.

struct DolSectionSpec {
    bool text = false;
    std::uint32_t index = 0;
    GuestOffset32 file_offset = 0;
    GuestAddress32 address = 0;
    GuestWord32 size = 0;
};

struct InstructionPin {
    GuestAddress32 address = 0;
    GuestWord32 word = 0;
};

struct NativeBootManifest {
    static constexpr const char* revision = "GUPP8P";
    static constexpr HostByteCount64 dol_size = 0x5816E0ull;
    static constexpr GuestAddress32 entry = 0x80003154u;
    static constexpr GuestAddress32 register_helper = 0x800032B0u;
    static constexpr GuestAddress32 stop_before_hardware = 0x80003158u;
    static constexpr GuestAddress32 hardware_helper = 0x80003400u;
    static constexpr GuestAddress32 crt_initializer = 0x80003340u;
    static constexpr GuestAddress32 bss_address = 0x8056FE00u;
    static constexpr GuestWord32 bss_size = 0x0008C7ECu;
    static constexpr GuestWord32 stack_base = 0x8060C5F0u;
    static constexpr GuestWord32 sdata2_base = 0x805FA780u;
    static constexpr GuestWord32 sdata_base = 0x805EC500u;

    // Consumed by the FPR helper. Not a DOL section. Never synthesized.
    static constexpr GuestAddress32 fpr_source = 0x805F1F30u;
    // Read by psq_l, then overwritten before that helper returns.
    static constexpr GuestAddress32 paired_source = 0x805F1F38u;

    static constexpr GuestPtr32 copy_table{0x80005544u};
    static constexpr GuestPtr32 zero_table{0x800055C8u};

    static constexpr std::array<DolSectionSpec, 10> sections{{
        {true, 0, 0x00000100u, 0x80003100u, 0x00002500u},
        {true, 1, 0x00002600u, 0x80008D40u, 0x004A1F20u},
        {false, 0, 0x004A4520u, 0x80005600u, 0x00001F20u},
        {false, 1, 0x004A6440u, 0x80007520u, 0x00001820u},
        {false, 2, 0x004A7C60u, 0x804AAC60u, 0x00000480u},
        {false, 3, 0x004A80E0u, 0x804AB0E0u, 0x00000020u},
        {false, 4, 0x004A8100u, 0x804AB100u, 0x00072420u},
        {false, 5, 0x0051A520u, 0x8051D520u, 0x000528E0u},
        {false, 6, 0x0056CE00u, 0x805E4500u, 0x0000AB20u},
        {false, 7, 0x00577920u, 0x805F2780u, 0x00009DC0u},
    }};

    // Entry and the instruction it must return to. The second word is not executed.
    static constexpr std::array<InstructionPin, 2> entry_pins{{
        {0x80003154u, 0x4800015Du},
        {0x80003158u, 0x480002A9u},
    }};

    // 0x800032B0..0x8000333C inclusive. Clears every GPR, then sets r1/r2/r13.
    static constexpr std::array<InstructionPin, 36> register_helper_pins{{
        {0x800032B0u, 0x38000000u}, {0x800032B4u, 0x38600000u}, {0x800032B8u, 0x38800000u},
        {0x800032BCu, 0x38A00000u}, {0x800032C0u, 0x38C00000u}, {0x800032C4u, 0x38E00000u},
        {0x800032C8u, 0x39000000u}, {0x800032CCu, 0x39200000u}, {0x800032D0u, 0x39400000u},
        {0x800032D4u, 0x39600000u}, {0x800032D8u, 0x39800000u}, {0x800032DCu, 0x39C00000u},
        {0x800032E0u, 0x39E00000u}, {0x800032E4u, 0x3A000000u}, {0x800032E8u, 0x3A200000u},
        {0x800032ECu, 0x3A400000u}, {0x800032F0u, 0x3A600000u}, {0x800032F4u, 0x3A800000u},
        {0x800032F8u, 0x3AA00000u}, {0x800032FCu, 0x3AC00000u}, {0x80003300u, 0x3AE00000u},
        {0x80003304u, 0x3B000000u}, {0x80003308u, 0x3B200000u}, {0x8000330Cu, 0x3B400000u},
        {0x80003310u, 0x3B600000u}, {0x80003314u, 0x3B800000u}, {0x80003318u, 0x3BA00000u},
        {0x8000331Cu, 0x3BC00000u}, {0x80003320u, 0x3BE00000u}, {0x80003324u, 0x3C208060u},
        {0x80003328u, 0x6021C5F0u}, {0x8000332Cu, 0x3C40805Fu}, {0x80003330u, 0x6042A780u},
        {0x80003334u, 0x3DA0805Eu}, {0x80003338u, 0x61ADC500u}, {0x8000333Cu, 0x4E800020u},
    }};
};
