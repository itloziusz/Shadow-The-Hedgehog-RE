#include "constructor_1.h"
#include <bit>
#include <cfenv>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace {
using W = GuestWord32;
using A = GuestAddress32;
W Imm(W instruction) {
    const auto low = instruction & 0xFFFFu;
    return low & 0x8000u ? low | 0xFFFF0000u : low;
}
A Address(const BootImage& image, A lis, A addi) {
    return ((image.ReadWord(lis) & 0xFFFFu) << 16) + Imm(image.ReadWord(addi));
}
A Target(const BootImage& image, A pc, bool conditional = false) {
    W d = image.ReadWord(pc) & (conditional ? 0xFFFCu : 0x03FFFFFCu);
    if (d & (conditional ? 0x8000u : 0x02000000u)) d |= conditional ? 0xFFFF0000u : 0xFC000000u;
    return pc + d;
}
void Require(bool value, const char* message) { if (!value) throw BootError(message); }
void Event(Constructor1Result& r, std::string_view kind, A pc, A address, unsigned size,
           std::uint64_t value) {
    r.events.push_back({kind, pc, address, size, Constructor1Hex(value, size ? size : 4)});
}
void Token(Constructor1Result& r, std::string_view kind, A pc, A address, std::string_view token) {
    r.events.push_back({kind, pc, address, 4, std::string(token)});
}
// Normal finite binary32 operands only: precisely the domain of this constructor.
float Float(W bits) {
    const auto value = std::bit_cast<float>(bits);
    Require(std::isnormal(value) || value == 0.0f, "unproven constructor float domain");
    return value;
}
W Single(double value, W& fpscr, bool multiply) {
    Require(std::fegetround() == FE_TONEAREST && (fpscr & 3u) == 0,
            "constructor requires proven round-to-nearest mode");
    const float rounded = static_cast<float>(value);
    Require(std::isnormal(rounded) || rounded == 0.0f, "unproven constructor arithmetic range");
    const W classification = rounded == 0.0f ? (std::signbit(rounded) ? 0x12u : 2u) :
                             (rounded < 0.0f ? 8u : 4u);
    fpscr = (fpscr & ~0x1F000u) | (classification << 12);
    // Dolphin's finite fmuls path clears FI/FR; it does not model sticky XX
    // for these operations. This status is explicitly oracle-scoped, not silicon proof.
    if (multiply) fpscr &= ~0x60000u;
    return std::bit_cast<W>(rounded);
}
std::uint64_t Widen(W bits) { return std::bit_cast<std::uint64_t>(static_cast<double>(Float(bits))); }
W PairedStore(std::uint64_t bits) {
    // GQR0 type=float: bit truncation, with small values flushed, NOT an RN cast.
    const W exponent = static_cast<W>((bits >> 52) & 0x7FFu);
    const W sign = static_cast<W>(bits >> 32) & 0x80000000u;
    if (exponent <= 896u) return sign;
    return sign | (static_cast<W>(bits >> 32) & 0x40000000u) |
           (static_cast<W>(bits >> 29) & 0x3FFFFFFFu);
}
W PairedRoundTrip(W bits) {
    return (bits & 0x7F800000u) == 0 ? bits & 0x80000000u : bits;
}

