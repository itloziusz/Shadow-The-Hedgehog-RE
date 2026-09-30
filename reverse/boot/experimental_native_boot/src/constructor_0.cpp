#include "constructor_0.h"
#include <cctype>

namespace {
GuestWord32 Fingerprint(const BootImage& image, GuestAddress32 begin, GuestAddress32 end) {
    GuestWord32 hash = 2166136261u;
    for (auto pc = begin; pc < end; pc += 4) { hash ^= image.ReadWord(pc); hash *= 16777619u; }
    return hash;
}
GuestWord32 Immediate(GuestWord32 instruction) {
    const auto low = instruction & 0xFFFFu;
    return (low & 0x8000u) ? low | 0xFFFF0000u : low;
}
GuestAddress32 ConstantAddress(const BootImage& image, GuestAddress32 lis, GuestAddress32 addi) {
    return ((image.ReadWord(lis) & 0xFFFFu) << 16) + Immediate(image.ReadWord(addi));
}
GuestAddress32 BranchTarget(const BootImage& image, GuestAddress32 pc, bool conditional = false) {
    const auto word = image.ReadWord(pc);
    GuestWord32 displacement = word & (conditional ? 0xFFFCu : 0x03FFFFFCu);
    if (displacement & (conditional ? 0x8000u : 0x02000000u))
        displacement |= conditional ? 0xFFFF0000u : 0xFC000000u;
    return (word & 2u) ? displacement : pc + displacement;
}
void PinBodies(const BootImage& image) {
    if (Fingerprint(image, 0x803A2520u, 0x803A255Cu) != 0x6683F477u ||
        Fingerprint(image, 0x803A3954u, 0x803A3988u) != 0x23355F7Du ||
        Fingerprint(image, 0x803796ACu, 0x80379720u) != 0xE5245B04u)
        throw BootError("constructor/registration/walker instructions changed");
}
void Event(std::vector<ConstructorEvent>& events, std::string_view kind,
           GuestAddress32 pc, GuestAddress32 address, GuestWord32 value) {
    events.push_back({kind, pc, address, value});
}
GuestWord32 CrtZeroWord(const BootImage& image, const CrtSemantics& crt, GuestAddress32 address) {
    if (crt.pc != 0x80003170u || crt.zero_ranges != 3 || crt.identity_copies != 10 ||
        crt.copies_executed || crt.guest_image_zeroed)
        throw BootError("constructor initialized before verified CRT");
    for (unsigned i = 0; i < 3; ++i) {
        const auto base = image.ReadWord(NativeBootManifest::zero_table.raw + i * 8);
        const auto size = image.ReadWord(NativeBootManifest::zero_table.raw + i * 8 + 4);
        if (address >= base && HostByteCount64{address} + 4 <= HostByteCount64{base} + size) return 0;
    }
    throw BootError("constructor slot has no CRT zero provenance");
}
bool Matches(const FragmentState& state, const FragmentObservation& observed) {
    return state.registration_id == observed.id && state.descriptor.raw == observed.slot[0] &&
           state.sda2.raw == observed.slot[1] && state.occupied == observed.slot[2];
}
} // namespace

void RunConstructor0Body(const BootImage& image, FragmentState& state,
                        GuestWord32 sda2, GuestWord32 stack, GuestWord32 return_pc,
                        std::vector<ConstructorEvent>& events) {
    PinBodies(image);
    const auto local_stack = stack + Immediate(image.ReadWord(0x803A2520u));
    // Native automatic values replace the two PPC stack spills. No unknown
    // caller register or stack word is fabricated.
    Event(events, "stack_write", 0x803A2520u, local_stack, stack);
    const GuestWord32 saved_return = return_pc;
    Event(events, "stack_write", 0x803A2528u, local_stack + 0x14u, saved_return);
    const auto id_address = NativeBootManifest::sdata_base + Immediate(image.ReadWord(0x803A252Cu));
    Event(events, "read", 0x803A252Cu, id_address, state.registration_id);
    if (state.registration_id == Immediate(image.ReadWord(0x803A2530u))) {
        const auto descriptor = ConstantAddress(image, 0x803A2538u, 0x803A2540u);
        const auto callee = BranchTarget(image, 0x803A2544u);
        Event(events, "call", 0x803A2544u, callee, descriptor);
        Event(events, "argument_r4", 0x803A2544u, 0, sda2);
        const auto slot_address = ConstantAddress(image, 0x803A3954u, 0x803A3958u);
        Event(events, "read", 0x803A395Cu, slot_address + 8, state.occupied);
        GuestWord32 result;
        if (state.occupied == 0) {
            state.descriptor = GuestPtr32{descriptor};
            Event(events, "write", 0x803A3968u, slot_address, state.descriptor.raw);
            state.sda2 = GuestPtr32{sda2};
            Event(events, "write", 0x803A3974u, slot_address + 4, state.sda2.raw);
            state.occupied = Immediate(image.ReadWord(0x803A396Cu));
            Event(events, "write", 0x803A3978u, slot_address + 8, state.occupied);
            result = Immediate(image.ReadWord(0x803A3970u));
            Event(events, "return", 0x803A397Cu, 0x803A2548u, result);
        } else {
            result = Immediate(image.ReadWord(0x803A3980u));
            Event(events, "return", 0x803A3984u, 0x803A2548u, result);
        }
        state.registration_id = result;
        Event(events, "write", 0x803A2548u, id_address, result);
    }
    Event(events, "stack_read", 0x803A254Cu, local_stack + 0x14u, saved_return);
    Event(events, "return", 0x803A2558u, saved_return, 0);
}

