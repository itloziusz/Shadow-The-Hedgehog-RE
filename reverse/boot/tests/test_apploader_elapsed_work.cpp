#include "shadow/boot/ApploaderElapsedWork.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
void Require(bool condition,const char* reason) { if(!condition) throw std::runtime_error(reason); }
template<class F> void Reject(F action,const char* reason) {
    try { action(); } catch(const std::exception&) { return; }
    throw std::runtime_error(reason);
}
std::vector<std::uint8_t> Read(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);
    Require(bool(file),"original apploader fixture unavailable");
    return {std::istreambuf_iterator<char>{file},{}};
}
}

int main(int argc,char** argv) {
    using namespace shadow::boot;
    try {
        Require(argc==2,"PAL DOL fixture path required; siblings boot/BI2/apploader are read only");
        const std::filesystem::path path(argv[1]);
        const auto dol=Read(path), app=Read(path.parent_path()/"apploader.img");
        const auto boot=Read(path.parent_path()/"boot.bin"), bi2=Read(path.parent_path()/"bi2.bin");
        Require(dol.size()>=256,"DOL header unavailable");
        const std::vector<std::uint8_t> header(dol.begin(),dol.begin()+256);
        const auto image=LoadValidatedFixture(dol);
        const auto p=FreshGcHleApploaderSourceInitialization();
        const auto work=DeriveApploaderElapsedWork(image,app,boot,bi2,header,p);
        // Independent original per-callback captures are validation outputs,
        // never arguments to the candidate's analytical production.
        const std::array<std::uint64_t,16> captured{{98382,98390,99200,98423,
            334052,553816,99234,98992,98521,98416,142288,130114,102520,102199,107942,98405}};
        Require(work.entry_steps==98726 && work.init_steps==98541 && work.close_steps==98332,
                "entry/init/close differ from independent capture");
        Require(work.main_steps.size()==captured.size(),"callback topology differs");
        for(std::size_t i=0;i<captured.size();++i)
            Require(work.main_steps[i]==captured[i],"Main path differs from independent capture");
        Require(work.callback_count==19 && work.return_thunk_bytes==76 && work.return_thunk_steps==98320,
                "copied return-thunk work missing");
        Require(work.footprint_steps==221 && work.each_bounds_check_steps==319
                && work.bss_fill_steps==179871 && work.bss_flush_steps==53962 && work.state6_steps==234751,
                "raw loop or devkit two-check branch work differs");
        Require(work.total_steps==2656493,"analytical sum differs from independent capture");

        unsigned negatives=0;
        // Mutate copy direction, optimized fill body, wrapper, return thunk,
        // state dispatch, devkit branch, descriptor loop, syscall, F0 reader,
        // and loaded trailer writer. Every original byte is an authority gate.
        for(const auto pc:std::array<std::uint32_t,10>{{0x812000f8u,0x812000b4u,0x81200264u,
            0x81200234u,0x8120081cu,0x81200ce4u,0x812006b0u,0x812011b8u,0x81201234u,0x81205bf0u}}) {
            auto changed=app;changed.at(32u+pc-0x81200000u+3u)^=1u;
            Reject([&]{DeriveApploaderElapsedWork(image,changed,boot,bi2,header,p);},"mutated apploader admitted");++negatives;
        }
        for(const auto offset:std::array<std::size_t,4>{{0x90u,0x98u,0xd8u,0xdcu}}) {
            auto changed=header;changed.at(offset+3)^=1u;
            Reject([&]{DeriveApploaderElapsedWork(image,app,boot,bi2,changed,p);},"mutated descriptor/BSS admitted");++negatives;
        }
        for(const auto offset:std::array<std::size_t,3>{{0x420u,0x424u,0x428u}}) {
            auto changed=boot;changed.at(offset+3)^=1u;
            Reject([&]{DeriveApploaderElapsedWork(image,app,changed,bi2,header,p);},"mutated request/layout header admitted");++negatives;
        }
        for(const auto offset:std::array<std::size_t,3>{{0u,4u,0x28u}}) {
            auto changed=bi2;changed.at(offset+3)^=1u;
            Reject([&]{DeriveApploaderElapsedWork(image,app,boot,changed,header,p);},"mutated BI2 branch admitted");++negatives;
        }
        auto shorter=app;shorter.pop_back();
        Reject([&]{DeriveApploaderElapsedWork(image,shorter,boot,bi2,header,p);},"truncated image admitted");++negatives;
        for(unsigned field=0;field<10;++field) {
            auto bad=p;
            switch(field) {
                case 0:bad.memory_owner=ApploaderSdkInitialization::MemoryOwner::Unknown;break;
                case 1:bad.step_policy=ApploaderSdkInitialization::StepPolicy::Unknown;break;
                case 2:bad.physical_ram_bytes=0x01000000u;break;
                case 3:bad.console_type=0u;break; // different one-check branch
                case 4:bad.si_poll|=0x10u;break; // opens extra SI reads
                case 5:bad.syscall_handler_word=0x60000000u;break;
                case 6:bad.report_stub_word=0x60000000u;break;
                case 7:bad.original_disc_without_patches=false;break;
                case 8:bad.no_additional_guest_paths_or_exceptions=false;break;
                case 9:bad.si_poll=0u;break; // known branch agreement still lacks selected producer
            }
            Reject([&]{DeriveApploaderElapsedWork(image,app,boot,bi2,header,bad);},"unresolved SDK profile admitted");++negatives;
        }
        std::cout<<"Source-conditioned apploader research: Entry="<<work.entry_steps
                 <<" Init="<<work.init_steps<<" Main=";
        for(std::size_t i=0;i<work.main_steps.size();++i) std::cout<<(i?",":"")<<work.main_steps[i];
        std::cout<<" Close="<<work.close_steps<<" total="<<work.total_steps
                 <<"; "<<negatives<<" negative gates passed; event ownership unresolved\n";
        return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