void LazyFpu(Constructor1Result& r, const Constructor1Inputs& in) {
    if (in.fpu_enabled) return;
    Require(in.recoverable_exception, "non-recoverable FPU exception is not proven");
    const A physical = in.physical_context, context = in.current_context;
    Require(physical == (context & 0x3FFFFFFFu), "context physical/cached aliases disagree");
    r.exception_save = Constructor1Result::ExceptionSave{0, in.sda2, 0x805A61F0u,
                                                        0x40000000u, in.return_pc, 0x8000AB00u};
    // This is a semantic translation of the copied exception stub and the
    // first-use branch of __OSSwitchFPUContext, not an MSR/register emulator.
    Event(r, "trap", 0x8000AB00u, 0x800u, 0, 0x8000AB00u);
    Event(r, "read", 0x804u, 0xC0u, 4, physical);
    Event(r, "write", 0x808u, physical + 0xCu, 4, 0); // constructor 0 returns registration ID 0
    Event(r, "write", 0x810u, physical + 0x10u, 4, in.sda2);
    Event(r, "write", 0x814u, physical + 0x14u, 4, 0x805A61F0u);
    W flags = in.context_flags;
    Event(r, "read", 0x818u, physical + 0x1A2u, 2, flags);
    flags |= 2u;
    Event(r, "write", 0x820u, physical + 0x1A2u, 2, flags);
    Event(r, "write", 0x828u, physical + 0x80u, 4, 0x40000000u); // unsigned table entry > 0
    Event(r, "write", 0x830u, physical + 0x84u, 4, in.return_pc);
    Token(r, "write", 0x838u, physical + 0x88u, "ENTRY_CTR");
    Token(r, "write", 0x840u, physical + 0x8Cu, "ENTRY_XER");
    Event(r, "write", 0x848u, physical + 0x198u, 4, 0x8000AB00u);
    Token(r, "write", 0x850u, physical + 0x19Cu, "ENTRY_MSR");
    Event(r, "read", 0x86Cu, 0xD4u, 4, context);
    Event(r, "branch", 0x874u, 0x888u, 0, 1);
    Event(r, "read", 0x88Cu, 0x301Cu, 4, in.fpu_handler);
    Require(in.fpu_handler == 0x80373100u, "unproven FPU exception handler");
    Event(r, "return", 0x894u, in.fpu_handler, 0, 0);
    Token(r, "read", 0x80373110u, context + 0x19Cu, "ENTRY_MSR");
    Event(r, "read", 0x80373120u, 0x800000D8u, 4, in.fpu_owner);
    r.fpu_owner = context;
    Event(r, "write", 0x80373124u, 0x800000D8u, 4, r.fpu_owner);
    const bool already_owned = in.fpu_owner == context;
    Event(r, "branch", 0x8037312Cu, already_owned ? 0x80373140u : 0x80373130u, 0, already_owned);
    if (!already_owned) {
        Event(r, "branch", 0x80373134u, in.fpu_owner == 0 ? 0x8037313Cu : 0x80373138u,
              0, in.fpu_owner == 0);
        Require(in.fpu_owner == 0, "previous FPU owner requires an unproven context save");
        Event(r, "call", 0x8037313Cu, 0x8037292Cu, 0, 0x80373140u);
        Event(r, "read", 0x8037292Cu, context + 0x1A2u, 2, flags);
        const bool fresh = (flags & 1u) == 0;
        Event(r, "branch", 0x80372934u, fresh ? 0x80372A4Cu : 0x80372938u, 0, fresh);
        Require(fresh, "saved FPU image is not proven; cannot zero or skip it");
        Event(r, "return", 0x80372A4Cu, 0x80373140u, 0, 0);
    }
    Event(r, "read", 0x80373140u, context + 0x80u, 4, 0x40000000u);
    Event(r, "read", 0x80373148u, context + 0x84u, 4, in.return_pc);
    Event(r, "read", 0x80373150u, context + 0x198u, 4, 0x8000AB00u);
    Token(r, "read", 0x80373158u, context + 0x88u, "ENTRY_CTR");
    Token(r, "read", 0x80373160u, context + 0x8Cu, "ENTRY_XER");
    Event(r, "read", 0x80373168u, context + 0x1A2u, 2, flags);
    flags &= ~2u;
    Event(r, "write", 0x80373170u, context + 0x1A2u, 2, flags);
    Event(r, "read", 0x80373174u, context + 0x14u, 4, 0x805A61F0u);
    Event(r, "read", 0x80373178u, context + 0xCu, 4, 0);
    Event(r, "read", 0x8037317Cu, context + 0x10u, 4, in.sda2);
    Event(r, "return", 0x80373180u, 0x8000AB00u, 0, 0);
    r.context_flags = flags;
    r.fpu_enabled = true;
}
} // namespace

std::string Constructor1Hex(std::uint64_t value, unsigned size) {
    std::ostringstream out;
    out << std::hex << std::setfill('0') << std::setw(static_cast<int>(size * 2)) << value;
    return out.str();
}
W Constructor1Radians(W degrees, W pi, W half_turn, W& fpscr) {
    Require(Float(half_turn) != 0.0f, "zero radians denominator");
    const W scale = Single(static_cast<double>(Float(pi)) / Float(half_turn), fpscr, false);
    return Single(static_cast<double>(Float(degrees)) * Float(scale), fpscr, true);
}

