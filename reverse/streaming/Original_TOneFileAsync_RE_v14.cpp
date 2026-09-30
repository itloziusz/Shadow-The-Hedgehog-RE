// Shadow the Hedgehog (GC) - TOneFileAsync<T> reconstruction checkpoint v1.4
// Source: main.dol PPC disassembly. [H] = directly evidenced, [M] = semantic name inferred.
#include <cstddef>
#include <cstdint>

namespace original_shadow_re {

enum class OneAsyncState : std::uint32_t {
    PrepareRead = 0,        // [H] dispatch state 0
    WaitTypedDecode = 1,    // [M] waits for +0x50 pending byte / +0x48 typed result
    BuildRuntimeObject = 2, // [M] constructs secondary runtime object at +0x3C
    WaitPublishGate = 3,    // [M] waits for byte +0x58 before state 4
    FinalizePublish = 4     // [M] cleanup + callback + task completion flag
};

// [H] Common object offsets observed in every specialization state machine.
struct TOneFileAsyncLayout32 {
    std::byte unknown_00[0x18];
    const void* specialization_vtable;       // +0x18 [H]
    std::byte unknown_1C[0x0C];
    OneAsyncState state;                     // +0x28 [H]
    void (*completion_callback)(void*,void*);// +0x2C [M], called through CTR in state 4
    void* completion_user;                   // +0x30 [M], passed as r4 to callback
    std::uint32_t resource_entry_index;      // +0x34 [M], used with ResourceOneFile lookup @ 0x8004CA50
    void* aligned_read_buffer;               // +0x38 [H], allocated/freed through heap funcs
    void* runtime_object;                    // +0x3C [M], secondary typed object produced in state 2
    void* one_archive_context;               // +0x40 [M], argument to resource lookup/start helper
    std::byte unknown_44[4];
    void* typed_decode_result;                // +0x48 [M], async callback stores r3 here
    void* async_job_or_message;               // +0x4C [M], created in state 2
    std::uint8_t typed_decode_pending;        // +0x50 [H], callbacks clear to zero
    std::byte unknown_51[3];
    void* support_object;                     // +0x54 [M], consumed by state 2
    std::uint8_t publish_gate;                // +0x58 [H], state 3 waits for nonzero
};
static_assert(offsetof(TOneFileAsyncLayout32,state)==0x28);
static_assert(offsetof(TOneFileAsyncLayout32,aligned_read_buffer)==0x38);
static_assert(offsetof(TOneFileAsyncLayout32,typed_decode_result)==0x48);
static_assert(offsetof(TOneFileAsyncLayout32,typed_decode_pending)==0x50);

struct SpecializationAddress {
    const char* type_name;
    std::uint32_t vtable_block;
    std::uint32_t state_machine;
    std::uint32_t deleting_destructor;
    std::uint32_t update_wrapper;
};
constexpr SpecializationAddress kSpecializations[] = {
    {"RpDMorphAnimation", 0x8051E708, 0x8004AE60, 0x8004D05C, 0x8004D0B4},
    {"RpWorld",           0x8051E6EC, 0x8004B3A0, 0x8004D0D8, 0x8004D130},
    {"RtDict",            0x8051E6D0, 0x8004B7BC, 0x8004D154, 0x8004D1AC},
    {"RpClump",           0x8051E6B4, 0x8004BBD4, 0x8004D1D0, 0x8004D228},
    {"RwTexDictionary",   0x8051E698, 0x8004BFF0, 0x8004D24C, 0x8004D2A4},
    {"void",              0x8051E67C, 0x8004C408, 0x8004D2C8, 0x8004D320},
};

// Named pseudocode reconstructed from the RpWorld specialization @ 0x8004B3A0.
// Calls retain their original addresses when semantics are not yet proved.
bool TOneFileAsync_RpWorld_Update(TOneFileAsyncLayout32* self, void* optional_output) {
    switch (self->state) {
    case OneAsyncState::PrepareRead: {
        // 0x8004B3E8..0x8004B3F4: two global/precondition helpers; requires result >= 16.
        // Resource metadata = sub_8004CA50(self->one_archive_context, self->resource_entry_index).
        // metadata +0x2C is the stored byte count [M].
        // Allocation is 32-byte aligned [H]. The code first attempts size+0x401F,
        // may free it, then retries aligned(size), preserving an unexplained 0x4000 reserve path.
        // sub_8004D564 starts/attaches the read operation [M].
        // sub_8004C770 performs specialization-specific conversion [M].
        // If completion_callback != nullptr it launches async conversion and sets +0x50=1.
        // Else it stores the immediate typed result at +0x48 and clears +0x50.
        self->state = OneAsyncState::WaitTypedDecode;
        return false;
    }
    case OneAsyncState::WaitTypedDecode:
        if (self->typed_decode_pending) return false;
        if (!self->typed_decode_result) {
            // [H] frees +0x38 and moves directly to state 4.
            self->state = OneAsyncState::FinalizePublish;
            return false;
        }
        self->state = OneAsyncState::BuildRuntimeObject;
        // [H] invokes shared helper @ 0x8004D424 after state transition.
        return false;
    case OneAsyncState::BuildRuntimeObject:
        // [H] creates an object/message with helper @ 0x804823B8, stores +0x4C,
        // invokes method-id 11 through @ 0x8047DFA0, consumes support_object +0x54,
        // and stores a resulting pointer at +0x3C.
        self->state = OneAsyncState::FinalizePublish;
        return false;
    case OneAsyncState::WaitPublishGate:
        if (!self->publish_gate) return false;
        self->state = OneAsyncState::FinalizePublish;
        [[fallthrough]];
    case OneAsyncState::FinalizePublish:
        // [H] conditionally frees aligned_read_buffer, tears down +0x4C,
        // optionally writes runtime_object to optional_output, invokes callback(runtime_object,user),
        // decrements two counters, sets task flag bit 0 at this+4, and returns true.
        (void)optional_output;
        return true;
    }
    return false;
}

// [H] Representative async completion callback shape (e.g. 0x8004B678 / siblings):
//   stw r3, 0x48(r4)
//   li  r0, 0
//   stb r0, 0x50(r4)
//   blr
inline void TypedDecodeComplete(TOneFileAsyncLayout32* self, void* typed_result) {
    self->typed_decode_result = typed_result;
    self->typed_decode_pending = 0;
}

} // namespace original_shadow_re
