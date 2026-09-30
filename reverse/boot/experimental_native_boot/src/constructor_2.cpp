#include "constructor_2.h"
#include <algorithm>

namespace {
using W = GuestWord32;
using A = GuestAddress32;
void Require(bool condition, const char* message) { if (!condition) throw BootError(message); }
W Imm(W word) { const W low = word & 0xFFFFu; return low & 0x8000u ? low | 0xFFFF0000u : low; }
A Address(const BootImage& image, A lis, A addi) {
    return ((image.ReadWord(lis) & 0xFFFFu) << 16) + Imm(image.ReadWord(addi));
}
A Target(const BootImage& image, A pc, bool conditional = false) {
    W d = image.ReadWord(pc) & (conditional ? 0xFFFCu : 0x3FFFFFCu);
    if (d & (conditional ? 0x8000u : 0x2000000u)) d |= conditional ? 0xFFFF0000u : 0xFC000000u;
    return pc + d;
}
void Event(Constructor2BodyResult& r, std::string_view kind, A pc, A address, unsigned size, std::uint64_t value) {
    r.events.push_back({kind,pc,address,size,Constructor1Hex(value,size ? size : 4)});
}
std::string Words(const auto& values) {
    std::string result;
    for (auto value : values) result += Constructor1Hex(value,4);
    return result;
}
W CrtWord(const BootImage& image, const CrtSemantics& crt, A address) {
    Require(crt.pc == 0x80003170u && crt.zero_ranges == 3 && crt.identity_copies == 10 &&
            !crt.copies_executed && !crt.guest_image_zeroed, "constructor 2 source requires verified CRT");
    for (unsigned i = 0; i < 3; ++i) {
        const A start = image.ReadWord(NativeBootManifest::zero_table.raw + i * 8);
        const W size = image.ReadWord(NativeBootManifest::zero_table.raw + i * 8 + 4);
        if (address >= start && HostByteCount64{address} + 4 <= HostByteCount64{start} + size) return 0;
    }
    throw BootError("constructor 2 source has no CRT provenance");
}
}

W ConstructorWalkerCompareCr(W cr, W xer, W next) {
    // cmplwi replaces all of CR0, taking SO from XER rather than stale CR0.SO.
    return (cr & 0x0FFFFFFFu) | ((next > 0 ? 4u : 2u) << 28) | ((xer >> 31) << 28);
}

std::uint64_t WidenConstructor2Single(W bits) {
    // lfs conversion, done as integer bit construction: no host arithmetic,
    // NaN quieting, ambient rounding, or denormal flushing is introduced.
    const std::uint64_t sign = std::uint64_t{bits & 0x80000000u} << 32;
    const W exponent = (bits >> 23) & 255u;
    W fraction = bits & 0x7FFFFFu;
    if (exponent == 255u) return sign | (0x7FFULL << 52) | (std::uint64_t{fraction} << 29);
    if (exponent != 0) return sign | (std::uint64_t{exponent + 896u} << 52) | (std::uint64_t{fraction} << 29);
    if (fraction == 0) return sign;
    W normalized_exponent = 897;
    while ((fraction & 0x800000u) == 0) { fraction <<= 1; --normalized_exponent; }
    return sign | (std::uint64_t{normalized_exponent} << 52) |
           (std::uint64_t{fraction & 0x7FFFFFu} << 29);
}

