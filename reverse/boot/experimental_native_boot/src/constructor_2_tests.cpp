#include "constructor_2.h"
#include "startup_clock.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

namespace {
void Require(bool condition, const char* message) { if (!condition) throw BootError(message); }
void Reject(const auto& action, const char* message) {
    bool rejected = false;
    try { action(); } catch (const BootError&) { rejected = true; }
    Require(rejected,message);
}
std::size_t Offset(GuestAddress32 address) {
    for (const auto& s : NativeBootManifest::sections)
        if (address >= s.address && address < s.address+s.size) return s.file_offset+address-s.address;
    throw BootError("test address outside fixture");
}
}

int main(int argc, char** argv) {
    try {
        Require(argc == 6,"usage: Constructor2ContractTests DOL startup C0 C1 C2");
        // Earlier fixed-capacity telemetry failed on C2's thirteenth mark.
        // Preserve every mark rather than truncating or just raising the limit.
        StartupClock clock;
        for (unsigned i = 0; i < 40; ++i) clock.Mark(i == 39 ? "stop" : "phase");
        std::ostringstream timing;
        auto* original = std::cout.rdbuf(timing.rdbuf());
        clock.Report();
        std::cout.rdbuf(original);
        const auto report = timing.str();
        Require(std::count(report.begin(),report.end(),'\n') == 40 &&
                report.find("TIMING stop ") != std::string::npos,"startup timing dropped marks after capacity");
        std::ifstream file(argv[1],std::ios::binary);
        const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file),{}};
        const auto image = LoadValidatedFixture(bytes);
        const auto pre = LoadPreEntryOracle(argv[2]);
        const auto hardware = InitializeNativeHardwareSemantics(pre);
        const auto crt = InitializeNativeCrt(image,hardware);
        const auto route = ResolveRuntimeRoute(image,ResolveLowMemoryPrelude(image,crt,pre));
        const auto first = ExecuteFirstConstructor(image,crt,hardware,route,pre,LoadConstructor0Oracle(argv[3]));
        const auto second = ExecuteSecondConstructor(image,hardware,pre,first,LoadConstructor1Oracle(argv[4]));
        const auto proof = LoadConstructor2Oracle(argv[5]);
        const auto run = [&](const Constructor2Oracle& p) { return ExecuteThirdConstructor(image,crt,pre,second,p); };
        const auto result = run(proof);
        // CR0.SO may differ from XER.SO: cmplwi must copy the latter.
        for (const auto cr : {0x01234567u,0xF1234567u}) {
            for (const auto xer : {0u,0x80000000u}) {
                auto changed=proof;
                changed.fields["entry_cr"]=Constructor1Hex(cr,4);
                changed.fields["entry_xer"]=changed.fields["final_xer"]=Constructor1Hex(xer,4);
                changed.fields["final_cr"]=Constructor1Hex(0x41234567u|((xer>>31)<<28),4);
                run(changed);
            }
        }
        Require(ConstructorWalkerCompareCr(0xF1234567u,0,0)==0x21234567u &&
                ConstructorWalkerCompareCr(0x01234567u,0x80000000u,0)==0x31234567u,
                "walker compare did not replace CR0/SO for zero");
        Require(result.constructors_executed == 3 && result.pc == 0x803796F8u &&
                result.cursor == 0x804AAC6Cu && result.next_constructor == 0x8002218Cu,
                "wrong constructor 3 boundary");
        Require(result.source == NativeVector3{0,0,0} && result.body.vectors[0] == result.source &&
                result.body.vectors[1] == result.source,"actual CRT vector mismatch");
        Require(result.preserved.vectors == second.vectors && result.preserved.angles == second.angles &&
                result.preserved.fpu_owner == second.fpu_owner && result.preserved.context_flags == second.context_flags &&
                result.preserved.fpscr == second.fpscr && result.preserved.f31_primary == second.f31_primary &&
                result.preserved.f31_secondary_source == second.f31_secondary_source,
                "constructor 2 damaged prior typed effects");
        std::vector<GuestAddress32> writes;
        for (const auto& e : result.body.events) {
            Require(!(e.kind == "call" && e.address == 0x8002218Cu),"constructor 3 called");
            if (e.kind == "write" && e.address >= 0x80571C10u && e.address < 0x80571C28u) writes.push_back(e.address);
        }
        Require(writes == std::vector<GuestAddress32>{0x80571C10u,0x80571C14u,0x80571C18u,
                0x80571C1Cu,0x80571C20u,0x80571C24u},"vector writes lost or reordered");
        Reject([&] { run({}); },"missing proof accepted");
        for (const auto key : {"after_crt_source","before_walker_source","constructor_2_entry_source","entry_source",
            "final_source","entry_fpscr","entry_f0","entry_f1","entry_owner","entry_fragment_slot",
            "entry_prior_vectors","entry_prior_angles","entry_pc","entry_r1","entry_r2","entry_r13","entry_r31",
            "final_destination","final_pc","final_r31","final_r12","final_f0","final_f1","final_f2","final_f31",
            "final_r30","final_fpscr","final_context","final_owner","final_msr"}) {
            auto changed = proof;
            auto& field = changed.fields.at(key); field.back() = field.back() == '0' ? '1' : '0';
            Reject([&] { run(changed); },"changed dependency/output accepted");
        }
        for (std::size_t i = 0; i < proof.startup_inputs.size(); ++i) {
            auto changed = proof; changed.startup_inputs[i] += '0';
            Reject([&] { run(changed); },"different startup lineage accepted");
        }
        auto no_crt = crt; no_crt.zero_ranges = 0;
        Reject([&] { ExecuteThirdConstructor(image,no_crt,pre,second,proof); },"guessed BSS zeros without CRT");
        auto wrong = second; wrong.constructors_executed = 3;
        Reject([&] { ExecuteThirdConstructor(image,crt,pre,wrong,proof); },"duplicate constructor execution");
        wrong = second; wrong.fpu_enabled = false;
        Reject([&] { ExecuteThirdConstructor(image,crt,pre,wrong,proof); },"skipped missing lazy initialization");
        for (const auto address : {0x8001C484u,0x8001C494u,0x8001C4D0u,0x800091F8u,0x804AAC6Cu,0x800055C8u}) {
            auto changed = bytes; changed[Offset(address)] ^= 1;
            const auto modified = LoadValidatedFixture(changed);
            Reject([&] { ExecuteThirdConstructor(modified,crt,pre,second,proof); },"changed fixture accepted");
        }
        for (const NativeVector3 source : {NativeVector3{0x3F800000u,0xC0200000u,0x40400000u},
             NativeVector3{0x80000000u,1u,0x7F800001u},NativeVector3{0x7FC12345u,0xFF800000u,0x007FFFFFu}}) {
            const auto alternate = RunConstructor2Body(image,source,second.r1,second.cursor,second.pc+4,true);
            Require(alternate.vectors[0] == source && alternate.vectors[1] == source,
                    "copy replaced with final zeros or host NaN/denormal conversion");
            Require(alternate.fpr01[0] == WidenConstructor2Single(source[2]) &&
                    alternate.fpr01[1] == WidenConstructor2Single(source[1]),"wrong surviving FPR assignment");
        }
        Require(WidenConstructor2Single(0x3F800000u) == 0x3FF0000000000000ULL &&
                WidenConstructor2Single(0x80000000u) == 0x8000000000000000ULL &&
                WidenConstructor2Single(1u) == 0x36A0000000000000ULL &&
                WidenConstructor2Single(0x7F800001u) == 0x7FF0000020000000ULL,
                "lfs bit widening changed sign, denormal, or signaling NaN");
        Reject([&] { RunConstructor2Body(image,{1,2,3},second.r1,second.cursor,second.pc+4,false); },
               "body forced FP availability");
        std::ifstream raw_file(argv[5]);
        const std::string raw{std::istreambuf_iterator<char>(raw_file),{}};
        Reject([&] { ParseConstructor2Oracle(raw.substr(0,raw.find_last_of('}'))); },"truncated oracle accepted");
        auto crossed = raw;
        const auto pos = crossed.find("\"constructor_3_executed\": false");
        Require(pos != std::string::npos,"missing oracle stop flag");
        crossed.replace(pos,31,"\"constructor_3_executed\": true");
        Reject([&] { ParseConstructor2Oracle(crossed); },"crossed boundary evidence accepted");
        std::cout << "PASS constructor2: CRT provenance, ordering, preserved state, true bit copies, FP prerequisite\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
