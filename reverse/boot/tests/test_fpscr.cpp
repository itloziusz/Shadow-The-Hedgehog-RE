#include "hardware_semantics.h"

#include <cstdint>
#include <exception>
#include <iostream>

int main() {
    struct Case {
        std::uint64_t fpr;
        std::uint32_t fpscr;
    };
    // IBM Gekko mtfsf FM=0xFF: summary bits FEX/VX are derived, not copied.
    // The first nonzero case is an ordinary finite binary64 value whose low
    // word happens to have raw FEX set. A plain low-word copy gets it wrong.
    constexpr Case cases[] = {
        {0x0000000000000000ull, 0x00000000u},
        {0x3FF0000040000000ull, 0x00000000u}, // raw FEX without exceptions
        {0x3FF0000020000000ull, 0x00000000u}, // raw VX without invalid flags
        {0x3FF0000001000000ull, 0x21000000u}, // invalid flag derives VX
        {0x3FF0000001000080ull, 0x61000080u}, // VX+VE derives FEX
        {0x3FF0000010000040ull, 0x50000040u}, // OX+OE derives FEX
        {0x3FF0000080000000ull, 0x80000000u}, // FX copied
        {0x3FF0000000000800ull, 0x00000000u}, // reserved bit 20 ignores a write
    };
    for (const Case& item : cases) {
        if (DeriveGekkoFPSCR(item.fpr) != item.fpscr) {
            std::cerr << "Gekko mtfsf summary arithmetic differs\n";
            return 1;
        }
    }
    PreEntryOracle exceptional;
    exceptional.present = true;
    exceptional.memory_0x805f1f30 = "0000000000000000";
    exceptional.memory_0x805f1f38 = "0000000000000001"; // PS1 denormal
    bool declined = false;
    try {
        (void)InitializeNativeHardwareSemantics(exceptional);
    } catch (const std::exception&) {
        declined = true;
    }
    if (!declined) {
        std::cerr << "unvalidated paired-single denormal was accepted\n";
        return 1;
    }
    std::cout << "Gekko mtfsf summary arithmetic verified\n";
    return 0;
}