Constructor2BodyResult RunConstructor2Body(const BootImage& image, const NativeVector3& source,
    W stack, W cursor, W return_pc, bool fpu_enabled) {
    image.RequirePalFixtureDigest();
    Require(fpu_enabled, "constructor 2 needs the proven constructor-1 FPU initialization");
    Constructor2BodyResult r;
    r.source_address = Address(image,0x8001C48Cu,0x8001C494u);
    r.destination_address = Address(image,0x8001C490u,0x8001C49Cu);
    const A outer = stack + Imm(image.ReadWord(0x8001C484u));
    Event(r,"write",0x8001C484u,outer,4,stack);
    Event(r,"write",0x8001C498u,outer+0x14u,4,return_pc);
    const A helper = Target(image,0x8001C4A4u), helper_return = 0x8001C4A8u;
    Require(helper == 0x8001C4B8u, "constructor 2 helper changed");
    Event(r,"call",0x8001C4A4u,helper,0,helper_return);
    const A local = outer + Imm(image.ReadWord(helper));
    Event(r,"write",helper,local,4,outer);
    Event(r,"write",0x8001C4C0u,local+0x14u,4,helper_return);
    // stmw/lmw preserve r30 without inspecting its bits. Never assume zero.
    r.events.push_back({"write",0x8001C4C4u,local+8,4,"ENTRY_R30"});
    Event(r,"write",0x8001C4C4u,local+12,4,cursor);
    const auto copy_vector = [&](A call, unsigned index, A destination) {
        const A copy = Target(image,call);
        Require(copy == 0x800091ECu, "unproven vector-copy helper");
        Event(r,"call",call,copy,0,call+4);
        const auto load = [&](A pc, unsigned component, unsigned fpr) {
            const W value = source[component];
            Event(r,"read",pc,r.source_address+component*4,4,value);
            r.fpr01[fpr] = WidenConstructor2Single(value);
            Event(r,"fpr",pc,fpr,8,r.fpr01[fpr]);
            return value;
        };
        const auto store = [&](A pc, unsigned component, W value) {
            r.vectors[index][component] = value; // stfs reverses lfs exactly; no arithmetic.
            Event(r,"write",pc,destination+component*4,4,value);
        };
        const W x = load(copy,0,0);
        const W y = load(copy+4,1,1);
        store(copy+8,0,x);
        const W z = load(copy+12,2,0);
        store(copy+16,1,y); store(copy+20,2,z);
        Event(r,"return",copy+24,call+4,0,0);
    };
    copy_vector(0x8001C4D0u,0,r.destination_address);
    copy_vector(0x8001C4DCu,1,r.destination_address+Imm(image.ReadWord(0x8001C4D8u)));
    r.returned_r3 = r.destination_address;
    r.events.push_back({"read",0x8001C4E4u,local+8,4,"ENTRY_R30"});
    Event(r,"read",0x8001C4E4u,local+12,4,cursor);
    Event(r,"read",0x8001C4E8u,local+0x14u,4,helper_return);
    const A restored_outer = local + Imm(image.ReadWord(0x8001C4F0u));
    Require(restored_outer == outer, "helper stack not restored");
    Event(r,"return",0x8001C4F4u,helper_return,0,0);
    Event(r,"read",0x8001C4A8u,restored_outer+0x14u,4,return_pc);
    r.r1 = restored_outer + Imm(image.ReadWord(0x8001C4B0u));
    r.return_pc = return_pc;
    Event(r,"return",0x8001C4B4u,return_pc,0,0);
    return r;
}

