#include "boot_image.h"
#include "checkpoints.h"
#include "contract_tests.h"
#include "crt_semantics.h"
#include "display_init_guard.h"
#include "golden_trace.h"
#include "hardware_frontier.h"
#include "hardware_semantics.h"
#include "low_memory_semantics.h"
#include "register_startup.h"
#include "runtime_route.h"
#include "startup_clock.h"
#include "constructor_0.h"
#include "constructor_1.h"
#include "constructor_2.h"
#include "constructor_3.h"
#include "constructor_4.h"
#include "constructor_5.h"
#include "constructor_6.h"
#include "constructor_7.h"
#include "constructor_8.h"
#include "boot_batch_generated.h"
#include "boot_region_generated.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::vector<std::uint8_t> ReadFixture(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw BootError("cannot open DOL fixture");
    }
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>{input}, {});
}

void ExpectTamperRejected(std::vector<std::uint8_t> dol) {
    if (dol.size() < 0xE4u) {
        throw BootError("fixture is too small to tamper");
    }
    dol[0xE0] = static_cast<std::uint8_t>(dol[0xE0] ^ 0x01u);
    bool rejected = false;
    try {
        (void)LoadValidatedFixture(dol);
    } catch (const BootError&) {
        rejected = true;
    }
    if (!rejected) {
        throw BootError("tampered DOL entry was accepted");
    }
}

std::string Hex8(GuestWord32 value) {
    std::ostringstream out;
    out << std::hex << std::uppercase << value;
    const std::string text = out.str();
    return std::string(8u - text.size(), '0') + text;
}

