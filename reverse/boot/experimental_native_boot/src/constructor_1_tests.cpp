#include "constructor_1.h"
#include <bit>
#include <cfenv>
#include <fstream>
#include <iostream>
#include <iterator>
#include <algorithm>

namespace {
void Require(bool ok, const char* message) { if (!ok) throw BootError(message); }
void Reject(const auto& action, const char* message) {
    bool rejected = false;
    try { action(); } catch (const BootError&) { rejected = true; }
    Require(rejected, message);
}
std::size_t Offset(GuestAddress32 address) {
    for (const auto& s : NativeBootManifest::sections)
        if (address >= s.address && address < s.address + s.size) return s.file_offset + address - s.address;
    throw BootError("test address unmapped");
}
}
int main(int argc, char** argv) {
    try {
        Require(argc == 5, "usage: Constructor1ContractTests DOL startup C0 C1");
        std::ifstream file(argv[1], std::ios::binary);
        const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file), {}};
        const auto image = LoadValidatedFixture(bytes);
        const auto pre = LoadPreEntryOracle(argv[2]);
        const auto hw = InitializeNativeHardwareSemantics(pre);
        const auto crt = InitializeNativeCrt(image, hw);
        const auto route = ResolveRuntimeRoute(image, ResolveLowMemoryPrelude(image, crt, pre));
        const auto first = ExecuteFirstConstructor(image, crt, hw, route, pre, LoadConstructor0Oracle(argv[3]));
        const auto proof = LoadConstructor1Oracle(argv[4]);
        const auto run = [&](const Constructor1Oracle& p) {
            return ExecuteSecondConstructor(image, hw, pre, first, p);
        };
        const auto r = run(proof);
        Require(r.constructors_executed == 2 && r.pc == 0x803796F8u && r.cursor == 0x804AAC68u &&
                r.next_constructor == 0x8001C484u, "constructor 2 boundary crossed");
        Require(r.angles == std::array<GuestWord32,2>{0x3F8D4D5Cu,0x3F6D6336u}, "oracle angle mismatch");
        Require(r.fpu_owner == r.context && r.fpu_enabled && r.context_flags == 0 && r.exception_save.has_value(),
                "lazy FPU surviving state lost");
        Require(r.exception_save->fault_pc == 0x8000AB00u && r.exception_save->ctr == "ENTRY_CTR" &&
                r.exception_save->xer == "ENTRY_XER" && r.exception_save->msr == "ENTRY_MSR",
                "opaque preserved context fields guessed or dropped");
        Require(std::none_of(r.events.begin(), r.events.end(), [](const auto& e) {
            return e.kind == "call" && e.address == 0x8001C484u;
        }), "constructor 2 called");
        Require(std::count_if(r.events.begin(), r.events.end(), [](const auto& e) {
            return e.kind == "write" && ((e.address >= 0x8056FEE8u && e.address < 0x8056FF0Cu) ||
                                        (e.address >= 0x805EF048u && e.address < 0x805EF050u));
        }) == 11, "missing global float store");
        Reject([&] { run({}); }, "absent proof accepted");
        for (const auto& key : {"entry_f31", "entry_context_address", "entry_fpu_handler", "entry_r1",
            "entry_r2", "entry_r13", "entry_r31", "entry_fpscr", "final_vectors", "final_angles",
            "paired_spill", "fault_xer", "final_pc", "final_r31", "final_r12", "final_f31",
            "final_fpscr", "final_f0", "final_f1", "final_f2", "constant_0x805FBDB0"}) {
            auto changed = proof;
            auto& value = changed.fields.at(key);
            value.back() = value.back() == '0' ? '1' : '0';
            Reject([&] { run(changed); }, "changed oracle dependency/effect accepted");
        }
        for (std::size_t i = 0; i < proof.startup_inputs.size(); ++i) {
            auto changed = proof; changed.startup_inputs[i] += '0';
            Reject([&] { run(changed); }, "changed startup lineage accepted");
        }
        for (const auto address : {0x8000AB98u,0x805FBDB0u,0x80370BE8u,0x80378328u,0x80373100u,0x80371738u}) {
            auto changed = bytes; changed[Offset(address)] ^= 1;
            const auto wrong = LoadValidatedFixture(changed);
            Reject([&] { ExecuteSecondConstructor(wrong, hw, pre, first, proof); }, "changed source accepted");
        }
        auto out_of_order = first; out_of_order.constructors_executed = 2;
        Reject([&] { ExecuteSecondConstructor(image, hw, pre, out_of_order, proof); }, "duplicate execution accepted");
        Constructor1Inputs input{first.r1,first.cursor,first.pc+4,hw.r2,hw.r13,
            std::bit_cast<std::uint64_t>(1.25),0x40000000u,4,0x80587450u,0x00587450u,0,0x80373100u,0,false,true};
        const auto alternate = RunConstructor1Body(image, input);
        Require(alternate.f31_primary == input.f31_primary && alternate.f31_secondary_source == 0x40000000u &&
                alternate.paired_spill == 0x3FA0000040000000ULL, "F31 lanes not independently preserved");
        input.f31_primary = 1; input.f31_secondary_source = 1;
        const auto tiny = RunConstructor1Body(image, input);
        Require(tiny.f31_primary == 1 && tiny.f31_secondary_source == 0 && tiny.paired_spill == 0,
                "paired store must flush tiny lane while double save preserves PS0");
        input.fpu_owner = 0x80587768u;
        Reject([&] { RunConstructor1Body(image, input); }, "foreign FPU context was skipped");
        input.fpu_owner = 0; input.context_flags = 1;
        Reject([&] { RunConstructor1Body(image, input); }, "saved FPU image was skipped or zeroed");
        input.context_flags = 0; input.recoverable_exception = false;
        Reject([&] { RunConstructor1Body(image, input); }, "nonrecoverable exception forced to return");
        GuestWord32 status = 4;
        Require(Constructor1Radians(std::bit_cast<GuestWord32>(90.0f),std::bit_cast<GuestWord32>(3.0f),
                std::bit_cast<GuestWord32>(180.0f),status) == 0x3FC00001u,
                "conversion must round the quotient before multiplication (not reassociate)");
        status = 4;
        Require(Constructor1Radians(std::bit_cast<GuestWord32>(-90.0f),std::bit_cast<GuestWord32>(3.0f),
                std::bit_cast<GuestWord32>(180.0f),status) == 0xBFC00001u && status == 0x8004,
                "negative conversion or classification incorrect");
        std::fesetround(FE_DOWNWARD);
        Reject([&] { Constructor1Radians(0x42B40000u,0x40400000u,0x43340000u,status); }, "host rounding ignored");
        std::fesetround(FE_TONEAREST);
        auto distinct = pre;
        distinct.memory_0x805f1f30 = "3ff4000000000000";
        distinct.memory_0x805f1f38 = "3f80000040000000";
        const auto lanes = InitializeNativeHardwareSemantics(distinct);
        Require(lanes.fpr_binary64 == 0x3FF4000000000000ULL && lanes.paired_lane1_binary32 == 0x40000000u,
                "M3 lfd/fmr incorrectly discarded or duplicated the secondary lane");
        std::cout << "PASS constructor1 contracts: lineage, effects, order, FPU branches, paired lanes, rounding\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
