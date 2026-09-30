#include "crt_semantics.h"
#include "si_type_query_initial_low.h"
#include "contract_tests.h"
#include <fstream>
#include <iostream>
#include <iterator>

static void Require(bool value, const char* message) { if (!value) throw BootError(message); }
static void Reject(const auto& action) {
    bool failed = false;
    try { action(); } catch (const BootError&) { failed = true; }
    Require(failed, "unproven SI low-word path accepted");
}
static std::size_t Offset(GuestAddress32 address) {
    for (const auto& s : NativeBootManifest::sections)
        if (address >= s.address && address < s.address + s.size)
            return s.file_offset + address - s.address;
    throw BootError("missing test fixture address");
}
int main(int argc, char** argv) {
    try {
        Require(argc==3, "usage: SiTypeQueryInitialLowTests DOL pre-entry-oracle");
        std::ifstream file(argv[1],std::ios::binary);
        Require(bool(file),"missing fixture");
        const std::vector<std::uint8_t> dol{std::istreambuf_iterator<char>(file),{}};
        const auto pre=LoadPreEntryOracle(argv[2]);
        const auto hardware=InitializeNativeHardwareSemantics(pre);
        auto image=LoadValidatedFixture(dol);
        SiTypeQueryInitialLow absent;
        Reject([&]{(void)absent.ConsumeInitialLow();});
        auto crt=InitializeNativeCrt(image,hardware);
        const auto low=crt.si_type_query_channel0.ConsumeInitialLow();
        // Zero is an expected test result, not an input to the implementation.
        Require(low==0,"CRT-produced SI low word differs");
        Reject([&]{(void)crt.si_type_query_channel0.ConsumeInitialLow();});
        for (auto address : {0x800055C8u,0x800055CCu,0x800033D4u,0x800054B0u}) {
            auto changed=dol;changed.at(Offset(address))^=1;
            Reject([&]{auto bad=LoadValidatedFixture(changed);(void)InitializeNativeCrt(bad,hardware);});
        }
        auto bad_hardware=hardware;bad_hardware.pc=0;
        Reject([&]{(void)InitializeNativeCrt(image,bad_hardware);});
        std::cout << "READ 805a6834 4 " << low << '\n';
        std::cout << "SLICE_STOP 803b6e88 UNPROVEN_MEMORY 805a6830\n";
        std::cout << "NATIVE_BOOT_PC 800510c0 SI_BLOCKED\n";
        // No high-word read, subtraction, branch, callback or SI hardware here.
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