void ExpectReadPrefix(const BootImage& image, const std::vector<GuestAddress32>& expected) {
    const std::vector<GuestAddress32>& actual = image.guest_reads();
    if (actual.size() < expected.size()) {
        throw BootError("executed slice read fewer guest words than its pins");
    }
    for (std::size_t index = 0; index < expected.size(); ++index) {
        if (actual[index] != expected[index]) {
            throw BootError("executed slice read an unexpected guest address");
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    try {
        StartupClock clock;
        std::string dol_path;
        std::string oracle_path;
        std::string constructor_oracle_path;
        std::string constructor1_oracle_path;
        std::string constructor2_oracle_path;
        std::string constructor3_oracle_path;
        std::string constructor4_oracle_path;
        std::string constructor5_oracle_path;
        std::string constructor6_oracle_path;
        std::string constructor7_oracle_path;
        std::string constructor8_oracle_path;
        bool generated_constructor_batch = false;
        bool generated_application_startup = false;
        for (int index = 1; index < argc; ++index) {
            const std::string argument = argv[index];
            if (argument == "--generated-application-startup") {
                generated_application_startup = true;
                generated_constructor_batch = true;
            } else if (argument == "--generated-constructor-batch") {
                generated_constructor_batch = true;
            } else if (argument == "--constructor-8-oracle") {
                if (index + 1 >= argc) throw BootError("missing constructor 8 oracle path");
                constructor8_oracle_path = argv[++index];
            } else if (argument == "--constructor-7-oracle") {
                if (index + 1 >= argc) throw BootError("missing constructor 7 oracle path");
                constructor7_oracle_path = argv[++index];
            } else if (argument == "--constructor-6-oracle") {
                if (index + 1 >= argc) throw BootError("missing constructor 6 oracle path");
                constructor6_oracle_path = argv[++index];
            } else if (argument == "--constructor-5-oracle") {
                if (index + 1 >= argc) throw BootError("missing constructor 5 oracle path");
                constructor5_oracle_path = argv[++index];
            } else if (argument == "--constructor-4-oracle") {
                if (index + 1 >= argc) throw BootError("missing constructor 4 oracle path");
                constructor4_oracle_path = argv[++index];
            } else if (argument == "--constructor-3-oracle") {
                if (index + 1 >= argc) throw BootError("missing constructor 3 oracle path");
                constructor3_oracle_path = argv[++index];
            } else if (argument == "--constructor-2-oracle") {
                if (index + 1 >= argc) throw BootError("missing constructor 2 oracle path");
                constructor2_oracle_path = argv[++index];
            } else if (argument == "--constructor-1-oracle") {
                if (index + 1 >= argc) throw BootError("missing constructor 1 oracle path");
                constructor1_oracle_path = argv[++index];
            } else if (argument == "--constructor-oracle") {
                if (index + 1 >= argc) throw BootError("missing constructor oracle path");
                constructor_oracle_path = argv[++index];
            } else if (argument == "--oracle") {
                if (index + 1 >= argc) {
                    throw BootError("missing oracle path");
                }
                oracle_path = argv[++index];
            } else if (!argument.empty() && argument[0] == '-') {
                throw BootError("unknown argument");
            } else if (!dol_path.empty()) {
                throw BootError("unexpected extra argument");
            } else {
                dol_path = argument;
            }
        }
        if (generated_constructor_batch && constructor8_oracle_path.empty())
            throw BootError("generated batch requires the validated constructor 8 predecessor");
        if (dol_path.empty()) {
            throw BootError("usage: ShadowNativeBootTest <main.dol> [--oracle <pre-entry.json>]");
        }
        if (!constructor1_oracle_path.empty() && (constructor_oracle_path.empty() || oracle_path.empty()))
            throw BootError("constructor 1 requires pre-entry and constructor 0 evidence");
        if (!constructor2_oracle_path.empty() && constructor1_oracle_path.empty())
            throw BootError("constructor 2 requires constructor 1 evidence");
        if (!constructor3_oracle_path.empty() && constructor2_oracle_path.empty())
            throw BootError("constructor 3 requires constructor 2 evidence");
        if (!constructor4_oracle_path.empty() && constructor3_oracle_path.empty())
            throw BootError("constructor 4 requires constructor 3 evidence");
        if (!constructor5_oracle_path.empty() && constructor4_oracle_path.empty())
            throw BootError("constructor 5 requires constructor 4 evidence");
        if (!constructor8_oracle_path.empty() && constructor7_oracle_path.empty())
            throw BootError("constructor 8 requires constructor 7 evidence");
        if (!constructor7_oracle_path.empty() && constructor6_oracle_path.empty())
            throw BootError("constructor 7 requires constructor 6 evidence");
        if (!constructor6_oracle_path.empty() && constructor5_oracle_path.empty())
            throw BootError("constructor 6 requires constructor 5 evidence");

        std::cout << "FIXTURE main.dol DEVELOPMENT_VALIDATION_ONLY\n";
        const std::vector<std::uint8_t> dol = ReadFixture(dol_path);
        clock.Mark("fixture_load");
        ExpectTamperRejected(dol);
        RunFixtureContractTests(dol);
        clock.Mark("contract_tests");
        const BootImage image = LoadValidatedFixture(dol);
        clock.Mark("manifest_validation");

        LogReached(Checkpoint::BootInputValidated,
                   CheckpointFacts{
                       NativeBootManifest::entry,
                       "fixture DOL checked against the typed GUPP8P manifest; no guest machine created",
                       "none read",
                       "hardware helper, CRT execution, OS init",
                       "MATCH_MANIFEST",
                   });
        LogReached(Checkpoint::DolEntryReached,
                   CheckpointFacts{
                       NativeBootManifest::entry,
                       "logical entry only; apploader and IPL are not reconstructed",
                       "entry GPRs are not claimed",
                       "0x80003400",
                       "NO_ENTRY_GPR_CLAIM",
                   });

        const StartupState state = EnterRegisterStartup(image);
        clock.Mark("register_startup");
        std::vector<GuestAddress32> executed_reads;
        for (const InstructionPin& pin : NativeBootManifest::entry_pins) {
            executed_reads.push_back(pin.address);
        }
        for (const InstructionPin& pin : NativeBootManifest::register_helper_pins) {
            executed_reads.push_back(pin.address);
        }
        ExpectReadPrefix(image, executed_reads);

        const std::string globals = "r1=0x" + Hex8(state.gpr[1]) + " r2=0x" + Hex8(state.gpr[2]) +
                                    " r13=0x" + Hex8(state.gpr[13]) + " lr=0x" + Hex8(state.lr) +
                                    " other_gpr=0";
        LogReached(Checkpoint::RegisterInitComplete,
                   CheckpointFacts{
                       state.pc,
                       "translated register helper; low-memory reads in this slice=0",
                       globals,
                       "0x805F1F30 and 0x805F1F38 are outside the fixture; 0x80003400 not entered",
                       "MATCH_CONFIRMED_HELPER_RESULT",
                   });

        PreEntryOracle oracle;
        if (!oracle_path.empty()) {
            oracle = LoadPreEntryOracle(oracle_path);
        }
        clock.Mark("oracle_parse");
        IsolateHardwareFrontier(image, oracle, oracle.present);

        if (!oracle.present) {
            clock.Mark("stop");
            clock.Report();
            std::cout << "STOP milestone=M2 pc=0x" << Hex8(state.pc)
                      << " hardware_semantics=NOT_REACHED crt=NOT_EXECUTED os_init=NOT_STARTED\n";
            std::cout << "GUEST_READS fixture_words=" << image.guest_reads().size()
                      << " data_reads=0 data_writes=0\n";
            return 0;
        }

        const HardwareSemantics hardware = InitializeNativeHardwareSemantics(oracle);
        clock.Mark("hardware_semantics");
        if (hardware.pc != 0x8000315Cu || hardware.fpscr != static_cast<GuestWord32>(hardware.fpr_binary64) ||
            !hardware.paired_temporary_survives || hardware.r1 != state.gpr[1]) {
            throw BootError("native hardware semantics diverged from the helper's surviving state");
        }
        std::ostringstream hardware_globals;
        hardware_globals << "r1=0x" << Hex8(hardware.r1) << " r2=0x" << Hex8(hardware.r2)
                         << " r13=0x" << Hex8(hardware.r13) << " lr=0x" << Hex8(hardware.lr)
                         << " fpr_binary64=0x" << std::hex << std::uppercase << hardware.fpr_binary64
                         << " fpscr=0x" << Hex8(hardware.fpscr) << " paired_survives=1 ps1_source=0x"
                         << Hex8(hardware.paired_lane1_binary32);
        LogReached(Checkpoint::HardwareSemanticsComplete,
                   CheckpointFacts{
                       hardware.pc,
                       "FPR/FPSCR image only; MSR HID GQR and caches are not objects",
                       hardware_globals.str(),
                       "none inside 0x80003400",
                       "MATCH",
                   });
        std::cout << "EXPLAINED store 0x80580000=0x803726D8 has no reader before CRT fill;"
                     " not installed\n";

        const CrtSemantics crt = InitializeNativeCrt(image, hardware);
        // Preserve this CRT-produced typed state without executing the later
        // application initializer or treating it as Native SI approval.
        [[maybe_unused]] auto display_init_guard = NativeDisplayInitGuard::FromValidatedCrt(image, crt);
        clock.Mark("crt_startup");
        clock.ReportMemory("after_m4");
        if (crt.copies_executed || crt.guest_image_zeroed || crt.identity_copies != 10 ||
            crt.zero_ranges != 3) {
            throw BootError("native CRT semantics diverged from the identity/zero tables");
        }
        std::ostringstream crt_globals;
        crt_globals << "r0=0x" << Hex8(crt.r0) << " r1=0x" << Hex8(crt.r1) << " stack0=0x"
                    << Hex8(crt.stack_word_at_r1) << " stack4=0x" << Hex8(crt.stack_word_at_r1_plus_4)
                    << " fpr_binary64=0x" << std::hex << std::uppercase << hardware.fpr_binary64;
        LogReached(Checkpoint::CrtInitComplete,
                   CheckpointFacts{
                       crt.pc,
                       "10 identity copies skipped; 3 zero ranges are semantic, not a guest image",
                       crt_globals.str(),
                       "0x80003170 low-memory sequence",
                       "MATCH",
                   });
        if (!oracle.has_lowmem) {
            clock.Mark("stop");
            clock.Report();
            std::cout << "STOP milestone=M4 pc=0x" << Hex8(crt.pc)
                      << " next=0x80003170 os_init=NOT_STARTED\n";
            std::cout << "GUEST_READS fixture_words=" << image.guest_reads().size()
                      << " data_reads=0 data_writes=0 guest_ram_bytes=0\n";
            return 0;
        }

        const LowMemorySemantics low_memory = ResolveLowMemoryPrelude(image, crt, oracle);
        clock.Mark("low_memory_startup");
        clock.ReportMemory("after_m5");
        if (low_memory.debug_transfer || low_memory.argument_relocation || low_memory.r14 != 0u ||
            low_memory.r15 != 0u || low_memory.boot_word != 0u) {
            throw BootError("low-memory prelude left the retail path");
        }
        const RuntimeRoute entered = ResolveRuntimeRoute(image, low_memory);
        std::ostringstream entered_globals;
        entered_globals << "r1=0x" << Hex8(crt.r1) << " r2=0x" << Hex8(hardware.r2) << " r13=0x"
                        << Hex8(hardware.r13) << " debug_flag=0 argument_offset=0 boot_word=0 r14=0 r15=0";
        LogReached(Checkpoint::OsInitEntered,
                   CheckpointFacts{
                       entered.os_entry_pc,
                       "retail prelude skipped debug transfer and argument relocation",
                       entered_globals.str(),
                       "runtime body at 0x80370E68",
                       "MATCH",
                   });

        clock.Mark("os_runtime");
        clock.ReportMemory("after_m6");
        if (entered.os_body_executed || entered.constructors_executed || entered.guest_ram_allocated ||
            entered.constructor_count != 282 || entered.pc != 0x8000329Cu) {
            throw BootError("runtime route did not stop before the constructor walker");
        }
        std::ostringstream os_globals;
        os_globals << "arena_low=0x" << Hex8(entered.arena_low) << " arena_high=0x" << Hex8(entered.arena_high)
                   << " link_e4=0x" << Hex8(entered.link_800030e4) << " link_e6=0x" << Hex8(entered.link_800030e6)
                   << " init_byte=0 constructors=" << std::dec << entered.constructor_count
                   << " first=0x" << Hex8(entered.first_constructor);
        LogReached(Checkpoint::OsInitComplete,
                   CheckpointFacts{
                       entered.pc,
                       "route validated; runtime body not executed; no guest RAM image",
                       os_globals.str(),
                       "constructor walker 0x803796AC not entered",
                       "MATCH",
                   });
        if (!constructor_oracle_path.empty()) {
            const auto observation = LoadConstructor0Oracle(constructor_oracle_path);
            const auto result = ExecuteFirstConstructor(image, crt, hardware, entered, oracle, observation);
            clock.Mark("constructor_0");
            clock.ReportMemory("after_constructor_0");
            for (const auto& event : result.events) {
                std::cout << "C0_TRACE " << event.kind << " pc=0x" << Hex8(event.pc)
                          << " address=0x" << Hex8(event.address) << " value=0x" << Hex8(event.value) << '\n';
            }
            const std::string fragment_globals = "registration_id=0x" + Hex8(result.fragment.registration_id) +
                " descriptor=0x" + Hex8(result.fragment.descriptor.raw) + " sda2=0x" + Hex8(result.fragment.sda2.raw) +
                " occupied=0x" + Hex8(result.fragment.occupied);
            LogReached(Checkpoint::Constructor0Complete, CheckpointFacts{
                result.pc, "one native constructor executed; four-word typed fragment state; next call not made",
                fragment_globals,
                "constructor 1 at 0x8000AAF4: FPR31 paired spill and float input 0x805F27AC not yet validated",
                "MATCH_CONSTRUCTOR_ORACLE"});
            if (!constructor1_oracle_path.empty()) {
                const auto second = ExecuteSecondConstructor(image, hardware, oracle, result,
                    LoadConstructor1Oracle(constructor1_oracle_path));
                for (const auto& e : second.events)
                    std::cout << "C1_TRACE " << e.kind << " pc=" << Constructor1Hex(e.pc, 4)
                              << " address=" << Constructor1Hex(e.address, 4) << " size=" << e.size
                              << " value=" << e.value << '\n';
                clock.Mark("constructor_1");
                clock.ReportMemory("after_constructor_1");
                LogReached(Checkpoint::Constructor1Complete, CheckpointFacts{
                    second.pc, "constructor 1 and lazy-FPU first use executed; constructor 2 not called",
                    "nine vector words; two angle words; FPU owner/context saves; paired F31 retained",
                    "constructor 2 at 0x8001C484; helper 0x8001C4B8 dependencies are unproven",
                    "MATCH_CONSTRUCTOR_1_ORACLE"});
                if (!constructor2_oracle_path.empty()) {
                    const auto proof2 = LoadConstructor2Oracle(constructor2_oracle_path);
                    const auto third = ExecuteThirdConstructor(image, crt, oracle, second,
                        proof2);
                    for (const auto& e : third.body.events)
                        std::cout << "C2_TRACE " << e.kind << " pc=" << Constructor1Hex(e.pc,4)
                                  << " address=" << Constructor1Hex(e.address,4) << " size=" << e.size
                                  << " value=" << e.value << '\n';
                    clock.Mark("constructor_2"); clock.ReportMemory("after_constructor_2");
                    LogReached(Checkpoint::Constructor2Complete, CheckpointFacts{
                        third.pc, "constructor 2 executed; two typed vectors copied from proven CRT source",
                        "six vector words; f0/f1 updated; prior globals and FPU ownership preserved",
                        "constructor 3 at 0x8002218C, table cursor 0x804AAC6C, not executed",
                        "MATCH_CONSTRUCTOR_2_ORACLE"});
                    if (!constructor3_oracle_path.empty()) {
                        const auto proof3 = LoadConstructor3Oracle(constructor3_oracle_path);
                        const auto fourth = ExecuteFourthConstructor(image, oracle, third, proof2,
                            proof3);
                        for (const auto& e : fourth.body.events)
                            std::cout << "C3_TRACE " << e.kind << " pc=" << Constructor1Hex(e.pc,4)
                                      << " address=" << Constructor1Hex(e.address,4) << " size=" << e.size
                                      << " value=" << e.value << '\n';
                        clock.Mark("constructor_3"); clock.ReportMemory("after_constructor_3");
                        LogReached(Checkpoint::Constructor3Complete, CheckpointFacts{
                            fourth.pc, "constructor 3 executed; three typed vector stores from proven DOL constants",
                            "vector and f0/f1 updated; prior effects preserved",
                            "constructor 4 at 0x80035D4C, cursor 0x804AAC70, not executed",
                            "MATCH_CONSTRUCTOR_3_ORACLE"});
                        if (!constructor4_oracle_path.empty()) {
                            const auto proof4 = LoadConstructor4Oracle(constructor4_oracle_path);
                            const auto fifth = ExecuteFifthConstructor(image, crt, oracle, fourth, proof3,
                                proof4);
                            for (const auto& e : fifth.body.events)
                                std::cout << "C4_TRACE " << e.kind << " pc=" << Constructor1Hex(e.pc,4)
                                          << " address=" << Constructor1Hex(e.address,4) << " size=" << e.size
                                          << " value=" << e.value << '\n';
                            clock.Mark("constructor_4"); clock.ReportMemory("after_constructor_4");
                            LogReached(Checkpoint::Constructor4Complete, CheckpointFacts{
                                fifth.pc, "constructor 4 executed; object initialized and destructor record linked",
                                "two object words; three registration words; list head; no destructor invoked",
                                "constructor 5 at 0x80039B88, cursor 0x804AAC74, not executed",
                                "MATCH_CONSTRUCTOR_4_ORACLE"});
                            if (!constructor5_oracle_path.empty()) {
                                const auto proof5 = LoadConstructor5Oracle(constructor5_oracle_path);
                                const auto sixth = ExecuteSixthConstructor(image, oracle, fifth, proof4,
                                    proof5);
                                for (const auto& e : sixth.body.events)
                                    std::cout << "C5_TRACE " << e.kind << " pc=" << Constructor1Hex(e.pc,4)
                                              << " address=" << Constructor1Hex(e.address,4) << " size=" << e.size
                                              << " value=" << e.value << '\n';
                                clock.Mark("constructor_5"); clock.ReportMemory("after_constructor_5");
                                LogReached(Checkpoint::Constructor5Complete, CheckpointFacts{
                                    sixth.pc, "constructor 5 executed; four ordered byte stores from li arguments",
                                    "four native bytes; prior registration and other effects preserved",
                                    "constructor 6 at 0x8003AFB8, cursor 0x804AAC78, not executed",
                                    "MATCH_CONSTRUCTOR_5_ORACLE"});
                                if (!constructor6_oracle_path.empty()) {
                                    const auto proof6 = LoadConstructor6Oracle(constructor6_oracle_path);
                                    const auto seventh = ExecuteSeventhConstructor(image, oracle, sixth, proof5, proof6);
                                    for (const auto& e : seventh.body.events)
                                        std::cout << "C6_TRACE " << e.kind << " pc=" << Constructor1Hex(e.pc,4)
                                                  << " address=" << Constructor1Hex(e.address,4) << " size=" << e.size
                                                  << " value=" << e.value << '\n';
                                    clock.Mark("constructor_6"); clock.ReportMemory("after_constructor_6");
                                    LogReached(Checkpoint::Constructor6Complete, CheckpointFacts{
                                        seventh.pc, "four elements constructed via proven indirect target; registration prepended",
                                        "eight halfwords; new node links to earlier node; no destructor invoked",
                                        "constructor 7 at 0x8004336C, cursor 0x804AAC7C, not executed",
                                        "MATCH_CONSTRUCTOR_6_ORACLE"});
                                    if (!constructor7_oracle_path.empty()) {
                                        const auto proof7 = LoadConstructor7Oracle(constructor7_oracle_path);
                                        const auto eighth = ExecuteEighthConstructor(image, oracle, seventh, proof6, proof7);
                                        for (const auto& e : eighth.body.events)
                                            std::cout << "C7_TRACE " << e.kind << " pc=" << Constructor1Hex(e.pc,4)
                                                      << " address=" << Constructor1Hex(e.address,4) << " size=" << e.size
                                                      << " value=" << e.value << '\n';
                                        clock.Mark("constructor_7"); clock.ReportMemory("after_constructor_7");
                                        LogReached(Checkpoint::Constructor7Complete, CheckpointFacts{
                                            eighth.pc, "four null objects initialized and registered in original order",
                                            "four words; four list nodes; prior chain retained; no callbacks invoked",
                                            "constructor 8 at 0x80045F48, cursor 0x804AAC80, not executed",
                                            "MATCH_CONSTRUCTOR_7_ORACLE"});
                                        if (!constructor8_oracle_path.empty()) {
                                            const auto ninth = ExecuteNinthConstructor(image, oracle, eighth, proof7,
                                                LoadConstructor8Oracle(constructor8_oracle_path));
                                            for (const auto& e : ninth.body.events)
                                                std::cout << "C8_TRACE " << e.kind << " pc=" << Constructor1Hex(e.pc,4)
                                                          << " address=" << Constructor1Hex(e.address,4) << " size=" << e.size
                                                          << " value=" << e.value << '\n';
                                            clock.Mark("constructor_8"); clock.ReportMemory("after_constructor_8");
                                            LogReached(Checkpoint::Constructor8Complete, CheckpointFacts{
                                                ninth.pc, "two stack-local word pairs initialized via reusable helper summary",
                                                "four global words; existing registration chain preserved",
                                                "constructor 9 at 0x80047710, cursor 0x804AAC84, not executed",
                                                "MATCH_CONSTRUCTOR_8_ORACLE"});
                                            if (generated_constructor_batch) {
                                                const auto batch = Reconstructed::RunConstructorBatch(ninth,
                                                    LoadConstructor8Oracle(constructor8_oracle_path), std::cout);
                                                clock.Mark("generated_constructor_batch");
                                                clock.ReportMemory("after_generated_batch");
                                                if (generated_application_startup) {
                                                    if (batch.stop_pc != 0x8037970Cu || batch.cursor != 0x804AB0C8u || batch.next_constructor != 0)
                                                        throw BootError("application entry requires the complete constructor table");
                                                    const auto arguments = Reconstructed::CompleteConstructorStartup(low_memory, batch.constructors_executed);
                                                    std::cout << "CHECKPOINT APPLICATION_ENTRY generated_native=1 argc=" << arguments.values.size() << '\n';
                                                    clock.Mark("native_application_entry");
                                                    clock.Mark("stop"); clock.Report();
                                                    std::cout << "STOP milestone=M8_APPLICATION_ENTRY pc=0x800510C0 constructors_executed="
                                                              << batch.constructors_executed << " application_body_executed=0 guest_ram_bytes=0\n";
                                                    return 0;
                                                }
                                                clock.Mark("stop"); clock.Report();
                                                std::cout << "STOP milestone=M7_BATCH pc=0x" << Hex8(batch.stop_pc) << " cursor=0x" << Hex8(batch.cursor)
                                                          << " next=0x" << Hex8(batch.next_constructor)
                                                          << " constructors_executed=" << batch.constructors_executed
                                                          << " next_constructor_executed=0 guest_ram_bytes=0\n";
                                                return 0;
                                            }
                                            clock.Mark("stop"); clock.Report();
                                            std::cout << "STOP milestone=M7_C8 pc=0x" << Hex8(ninth.pc)
                                                      << " cursor=0x" << Hex8(ninth.cursor) << " next=0x" << Hex8(ninth.next_constructor)
                                                      << " constructors_executed=" << ninth.constructors_executed
                                                      << " constructor_9_executed=0 guest_ram_bytes=0\n";
                                            return 0;
                                        }
                                        clock.Mark("stop"); clock.Report();
                                        std::cout << "STOP milestone=M7_C7 pc=0x" << Hex8(eighth.pc)
                                                  << " cursor=0x" << Hex8(eighth.cursor) << " next=0x" << Hex8(eighth.next_constructor)
                                                  << " constructors_executed=" << eighth.constructors_executed
                                                  << " constructor_8_executed=0 guest_ram_bytes=0\n";
                                        return 0;
                                    }
                                    clock.Mark("stop"); clock.Report();
                                    std::cout << "STOP milestone=M7_C6 pc=0x" << Hex8(seventh.pc)
                                              << " cursor=0x" << Hex8(seventh.cursor) << " next=0x" << Hex8(seventh.next_constructor)
                                              << " constructors_executed=" << seventh.constructors_executed
                                              << " constructor_7_executed=0 guest_ram_bytes=0\n";
                                    return 0;
                                }
                                clock.Mark("stop"); clock.Report();
                                std::cout << "STOP milestone=M7_C5 pc=0x" << Hex8(sixth.pc)
                                          << " cursor=0x" << Hex8(sixth.cursor) << " next=0x" << Hex8(sixth.next_constructor)
                                          << " constructors_executed=" << sixth.constructors_executed
                                          << " constructor_6_executed=0 guest_ram_bytes=0\n";
                                return 0;
                            }
                            clock.Mark("stop"); clock.Report();
                            std::cout << "STOP milestone=M7_C4 pc=0x" << Hex8(fifth.pc)
                                      << " cursor=0x" << Hex8(fifth.cursor) << " next=0x" << Hex8(fifth.next_constructor)
                                      << " constructors_executed=" << fifth.constructors_executed
                                      << " constructor_5_executed=0 guest_ram_bytes=0\n";
                            return 0;
                        }
                        clock.Mark("stop"); clock.Report();
                        std::cout << "STOP milestone=M7_C3 pc=0x" << Hex8(fourth.pc)
                                  << " cursor=0x" << Hex8(fourth.cursor) << " next=0x" << Hex8(fourth.next_constructor)
                                  << " constructors_executed=" << fourth.constructors_executed
                                  << " constructor_4_executed=0 guest_ram_bytes=0\n";
                        return 0;
                    }
                    clock.Mark("stop"); clock.Report();
                    std::cout << "STOP milestone=M7_C2 pc=0x" << Hex8(third.pc)
                              << " cursor=0x" << Hex8(third.cursor) << " next=0x" << Hex8(third.next_constructor)
                              << " constructors_executed=" << third.constructors_executed
                              << " constructor_3_executed=0 guest_ram_bytes=0\n";
                    return 0;
                }
                clock.Mark("stop"); clock.Report();
                std::cout << "STOP milestone=M7_C1 pc=0x" << Hex8(second.pc)
                          << " cursor=0x" << Hex8(second.cursor) << " next=0x" << Hex8(second.next_constructor)
                          << " constructors_executed=" << second.constructors_executed
                          << " constructor_2_executed=0 guest_ram_bytes=0\n";
                return 0;
            }
            clock.Mark("stop");
            clock.Report();
            std::cout << "STOP milestone=M7_C0 pc=0x" << Hex8(result.pc)
                      << " cursor=0x" << Hex8(result.cursor) << " next=0x" << Hex8(result.next_constructor)
                      << " constructors_executed=" << result.constructors_executed
                      << " constructor_1_executed=0 guest_ram_bytes=0\n";
            return 0;
        }
        clock.Mark("stop");
        clock.Report();
        std::cout << "STOP milestone=M6 pc=0x" << Hex8(entered.pc)
                  << " constructors=" << entered.constructor_count
                  << " first=0x" << Hex8(entered.first_constructor)
                  << " constructors_executed=0 guest_ram_bytes=0\n";
        std::cout << "GUEST_READS fixture_words=" << image.guest_reads().size()
                  << " data_reads=0 data_writes=0\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "native boot: " << error.what() << '\n';
        return 1;
    }
}
