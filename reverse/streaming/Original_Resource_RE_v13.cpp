// Shadow the Hedgehog (GC) resource/streaming reconstruction checkpoint.
// [H] direct PPC instruction evidence. [M] semantic name, still subject to refinement.
// This file documents the original 32-bit ABI. It is NOT used as the PC runtime ABI.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace shadow_original_re {

using GuestPtr = std::uint32_t;

#pragma pack(push,1)
struct OneResourceEntry32 {
    char lookup_key[0x20];               // [M] entry begins with comparison key/string
    GuestPtr auxiliary_resource;         // [H/M] +0x20 read by function 0x80014640
    std::byte unknown_24_37[0x14];
    GuestPtr payload;                    // [H] +0x38 returned by 0x800145A4
};
static_assert(sizeof(OneResourceEntry32) == 0x3C);

struct ResourceOneFile32 {
    std::byte unknown_00_13[0x14];
    GuestPtr entries;                    // [H] +0x14
    std::int32_t entry_count;            // [H] +0x18
};
#pragma pack(pop)

// [H] PPC 0x800145A4..0x80014618
// - scans backwards
// - stride 0x3C
// - comparison routine 0x803AD96C
// - returns entry+0x38
GuestPtr FindPayloadByName_RE(const ResourceOneFile32& self,
                              const std::byte* guest_memory,
                              const char* requested_name) {
    const auto* entries = guest_memory + self.entries;
    for (std::int32_t i = self.entry_count - 1; i >= 0; --i) {
        const auto* raw = entries + std::size_t(i) * 0x3C;
        if (std::strcmp(reinterpret_cast<const char*>(raw), requested_name) == 0) {
            GuestPtr payload{};
            std::memcpy(&payload, raw + 0x38, sizeof(payload));
            return payload;
        }
    }
    return 0;
}

// [H] PPC 0x800453E0 scans exactly three pointers at 0x805742A0.
inline constexpr std::uintptr_t kAsyncSlotTable = 0x805742A0;
inline constexpr std::uint32_t kOriginalAsyncSlotCount = 3;
inline constexpr std::uint32_t kOriginalReadAlignment = 32;
inline constexpr std::uint32_t kBackgroundWorkerObjectBytes = 0x390;

struct AsyncReadSlot_RE {
    GuestPtr control_object;             // [H] table entry itself points to an object
    GuestPtr aligned_buffer;             // [M] observed allocation-associated field near slot+4 path
    std::uint32_t aligned_byte_count;    // [H/M] rounded with +31 and mask low five bits
};

// Readable semantic reconstruction of the key scheduling block at 0x800453E0.
// This intentionally does not invent names for calls whose exact SDK identity is not yet proven.
void TFileControlAsycManager_Update_RE() {
    // [H] Find first occupied slot in a fixed three-slot table.
    // [H] If no occupied slot is found, call 0x80045370(false) and return.
    // [H] If one is found, call 0x80045370(true).
    // [H] Query/start operation through 0x8047AFA0.
    // [H] If a destination buffer has not been allocated yet:
    //       aligned_size = (requested_size + 31) & ~31;
    //       buffer = 0x80049BB8(aligned_size, heap_or_mode=2);
    // [H] Allocate 0x390 bytes via operator-new-like 0x803A1380.
    // [H] Construct background worker/control object at 0x80045778.
    // [H] Final slot/control cleanup uses 0x80049B5C and rewrites the slot table entry.
}

// PC modernization rule:
// preserve request/completion semantics, priority and ownership;
// do NOT preserve the 3-request concurrency ceiling, guest pointers or 32-bit ABI.

} // namespace shadow_original_re