Constructor1Result RunConstructor1Body(const BootImage& image, const Constructor1Inputs& in) {
    image.RequirePalFixtureDigest();
    Constructor1Result r;
    r.fpscr = in.fpscr; r.context = in.current_context; r.context_flags = in.context_flags;
    r.fpu_owner = in.fpu_owner; r.fpu_enabled = in.fpu_enabled;
    const A local = in.stack + Imm(image.ReadWord(0x8000AAF4u));
    Event(r, "write", 0x8000AAF4u, local, 4, in.stack);
    Event(r, "write", 0x8000AAFCu, local + 0x24u, 4, in.return_pc);
    LazyFpu(r, in);
    Event(r, "write", 0x8000AB00u, local + 0x10u, 8, in.f31_primary);
    r.paired_spill = (std::uint64_t{PairedStore(in.f31_primary)} << 32) |
                     PairedRoundTrip(in.f31_secondary_source);
    Event(r, "write", 0x8000AB04u, local + 0x18u, 8, r.paired_spill);
    Event(r, "write", 0x8000AB08u, local + 0xCu, 4, in.cursor);
    r.vector_address = Address(image, 0x8000AB0Cu, 0x8000AB14u);
    r.angle_address = in.sda + Imm(image.ReadWord(0x8000AB5Cu));
    const auto load = [&](A pc) {
        const A address = in.sda2 + Imm(image.ReadWord(pc));
        const W bits = image.ReadWord(address);
        Event(r, "read", pc, address, 4, bits);
        return bits;
    };
    const auto vector_store = [&](A pc, std::size_t index, W bits) {
        r.vectors[index] = bits;
        Event(r, "write", pc, r.vector_address + static_cast<W>(index) * 4, 4, bits);
    };
    const W zero = load(0x8000AB10u), twenty = load(0x8000AB18u);
    vector_store(0x8000AB20u, 0, zero);
    const W angle0 = load(0x8000AB28u);
    vector_store(0x8000AB2Cu, 1, twenty); vector_store(0x8000AB30u, 2, twenty);
    vector_store(0x8000AB34u, 3, zero); vector_store(0x8000AB38u, 4, zero);
    vector_store(0x8000AB3Cu, 5, zero);
    const auto radians = [&](A call, W degrees) {
        const A helper = Target(image, call);
        Require(helper == 0x8000AB90u, "unknown conversion helper");
        Event(r, "call", call, helper, 0, call + 4);
        const W pi = load(helper), half_turn = load(helper + 4);
        Require(Float(half_turn) != 0.0f, "zero radians denominator");
        const W scale = Single(static_cast<double>(Float(pi)) / Float(half_turn), r.fpscr, false);
        Event(r, "fp", helper + 8, 0, 4, scale);
        Event(r, "fpscr", helper + 8, 0, 4, r.fpscr);
        const W bits = Single(static_cast<double>(Float(degrees)) * Float(scale), r.fpscr, true);
        Event(r, "fp", helper + 12, 1, 4, bits);
        Event(r, "fpscr", helper + 12, 0, 4, r.fpscr);
        Event(r, "return", helper + 16, call + 4, 0, 0);
        return bits;
    };
    const W first_radians = radians(0x8000AB40u, angle0);
    const W angle1 = load(0x8000AB48u);
    const W second_radians = radians(0x8000AB4Cu, angle1);
    const W zero_again = load(0x8000AB50u), one = load(0x8000AB58u);
    r.angles = {second_radians, first_radians};
    Event(r, "write", 0x8000AB60u, r.angle_address, 4, r.angles[0]);
    Event(r, "write", 0x8000AB64u, r.angle_address + 4, 4, r.angles[1]);
    vector_store(0x8000AB68u, 6, zero_again); vector_store(0x8000AB6Cu, 7, one);
    vector_store(0x8000AB70u, 8, zero_again);
    Event(r, "read", 0x8000AB74u, local + 0x18u, 8, r.paired_spill);
    Event(r, "read", 0x8000AB78u, local + 0x24u, 4, in.return_pc);
    Event(r, "read", 0x8000AB7Cu, local + 0x10u, 8, in.f31_primary);
    Event(r, "read", 0x8000AB80u, local + 0xCu, 4, in.cursor);
    r.f31_primary = in.f31_primary;
    r.fpr_primary = {Widen(one), Widen(second_radians), Widen(zero_again)};
    r.f31_secondary_source = static_cast<W>(r.paired_spill);
    r.r1 = local + Imm(image.ReadWord(0x8000AB88u));
    r.lr = in.return_pc;
    Event(r, "return", 0x8000AB8Cu, r.lr, 0, 0);
    r.pc = r.lr; r.cursor = in.cursor; r.next_constructor = 0; r.constructors_executed = 2;
    return r;
}

