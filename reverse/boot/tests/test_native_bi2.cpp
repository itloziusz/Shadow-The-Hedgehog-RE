#include "shadow/boot/NativeBi2Prefix.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <algorithm>

namespace {
void Require(bool v,const char* why){if(!v)throw std::runtime_error(why);}
template<class F>void Reject(F f,const char* why){try{f();}catch(const std::exception&){return;}throw std::runtime_error(why);}
std::uint32_t Word(const std::vector<std::uint8_t>& b,std::size_t o){std::uint32_t v=0;for(unsigned n=0;n<4;++n)v=(v<<8)|b.at(o+n);return v;}
void Put(std::vector<std::uint8_t>& b,std::size_t o,std::uint32_t v){for(unsigned n=0;n<4;++n)b.at(o+n)=static_cast<std::uint8_t>(v>>(24-8*n));}
const shadow::boot::NativeBi2Checkpoint& At(const shadow::boot::NativeBi2Run& r,std::uint32_t pc){for(const auto& cp:r.checkpoints)if(cp.boot.boot.native.state.machine.cpu.pc==pc)return cp;throw std::runtime_error("missing BI2 checkpoint");}
}
int main(int argc,char** argv){
    using namespace shadow::boot;
    try{
        if(argc!=2)throw std::runtime_error("PAL fixture required");
        std::ifstream file(argv[1],std::ios::binary);const std::vector<std::uint8_t> raw(std::istreambuf_iterator<char>{file},{});
        const auto image=LoadValidatedFixture(raw);
        NativeBi2Inputs input{};input.crt.l2.entry.msr=0x32u;input.crt.l2.entry.hid0=0xC000u;
        input.crt.l2.entry.source_address=0x805F1F30u;input.crt.l2.entry.xer=0xC000007Fu;
        input.crt.l2.entry.cr=0xABCDEF12u;input.crt.l2.entry.ctr=0x89312456u;input.crt.old_handler=0xDEADBEEFu;
        input.pointer=0x817E54E0u;input.loaded_bi2.resize(0x2000u);
        for(auto debug:{0u,1u,4u,5u,0xFFFFFFFFu})for(auto enabled:{0u,0x80000000u}){
            Put(input.loaded_bi2,12u,debug);input.crt.l2.l2cr=enabled;
            const auto run=RunImmutableNativeBi2Prefix(image,input);const auto& cp=run.checkpoints.back();
            const auto& boot=cp.boot.boot;const auto& s=boot.native.state;const auto& c=s.machine.cpu;
            Require(c.pc==0x80379628u&&c.lr==0x8037966Cu&&c.gpr[1]==0x8060C5B0u,"clock entry/frame differs");
            Require(c.gpr[0]==0x80370EA8u&&c.gpr[3]==0u&&c.gpr[4]==0x2032u&&c.gpr[31]==0u&&c.gpr[30]==0x805610F8u,"clock prologue/EE provenance differs");
            Require(c.gpr[5]==*input.pointer&&c.gpr[6]==0u&&c.gpr[7]==debug&&c.gpr[14]==0u&&c.gpr[15]==0u,"BI2 skip/unsigned debug result differs");
            Require(s.xer==0xE000007Fu&&s.cr==0x39CDEF12u&&boot.native.ctr==input.crt.l2.entry.ctr,"CR/SO/CA/CTR clobbered");
            Require(cp.sda==std::array<std::uint32_t,4>{0x80000040u,1u,1u,debug==4u?0x01000000u:0u},"SDA aliases/byte width differs");
            Require(cp.lowmem48==0x00370C60u&&!cp.arena_high,"metadata/arena values invented");
            Require(boot.stack.LoadBE32(0x8060C5ECu)==0x80003268u&&boot.stack.LoadBE32(0x8060C5D4u)==0x80370EA8u,"actual saved return bytes differ");
            Require(boot.stack.LoadBE32(0x8060C5CCu)==0x80586C40u&&boot.stack.LoadBE32(0x8060C5C8u)==0x805610F8u,"clock spill alias differs");
            Require(run.stores.size()==14u&&run.byte_stores.size()==(debug==4u?1u:0u),"ordered side effects missing");
            Require(run.bi2==input.loaded_bi2,"read-only BI2 branch mutated blob");
            const auto& prior=run.prefix.checkpoints.back().boot.native;
            for(unsigned f=0;f<32;++f)Require(s.fpr[f].ps0==prior.state.fpr[f].ps0&&s.fpr[f].ps1==prior.state.fpr[f].ps1,"FP state altered");
            Require(s.fpscr==prior.state.fpscr&&boot.native.hid0==prior.hid0&&boot.native.gqr==prior.gqr,"old CPU proofs regressed");
            const auto& current=run.globals[1];Require(Word(current.bytes,0x805F1F40u-current.address)==1u,"current storage disagrees with guard view");
            Require(Word(run.prefix.zero_ranges[1].bytes,0x805F1F40u-current.address)==0u,"earlier fill snapshot was retroactively changed");
        }
        input.loaded_bi2.assign(0x2000u,0u);Put(input.loaded_bi2,8,0x100u);
        for(auto count:{0u,1u,3u,7u}){
            Put(input.loaded_bi2,0x100,count);
            for(unsigned n=0;n<count;++n)Put(input.loaded_bi2,0x104+4*n,n%2u?0xFFFFFFFFu:0x200u+n);
            const auto run=RunImmutableNativeBi2Prefix(image,input);const auto& cp=run.checkpoints.back();const auto& c=cp.boot.boot.native.state.machine.cpu;
            Require(c.gpr[6]==*input.pointer+0x100u+4u*count,"relocation pointer advancement differs");
            Require(c.gpr[14]==count&&c.gpr[15]==(count?*input.pointer+0x104u:0u),"array/count result differs");
            Require(cp.boot.boot.native.ctr==(count?0u:input.crt.l2.entry.ctr),"CTR skip/decrement differs");
            if(count){Require(cp.arena_high==((*input.pointer+0x104u)&0xFFFFFFE0u),"arena is not aligned array START");Require(c.gpr[5]==0x80000034u,"arena address producer differs");}
            else Require(!cp.arena_high&&c.gpr[5]==*input.pointer,"zero-count invented arena state");
            for(unsigned n=0;n<count;++n)Require(Word(run.bi2,0x104+4*n)==Word(input.loaded_bi2,0x104+4*n)+*input.pointer,"word addition must wrap modulo32");
        }
        // An array overlapping the offset/debug words must update one owner.
        input.loaded_bi2.assign(0x2000u,0u);Put(input.loaded_bi2,8,8u);Put(input.loaded_bi2,12,4u);
        const auto alias=RunImmutableNativeBi2Prefix(image,input);
        Require(Word(alias.bi2,8)==8u&&Word(alias.bi2,12)==4u+*input.pointer,"same-owner BI2 alias lost");
        Require(alias.byte_stores.size()==1u&&alias.checkpoints.back().boot.boot.native.state.machine.cpu.gpr[14]==8u,"debug/offset reread after relocation");
        for(auto debug:{2u,3u}){
            input.loaded_bi2.assign(0x2000u,0u);Put(input.loaded_bi2,12,debug);
            const auto run=RunImmutableNativeBi2Prefix(image,input);const auto& c=run.checkpoints.back().boot.boot.native.state.machine.cpu;
            Require(c.pc==0x800031F4u&&c.lr==0x8039F8E0u&&c.gpr[6]==0x8039F8E0u&&c.gpr[5]==debug-2u,"debug context call skipped/forced");
            Require(run.stores.empty(),"unvalidated debug context propagated");
        }
        input.loaded_bi2.assign(0x2000u,0u);
        for(auto bad:{0x817E54E1u,0xFFFFFFFCu,0x80003100u,0x805EF020u,0x8060C580u,0x800000F4u}){input.pointer=bad;Reject([&]{RunImmutableNativeBi2Prefix(image,input);},"unproven mapping admitted");}
        input.pointer=0x817E54E0u;
        for(auto offset:{0xFFFFFFFFu,0x2000u,0x101u}){Put(input.loaded_bi2,8,offset);Reject([&]{RunImmutableNativeBi2Prefix(image,input);},"unproven table pointer consumed");}
        Put(input.loaded_bi2,8,0x1FFCu);Put(input.loaded_bi2,0x1FFC,1u);Reject([&]{RunImmutableNativeBi2Prefix(image,input);},"out-of-owner table extent");
        input.loaded_bi2.clear();Require(RunImmutableNativeBi2Prefix(image,input).checkpoints.back().boot.boot.native.state.machine.cpu.pc==0x80003194u,"unknown blob consumed");
        input.loaded_bi2.resize(4u);Reject([&]{RunImmutableNativeBi2Prefix(image,input);},"partial blob admitted");
        input.loaded_bi2.clear();input.pointer=0u;Require(RunImmutableNativeBi2Prefix(image,input).checkpoints.back().boot.boot.native.state.machine.cpu.pc==0x800031A4u,"null pointer fallback bypassed");
        input.pointer.reset();Require(RunImmutableNativeBi2Prefix(image,input).checkpoints.back().boot.boot.native.state.machine.cpu.pc==0x80003188u,"unknown pointer became concrete");
        input.pointer=0x817E54E0u;input.loaded_bi2.assign(0x2000u,0u);
        unsigned mutations=0;for(auto range:std::array<std::pair<std::uint32_t,std::uint32_t>,6>{{{0x80003188u,0x80003264u},{0x80003140u,0x80003148u},{0x80370BF0u,0x80370C14u},{0x80370E68u,0x80370EA4u},{0x80379648u,0x80379668u},{0x80379628u,0x80379628u}}})for(auto pc=range.first;pc<=range.second;pc+=4u){
            auto changed=raw;std::size_t off=0;for(unsigned n=0;n<18;++n){const auto a=Word(raw,0x48+4*n),size=Word(raw,0x90+4*n);if(pc>=a&&std::uint64_t(pc)+4<=std::uint64_t(a)+size){off=Word(raw,4*n)+pc-a;break;}}
            Require(off!=0,"mutation mapping unavailable");changed[off+3]^=1u;Reject([&]{RunImmutableNativeBi2Prefix(LoadValidatedFixture(changed),input);},"raw mutation admitted");++mutations;
        }
        Require(mutations==95u,"raw coverage lost");std::cout<<"PAL native BI2/OS prefix gates passed; 95 raw-word mutations rejected\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
