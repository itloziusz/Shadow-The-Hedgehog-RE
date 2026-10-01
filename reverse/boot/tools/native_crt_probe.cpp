#include "shadow/boot/NativeCrtPrefix.hpp"
#include "NativeTraceIO.hpp"
#include <fstream>
#include <iterator>
#include <sstream>

int main(int argc,char** argv) {
    using namespace shadow::boot;
    try {
        if(argc!=3)throw std::runtime_error("usage: native_crt <main.dol> <explicit-entry-with-l2-handler.txt>");
        std::ifstream dol(argv[1],std::ios::binary),input(argv[2]);
        if(!dol||!input)throw std::runtime_error("cannot open explicit fixture");
        const auto image=LoadValidatedFixture(std::vector<std::uint8_t>(std::istreambuf_iterator<char>{dol},{}));
        NativeCrtInputs data{trace::ReadL2(input),std::nullopt};
        std::string word;
        if(!(input>>word))throw std::runtime_error("missing explicit handler word or unknown marker");
        if(word!="-") {
            if(word.size()!=8||word.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos)
                throw std::runtime_error("invalid handler word");
            std::istringstream field(word);data.old_handler=trace::Read32(field);
        }
        if(input>>word)throw std::runtime_error("unexpected extra entry fields");
        const auto run=RunImmutableNativeCrtPrefix(image,data);
        std::cout<<std::hex<<std::setfill('0');
        for(const auto& cp:run.prefix.prefix.checkpoints) {
            trace::Print(cp,data.l2.l2cr,nullptr);std::cout<<'\n';
        }
        for(const auto& cp:run.prefix.checkpoints) {
            trace::Print(cp.native,cp.l2cr,&cp.stack);std::cout<<'\n';
        }
        for(const auto& cp:run.checkpoints) {
            trace::Print(cp.boot.native,cp.boot.l2cr,&cp.boot.stack);
            std::cout<<' ';if(cp.handler)std::cout<<std::setw(8)<<*cp.handler;else std::cout<<'-';
            std::cout<<' ';if(cp.lowmem44)std::cout<<std::setw(8)<<*cp.lowmem44;else std::cout<<'-';
            std::cout<<'\n';
        }
        for(const auto& store:run.stores)
            std::cout<<"BOOT_STORE "<<std::setw(8)<<store.pc<<' '<<std::setw(8)<<store.address
                     <<' '<<std::setw(8)<<store.value<<' '<<std::setw(8)<<store.readback<<'\n';
        for(const auto& range:run.zero_ranges) {
            std::cout<<"ZERO_RANGE "<<std::setw(8)<<range.address<<' '<<std::setw(8)<<range.bytes.size()<<' ';
            for(auto byte:range.bytes)std::cout<<std::setw(2)<<unsigned(byte);
            std::cout<<'\n';
        }
        const auto pc=run.checkpoints.back().boot.native.state.machine.cpu.pc;
        std::cout<<"STOP pc=0x"<<std::setw(8)<<pc<<" reason="
                 <<(pc==0x803733B4u?"LIVE_HANDLER_SLOT_UNRESOLVED":"LIVE_BI2_POINTER_UNRESOLVED")<<'\n';
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"Native CRT prefix declined: "<<error.what()<<'\n';return 1;
    }
}
