#include "si_sampling_parameter.h"
#include "contract_tests.h"
#include <fstream>
#include <iostream>
#include <iterator>

static void Require(bool v,const char* message) {if(!v) throw BootError(message);}
static void Reject(const auto& action) {
    bool rejected=false;try {action();} catch(const BootError&) {rejected=true;}
    Require(rejected,"unproven sampling parameter accepted");
}
static std::vector<std::uint8_t> Read(const char* path) {
    std::ifstream input(path,std::ios::binary);Require(bool(input),"missing original input");
    return {std::istreambuf_iterator<char>(input),{}};
}
int main(int argc,char** argv) {
    try {
        Require(argc==4,"usage: SiSamplingParameterTests DOL pre-entry-oracle BI2");
        const auto dol=Read(argv[1]);const auto bi2=Read(argv[3]);
        auto image=LoadValidatedFixture(dol);
        const auto hardware=InitializeNativeHardwareSemantics(LoadPreEntryOracle(argv[2]));
        const auto crt=InitializeNativeCrt(image,hardware);
        auto selector=EarlySiSelector::FromValidatedCrt(image,crt);
        const auto selection=selector.ConsumeInitialFamily();
        auto clock=StartupSamplingClock::FromPalBootMetadata(bi2);const auto band=clock.ConsumeInitialBand();
        InitialSiSamplingParameter absent;Reject([&]{(void)absent.ConsumeBaseInterval();});
        Reject([&]{(void)InitialSiSamplingParameter::FromPalData({},selection,0);});
        for(auto index:{1u,11u,12u,0xffffffffu})
            Reject([&]{(void)InitialSiSamplingParameter::FromPalData(dol,selection,index);});
        auto wrong=selection;wrong.family=1;
        Reject([&]{(void)InitialSiSamplingParameter::FromPalData(dol,wrong,0);});
        wrong=selection;wrong.selector=1;
        Reject([&]{(void)InitialSiSamplingParameter::FromPalData(dol,wrong,0);});
        for(auto offset:{0x566a60u,0x566a61u}) {
            auto changed=dol;changed.at(offset)^=1;
            Reject([&]{(void)InitialSiSamplingParameter::FromPalData(changed,selection,0);});
        }
        for(auto value:{0u,1u,246u,32768u,65535u}) {
            const auto base=static_cast<std::uint16_t>(value);
            Require(ScaleSiSamplingBase(base,SamplingClockBand::MHz27)==value,"wrong scale1");
            Require(ScaleSiSamplingBase(base,SamplingClockBand::MHz54)==2u*value,"wrong unsigned scale2");
        }
        Reject([&]{(void)ScaleSiSamplingBase(246,static_cast<SamplingClockBand>(0));});
        auto parameter=InitialSiSamplingParameter::FromPalData(dol,selection,0);
        const auto base=parameter.ConsumeBaseInterval();Require(base==246,"wrong PAL DOL parameter");
        Reject([&]{(void)parameter.ConsumeBaseInterval();});
        std::cout<<"READ_SAMPLING_BASE 80569a60 2 "<<base<<'\n';
        std::cout<<"UNEXECUTED_ARITHMETIC_PROJECTION "<<ScaleSiSamplingBase(base,band)<<'\n';
        std::cout<<"SLICE_STOP 803b71ec UNPROVEN_MEMORY 80569a62 SIZE 1\n";
        std::cout<<"POLLING_CONTROL UNKNOWN\nNATIVE_BOOT_PC 800510c0 SI_BLOCKED\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
