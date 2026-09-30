#include "runtime_route.h"

namespace {

GuestWord32 Fingerprint(const BootImage& image, GuestAddress32 begin, GuestAddress32 end) {
    std::uint32_t hash = 2166136261u;
    for (GuestAddress32 pc = begin; pc <= end; pc += 4u) {
        hash ^= image.ReadWord(pc);
        hash *= 16777619u;
    }
    return hash;
}

void Pin(const BootImage& image, GuestAddress32 address, GuestWord32 expected) {
    if (image.ReadWord(address) != expected) {
        throw BootError("runtime-route fingerprint mismatch");
    }
}

}  // namespace

RuntimeRoute ResolveRuntimeRoute(const BootImage& image, const LowMemorySemantics& low_memory) {
    if (low_memory.pc != 0x80003260u || low_memory.debug_transfer || low_memory.argument_relocation) {
        throw BootError("runtime route received a prelude that left the retail path");
    }
    if (Fingerprint(image, 0x80370BF0u, 0x80370C14u) != 0xB29AFE29u ||
        Fingerprint(image, 0x80003100u, 0x8000313Cu) != 0x576EE230u ||
        Fingerprint(image, 0x8000314Cu, 0x80003150u) != 0xAC83C45Du ||
        Fingerprint(image, 0x803796ACu, 0x8037971Cu) != 0xE5245B04u) {
        throw BootError("runtime or constructor walker body changed");
    }
    // Arena selection and the skipped halfword store.
    // First OSSetArenaLo (0x80370F74..0x80370F8C): BootInfo->arenaLo, or the
    // linker default 0x8060E600 when that word is zero.
    Pin(image, 0x80370F74u, 0x80630030u);
    Pin(image, 0x80370F7Cu, 0x40820010u);
    Pin(image, 0x80370F80u, 0x3C608061u);
    Pin(image, 0x80370F84u, 0x3863E600u);
    Pin(image, 0x80370F8Cu, 0x480012A1u);
    // BI2DebugFlag = &BI2->debugFlag when the BI2 pointer at 0x800000F4 is
    // nonzero (0x80370F08..0x80370F18; the only other producer, 0x80370F44..,
    // is the zero-pointer branch this prelude rejects).
    Pin(image, 0x80370EFCu, 0x900D5A24u);
    Pin(image, 0x80370F08u, 0x806400F4u);
    Pin(image, 0x80370F10u, 0x41820034u);
    Pin(image, 0x80370F14u, 0x3803000Cu);
    Pin(image, 0x80370F18u, 0x900D5A24u);
    // Second, conditional OSSetArenaLo (0x80370F90..0x80370FC8): when
    // BootInfo->arenaLo is zero and *BI2DebugFlag < 2, the arena starts at the
    // 32-byte-aligned stack top ALIGN32(0x8060C5F0) = 0x8060C600. Omitting this
    // second store made the route report 0x8060E600 although the reference
    // reads 0x8060C600 at 0x8037221C (lane 5 finding, coordinator-verified).
    Pin(image, 0x80370F94u, 0x80030030u);
    Pin(image, 0x80370F9Cu, 0x40820030u);
    Pin(image, 0x80370FA0u, 0x806D5A24u);
    Pin(image, 0x80370FA8u, 0x41820024u);
    Pin(image, 0x80370FACu, 0x80030000u);
    Pin(image, 0x80370FB0u, 0x28000002u);
    Pin(image, 0x80370FB4u, 0x40800018u);
    Pin(image, 0x80370FB8u, 0x3C608061u);
    Pin(image, 0x80370FBCu, 0x3863C5F0u);
    Pin(image, 0x80370FC0u, 0x3803001Fu);
    Pin(image, 0x80370FC4u, 0x54030034u);
    Pin(image, 0x80370FC8u, 0x48001265u);
    Pin(image, 0x80370FDCu, 0x3C60817Au);
    Pin(image, 0x80370FE0u, 0x38630000u);
    Pin(image, 0x803712ECu, 0x800D5A2Cu);
    Pin(image, 0x803712F4u, 0x41820018u);
    Pin(image, 0x80371304u, 0xB00330E6u);
    Pin(image, 0x80003264u, 0x4836DC05u);
    Pin(image, 0x8000329Cu, 0x48376411u);

    const GuestWord32 link_e4 = 0;
    const GuestWord32 link_e6 = 0;
    // No store in the DOL writes r13+0x5A2C, so the 0x9000 store is not taken.
    // No store writes 0x800030E4. The flag byte's only writer is the debug-flag-4
    // path, which this prelude did not take. The post-return checks therefore see zeros.
    if (low_memory.debug_flag != 0u) {
        throw BootError("debug flag would change the post-runtime branch");
    }

    RuntimeRoute route;
    route.os_entry_pc = 0x80003264u;
    route.os_marker_physical = 0x00370C60u;
    route.os_marker_ready = 1u;
    if (low_memory.bi2_observed == 0u) {
        throw BootError("arena selection requires the proven nonzero BI2 pointer");
    }
    if (low_memory.arena_low_observed == 0u) {
        // BI2DebugFlag points at the BI2 debug word (low_memory.debug_flag, proven 0).
        route.arena_low = low_memory.debug_flag < 2u ? ((0x8060C5F0u + 0x1Fu) & ~0x1Fu) : 0x8060E600u;
    } else {
        route.arena_low = low_memory.arena_low_observed;
    }
    route.arena_high = low_memory.arena_high == 0u ? 0x817A0000u : low_memory.arena_high;
    route.link_800030e4 = link_e4;
    route.link_800030e6 = link_e6;
    route.init_flag_byte = 0;
    route.constructor_walker = 0x803796ACu;
    route.constructor_table = 0x804AAC60u;
    route.pc = 0x8000329Cu;
    route.constructors_executed = false;
    route.guest_ram_allocated = false;
    route.os_body_executed = false;

    GuestAddress32 cursor = route.constructor_table;
    std::uint32_t hash = 2166136261u;
    int count = 0;
    GuestAddress32 first = 0;
    for (;;) {
        const GuestWord32 pointer = image.ReadWord(cursor);
        hash ^= pointer;
        hash *= 16777619u;
        if (pointer == 0u) {
            break;
        }
        if (count == 0) {
            first = pointer;
        }
        ++count;
        cursor += 4u;
        if (count > 400) {
            throw BootError("constructor table did not terminate");
        }
    }
    if (count != 282 || first != 0x803A2520u || hash != 0x337F88A7u) {
        throw BootError("constructor table does not match the regenerated PAL table");
    }
    route.constructor_count = count;
    route.first_constructor = first;
    if (route.constructors_executed || route.guest_ram_allocated || route.os_body_executed) {
        throw BootError("runtime route executed a body it was required to stop before");
    }
    return route;
}
