#include "si_polling_state.h"
#include <fstream>
#include <iostream>
#include <iterator>
static void Require(bool b){if(!b)throw BootError("polling state regression");}
static void Reject(const auto& fn){bool b=false;try{fn();}catch(const BootError&){b=true;}Require(b);}
static std::vector<std::uint8_t> Read(const char* p){std::ifstream f(p,std::ios::binary);Require(bool(f));return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){try{
    Require(argc==4);const auto dol=Read(argv[1]);const auto bi2=Read(argv[3]);const auto image=LoadValidatedFixture(dol);
    const auto hw=InitializeNativeHardwareSemantics(LoadPreEntryOracle(argv[2]));
    const auto crt=InitializeNativeCrt(image,hw);auto selector=EarlySiSelector::FromValidatedCrt(image,crt);
    const auto selection=selector.ConsumeInitialFamily();
    auto row=InitialSiSamplingRow::FromPalData(dol,selection,0);auto clock=StartupSamplingClock::FromPalBootMetadata(bi2);
    InitialSiPollingState absent;Reject([&]{(void)absent.UpdateInitial(row,clock);});
    InitialSiPollingState state;state.ResetForSampling(image);Reject([&]{state.ResetForSampling(image);});
    const auto result=state.UpdateInitial(row,clock);Require(result.value==0x00F60200u);
    Reject([&]{(void)state.UpdateInitial(row,clock);});
    auto changed=dol;changed[0x566990]^=1;Reject([&]{InitialSiPollingState s;s.ResetForSampling(LoadValidatedFixture(changed));});
    for(auto previous:{0u,0xffffffffu,0xfc0000ffu,0x03ffff00u,0x80000080u})
    for(auto x:{0u,246u,65535u})for(auto y:{0u,2u,255u})for(auto band:{SamplingClockBand::MHz27,SamplingClockBand::MHz54}){
        const auto update=UpdatePollingFields(previous,{static_cast<std::uint16_t>(x),static_cast<std::uint8_t>(y)},band);
        const auto retained=previous&0xfc0000ffu;
        const auto expected=retained|((x*(band==SamplingClockBand::MHz27?1u:2u))<<16)|(y<<8);
        Require(update.value==expected);
        Require(update.effects[0].value==previous && !update.effects[0].write);
        Require(update.effects[1].value==retained && update.effects[1].write);
        Require(update.effects[2].value==retained && !update.effects[2].write);
        Require(update.effects[3].value==expected && update.effects[3].write);
        Require(update.effects[4].value==expected && !update.effects[4].write);
    }
    Reject([&]{(void)UpdatePollingFields(0,{246,2},static_cast<SamplingClockBand>(0));});
    std::cout<<"RESET_POLLING 0\n";
    for(const auto& e:result.effects)std::cout<<(e.write?"WRITE_POLLING ":"READ_POLLING ")<<e.value<<'\n';
    std::cout<<"POLLING_CONTROL "<<result.value<<"\nSLICE_STOP 803b66e8 UNPROVEN_DEVICE_PUBLICATION cc006430\nNATIVE_BOOT_PC 800510c0 SI_BLOCKED\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
