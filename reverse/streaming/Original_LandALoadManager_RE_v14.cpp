// Shadow the Hedgehog (GC) - LandALoadManager reconstruction checkpoint v1.4
// [H] directly evidenced in PPC / embedded strings. [M] semantic interpretation.
#include <cstddef>
#include <cstdint>

namespace original_shadow_re {

// Embedded strings [H]:
// 0x804ACA8C "LandALoadMananger" (original typo)
// 0x804ACABC "%s/%s_TEX%02d.one"
// 0x804ACAD0 "%s/%s_%02d.one"
// 0x804ACAE0 "LandALoadManager"
// 0x804ACAF4 "LandManager.cpp"
// 0x804ACB18 "stg%04d/stg%04d_light.bin"

struct LandALoadManagerLayout32 {
    std::byte base[0x28];
    char stage_base_name[0x40];             // +0x28 [M], sprintf source is this+40
    std::byte unknown_68[0x40];
    std::uint32_t* external_status;         // +0xA8 [M], values 1/2/5 observed
    void* main_one_async;                   // +0xAC [M]
    void* texture_one_async;                // +0xB0 [M]
    std::uint32_t state;                    // +0xB4 [H], jump table 0..12
    std::uint32_t current_resource_index;   // +0xB8 [M], initialized 2 and incremented
    std::uint32_t parsed_two_digit_suffix;  // +0xBC [M]
    void* parent_or_land_manager;            // +0xC0 [M]
    std::uint8_t child_pending_flags[15];   // +0xC4 [H], cleared/scanned as 15 bytes
};
static_assert(offsetof(LandALoadManagerLayout32,state)==0xB4);
static_assert(offsetof(LandALoadManagerLayout32,child_pending_flags)==0xC4);

// Constructor @ 0x80046AF0 [H].
// - formats "%s/%s_%02d.one" into this+0x68
// - copies one supplied string into this+0x28
// - external_status = arg7; *external_status = 1
// - main_one_async/texture_one_async = nullptr
// - state = 1; current_resource_index = 2; parsed_two_digit_suffix = 0
// - registers task under RTTI/name "LandALoadManager" through @0x8025D580.

// Update state machine @ 0x800466B4 [H] has 13 jump-table states (0..12).
// High-confidence behavior fragments:
// - allocates an 88-byte TOneFile-style loader object @ 0x80046700 and starts the main ONE.
// - polls loader completion via @0x8004CB20.
// - iterates ONE resource entries using @0x8004CA50.
// - parses two ASCII decimal digits from a resource name into +0xBC.
// - for non-zero suffix, formats "%s/%s_TEX%02d.one" @ 0x80046874 and launches a second async ONE.
// - current_resource_index increments and iteration stops at (main archive count + 2).
// - child bookkeeping uses a global table indexed by the parsed 0..9 suffix and 15 pending flags.
// - state 12 is the terminal/idle state in this path [M].

// Stage light resource helper @ 0x80048738 [H]:
// snprintf-like formatting of "stg%04d/stg%04d_light.bin" into a stack buffer,
// then calls loader @ 0x8004867C with the same stage id twice.
inline const char* StageLightPathFormat() { return "stg%04d/stg%04d_light.bin"; }
inline const char* StageTexturePartFormat() { return "%s/%s_TEX%02d.one"; }
inline const char* StagePartFormat() { return "%s/%s_%02d.one"; }

} // namespace original_shadow_re