Constructor2Result ExecuteThirdConstructor(const BootImage& image, const CrtSemantics& crt,
    const PreEntryOracle& pre, const Constructor1Result& previous, const Constructor2Oracle& oracle) {
    Require(oracle.present && pre.present, "constructor 2 requires external dependency evidence");
    image.RequirePalFixtureDigest();
    const std::array<std::string,9> inputs{pre.memory_0x805f1f30,pre.memory_0x805f1f38,
        pre.lowmem_0x80000030,pre.lowmem_0x80000034,pre.lowmem_0x80000044,pre.lowmem_0x800000f4,
        pre.lowmem_0x800030e4,pre.lowmem_0x800030e6,pre.bi2_bytes};
    Require(oracle.startup_inputs == inputs, "constructor 2 startup lineage differs");
    Require(previous.constructors_executed == 2 && previous.pc == 0x803796F8u &&
            previous.cursor == 0x804AAC68u && previous.next_constructor == image.ReadWord(previous.cursor) &&
            previous.next_constructor == 0x8001C484u && previous.lr == previous.next_constructor,
            "constructor 2 executed out of order");
    Require(previous.fpu_enabled && previous.fpu_owner == previous.context && previous.exception_save &&
            previous.fragment, "constructor 1 required surviving state missing");
    const auto observed = [&](const std::string& key) -> const std::string& {
        const auto found = oracle.fields.find(key);
        if (found == oracle.fields.end()) throw BootError("missing constructor 2 field: " + key);
        return found->second;
    };
    const auto compare = [&](const std::string& key, std::uint64_t bits, unsigned size = 4) {
        if (observed(key) != Constructor1Hex(bits,size)) throw BootError("constructor 2 mismatch: " + key);
    };
    const A source_address = Address(image,0x8001C48Cu,0x8001C494u);
    NativeVector3 source;
    for (unsigned i = 0; i < 3; ++i) source[i] = CrtWord(image,crt,source_address+i*4);
    for (const std::string key : {"after_crt_source","before_walker_source","constructor_2_entry_source",
                                  "entry_source","final_source"})
        Require(observed(key) == Words(source), "constructor 2 CRT source projection not preserved");
    compare("entry_pc",previous.next_constructor); compare("entry_r1",previous.r1);
    compare("entry_r31",previous.cursor); compare("entry_lr",previous.pc+4);
    compare("entry_r2",NativeBootManifest::sdata2_base); compare("entry_r13",NativeBootManifest::sdata_base);
    for (unsigned i = 0; i < 3; ++i) compare("entry_f"+std::to_string(i),previous.fpr_primary[i],8);
    compare("entry_f31",previous.f31_primary,8); compare("entry_fpscr",previous.fpscr);
    compare("entry_owner",previous.fpu_owner);
    const W observed_msr = static_cast<W>(std::stoul(observed("entry_msr"),nullptr,16));
    Require((observed_msr & 0x2000u) != 0, "oracle entered constructor 2 without FP enabled");
    Require(observed("entry_prior_vectors") == Words(previous.vectors) &&
            observed("entry_prior_angles") == Words(previous.angles), "constructor 1 globals differ");
    compare("entry_fragment_id",previous.fragment->registration_id);
    const std::array<W,3> fragment{previous.fragment->descriptor.raw,previous.fragment->sda2.raw,previous.fragment->occupied};
    Require(observed("entry_fragment_slot") == Words(fragment), "fragment state not preserved");
    const auto& saved = *previous.exception_save;
    for (const auto [offset,value] : std::array<std::pair<unsigned,W>,7>{{{0xC,saved.r3},{0x10,saved.r4},
        {0x14,saved.r5},{0x80,saved.cr},{0x84,saved.lr},{0x198,saved.fault_pc},{0x1A2,previous.context_flags}}}) {
        const unsigned size = offset == 0x1A2 ? 2u : 4u;
        Require(observed("entry_context").substr(offset*2,size*2) == Constructor1Hex(value,size),
                "prior native exception-save state differs");
    }
    auto body = RunConstructor2Body(image,source,previous.r1,previous.cursor,previous.pc+4,previous.fpu_enabled);
    const A cursor = previous.cursor + Imm(image.ReadWord(body.return_pc));
    const A next = image.ReadWord(cursor);
    Event(body,"read",body.return_pc+4,cursor,4,next);
    const bool more = next != 0;
    const A dispatch = more ? Target(image,body.return_pc+12,true) : body.return_pc+16;
    Event(body,"branch",body.return_pc+12,dispatch,0,more);
    Require(more, "constructor table ended unexpectedly");
    Constructor2Result r{previous,source,std::move(body),dispatch+4,cursor,next,next,previous.constructors_executed+1};
    Require(observed("final_destination") == Words(r.body.vectors[0])+Words(r.body.vectors[1]),
            "constructor 2 native output mismatch");
    compare("final_pc",r.pc); compare("final_r31",r.cursor); compare("final_r12",r.next_constructor);
    compare("final_lr",r.lr); compare("final_r0",r.body.return_pc); compare("final_r1",r.body.r1);
    compare("final_r3",r.body.returned_r3); compare("final_r4",source_address); compare("final_r5",source_address);
    compare("final_f0",r.body.fpr01[0],8); compare("final_f1",r.body.fpr01[1],8);
    // Unmodified observations are compared, not numerically installed. In
    // particular r30 is an opaque save/restore value, not guessed from startup.
    for (unsigned i = 0; i < 32; ++i) {
        if (i != 0 && i != 1 && i != 3 && i != 4 && i != 5 && i != 12 && i != 31)
            Require(observed("entry_r"+std::to_string(i)) == observed("final_r"+std::to_string(i)), "unexpected GPR change");
        if (i >= 2)
            Require(observed("entry_f"+std::to_string(i)) == observed("final_f"+std::to_string(i)), "unexpected FPR change");
    }
    for (const std::string key : {"fpscr","msr","ctr","xer","context","owner","fragment_id",
                                  "fragment_slot","prior_vectors","prior_angles"})
        Require(observed("entry_"+key) == observed("final_"+key), "unexplained surviving side effect");
    const W entry_cr = static_cast<W>(std::stoul(observed("entry_cr"),nullptr,16));
    const W xer = static_cast<W>(std::stoul(observed("entry_xer"),nullptr,16));
    compare("final_cr",ConstructorWalkerCompareCr(entry_cr,xer,next));
    return r;
}
