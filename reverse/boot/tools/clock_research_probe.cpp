#include "shadow/boot/TimeBaseSemantics.hpp"
#include "NativeTraceIO.hpp"
#include <fstream>
#include <iterator>
#include <sstream>

int main(int argc,char** argv) {
    using namespace shadow::boot;
    try {
        if(argc!=11)throw std::runtime_error("usage: clock_research <dol> <bi2-entry> <rtc-seconds> <cpu-hz> <epoch-cycles> <frontier-cycles> <offset> <cached-tb> <exceptions> research-only");
        std::ifstream dol(argv[1],std::ios::binary),input(argv[2]);
        if(!dol||!input)throw std::runtime_error("explicit research files unavailable");
        const auto image=LoadValidatedFixture(std::vector<std::uint8_t>(std::istreambuf_iterator<char>{dol},{}));
        ClockResearchInputs data{};data.entry.crt.l2=trace::ReadL2(input);
        const auto number=[&](const std::string& text,std::size_t width) {
            if(text.size()!=width||text.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos)
                throw std::runtime_error("malformed exact-width research field");
            std::istringstream v(text);return trace::Read(v);
        };
        const auto optional=[&]() -> std::optional<std::uint32_t> {
            std::string w;if(!(input>>w))throw std::runtime_error("missing optional field");
            if(w=="-")return {};return static_cast<std::uint32_t>(number(w,8));
        };
        data.entry.crt.old_handler=optional();data.entry.pointer=optional();
        std::string blob;if(!(input>>blob))throw std::runtime_error("missing BI2 bytes");
        if(blob!="-") {
            if(blob.size()!=0x4000u)throw std::runtime_error("partial BI2 bytes");
            for(std::size_t n=0;n<blob.size();n+=2)data.entry.loaded_bi2.push_back(static_cast<std::uint8_t>(number(blob.substr(n,2),2)));
        }
        if(input>>blob)throw std::runtime_error("extra fixture field");
        const std::string mode=argv[10];
        if(mode=="research-rebased-epoch")data.rebased_epoch=number(argv[3],16);
        else if(mode=="research-only")data.epoch.rtc_seconds=static_cast<std::uint32_t>(number(argv[3],8));
        else throw std::runtime_error("explicit research-only mode required");
        data.epoch.cpu_hz=static_cast<std::uint32_t>(number(argv[4],8));
        data.epoch.source_cycles=number(argv[5],16);data.frontier_cycles=number(argv[6],16);
        data.offset=number(argv[7],16);data.cached_tb=number(argv[8],16);
        data.exceptions=static_cast<std::uint32_t>(number(argv[9],8));

        const auto run=ProjectClockSingleStep(image,data);
        std::cout<<std::hex<<std::setfill('0');
        for(const auto& cp:run.checkpoints) {
            trace::Print(cp.boot.boot.native,cp.boot.boot.l2cr,&cp.boot.boot.stack);
            std::cout<<' '<<std::setw(8)<<*cp.boot.handler<<' '<<std::setw(8)<<*cp.boot.lowmem44<<'\n';
            std::cout<<"CLOCK_RESEARCH "<<std::setw(8)<<cp.boot.boot.native.state.machine.cpu.pc
                <<' '<<std::setw(16)<<cp.cycles<<' '<<std::setw(16)<<cp.cached_tb;
            for(auto v:cp.clock_global)std::cout<<' '<<std::setw(8)<<v;
            std::cout<<'\n';
        }
        for(auto e:run.stores)std::cout<<"BOOT_STORE "<<std::setw(8)<<e.pc<<' '<<std::setw(8)<<e.address<<' '<<std::setw(8)<<e.value<<' '<<std::setw(8)<<e.readback<<'\n';
        for(const auto& r:run.globals) {
            std::cout<<"RESEARCH_GLOBAL_RANGE "<<std::setw(8)<<r.address<<' '<<std::setw(8)<<r.bytes.size()<<' ';
            for(auto b:r.bytes)std::cout<<std::setw(2)<<unsigned(b);std::cout<<'\n';
        }
        const auto pc=run.checkpoints.empty()?run.prefix.checkpoints.back().boot.boot.native.state.machine.cpu.pc:run.checkpoints.back().boot.boot.native.state.machine.cpu.pc;
        std::cout<<"RESEARCH_STOP pc=0x"<<std::setw(8)<<pc<<" reason=NO_NATIVE_CLOCK_PROVIDER\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"Clock research declined: "<<e.what()<<'\n';return 1;}
}
