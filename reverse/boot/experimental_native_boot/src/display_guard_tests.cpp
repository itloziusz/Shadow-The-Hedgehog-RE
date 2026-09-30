#include "display_init_guard.h"
#include "contract_tests.h"
#include <fstream>
#include <iostream>
#include <iterator>

static void Require(bool v, const char* message) { if (!v) throw BootError(message); }
static void Reject(const auto& action) {
    bool rejected=false;
    try { action(); } catch(const BootError&) { rejected=true; }
    Require(rejected,"unproven display guard accepted");
}
static std::size_t Offset(GuestAddress32 address) {
    for (const auto& s : NativeBootManifest::sections)
        if (address >= s.address && address < s.address+s.size) return s.file_offset+address-s.address;
    throw BootError("missing fixture address");
}
int main(int argc, char** argv) {
    try {
        Require(argc==3,"usage: DisplayGuardTests DOL pre-entry-oracle");
        std::ifstream file(argv[1],std::ios::binary);Require(bool(file),"missing DOL");
        const std::vector<std::uint8_t> dol{std::istreambuf_iterator<char>(file),{}};
        const auto hardware=InitializeNativeHardwareSemantics(LoadPreEntryOracle(argv[2]));
        auto image=LoadValidatedFixture(dol);
        const auto crt=InitializeNativeCrt(image,hardware);
        NativeDisplayInitGuard missing;
        Reject([&]{(void)missing.ConsumeInitialDecision();});
        auto guard=NativeDisplayInitGuard::FromValidatedCrt(image,crt);
        const auto decision=guard.ConsumeInitialDecision();
        Require(decision.read_value==0 && decision.enter_initializer,"CRT guard/branch mismatch");
        Reject([&]{(void)guard.ConsumeInitialDecision();});
        for (auto value : {1u,2u,0x80000000u,0xffffffffu})
            Require(!NativeDisplayInitGuard::NeedsInitialization(value),"nonzero guard forced into initializer");
        auto early=crt;early.pc=0;
        Reject([&]{(void)NativeDisplayInitGuard::FromValidatedCrt(image,early);});
        for (auto address : {0x800033D4u,0x800054A4u,0x800055D0u,0x800055D4u,0x8037E850u,0x8037E854u,0x8037E858u}) {
            auto changed=dol;changed.at(Offset(address))^=1;
            Reject([&]{auto bad=LoadValidatedFixture(changed);(void)NativeDisplayInitGuard::FromValidatedCrt(bad,crt);});
        }
        std::cout << "READ 805f20c8 4 " << decision.read_value << '\n';
        std::cout << "ENTER_INITIALIZER " << decision.enter_initializer << '\n';
        std::cout << "SLICE_STOP 8037e85c UNPROVEN_MEMORY 805eec60\n";
        std::cout << "NATIVE_BOOT_PC 800510c0 SI_BLOCKED\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
