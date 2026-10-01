#include "shadow/boot/TimeBaseSemantics.hpp"
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>
namespace {
void Require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F>void Reject(F f){try{f();}catch(const std::exception&){return;}throw std::runtime_error("unexplained state accepted");}
}
int main(int argc,char** argv) {
    using namespace shadow::boot;
    try {
        Require(argc==2,"PAL fixture required");
        std::ifstream f(argv[1],std::ios::binary);std::vector<std::uint8_t> raw(std::istreambuf_iterator<char>{f},{});
        const auto image=LoadValidatedFixture(raw);
        CycleTimeBaseEpoch e{0u,486000000u,0u};
        for(auto cycle:{0ull,1ull,11ull,12ull,13ull,0xFFFFFFFFull,0x100000000ull,UINT64_MAX})
            Require(TimeBaseAt(e,cycle)==cycle/12u,"source phase or unit changed");
        Require(CounterTimeBaseAt(UINT64_MAX,0u,12u)==0u,"full64 TB wrap lost");
        Require(CounterTimeBaseAt(0u,UINT64_MAX,11u)==1u,"source64 subtraction wrap lost");
        e.cpu_hz=729000000u;Reject([&]{TimeBaseAt(e,0u);});e.cpu_hz=0u;Reject([&]{TimeBaseAt(e,0u);});
        unsigned cases=0;std::uint32_t v[]={0u,1u,2u,0x7FFFFFFFu,0x80000000u,0xFFFFFFFEu,0xFFFFFFFFu};
        for(auto th:v)for(auto tl:v)for(auto oh:v)for(auto ol:v)for(auto xer:{0u,0x20000000u,0x8000007Fu,0xE000007Fu}) {
            const auto t=(std::uint64_t(th)<<32)|tl,o=(std::uint64_t(oh)<<32)|ol;
            const auto sum=AddClockOffset(t,o,xer);
            Require(((std::uint64_t(sum.high)<<32)|sum.low)==t+o,"clock word arithmetic mismatch");
            const bool lc=std::uint64_t(tl)+ol>UINT32_MAX;
            const bool hc=std::uint64_t(th)+oh+lc>UINT32_MAX;
            Require(sum.xer_after_low==((xer&~0x20000000u)|(lc?0x20000000u:0u)),"addc carry/preserved bits mismatch");
            Require(sum.xer_after_high==((xer&~0x20000000u)|(hc?0x20000000u:0u)),"adde carry/preserved bits mismatch");++cases;
        }
        Require(CompareSignedHigh(0x7FFFFFFFu,0x80000000u,0xABCDEF12u,0x80000000u)==0x5BCDEF12u,"signed high comparison/SO lost");
        Require(CompareSignedHigh(0xFFFFFFFFu,0u,0xABCDEF12u,0u)==0x8BCDEF12u,"signed high comparison became unsigned");
        ClockResearchInputs input{};auto& boot=input.entry.crt.l2.entry;
        boot.msr=0x32u;boot.hid0=0xC000u;boot.source_address=0x805F1F30u;boot.cr=0xABCDEF12u;boot.xer=0xC000007Fu;
        input.entry.crt.old_handler=0u;input.entry.pointer=0x817E54E0u;input.entry.loaded_bi2.resize(0x2000u);
        input.exceptions=0u;input.epoch={0u,486000000u,0u};input.frontier_cycles=11u;input.offset=UINT64_MAX;
        const auto phase=ProjectClockSingleStep(image,input);
        Require(phase.checkpoints[0].cached_tb==0u&&phase.checkpoints[0].cycles==12u,"TB must sample BEFORE post-step pending cycle");
        Require(phase.checkpoints[1].cached_tb==1u,"second source tick phase lost");
        Require(phase.checkpoints.back().boot.boot.native.state.machine.cpu.pc==0x80373AC4u,"research next unknown read bypassed");
        Require(phase.stores.size()==4u&&phase.stores[0].pc==0x80370EA8u&&phase.stores[1].pc==0x80370EACu,"clock low/high store order lost");
        Require(phase.stores[2].address==0x8060C5D4u&&phase.stores[3].address==0x8060C5C8u,"restored stack slot aliases lost");
        Require(phase.prefix.checkpoints.back().boot.boot.native.state.machine.cpu.pc==0x80379628u,"research advanced production frontier");
        const auto continuous=ProjectClockContinuousResearch(image,input);
        Require(continuous.checkpoints[0].cached_tb==0u&&continuous.checkpoints[0].cycles==11u,
                "continuous getter advanced unretired work");
        Require(continuous.checkpoints[1].cached_tb==0u&&continuous.checkpoints[2].cached_tb==0u,
                "same-block clock reads acquired different epochs");
        Require(continuous.checkpoints.back().cycles==51u,
                "continuous completed work or final pending four cycles changed");
        Require(continuous.unretired_work==4u&&phase.unretired_work==0u,
                "terminal pending work was silently lost or charged to stepped mode");
        Require(continuous.stores[0].value==0xFFFFFFFFu,
                "continuous low sample was replaced by stepped sample");
        input.epoch.rtc_seconds=129089431u;input.frontier_cycles=383u;input.offset=0u;
        Require(std::uint32_t(TimeBaseAt(input.epoch,0u))==0xFFFFFFE0u,"rollover fixture producer wrong");
        const auto roll=ProjectClockSingleStep(image,input);
        Require(roll.checkpoints.size()==phase.checkpoints.size()+5u,"source-produced high rollover must retry");
        Require((roll.checkpoints[3].boot.boot.native.state.cr&0xF0000000u)==0x90000000u,"failed signed LT/SO retry compare changed");
        Require(roll.checkpoints[4].boot.boot.native.state.machine.cpu.pc==0x80379628u,"retry target incorrect");
        input.rebased_epoch=0x12345678FFFFCDD9ull;input.epoch.source_cycles=0x28D70Du;
        input.frontier_cycles=0x2B30DFu;
        const auto rebased=ProjectClockSingleStep(image,input);
        Require(rebased.checkpoints.size()==phase.checkpoints.size()+5u,"pre-entry rebased counter rollover must retry");
        Require(rebased.checkpoints[0].cached_tb==0x12345678FFFFFFFFull,"rebased first high source phase changed");
        Require(rebased.checkpoints[2].cached_tb==0x1234567900000000ull,"rebased high sample did not roll");
        Require((rebased.checkpoints[3].boot.boot.native.state.cr&0xF0000000u)==0x90000000u,"rebased signed LT/SO comparison changed");
        input.rebased_epoch.reset();
        input.exceptions.reset();Reject([&]{ProjectClockSingleStep(image,input);});input.exceptions=1u;Reject([&]{ProjectClockSingleStep(image,input);});input.exceptions=0u;
        input.frontier_cycles=UINT64_MAX;Reject([&]{ProjectClockSingleStep(image,input);});
        input.frontier_cycles=0u;input.epoch.source_cycles=1u;Reject([&]{ProjectClockSingleStep(image,input);});
        input.epoch.source_cycles=0u;
        input.entry.pointer.reset();const auto unknown=ProjectClockSingleStep(image,input);Require(unknown.checkpoints.empty(),"unknown startup crossed clock");
        input.entry.pointer=0x817E54E0u;unsigned mutations=0;
        const std::pair<std::uint32_t,std::uint32_t> ranges[]={{0x80379628u,0x8037963Cu},{0x80379648u,0x803796A8u},{0x8037611Cu,0x8037612Cu},{0x80376144u,0x80376164u},{0x80370EA8u,0x80370EB8u},{0x80373AB4u,0x80373AF0u},{0x80376EBCu,0x80376ECCu}};
        for(auto range:ranges)for(auto pc=range.first;pc<=range.second;pc+=4u) {
            auto changed=raw;std::size_t off=0;
            for(unsigned n=0;n<18;++n) {
                const auto get=[&](std::size_t o){std::uint32_t v=0;for(unsigned k=0;k<4;++k)v=(v<<8u)|raw[o+k];return v;};
                const auto a=get(0x48u+4u*n),size=get(0x90u+4u*n);
                if(pc>=a&&std::uint64_t(pc)+4u<=std::uint64_t(a)+size){off=get(4u*n)+pc-a;break;}
            }
            Require(off!=0u,"raw mutation mapping missing");changed[off+3]^=1u;
            Reject([&]{ProjectClockSingleStep(LoadValidatedFixture(changed),input);});++mutations;
        }
        Require(mutations==71u,"clock raw coverage incomplete");
        std::cout<<"Time-base source phase/rollover and "<<cases<<" carry cases passed; research cannot advance boot\n";
        return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
