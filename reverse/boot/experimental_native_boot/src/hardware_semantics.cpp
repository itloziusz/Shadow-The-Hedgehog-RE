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

HardwareSemantics InitializeNativeHardwareSemantics(const PreEntryOracle& oracle) {
    if (!oracle.present) {
        throw BootError("hardware semantics require the external FPR image");
    }
    const std::uint64_t binary64 = ParseBinary64(oracle.memory_0x805f1f30);
    const auto paired = ParseBinary64(oracle.memory_0x805f1f38);

    HardwareSemantics result;
    result.pc = 0x8000315Cu;
    result.lr = 0x8000315Cu;
    result.r1 = NativeBootManifest::stack_base;
    result.r2 = NativeBootManifest::sdata2_base;
    result.r13 = NativeBootManifest::sdata_base;
    result.fpr_binary64 = binary64;
    // mtfsf 255 takes bits 32..63 of the source register.
    result.fpscr = static_cast<GuestWord32>(binary64);
    // lfd and fmr do NOT overwrite the second paired lane.
    result.paired_lane1_binary32 = static_cast<GuestWord32>(paired);
    result.paired_temporary_survives = true;
    g_fpr_consumed = true;
    return result;
}
