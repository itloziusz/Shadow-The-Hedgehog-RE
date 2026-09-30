#include "crt_semantics.h"
#include "contract_tests.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>

static void Require(bool value,const char* message) { if (!value) throw BootError(message); }
static void Reject(const auto& action) {
    bool failed=false;
    try { action(); } catch(const BootError&) { failed=true; }
    Require(failed,"unproven SI timestamp path accepted");
}
static std::size_t Offset(GuestAddress32 address) {
    for (const auto& s:NativeBootManifest::sections)
        if(address>=s.address && address<s.address+s.size) return s.file_offset+address-s.address;
    throw BootError("missing fixture address");
}
static std::uint64_t Hex(const char* text) {
    std::size_t consumed=0;
    const auto value=std::stoull(text,&consumed,16);
    Require(consumed==std::string(text).size(),"invalid arithmetic test input");
    return value;
}
int main(int argc,char** argv) {
    try {
        if(argc==4 && std::string(argv[1])=="--subtract") {
            // Differential arithmetic harness only; never populates startup state.
            std::cout<<std::hex<<std::setw(16)<<std::setfill('0')<<SiTypeTimestampElapsed(Hex(argv[2]),Hex(argv[3]))<<'\n';
            return 0;
        }
        Require(argc==3 || argc==4,"usage: SiTypeQueryTimestampTests DOL pre-entry [validation-Now]");
        std::ifstream file(argv[1],std::ios::binary);
        Require(bool(file),"missing fixture");
        const std::vector<std::uint8_t> dol{std::istreambuf_iterator<char>(file),{}};
        const auto hardware=InitializeNativeHardwareSemantics(LoadPreEntryOracle(argv[2]));
        auto image=LoadValidatedFixture(dol);
        SiTypeQueryTimestamp absent;
        Reject([&]{(void)absent.ConsumeInitialLow();});
        Reject([&]{(void)absent.ConsumeInitialHigh();});
        auto crt=InitializeNativeCrt(image,hardware);
        auto& timestamp=crt.si_type_query_channel0;
        Reject([&]{(void)timestamp.ConsumeInitialHigh();});
        const auto low=timestamp.ConsumeInitialLow();
        Reject([&]{(void)timestamp.ConsumeInitialLow();});
        const auto high=timestamp.ConsumeInitialHigh();
        Reject([&]{(void)timestamp.ConsumeInitialHigh();});
        Reject([&]{(void)timestamp.ConsumeInitialLow();});
        const auto value=(std::uint64_t{high}<<32)|low;
        Require(value==0,"CRT-produced timestamp differs"); // expected output, not an input
        for(auto address:{0x800055C8u,0x800055CCu,0x800033D4u,0x800054ACu,0x800054B0u}) {
            auto changed=dol;changed.at(Offset(address))^=1;
            Reject([&]{auto bad=LoadValidatedFixture(changed);(void)InitializeNativeCrt(bad,hardware);});
        }
        auto wrong=hardware;wrong.pc=0;
        Reject([&]{(void)InitializeNativeCrt(image,wrong);});
        // Synthetic boundary vectors, not sampled timebase or a runtime clock.
        Require(SiTypeTimestampElapsed(0,1)==UINT64_MAX,"borrow/wrap");
        Require(SiTypeTimestampElapsed(0x100000000ULL,1)==0xffffffffULL,"low borrow");
        Require(SiTypeTimestampElapsed(0,0x100000000ULL)==0xffffffff00000000ULL,"high wrap");
        Require(SiTypeTimestampElapsed(UINT64_MAX,UINT64_MAX)==0,"equal timestamps");
        const auto now=argc==4?Hex(argv[3]):std::uint64_t{0x100000001ULL};
        std::cout<<"READ 805a6834 4 "<<low<<'\n'<<"READ 805a6830 4 "<<high<<'\n';
        std::cout<<std::hex<<std::setfill('0')<<"TIMESTAMP "<<std::setw(16)<<value<<'\n';
        std::cout<<"ELAPSED "<<std::setw(16)<<SiTypeTimestampElapsed(now,value)<<'\n';
        std::cout<<"SLICE_STOP 803b6e94 UNPROVEN_MEMORY 80569990\n";
        std::cout<<"NATIVE_BOOT_PC 800510c0 SI_BLOCKED\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
