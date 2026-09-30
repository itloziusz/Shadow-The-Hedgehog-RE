#include "crt_semantics.h"

#include "native_boot_manifest.h"

namespace {

GuestWord32 Fingerprint(const BootImage& image, GuestAddress32 begin, GuestAddress32 end) {
    std::uint32_t hash = 2166136261u;
    for (GuestAddress32 pc = begin; pc <= end; pc += 4u) {
        hash ^= image.ReadWord(pc);
        hash *= 16777619u;
    }
    return hash;
}

void PinWord(const BootImage& image, GuestAddress32 address, GuestWord32 expected) {
    if (image.ReadWord(address) != expected) {
        throw BootError("CRT startup fingerprint mismatch");
    }
}

}  // namespace

CrtSemantics InitializeNativeCrt(const BootImage& image, const HardwareSemantics& hardware) {
    if (!FprSemanticsConsumed()) {
        throw BootError("CRT ran before the FPR source was consumed");
    }
    if (hardware.pc != 0x8000315Cu || hardware.r1 != NativeBootManifest::stack_base) {
        throw BootError("CRT received a hardware result that is not the M3 boundary");
    }
    // Whole initializer, and the fill routine it would have called. The native
    // path does not call either one.
    if (Fingerprint(image, 0x80003340u, 0x800033FCu) != 0x556BA682u) {
        throw BootError("CRT initializer body changed");
    }
    if (Fingerprint(image, 0x8000540Cu, 0x800054F0u) != 0x4EADA6C8u) {
        throw BootError("CRT fill routine changed");
    }

    // 0x8000315C li r0,-1; stwu r1,-8(r1); stw r0,4(r1); stw r0,0(r1); bl CRT.
    PinWord(image, 0x8000315Cu, 0x3800FFFFu);
    PinWord(image, 0x80003160u, 0x9421FFF8u);
    PinWord(image, 0x80003164u, 0x90010004u);
    PinWord(image, 0x80003168u, 0x90010000u);
    PinWord(image, 0x8000316Cu, 0x480001D5u);
    PinWord(image, 0x80003170u, 0x38000000u);

    GuestAddress32 cursor = NativeBootManifest::copy_table.raw;
    int copies = 0;
    for (;;) {
        const GuestWord32 source = image.ReadWord(cursor);
        const GuestWord32 destination = image.ReadWord(cursor + 4u);
        const GuestWord32 size = image.ReadWord(cursor + 8u);
        cursor += 12u;
        if (size == 0u && source == 0u && destination == 0u) {
            break;
        }
        if (source != destination || size == 0u) {
            throw BootError("CRT copy entry is not an identity no-op");
        }
        ++copies;
        if (copies > 10) {
            throw BootError("CRT copy table did not terminate");
        }
    }
    if (copies != 10) {
        throw BootError("CRT copy table does not have 10 identity entries");
    }

    struct ZeroRange {
        GuestWord32 address;
        GuestWord32 size;
    };
    const ZeroRange expected[] = {
        {0x8056FE00u, 0x00074700u},
        {0x805EF020u, 0x0000375Cu},
        {0x805FC540u, 0x000000ACu},
    };
    cursor = NativeBootManifest::zero_table.raw;
    int ranges = 0;
    for (const ZeroRange& range : expected) {
        if (image.ReadWord(cursor) != range.address || image.ReadWord(cursor + 4u) != range.size) {
            throw BootError("CRT zero table does not match the regenerated ranges");
        }
        cursor += 8u;
        ++ranges;
    }
    if (image.ReadWord(cursor) != 0u || image.ReadWord(cursor + 4u) != 0u) {
        throw BootError("CRT zero table did not terminate after 3 ranges");
    }
    const auto contains = [](GuestWord32 base, GuestWord32 size, GuestAddress32 address) {
        const HostByteCount64 begin = base;
        const HostByteCount64 end = begin + size;
        return HostByteCount64{address} >= begin && HostByteCount64{address} < end;
    };
    if (!contains(expected[1].address, expected[1].size, NativeBootManifest::fpr_source) ||
        !contains(expected[0].address, expected[0].size, 0x80580000u)) {
        throw BootError("CRT zero ranges do not cover the M3 addresses");
    }

    // Semantic translation of the four instructions, then a CRT step that
    // copies nothing and does not write a guest image.
    CrtSemantics result;
    result.r0 = 0xFFFFFFFFu;
    result.r1 = hardware.r1 - 8u;
    result.stack_word_at_r1 = 0xFFFFFFFFu;
    result.stack_word_at_r1_plus_4 = 0xFFFFFFFFu;
    result.pc = 0x80003170u;
    result.identity_copies = copies;
    result.zero_ranges = ranges;
    result.copies_executed = false;
    result.guest_image_zeroed = false;
    // Explicit projection of the original CRT fill, not default BSS/guest RAM.
    // Both halves independently proved: HIGH800054AC then LOW800054B0.
    // Publish one native timestamp only after validating the whole original range.
    if (!contains(expected[0].address, expected[0].size, 0x805A6830u) ||
        !contains(expected[0].address, expected[0].size, 0x805A6837u)) {
        throw BootError("CRT range does not produce the complete SI type timestamp");
    }
    const auto fill_byte = image.ReadWord(0x800033D4u) & 0xFFu; // original li r4,0
    result.si_type_query_channel0.ticks_ = std::uint64_t{fill_byte} * 0x0101010101010101ULL;
    if (result.r1 != 0x8060C5E8u || result.copies_executed || result.guest_image_zeroed) {
        throw BootError("CRT semantic result is not the proven no-copy boundary");
    }
    return result;
}
