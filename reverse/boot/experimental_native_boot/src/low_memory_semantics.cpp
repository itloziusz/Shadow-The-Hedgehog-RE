#include "low_memory_semantics.h"

namespace {

GuestWord32 Fingerprint(const BootImage& image, GuestAddress32 begin, GuestAddress32 end) {
    std::uint32_t hash = 2166136261u;
    for (GuestAddress32 pc = begin; pc <= end; pc += 4u) {
        hash ^= image.ReadWord(pc);
        hash *= 16777619u;
    }
    return hash;
}

int Digit(char character) {
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f') {
        return character - 'a' + 10;
    }
    if (character >= 'A' && character <= 'F') {
        return character - 'A' + 10;
    }
    throw BootError("low-memory oracle digit is not hexadecimal");
}

GuestWord32 ParseWord(const std::string& text) {
    std::size_t start = 0;
    if (text.size() >= 2u && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        start = 2;
    }
    if (text.size() - start != 8u) {
        throw BootError("low-memory oracle word is not 4 bytes");
    }
    GuestWord32 value = 0;
    for (std::size_t index = start; index < text.size(); ++index) {
        value = (value << 4) | static_cast<GuestWord32>(Digit(text[index]));
    }
    return value;
}

GuestWord32 Bi2Word(const std::string& hex, int byte_offset) {
    const std::size_t start = static_cast<std::size_t>(byte_offset) * 2u;
    if (hex.size() < start + 8u) {
        throw BootError("BI2 oracle image is shorter than the field");
    }
    GuestWord32 value = 0;
    for (std::size_t index = start; index < start + 8u; ++index) {
        value = (value << 4) | static_cast<GuestWord32>(Digit(hex[index]));
    }
    return value;
}

}  // namespace

LowMemorySemantics ResolveLowMemoryPrelude(const BootImage& image, const CrtSemantics& crt,
                                           const PreEntryOracle& oracle) {
    if (crt.pc != 0x80003170u) {
        throw BootError("low-memory prelude did not start at the M4 boundary");
    }
    if (!oracle.has_lowmem) {
        throw BootError("low-memory prelude requires observed branch inputs");
    }
    if (Fingerprint(image, 0x80003170u, 0x800032ACu) != 0xA6217309u) {
        throw BootError("low-memory prelude body changed");
    }

    const GuestAddress32 bi2 = ParseWord(oracle.lowmem_0x800000f4);
    const GuestWord32 arena_low = ParseWord(oracle.lowmem_0x80000030);
    const GuestWord32 arena_high = ParseWord(oracle.lowmem_0x80000034);
    const GuestWord32 debug_flag = Bi2Word(oracle.bi2_bytes, 0x0C);
    const GuestWord32 argument_offset = Bi2Word(oracle.bi2_bytes, 0x08);
    if (bi2 == 0u) {
        throw BootError("BI2 pointer is zero; that branch was not the captured retail path");
    }
    if (debug_flag != 0u || argument_offset != 0u) {
        throw BootError("BI2 debug or argument field leaves the proven retail path");
    }

    LowMemorySemantics result;
    result.boot_word = 0;
    result.debug_flag = debug_flag;
    result.argument_offset = argument_offset;
    result.country_code = Bi2Word(oracle.bi2_bytes, 0x18);
    result.bi2_word_0x24 = Bi2Word(oracle.bi2_bytes, 0x24);
    result.arena_low_observed = arena_low;
    result.arena_high = arena_high;
    result.bi2_observed = bi2;
    result.debug_transfer = false;
    result.argument_relocation = false;
    result.r14 = 0;
    result.r15 = 0;
    result.link_800030e4 = ParseWord(oracle.lowmem_0x800030e4);
    result.link_800030e6 = ParseWord(oracle.lowmem_0x800030e6);
    if ((result.link_800030e6 & 0xFFFFu) != 0u || (result.link_800030e4 & 0xFFFFu) != 0u) {
        throw BootError("post-runtime decision halfwords are not the captured zeros");
    }
    result.pc = 0x80003260u;
    return result;
}
