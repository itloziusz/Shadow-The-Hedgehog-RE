#include "shadow/boot/NativeCrtPrefix.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <algorithm>

namespace {
void Require(bool value,const char* reason) {if(!value)throw std::runtime_error(reason);}
template<class F>void Reject(F f,const char* reason) {try{f();}catch(const std::exception&){return;}throw std::runtime_error(reason);}
std::uint32_t Word(const std::vector<std::uint8_t>& raw,std::size_t offset) {
    std::uint32_t v=0;for(unsigned b=0;b<4;++b)v=(v<<8)|raw.at(offset+b);return v;
}
const shadow::boot::NativeCrtCheckpoint& At(const shadow::boot::NativeCrtRun& run,std::uint32_t pc) {
    for(const auto& cp:run.checkpoints)if(cp.boot.native.state.machine.cpu.pc==pc)return cp;
    throw std::runtime_error("checkpoint missing");
}
}
int main(int argc,char** argv) {
    using namespace shadow::boot;
    try {
        if(argc!=2)throw std::runtime_error("PAL fixture required");
        std::ifstream file(argv[1],std::ios::binary);
        const std::vector<std::uint8_t> raw(std::istreambuf_iterator<char>{file},{});
        const auto image=LoadValidatedFixture(raw);
        NativeCrtInputs input{};
        input.l2.entry.msr=0x32u;input.l2.entry.hid0=0xC000u;input.l2.entry.source_address=0x805F1F30u;
        input.l2.entry.cr=0xABCDEF12u;input.l2.entry.xer=0xC000007Fu;input.l2.entry.ctr=0x89312456u;
        for(auto enable:{0u,0x80000000u})for(auto value:{0u,1u,0x80370E68u,0xDEADBEEFu,0xFFFFFFFFu}) {
            input.l2.l2cr=enable;input.old_handler=value;
            const auto run=RunImmutableNativeCrtPrefix(image,input);
            const auto& final=run.checkpoints.back();const auto& s=final.boot.native.state;const auto& cpu=s.machine.cpu;
            Require(cpu.pc==0x80003188u&&cpu.gpr[1]==0x8060C5E8u&&cpu.lr==0x80003170u,"CRT return/frame differs");
            Require(cpu.gpr[3]==0x805FC540u&&cpu.gpr[4]==0x805FC5E8u&&cpu.gpr[5]==0u&&cpu.gpr[6]==0x800000F4u&&cpu.gpr[7]==0u,"fill/entry volatile results differ");
            Require(cpu.gpr[0]==0u&&cpu.gpr[31]==0x8000315Cu&&cpu.gpr[30]==0u&&cpu.gpr[29]==0u&&cpu.gpr[28]==0u,"nonvolatile provenance lost");
            Require(s.xer==0xE000007Fu&&s.cr==0x39CDEF12u&&final.boot.native.ctr==input.l2.entry.ctr,"CA/SO/OV/CR/CTR differs");
            Require(At(run,0x80372908u).boot.native.state.machine.cpu.gpr[3]==value,"old handler word forced/treated as callable");
            Require(At(run,0x803733C4u).handler==0x803726D8u&&final.handler==0u&&final.lowmem44==0u,"register/clear/lowmem timeline differs");
            Require(At(run,0x80372914u).boot.stack.LoadBE32(0x8060C5D0u)==0u,"r28 alias replaced with old helper backchain");
            Require(final.boot.stack.LoadBE32(0x8060C5E8u)==0xFFFFFFFFu&&final.boot.stack.LoadBE32(0x8060C5ECu)==0x80003170u,"sentinel overwritten incorrectly");
            Require(run.stores.size()==34u&&run.zero_ranges.size()==3u,"stores/ranges skipped");
            const std::array<std::uint32_t,3> addresses{0x8056FE00u,0x805EF020u,0x805FC540u},sizes{0x74700u,0x375Cu,0xACu};
            for(unsigned n=0;n<3;++n) {
                Require(run.zero_ranges[n].address==addresses[n]&&run.zero_ranges[n].bytes.size()==sizes[n],"blanket BSS or interval error");
                Require(std::all_of(run.zero_ranges[n].bytes.begin(),run.zero_ranges[n].bytes.end(),[](auto b){return b==0;}),"incomplete real fill");
            }
            const auto& prior=run.prefix.checkpoints.back().native;
            for(unsigned f=0;f<32;++f)Require(s.fpr[f].ps0==prior.state.fpr[f].ps0&&s.fpr[f].ps1==prior.state.fpr[f].ps1,"CRT altered FPR lanes");
            Require(s.fpscr==prior.state.fpscr&&final.boot.native.gqr==prior.gqr,"CRT altered floating controls");
        }
        input.old_handler.reset();const auto stopped=RunImmutableNativeCrtPrefix(image,input);
        Require(stopped.checkpoints.back().boot.native.state.machine.cpu.pc==0x803733B4u&&stopped.stores.size()==6u&&stopped.zero_ranges.empty(),"unknown slot propagated forward");
        NativeHandlerSlot slot(std::nullopt);
        Reject([&]{slot.Load(0x80586CB4u);},"unknown slot read");
        Reject([&]{slot.Store(0x80586CB0u,0u);},"wrong registry store address");
        slot.Store(0x80586CB4u,0xDEADBEEFu);
        Require(slot.Load(0x80586CB4u)==0xDEADBEEFu,"registry store not applied");
        Reject([&]{slot.Load(0x80586CB5u);},"unaligned registry address read");
        for(unsigned bit=0;bit<32;++bit)for(auto prior:{false,true}) {
            const auto value=1u<<bit;
            Require(WithoutExternalInterrupts(value)==(value&~0x8000u),"EE clear mask wrong");
            Require(RestoreExternalInterrupts(value,prior)==(prior?value|0x8000u:value&~0x8000u),"EE restoration bit projection wrong");
        }
        input.old_handler=0u;
        const std::array<std::pair<std::uint32_t,std::uint32_t>,10> ranges{{
            {0x80372904u,0x80372928u},{0x80373378u,0x803733C4u},{0x80373564u,0x8037358Cu},
            {0x8037611Cu,0x8037612Cu},{0x80376144u,0x80376164u},{0x8000341Cu,0x80003420u},
            {0x8000315Cu,0x80003188u},{0x80003340u,0x800033FCu},{0x8000540Cu,0x800054F0u},{0x80005544u,0x800055E4u}}};
        unsigned mutations=0;
        for(auto range:ranges)for(auto pc=range.first;pc<=range.second;pc+=4u) {
            auto changed=raw;std::size_t offset=0;
            for(unsigned n=0;n<18;++n) {
                const auto address=Word(raw,0x48+4*n),size=Word(raw,0x90+4*n);
                if(pc>=address&&std::uint64_t(pc)+4<=std::uint64_t(address)+size){offset=Word(raw,4*n)+pc-address;break;}
            }
            Require(offset!=0,"mutation outside DOL");changed[offset+3]^=1u;
            Reject([&]{RunImmutableNativeCrtPrefix(LoadValidatedFixture(changed),input);},"changed raw word silently accepted");++mutations;
        }
        Require(mutations==216u,"mutation coverage lost");
        std::cout<<"PAL native handler/CRT gates passed; 216 raw-word mutations rejected\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
