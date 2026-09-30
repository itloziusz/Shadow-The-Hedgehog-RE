#include "si_sampling_row.h"
#include <fstream>
#include <iostream>
#include <iterator>
static void Require(bool v) {if(!v)throw BootError("sampling row regression");}
static void Reject(const auto& fn) {bool rejected=false;try{fn();}catch(const BootError&){rejected=true;}Require(rejected);}
static std::vector<std::uint8_t> Read(const char* path) {
    std::ifstream input(path,std::ios::binary);Require(bool(input));return {std::istreambuf_iterator<char>(input),{}};
}
int main(int argc,char** argv) {
    try {
        Require(argc==4);const auto dol=Read(argv[1]);const auto bi2=Read(argv[3]);
        const auto image=LoadValidatedFixture(dol);
        const auto hw=InitializeNativeHardwareSemantics(LoadPreEntryOracle(argv[2]));
        const auto crt=InitializeNativeCrt(image,hw);
        auto selector=EarlySiSelector::FromValidatedCrt(image,crt);const auto selection=selector.ConsumeInitialFamily();
        auto clock=StartupSamplingClock::FromPalBootMetadata(bi2);const auto band=clock.ConsumeInitialBand();
        InitialSiSamplingRow absent;Reject([&]{(void)absent.Consume();});
        Reject([&]{(void)InitialSiSamplingRow::FromPalData({},selection,0);});
        for(auto offset:{0x566a60u,0x566a61u,0x566a62u}) {
            auto changed=dol;changed[offset]^=1;Reject([&]{(void)InitialSiSamplingRow::FromPalData(changed,selection,0);});
        }
        for(auto index:{1u,11u,12u})Reject([&]{(void)InitialSiSamplingRow::FromPalData(dol,selection,index);});
        auto wrong=selection;wrong.family=1;Reject([&]{(void)InitialSiSamplingRow::FromPalData(dol,wrong,0);});
        for(auto x:{0u,246u,32768u,65535u})for(auto y:{0u,2u,128u,255u}) {
            const SiSamplingFields fields{static_cast<std::uint16_t>(x),static_cast<std::uint8_t>(y)};
            Require(SiSamplingContribution(fields,SamplingClockBand::MHz27)==((x<<16)|(y<<8)));
            Require(SiSamplingContribution(fields,SamplingClockBand::MHz54)==(((2u*x)<<16)|(y<<8)));
        }
        Reject([&]{(void)SiSamplingContribution({246,2},static_cast<SamplingClockBand>(0));});
        auto row=InitialSiSamplingRow::FromPalData(dol,selection,0);const auto fields=row.Consume();
        Require(fields.base_x==246 && fields.polling_y==2);Reject([&]{(void)row.Consume();});
        std::cout<<"READ_SAMPLING_BYTE 80569a62 1 "<<unsigned(fields.polling_y)<<'\n';
        std::cout<<"SCALED_X "<<ScaleSiSamplingBase(fields.base_x,band)<<'\n';
        std::cout<<"UNEXECUTED_PACKING_CONTRIBUTION "<<SiSamplingContribution(fields,band)<<'\n';
        std::cout<<"SLICE_STOP 803b71f4 UNPROVEN_HELPER 803b669c INPUT 80569990 READ 803b66c4\n";
        std::cout<<"POLLING_CONTROL UNKNOWN\nNATIVE_BOOT_PC 800510c0 SI_BLOCKED\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
