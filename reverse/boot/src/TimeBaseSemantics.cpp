#include "shadow/boot/TimeBaseSemantics.hpp"
#include <stdexcept>
#include <initializer_list>

namespace shadow::boot {
namespace {
constexpr std::uint32_t CA=0x20000000u, SO=0x80000000u;
void Check(const BootImage& image,std::uint32_t pc,std::initializer_list<std::uint32_t> words) {
    for(auto w:words) {if(image.ReadWord(pc)!=w)throw std::runtime_error("clock raw word mismatch");pc+=4u;}
}
std::uint32_t Word(const std::vector<std::uint8_t>& b,std::size_t o) {
    if((o&3u)||o+4u>b.size())throw std::runtime_error("clock owner read bounds");
    std::uint32_t v=0;for(unsigned n=0;n<4;++n)v=(v<<8u)|b[o+n];return v;
}
void Put(std::vector<std::uint8_t>& b,std::size_t o,std::uint32_t v) {
    if((o&3u)||o+4u>b.size())throw std::runtime_error("clock owner store bounds");
    for(unsigned n=0;n<4;++n)b[o+n]=static_cast<std::uint8_t>(v>>(24u-8u*n));
}
std::int64_t Signed(std::uint32_t v) {return v<0x80000000u?std::int64_t(v):std::int64_t(v)-0x100000000ll;}
}
std::uint64_t CounterTimeBaseAt(std::uint64_t epoch,std::uint64_t origin_cycles,std::uint64_t cycles) {
    return epoch+(cycles-origin_cycles)/12u;
}
std::uint64_t TimeBaseAt(const CycleTimeBaseEpoch& e,std::uint64_t cycles) {
    if(e.cpu_hz!=486000000u)throw std::runtime_error("unvalidated clock frequency/domain");
    // Unsigned subtraction/addition deliberately preserve the inspected
    // producer's modulo64 behavior, including source counter wrap.
    return CounterTimeBaseAt(std::uint64_t(e.rtc_seconds)*(e.cpu_hz/12u),e.source_cycles,cycles);
}
std::uint32_t CompareSignedHigh(std::uint32_t a,std::uint32_t b,std::uint32_t cr,std::uint32_t xer) {
    return (cr&0x0FFFFFFFu)|(Signed(a)<Signed(b)?0x80000000u:Signed(a)>Signed(b)?0x40000000u:0x20000000u)
        |((xer&SO)?0x10000000u:0u);
}
ClockWordSum AddClockOffset(std::uint64_t tb,std::uint64_t offset,std::uint32_t xer) {
    const auto low=std::uint64_t(std::uint32_t(tb))+std::uint32_t(offset);
    const auto high=(tb>>32u)+(offset>>32u)+(low>>32u);
    return {static_cast<std::uint32_t>(high),static_cast<std::uint32_t>(low),
        (xer&~CA)|((low>>32u)?CA:0u),(xer&~CA)|((high>>32u)?CA:0u)};
}
ClockResearchRun ProjectClockSingleStep(const BootImage& image,const ClockResearchInputs& input) {
    Check(image,0x80379628u,{0x7C6D42E6u,0x7C8C42E6u,0x7CAD42E6u,0x7C032800u,0x4082FFF0u,0x4E800020u});
    Check(image,0x80379648u,{0x7C0802A6u,0x90010004u,0x9421FFE0u,0x93E1001Cu,0x93C10018u,0x93A10014u,0x4BFFCABDu,0x7C7F1B78u,0x4BFFFFC1u,0x3CC08000u,0x80A630DCu,0x800630D8u,0x7FA52014u,0x7FC01914u,0x7FE3FB78u,0x4BFFCAC1u,0x7FA4EB78u,0x7FC3F378u,0x80010024u,0x83E1001Cu,0x83C10018u,0x83A10014u,0x38210020u,0x7C0803A6u,0x4E800020u});
    Check(image,0x8037611Cu,{0x7C6000A6u,0x5464045Eu,0x7C800124u,0x54638FFEu,0x4E800020u});
    Check(image,0x80376144u,{0x2C030000u,0x7C8000A6u,0x4182000Cu,0x60858000u,0x48000008u,0x5485045Eu,0x7CA00124u,0x54838FFEu,0x4E800020u});
    Check(image,0x80370EA8u,{0x908D5A54u,0x906D5A50u,0x4800526Du,0x387F0050u,0x48002BFDu});
    Check(image,0x80373AB4u,{0x7C0802A6u,0x90010004u,0x9421FFF8u,0x3CA08000u,0x808530F0u,0x7C052040u,0x41810010u,0x38A0001Cu,0x4BC91A21u,0x4800000Cu,0x38000000u,0x90030000u,0x8001000Cu,0x38210008u,0x7C0803A6u,0x4E800020u});
    Check(image,0x80376EBCu,{0x7CAC42E6u,0x7CCC42E6u,0x7CE53050u,0x28071124u,0x4180FFF4u});
    ClockResearchRun run{};run.prefix=RunImmutableNativeBi2Prefix(image,input.entry);
    if(run.prefix.checkpoints.empty()||run.prefix.checkpoints.back().boot.boot.native.state.machine.cpu.pc!=0x80379628u)
        return run; // Preserve every earlier unknown-input/context decline.
    if(!input.exceptions||*input.exceptions)throw std::runtime_error("unknown/pending clock exceptions");
    auto live=run.prefix.checkpoints.back().boot;
    auto& b=live.boot;auto& state=b.native.state;auto& cpu=state.machine.cpu;
    run.globals=run.prefix.globals;
    auto cycles=input.frontier_cycles,cached=input.cached_tb;
    // The observed finite step interval must not silently reverse its cycle
    // source or rebase its epoch. General wrap remains testable in TimeBaseAt.
    if(cycles<input.epoch.source_cycles||cycles>UINT64_MAX-128u)throw std::runtime_error("unexplained step cycle extent");
    (void)TimeBaseAt(input.epoch,cycles); // validates the clock domain even when rebased
    const auto sample=[&](){return input.rebased_epoch
        ?CounterTimeBaseAt(*input.rebased_epoch,input.epoch.source_cycles,cycles)
        :TimeBaseAt(input.epoch,cycles);};
    const auto owner=[&](std::uint32_t address)->NativeZeroRange& {
        for(auto& r:run.globals)if(address>=r.address&&std::uint64_t(address)+4u<=std::uint64_t(r.address)+r.bytes.size())return r;
        throw std::runtime_error("clock SDA lacks connected owner");
    };
    const auto global=[&](std::uint32_t a){auto& r=owner(a);return Word(r.bytes,a-r.address);};
    const auto save=[&](std::uint32_t pc) {
        cpu.pc=pc;
        for(unsigned n=0;n<16;++n) {
            auto off=b.native.paired_stack.base+n-b.stack.base;
            b.native.paired_stack.bytes[n]=b.stack.bytes[off];b.native.paired_stack.valid[n]=b.stack.valid[off];
        }
        run.checkpoints.push_back({live,cycles,cached,{global(0x805F1F50u),global(0x805F1F54u)}});
    };
    const auto step=[&](std::uint32_t next,auto effect){effect();++cycles;save(next);};
    const auto nop=[](){};
    const auto stack_store=[&](std::uint32_t pc,std::uint32_t a,std::uint32_t v) {
        b.stack.StoreBE32(a,v);run.stores.push_back({pc,a,v,b.stack.LoadBE32(a)});
    };
    const auto store=[&](std::uint32_t pc,std::uint32_t a,std::uint32_t v) {
        auto& r=owner(a);Put(r.bytes,a-r.address,v);run.stores.push_back({pc,a,v,global(a)});
    };
    unsigned attempts=0;
    do {
        if(++attempts>16u)throw std::runtime_error("stable sample retry bound exhausted");
        step(0x8037962Cu,[&]{cached=sample();cpu.gpr[3]=static_cast<std::uint32_t>(cached>>32u);});
        step(0x80379630u,[&]{cached=sample();cpu.gpr[4]=static_cast<std::uint32_t>(cached);});
        step(0x80379634u,[&]{cached=sample();cpu.gpr[5]=static_cast<std::uint32_t>(cached>>32u);});
        step(0x80379638u,[&]{state.cr=CompareSignedHigh(cpu.gpr[3],cpu.gpr[5],state.cr,state.xer);});
        step(cpu.gpr[3]!=cpu.gpr[5]?0x80379628u:0x8037963Cu,nop);
    }while(cpu.gpr[3]!=cpu.gpr[5]);
    if(cpu.lr!=0x8037966Cu)throw std::runtime_error("clock sample return target unexplained");
    step(cpu.lr,nop);
    step(0x80379670u,[&]{cpu.gpr[6]=0x80000000u;});
    step(0x80379674u,[&]{cpu.gpr[5]=static_cast<std::uint32_t>(input.offset);});
    step(0x80379678u,[&]{cpu.gpr[0]=static_cast<std::uint32_t>(input.offset>>32u);});
    const auto sum=AddClockOffset((std::uint64_t(cpu.gpr[3])<<32u)|cpu.gpr[4],input.offset,state.xer);
    step(0x8037967Cu,[&]{cpu.gpr[29]=sum.low;state.xer=sum.xer_after_low;});
    step(0x80379680u,[&]{cpu.gpr[30]=sum.high;state.xer=sum.xer_after_high;});
    step(0x80379684u,[&]{cpu.gpr[3]=cpu.gpr[31];});
    step(0x80376144u,[&]{cpu.lr=0x80379688u;});
    step(0x80376148u,[&]{state.cr=CompareSignedHigh(cpu.gpr[3],0u,state.cr,state.xer);});
    step(0x8037614Cu,[&]{cpu.gpr[4]=state.machine.msr;});
    if(cpu.gpr[3]) {
        step(0x80376150u,nop);
        step(0x80376154u,[&]{cpu.gpr[5]=RestoreExternalInterrupts(cpu.gpr[4],true);});
        step(0x8037615Cu,nop);
    } else {
        step(0x80376158u,nop);
        step(0x8037615Cu,[&]{cpu.gpr[5]=RestoreExternalInterrupts(cpu.gpr[4],false);});
    }
    step(0x80376160u,[&]{state.machine.msr=cpu.gpr[5];});
    step(0x80376164u,[&]{cpu.gpr[3]=(cpu.gpr[4]>>15u)&1u;});
    step(cpu.lr,nop);
    step(0x8037968Cu,[&]{cpu.gpr[4]=cpu.gpr[29];});
    step(0x80379690u,[&]{cpu.gpr[3]=cpu.gpr[30];});
    step(0x80379694u,[&]{cpu.gpr[0]=b.stack.LoadBE32(cpu.gpr[1]+36u);});
    step(0x80379698u,[&]{cpu.gpr[31]=b.stack.LoadBE32(cpu.gpr[1]+28u);});
    step(0x8037969Cu,[&]{cpu.gpr[30]=b.stack.LoadBE32(cpu.gpr[1]+24u);});
    step(0x803796A0u,[&]{cpu.gpr[29]=b.stack.LoadBE32(cpu.gpr[1]+20u);});
    step(0x803796A4u,[&]{cpu.gpr[1]+=32u;});
    step(0x803796A8u,[&]{cpu.lr=cpu.gpr[0];});
    if(cpu.lr!=0x80370EA8u)throw std::runtime_error("clock frame return target unexplained");
    step(cpu.lr,nop);
    step(0x80370EACu,[&]{store(0x80370EA8u,cpu.gpr[13]+0x5A54u,cpu.gpr[4]);});
    step(0x80370EB0u,[&]{store(0x80370EACu,cpu.gpr[13]+0x5A50u,cpu.gpr[3]);});
    step(0x8037611Cu,[&]{cpu.lr=0x80370EB4u;});
    step(0x80376120u,[&]{cpu.gpr[3]=state.machine.msr;});
    step(0x80376124u,[&]{cpu.gpr[4]=WithoutExternalInterrupts(cpu.gpr[3]);});
    step(0x80376128u,[&]{state.machine.msr=cpu.gpr[4];});
    step(0x8037612Cu,[&]{cpu.gpr[3]=(cpu.gpr[3]>>15u)&1u;});
    step(cpu.lr,nop);
    step(0x80370EB8u,[&]{cpu.gpr[3]=cpu.gpr[31]+0x50u;});
    step(0x80373AB4u,[&]{cpu.lr=0x80370EBCu;});
    step(0x80373AB8u,[&]{cpu.gpr[0]=cpu.lr;});
    step(0x80373ABCu,[&]{stack_store(0x80373AB8u,cpu.gpr[1]+4u,cpu.gpr[0]);});
    step(0x80373AC0u,[&]{auto old=cpu.gpr[1];cpu.gpr[1]-=8u;stack_store(0x80373ABCu,cpu.gpr[1],old);});
    step(0x80373AC4u,[&]{cpu.gpr[5]=0x80000000u;});
    return run; // NEVER read the unprovided 800030F0 pointer.
}
}