Constructor1Result ExecuteSecondConstructor(const BootImage& image, const HardwareSemantics& hw,
    const PreEntryOracle& pre, const Constructor0Result& previous, const Constructor1Oracle& oracle) {
    Require(oracle.present, "constructor 1 needs its dependency proof");
    image.RequirePalFixtureDigest();
    const std::array<std::string, 9> inputs{pre.memory_0x805f1f30, pre.memory_0x805f1f38,
        pre.lowmem_0x80000030, pre.lowmem_0x80000034, pre.lowmem_0x80000044, pre.lowmem_0x800000f4,
        pre.lowmem_0x800030e4, pre.lowmem_0x800030e6, pre.bi2_bytes};
    Require(pre.present && oracle.startup_inputs == inputs, "constructor 1 startup provenance differs");
    Require(previous.constructors_executed == 1 && previous.pc == 0x803796F8u &&
            previous.cursor == 0x804AAC64u && previous.next_constructor == image.ReadWord(previous.cursor) &&
            previous.next_constructor == 0x8000AAF4u && previous.lr == previous.next_constructor,
            "constructor 1 called out of original order");
    Require(previous.fragment.registration_id == 0 && previous.fragment.occupied == 1 &&
            previous.fragment.descriptor.raw == 0x80008D14u && previous.fragment.sda2.raw == hw.r2,
            "constructor 0 fragment effects missing");
    Require(hw.paired_temporary_survives, "startup lost PS1");
    // Exact producers, not captured values: __OSThreadInit's embedded context;
    // OSClearContext sets its flags to zero and clears the owner it just selected;
    // OSSetCurrentContext installs cached/physical aliases, clears FP, sets RI.
    const A context = Address(image, 0x80378328u, 0x80378350u) + Imm(image.ReadWord(0x80378354u));
    const W context_flags = Imm(image.ReadWord(0x80372D78u));
    const W owner_after_clear = context_flags;
    const A handler = Address(image, 0x80373190u, 0x80373194u);
    const W fpscr = hw.fpscr | (1u << (31u - ((image.ReadWord(0x80370BE8u) >> 21) & 31u)));
    const auto observed = [&](const std::string& name) -> const std::string& {
        const auto found = oracle.fields.find(name);
        if (found == oracle.fields.end()) throw BootError("missing constructor 1 observation: " + name);
        return found->second;
    };
    const auto compare = [&](const std::string& name, std::uint64_t bits, unsigned size = 4) {
        if (observed(name) != Constructor1Hex(bits, size)) throw BootError("constructor 1 mismatch: " + name);
    };
    const auto compare_piece = [&](const std::string& name, unsigned offset, unsigned size, std::uint64_t bits) {
        if (observed(name).substr(offset * 2, size * 2) != Constructor1Hex(bits, size))
            throw BootError("constructor 1 context mismatch: " + name);
    };
    const A return_pc = previous.pc + 4;
    compare("entry_pc", previous.next_constructor); compare("entry_r1", previous.r1);
    compare("entry_r2", hw.r2); compare("entry_r13", hw.r13); compare("entry_r31", previous.cursor);
    compare("entry_lr", return_pc); compare("entry_cr", 0x40000000u); compare("entry_fpscr", fpscr);
    compare("entry_r3", previous.fragment.registration_id); compare("entry_r4", hw.r2);
    compare("entry_r5", 0x805A61F0u);
    compare("entry_f31", hw.fpr_binary64, 8); compare("entry_context_address", context);
    compare("entry_fpu_handler", handler);
    compare_piece("entry_low_context", 0, 4, context & 0x3FFFFFFFu);
    compare_piece("entry_low_context", 20, 4, context);
    compare_piece("entry_low_context", 24, 4, owner_after_clear);
    compare_piece("entry_context", 0x1A2, 2, context_flags);
    const auto msr = static_cast<W>(std::stoul(observed("entry_msr"), nullptr, 16));
    Require((msr & 0x2032u) == 0x32u, "OSSetCurrentContext FP/RI/translation proof mismatch");
    // Every read-only constant is compared to the DOL, never installed from the oracle.
    for (const A a : {0x805F27ACu,0x805F27B8u,0x805F27BCu,0x805F27C0u,0x805F27C4u,0x805F27C8u,0x805FBDB0u}) {
        auto name = Constructor1Hex(a, 4);
        for (auto& c : name) if (c >= 'a' && c <= 'f') c -= 'a' - 'A';
        compare("constant_0x" + name, image.ReadWord(a));
    }
    const Constructor1Inputs body{previous.r1, previous.cursor, return_pc, hw.r2, hw.r13,
        hw.fpr_binary64, hw.paired_lane1_binary32, fpscr, context, context & 0x3FFFFFFFu,
        owner_after_clear, handler, context_flags, false, true};
    auto r = RunConstructor1Body(image, body);
    r.fragment = previous.fragment;
    r.cursor += Imm(image.ReadWord(return_pc));
    r.next_constructor = image.ReadWord(r.cursor);
    Event(r, "read", return_pc + 4, r.cursor, 4, r.next_constructor);
    const bool more = r.next_constructor != 0;
    const A dispatch = more ? Target(image, return_pc + 12, true) : return_pc + 16;
    Event(r, "branch", return_pc + 12, dispatch, 0, more);
    Require(more, "constructor table unexpectedly terminates");
    r.lr = r.next_constructor; r.pc = dispatch + 4;
    r.constructors_executed = previous.constructors_executed + 1;
    std::string vectors, angles;
    for (const auto v : r.vectors) vectors += Constructor1Hex(v, 4);
    for (const auto v : r.angles) angles += Constructor1Hex(v, 4);
    Require(observed("final_vectors") == vectors && observed("final_angles") == angles,
            "constructor 1 native float outputs differ");
    compare("paired_spill", r.paired_spill, 8); compare("final_f31", r.f31_primary, 8);
    compare("final_f0", Widen(r.vectors[7]), 8); compare("final_f1", Widen(r.angles[0]), 8);
    compare("final_f2", Widen(r.vectors[8]), 8); compare("final_fpscr", r.fpscr);
    compare("final_pc", r.pc); compare("final_r31", r.cursor); compare("final_r12", r.next_constructor);
    compare("final_r1", r.r1); compare("final_lr", r.lr); compare("final_r0", return_pc);
    compare("final_r2", hw.r2); compare("final_r13", hw.r13); compare("final_r3", r.vector_address + 24);
    compare("final_r4", r.angle_address); compare("final_r5", 0x805A61F0u); compare("final_cr", 0x40000000u);
    compare("final_fragment_id", r.fragment->registration_id);
    const auto slot = Constructor1Hex(r.fragment->descriptor.raw, 4) + Constructor1Hex(r.fragment->sda2.raw, 4) +
                      Constructor1Hex(r.fragment->occupied, 4);
    Require(observed("final_fragment_slot") == slot && observed("entry_fragment_slot") == slot,
            "constructor 1 changed fragment state");
    compare("entry_fragment_id", r.fragment->registration_id);
    compare_piece("final_low_context", 24, 4, r.fpu_owner);
    compare_piece("final_context", 0x1A2, 2, r.context_flags);
    for (const auto& e : r.events) {
        if (e.kind != "write" || e.address < 0x00587450u || e.address >= 0x00587718u ||
            e.address == (context & 0x3FFFFFFFu) + 0x1A2u) continue;
        const auto expected = e.value == "ENTRY_CTR" ? observed("entry_ctr") :
                              e.value == "ENTRY_XER" ? observed("fault_xer") :
                              e.value == "ENTRY_MSR" ? observed("entry_msr") : e.value;
        const auto actual = observed("final_context").substr((e.address - (context & 0x3FFFFFFFu)) * 2, e.size * 2);
        if (actual != expected) throw BootError("lazy-FPU context mismatch at " + Constructor1Hex(e.pc, 4) +
                                               ": native=" + expected + " observed=" + actual);
    }
    Require(observed("entry_ctr") == observed("final_ctr") && observed("fault_xer") == observed("final_xer"),
            "lazy-FPU pass-through state changed");
    compare("final_msr", msr | 0x2000u); // comparison only; the native state is fpu_enabled.
    Require(r.fpu_enabled && r.constructors_executed == 2 && r.pc == 0x803796F8u,
            "constructor 1 completion boundary differs");
    return r;
}
