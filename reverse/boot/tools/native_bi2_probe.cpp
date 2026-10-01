#include "shadow/boot/NativeBi2Prefix.hpp"
#include "NativeTraceIO.hpp"
#include <fstream>
#include <iterator>
#include <sstream>

int main(int argc,char** argv) {
    using namespace shadow::boot;
    try {
        if(argc!=3)throw std::runtime_error("usage: native_bi2 <main.dol> <explicit-entry-with-handler-pointer-blob.txt>");
        std::ifstream dol(argv[1],std::ios::binary),input(argv[2]);
        if(!dol||!input)throw std::runtime_error("cannot open explicit fixture");
        const auto image=LoadValidatedFixture(std::vector<std::uint8_t>(std::istreambuf_iterator<char>{dol},{}));
        NativeBi2Inputs data{};data.crt.l2=trace::ReadL2(input);
        const auto optional=[&]() -> std::optional<std::uint32_t> {
            std::string word;if(!(input>>word))throw std::runtime_error("missing explicit optional word");
            if(word=="-")return {};
            if(word.size()!=8||word.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos)
                throw std::runtime_error("invalid optional word");
            std::istringstream field(word);return trace::Read32(field);
        };
        data.crt.old_handler=optional();data.pointer=optional();
        std::string blob;if(!(input>>blob))throw std::runtime_error("missing BI2 blob or unknown marker");
        if(blob!="-") {
            if(blob.size()!=0x4000u||blob.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos)
                throw std::runtime_error("invalid complete BI2 blob");
            for(std::size_t n=0;n<blob.size();n+=2)data.loaded_bi2.push_back(static_cast<std::uint8_t>(std::stoul(blob.substr(n,2),nullptr,16)));
        }
        if(input>>blob)throw std::runtime_error("unexpected extra entry fields");
        const auto run=RunImmutableNativeBi2Prefix(image,data);
        std::cout<<std::hex<<std::setfill('0');
        for(const auto& cp:run.prefix.prefix.prefix.checkpoints) {trace::Print(cp,data.crt.l2.l2cr,nullptr);std::cout<<'\n';}
        for(const auto& cp:run.prefix.prefix.checkpoints) {trace::Print(cp.native,cp.l2cr,&cp.stack);std::cout<<'\n';}
        const auto print_crt=[&](const NativeCrtCheckpoint& cp) {
            trace::Print(cp.boot.native,cp.boot.l2cr,&cp.boot.stack);
            std::cout<<' ';if(cp.handler)std::cout<<std::setw(8)<<*cp.handler;else std::cout<<'-';
            std::cout<<' ';if(cp.lowmem44)std::cout<<std::setw(8)<<*cp.lowmem44;else std::cout<<'-';
            std::cout<<'\n';
        };
        for(const auto& cp:run.prefix.checkpoints)print_crt(cp);
        for(const auto& cp:run.checkpoints) {
            print_crt(cp.boot);const auto pc=cp.boot.boot.native.state.machine.cpu.pc;
            std::cout<<"BI2_MEMORY "<<std::setw(8)<<pc<<' ';
            if(data.pointer)std::cout<<std::setw(8)<<*data.pointer;else std::cout<<'-';
            std::cout<<' ';if(cp.bi2.empty())std::cout<<'-';else for(auto b:cp.bi2)std::cout<<std::setw(2)<<unsigned(b);
            std::cout<<'\n'<<"BI2_GLOBALS "<<std::setw(8)<<pc;
            for(auto word:cp.sda)std::cout<<' '<<std::setw(8)<<word;
            std::cout<<' ';if(cp.arena_high)std::cout<<std::setw(8)<<*cp.arena_high;else std::cout<<'-';
            std::cout<<' ';if(cp.lowmem48)std::cout<<std::setw(8)<<*cp.lowmem48;else std::cout<<'-';
            std::cout<<'\n';
        }
        const auto stores=[&](const auto& list) {for(const auto& s:list)std::cout<<"BOOT_STORE "<<std::setw(8)<<s.pc<<' '<<std::setw(8)<<s.address<<' '<<std::setw(8)<<s.value<<' '<<std::setw(8)<<s.readback<<'\n';};
        stores(run.prefix.stores);stores(run.stores);
        for(const auto& s:run.byte_stores)std::cout<<"BOOT_BYTE_STORE "<<std::setw(8)<<s.pc<<' '<<std::setw(8)<<s.address<<' '<<std::setw(2)<<unsigned(s.value)<<' '<<std::setw(2)<<unsigned(s.readback)<<'\n';
        const auto ranges=[&](const char* label,const auto& list) {
            for(const auto& range:list) {std::cout<<label<<' '<<std::setw(8)<<range.address<<' '<<std::setw(8)<<range.bytes.size()<<' ';for(auto b:range.bytes)std::cout<<std::setw(2)<<unsigned(b);std::cout<<'\n';}
        };
        ranges("ZERO_RANGE",run.prefix.zero_ranges);ranges("CURRENT_GLOBAL_RANGE",run.globals);
        const auto pc=run.checkpoints.empty()?run.prefix.checkpoints.back().boot.native.state.machine.cpu.pc:run.checkpoints.back().boot.boot.native.state.machine.cpu.pc;
        std::cout<<"STOP pc=0x"<<std::setw(8)<<pc<<" reason="
                 <<(pc==0x80379628u?"LIVE_TIME_BASE_UNRESOLVED":pc==0x800031F4u?"CONTEXT_TRANSFER_UNVALIDATED":pc==0x800031A4u?"ARENA_FALLBACK_UNRESOLVED":"LIVE_ENTRY_INPUT_UNRESOLVED")<<'\n';
        return 0;
    }catch(const std::exception& error){std::cerr<<"Native BI2 prefix declined: "<<error.what()<<'\n';return 1;}
}
