#include "constructor_0.h"
#include "contract_tests.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <algorithm>

namespace {
void Require(bool condition, const char* message) { if (!condition) throw BootError(message); }
void Reject(const auto& action, const char* message) {
    bool rejected = false;
    try { action(); } catch (const BootError&) { rejected = true; }
    Require(rejected, message);
}
std::size_t Offset(GuestAddress32 address) {
    for (const auto& section : NativeBootManifest::sections)
        if (address >= section.address && address < section.address + section.size)
            return section.file_offset + address - section.address;
    throw BootError("test address not in fixture");
}
}

int main(int argc, char** argv) {
    try {
        Require(argc == 4, "usage: Constructor0ContractTests DOL pre-entry-oracle constructor-oracle");
        std::ifstream file(argv[1], std::ios::binary);
        Require(static_cast<bool>(file), "missing fixture");
        const std::vector<std::uint8_t> dol{std::istreambuf_iterator<char>(file), {}};
        RunFixtureContractTests(dol);
        const auto image = LoadValidatedFixture(dol);
        const auto pre = LoadPreEntryOracle(argv[2]);
        const auto hardware = InitializeNativeHardwareSemantics(pre);
        const auto crt = InitializeNativeCrt(image, hardware);
        const auto low = ResolveLowMemoryPrelude(image, crt, pre);
        const auto route = ResolveRuntimeRoute(image, low);
        const auto oracle = LoadConstructor0Oracle(argv[3]);
        const auto run = [&](const Constructor0Oracle& o) {
            return ExecuteFirstConstructor(image, crt, hardware, route, pre, o);
        };
        const auto result = run(oracle);
        Require(result.constructors_executed == 1 && result.cursor == 0x804AAC64u &&
                result.pc == 0x803796F8u && result.next_constructor == 0x8000AAF4u,
                "did not stop before constructor 1");
        Require(result.fragment.registration_id == 0 && result.fragment.descriptor.raw == 0x80008D14u &&
                result.fragment.sda2.raw == hardware.r2 && result.fragment.occupied == 1,
                "registered fragment does not match the stepped oracle");
        std::vector<GuestAddress32> writes;
        for (const auto& event : result.events) if (event.kind == "write") writes.push_back(event.address);
        Require(writes == std::vector<GuestAddress32>{0x805A61F0u,0x805A61F4u,0x805A61F8u,0x805EED50u},
                "constructor stores missing or reordered");
        Require(std::none_of(result.events.begin(), result.events.end(), [](const auto& e) {
            return e.kind == "call" && e.address == 0x8000AAF4u;
        }), "constructor 1 was called");
        Reject([&] { (void)run(Constructor0Oracle{}); }, "missing evidence accepted");
        for (std::size_t boundary = 0; boundary < 4; ++boundary) {
            auto changed = oracle;
            changed.boundaries[boundary].id ^= 1;
            Reject([&] { (void)run(changed); }, "changed ID observation accepted");
        }
        for (std::size_t boundary = 1; boundary < 4; ++boundary) {
            for (std::size_t word = 0; word < 3; ++word) {
                auto changed = oracle;
                changed.boundaries[boundary].slot[word] ^= 1;
                Reject([&] { (void)run(changed); }, "OS slot mismatch ignored");
            }
            auto changed = oracle;
            changed.boundaries[boundary].r2 ^= 1;
            Reject([&] { (void)run(changed); }, "SDA2 preservation mismatch ignored");
        }
        auto changed = oracle;
        changed.final_state.id ^= 1;
        Reject([&] { (void)run(changed); }, "final mismatch ignored");
        changed = oracle;
        changed.final_pc += 4;
        Reject([&] { (void)run(changed); }, "wrong boundary accepted");
        auto early = crt;
        early.zero_ranges = 0;
        Reject([&] { (void)ExecuteFirstConstructor(image, early, hardware, route, pre, oracle); },
               "constructor slot initialized without CRT");
        for (const auto address : {0x803A252Cu,0x803A3970u,0x80379708u,0x805EED50u,0x80370E68u}) {
            auto modified = dol;
            modified[Offset(address)] ^= 1;
            const auto wrong = LoadValidatedFixture(modified);
            Reject([&] { (void)ExecuteFirstConstructor(wrong, crt, hardware, route, pre, oracle); },
                   "changed fixture accepted with old provenance");
        }
        for (std::size_t i = 0; i < oracle.startup_inputs.size(); ++i) {
            changed = oracle;
            changed.startup_inputs[i].back() ^= 1;
            Reject([&] { (void)run(changed); }, "different startup evidence accepted");
        }
        // Occupied registration MUST fail with -1, preserving all slot contents.
        FragmentState occupied{0xFFFFFFFEu,GuestPtr32{0x12345678u},GuestPtr32{0xABCDEF00u},7};
        std::vector<ConstructorEvent> events;
        RunConstructor0Body(image, occupied, 0x11223344u, 0x8060C5D0u, 0x803796FCu, events);
        Require(occupied.registration_id == 0xFFFFFFFFu && occupied.descriptor.raw == 0x12345678u &&
                occupied.sda2.raw == 0xABCDEF00u && occupied.occupied == 7, "occupied branch faked success");
        Require(std::count_if(events.begin(), events.end(), [](const auto& e) { return e.kind == "write"; }) == 1,
                "occupied branch wrote the slot");
        FragmentState alternate{0xFFFFFFFEu,GuestPtr32{42},GuestPtr32{43},0};
        events.clear();
        RunConstructor0Body(image, alternate, 0x11223344u, 0x8060C5D0u, 0x803796FCu, events);
        Require(alternate.sda2.raw == 0x11223344u, "argument replaced with hardcoded SDA2");
        for (const auto id : {0u,1u,0xFFFFFFFFu}) {
            FragmentState already{id,GuestPtr32{42},GuestPtr32{43},7};
            events.clear();
            RunConstructor0Body(image, already, hardware.r2, 0x8060C5D0u, 0x803796FCu, events);
            Require(already.registration_id == id && already.descriptor.raw == 42 && already.occupied == 7,
                    "already-registered guard changed state");
            Require(std::none_of(events.begin(), events.end(), [](const auto& e) {
                return e.kind == "call" || e.kind == "write";
            }), "guard branch performed registration");
        }
        std::cout << "PASS constructor0: provenance, ordered effects, real failure/guard branches, next-call boundary\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