Constructor0Result ExecuteFirstConstructor(const BootImage& image, const CrtSemantics& crt,
    const HardwareSemantics& hardware, const RuntimeRoute& route, const PreEntryOracle& pre_entry,
    const Constructor0Oracle& oracle) {
    if (!oracle.present) throw BootError("constructor input preservation is unknown without the oracle");
    if (!pre_entry.present || !pre_entry.has_lowmem) throw BootError("constructor lacks original startup inputs");
    const std::array<std::string, 9> inputs{
        pre_entry.memory_0x805f1f30, pre_entry.memory_0x805f1f38,
        pre_entry.lowmem_0x80000030, pre_entry.lowmem_0x80000034, pre_entry.lowmem_0x80000044,
        pre_entry.lowmem_0x800000f4, pre_entry.lowmem_0x800030e4, pre_entry.lowmem_0x800030e6, pre_entry.bi2_bytes};
    const auto normalized = [](std::string text) {
        for (auto& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return text;
    };
    for (std::size_t i = 0; i < inputs.size(); ++i)
        if (normalized(inputs[i]) != normalized(oracle.startup_inputs[i]))
            throw BootError("constructor oracle belongs to different startup inputs");
    if (route.pc != 0x8000329Cu || route.constructors_executed || route.guest_ram_allocated ||
        route.constructor_count != 282 || hardware.r2 != NativeBootManifest::sdata2_base ||
        hardware.r13 != NativeBootManifest::sdata_base)
        throw BootError("constructor did not receive the verified M6 route");
    image.RequirePalFixtureDigest();
    PinBodies(image);
    const auto id_address = hardware.r13 + Immediate(image.ReadWord(0x803A252Cu));
    const auto slot = ConstantAddress(image, 0x803A3954u, 0x803A3958u);
    // Derive state from the loaded data initializer and executed CRT semantics.
    // No observation below supplies values to this record.
    FragmentState state{image.ReadWord(id_address), GuestPtr32{CrtZeroWord(image, crt, slot)},
        GuestPtr32{CrtZeroWord(image, crt, slot + 4)}, CrtZeroWord(image, crt, slot + 8)};
    if (state.registration_id != oracle.boundaries[0].id)
        throw BootError("DOL initializer differs from the entry observation");
    for (std::size_t i = 1; i < oracle.boundaries.size(); ++i) {
        const auto& observed = oracle.boundaries[i];
        if (!Matches(state, observed) || hardware.r2 != observed.r2 || hardware.r13 != observed.r13)
            throw BootError("constructor input projection not preserved across original startup");
    }

    // Native translation of the pinned walker, bounded at the NEXT dispatch.
    // Control addresses are derived from original calls/branches and return links.
    const auto wrapper = BranchTarget(image, route.pc);
    const auto loop = BranchTarget(image, wrapper + 12);
    const auto cursor_start = ConstantAddress(image, loop + 16, loop + 20);
    const auto first = image.ReadWord(cursor_start);
    if (first != route.first_constructor || first != 0x803A2520u || cursor_start != route.constructor_table)
        throw BootError("constructor order changed");
    const auto dispatch = BranchTarget(image, loop + 60, true);
    const auto call_pc = dispatch + 4;
    const auto return_pc = call_pc + 4;
    const auto stack = crt.r1 + Immediate(image.ReadWord(wrapper + 8)) + Immediate(image.ReadWord(loop + 8));
    Constructor0Result result{state, 0, cursor_start, first, stack, first, 0, {}};
    Event(result.events, "table_read", loop + 52, cursor_start, first);
    Event(result.events, "call", call_pc, first, 0);
    RunConstructor0Body(image, result.fragment, hardware.r2, stack, return_pc, result.events);
    ++result.constructors_executed;
    result.cursor += Immediate(image.ReadWord(return_pc));
    result.next_constructor = image.ReadWord(result.cursor);
    Event(result.events, "table_read", return_pc + 4, result.cursor, result.next_constructor);
    if (result.next_constructor == 0) throw BootError("constructor table ended before index 1");
    result.pc = BranchTarget(image, return_pc + 12, true) + 4; // after mtlr, before blrl
    result.lr = result.next_constructor;
    if (!Matches(result.fragment, oracle.final_state) || result.pc != oracle.final_pc ||
        result.cursor != oracle.final_cursor || result.next_constructor != oracle.final_next ||
        result.r1 != oracle.final_r1 || result.lr != oracle.final_lr)
        throw BootError("constructor result diverged from the instruction-stepped oracle");
    return result;
}
