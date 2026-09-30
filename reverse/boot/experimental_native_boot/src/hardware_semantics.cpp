#include "hardware_semantics.h"

#include "boot_image.h"
#include "native_boot_manifest.h"

namespace {

bool g_fpr_consumed = false;

int HexValue(char character) {
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f') {
        return character - 'a' + 10;
    }
    if (character >= 'A' && character <= 'F') {
        return character - 'A' + 10;
    }
    throw BootError("FPR oracle digit is not hexadecimal");
}

std::uint64_t ParseBinary64(const std::string& text) {
    if (text.size() != 16u) {
        throw BootError("FPR oracle image is not 8 bytes");
    }
    std::uint64_t value = 0;
    for (const char character : text) {
        value = (value << 4) | static_cast<std::uint64_t>(HexValue(character));
    }
    return value;
}

}  // namespace

bool FprSemanticsConsumed() {
    return g_fpr_consumed;
}

GuestWord32 DeriveGekkoFPSCR(std::uint64_t source_fpr_bits) {
    // IBM Gekko User Manual v1.2, mtfsf: FEX and VX are derived after fields
    // are written. PPC bit numbers below count from the most significant bit.
    // FPSCR bit 20 is reserved and reads as zero (numeric mask 0x800).
    GuestWord32 word = static_cast<GuestWord32>(source_fpr_bits) & ~0x60000800u;
    constexpr GuestWord32 invalid_flags = 0x01F80700u; // bits 7..12, 21..23
    if ((word & invalid_flags) != 0) {
        word |= 0x20000000u; // VX, bit 2
    }
    const bool enabled_exception =
        ((word & 0x20000080u) == 0x20000080u) || // VX && VE
        ((word & 0x10000040u) == 0x10000040u) || // OX && OE
        ((word & 0x08000020u) == 0x08000020u) || // UX && UE
        ((word & 0x04000010u) == 0x04000010u) || // ZX && ZE
        ((word & 0x02000008u) == 0x02000008u);   // XX && XE
    if (enabled_exception) {
        word |= 0x40000000u; // FEX, bit 1
    }
    return word;
}

HardwareSemantics InitializeNativeHardwareSemantics(const PreEntryOracle& oracle) {
    if (!oracle.present) {
        throw BootError("hardware semantics require the external FPR image");
    }
    const std::uint64_t binary64 = ParseBinary64(oracle.memory_0x805f1f30);
    const auto paired = ParseBinary64(oracle.memory_0x805f1f38);
    const GuestWord32 ps1_source = static_cast<GuestWord32>(paired);
    const GuestWord32 exponent = ps1_source & 0x7F800000u;
    if ((exponent == 0 && (ps1_source & 0x007FFFFFu) != 0) ||
        exponent == 0x7F800000u) {
        throw BootError("paired-single PS1 exceptional source needs original-state validation");
    }

    HardwareSemantics result;
    result.pc = 0x8000315Cu;
    result.lr = 0x8000315Cu;
    result.r1 = NativeBootManifest::stack_base;
    result.r2 = NativeBootManifest::sdata2_base;
    result.r13 = NativeBootManifest::sdata_base;
    result.fpr_binary64 = binary64;
    result.fpscr = DeriveGekkoFPSCR(binary64);
    // lfd and fmr do NOT overwrite the second paired lane.
    result.paired_lane1_binary32 = ps1_source;
    result.paired_temporary_survives = true;
    g_fpr_consumed = true;
    return result;
}
