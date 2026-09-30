#include "early_si_selector.h"
#include "contract_tests.h"
#include <fstream>
#include <iostream>
#include <iterator>

static void Require(bool v,const char* message) { if(!v) throw BootError(message); }
static void Reject(const auto& action) {
    bool rejected=false;try { action(); } catch(const BootError&) { rejected=true; }
    Require(rejected,"unproven early selector accepted");
}
static std::size_t Offset(GuestAddress32 address) {
    for(const auto& s:NativeBootManifest::sections)
        if(address>=s.address && address<s.address+s.size) return s.file_offset+address-s.address;
    throw BootError("fixture address missing");
}
int main(int argc,char** argv) {
    try {
        Require(argc==3,"usage: EarlySiSelectorTests DOL pre-entry-oracle");
        std::ifstream input(argv[1],std::ios::binary);Require(bool(input),"missing fixture");
        const std::vector<std::uint8_t> dol{std::istreambuf_iterator<char>(input),{}};
        const auto hardware=InitializeNativeHardwareSemantics(LoadPreEntryOracle(argv[2]));
        auto image=LoadValidatedFixture(dol);const auto crt=InitializeNativeCrt(image,hardware);
        EarlySiSelector missing;Reject([&]{(void)missing.ConsumeInitialFamily();});
        auto selector=EarlySiSelector::FromValidatedCrt(image,crt);
        constexpr std::array<GuestWord32,8> expected{0,1,2,0,1,5,0,0};
        for(GuestWord32 i=0;i<expected.size();++i) Require(selector.MapSelector(i)==expected[i],"wrong mapping");
        Reject([&]{(void)selector.MapSelector(8);});Reject([&]{(void)selector.MapSelector(0xffffffffu);});
        const auto selected=selector.ConsumeInitialFamily();
        Require(selected.selector==0 && selected.family==0,"CRT selector/family disagreement");
        Reject([&]{(void)selector.ConsumeInitialFamily();});
        auto early=crt;early.pc=0;
        Reject([&]{(void)EarlySiSelector::FromValidatedCrt(image,early);});
        for(auto pc:{0x800033D4u,0x800054B0u,0x800055D0u,0x800055D4u,0x8038014Cu,0x80380170u,0x805631ACu}) {
            auto changed=dol;changed.at(Offset(pc))^=1;
            Reject([&]{auto bad=LoadValidatedFixture(changed);(void)EarlySiSelector::FromValidatedCrt(bad,crt);});
        }
        std::cout<<"READ 805f2114 4 "<<selected.selector<<'\n';
        std::cout<<"READ 805631ac 4 "<<selected.original_target<<'\n';
        std::cout<<"SAMPLING_FAMILY "<<selected.family<<'\n';
        std::cout<<"SLICE_STOP 803b71c8 UNPROVEN_MEMORY cc00206c SIZE 2\n";
        std::cout<<"NATIVE_BOOT_PC 800510c0 SI_BLOCKED\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
