#include "si_sampling_clock.h"
#include <fstream>
#include <iostream>
#include <iterator>

static void Require(bool v,const char* message) { if(!v) throw BootError(message); }
static void Reject(const auto& action) {
    bool rejected=false;try { action(); } catch(const BootError&) { rejected=true; }
    Require(rejected,"unproven sampling clock accepted");
}
int main(int argc,char** argv) {
    try {
        Require(argc==2,"usage: SiSamplingClockTests original-bi2");
        std::ifstream input(argv[1],std::ios::binary);Require(bool(input),"missing original BI2");
        const std::vector<std::uint8_t> bi2{std::istreambuf_iterator<char>(input),{}};
        StartupSamplingClock missing;Reject([&]{(void)missing.ConsumeInitialBand();});
        Reject([&]{(void)StartupSamplingClock::FromPalBootMetadata({});});
        for(auto offset:{0u,24u,25u,26u,27u,8191u}) {
            auto changed=bi2;changed.at(offset)^=1;
            Reject([&]{(void)StartupSamplingClock::FromPalBootMetadata(changed);});
        }
        for(auto region:{0u,1u,3u,4u,5u,255u}) {
            auto changed=bi2;changed.at(27)=static_cast<std::uint8_t>(region);
            Reject([&]{(void)StartupSamplingClock::FromPalBootMetadata(changed);});
        }
        Require(SamplingMultiplierForBand(SamplingClockBand::MHz27)==1,"wrong27MHz ratio");
        Require(SamplingMultiplierForBand(SamplingClockBand::MHz54)==2,"wrong54MHz ratio");
        for(auto raw:{0u,1u,2u,162000000u,486000000u})
            Reject([&]{(void)SamplingMultiplierForBand(static_cast<SamplingClockBand>(raw));});
        auto clock=StartupSamplingClock::FromPalBootMetadata(bi2);
        const auto band=clock.ConsumeInitialBand();const auto multiplier=SamplingMultiplierForBand(band);
        Require(band==SamplingClockBand::MHz27 && multiplier==1,"PAL metadata clock divergence");
        Reject([&]{(void)clock.ConsumeInitialBand();});
        std::cout<<"CONSUME_SAMPLING_CLOCK_HZ "<<static_cast<std::uint32_t>(band)<<'\n';
        std::cout<<"SAMPLING_MULTIPLIER "<<multiplier<<'\n';
        std::cout<<"SLICE_STOP 803b71e4 UNPROVEN_MEMORY 80569a60 SIZE 2\n";
        std::cout<<"POLLING_CONTROL UNKNOWN\nNATIVE_BOOT_PC 800510c0 SI_BLOCKED\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
