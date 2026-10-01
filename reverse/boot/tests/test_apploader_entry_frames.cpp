#include "shadow/boot/ApploaderEntryFrames.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
void Require(bool condition, const char* why) { if (!condition) throw std::runtime_error(why); }
template<class F> void Reject(F action, const char* why) {
    try { action(); } catch (const std::exception&) { return; }
    throw std::runtime_error(why);
}
std::vector<std::uint8_t> Read(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);
    Require(bool(file),"read-only original fixture unavailable");
    return {std::istreambuf_iterator<char>{file},{}};
}
std::string Bytes(const std::array<std::uint8_t,64>& bytes) {
    std::ostringstream s; s<<std::hex<<std::setfill('0');
    for (auto b:bytes) s<<std::setw(2)<<unsigned(b);
    return s.str();
}
void Snapshot(std::ostream& out, const shadow::boot::ApploaderEntryFrameState& s) {
    const auto& c=s.cpu;
    out<<"{\"pc\":"<<c.pc<<",\"npc\":"<<c.npc<<",\"r0\":"<<c.gpr[0]
       <<",\"r1\":"<<c.gpr[1]<<",\"lr\":"<<c.lr<<",\"msr\":"<<c.msr
       <<",\"exceptions\":"<<c.exceptions<<",\"cr\":"<<c.cr<<",\"xer\":"<<c.xer
       <<",\"ctr\":"<<c.ctr<<",\"fpscr\":"<<c.fpscr
       <<",\"hid0\":"<<c.hid0<<",\"hid1\":"<<c.hid1<<",\"hid2\":"<<c.hid2
       <<",\"dcache\":"<<unsigned(c.effective_dcache)
       <<",\"ram_real\":"<<c.ram_real<<",\"ram_mask\":"<<c.ram_mask
       <<",\"dbat0u\":"<<c.dbat0u<<",\"dbat0l\":"<<c.dbat0l
       <<",\"dbat1u\":"<<c.dbat1u<<",\"dbat1l\":"<<c.dbat1l
       <<",\"r3\":"<<c.gpr[3]<<",\"r4\":"<<c.gpr[4]<<",\"r5\":"<<c.gpr[5]
       <<",\"r29\":"<<c.gpr[29]<<",\"r30\":"<<c.gpr[30]<<",\"r31\":"<<c.gpr[31]
       <<",\"gpr\":[";
    for (unsigned i=0;i<32;++i) out<<(i?",":"")<<c.gpr[i];
    out<<"],\"stack_base\":"<<s.stack.logical_base<<",\"physical_base\":"<<s.stack.physical_base
       <<",\"stack\":\""<<Bytes(s.stack.bytes)<<"\",\"known\":\"";
    for (bool v:s.stack.known) out<<(v?'1':'0');
    out<<"\"}";
}
void Emit(std::ostream& out, const shadow::boot::ApploaderEntryFramesResearch& owner) {
    out<<"{\"schema\":\"apploader-entry-frames-research-42\",\"profile\":\"conditional-source-only\","
       <<"\"native_admitted\":false,\"falsification\":"<<(owner.IsFalsificationRun()?"true":"false")
       <<",\"completed_source_instructions\":"<<owner.CompletedSourceInstructions()
       <<",\"handler\":"<<owner.HandlerWord()<<",\"report\":"<<owner.ReportWord()
       <<",\"source_entry\":";
    Snapshot(out,owner.SourceEntryState()); out<<",\"before_nested\":";
    Snapshot(out,owner.BeforeNestedCall()); out<<",\"boundary\":";
    Snapshot(out,owner.State()); out<<",\"states\":[";
    bool comma=false;
    for (const auto& step:owner.Instructions()) {
        for (unsigned phase=0;phase<2;++phase) {
            if (comma) out<<',';
            comma=true;
            out<<"{\"instruction_pc\":"<<step.instruction_pc<<",\"word\":"<<step.word
               <<",\"stage\":\""<<(phase?"inner-exit":"enter")<<"\",\"state\":";
            Snapshot(out,phase?step.inner_exit:step.enter); out<<'}';
        }
    }
    out<<"],\"stores\":["; comma=false;
    for (const auto& w:owner.Stores()) {
        if (comma) out<<',';
        comma=true;
        out<<"{\"instruction_pc\":"<<w.instruction_pc<<",\"logical_address\":"<<w.logical_address
           <<",\"physical_address\":"<<w.physical_address<<",\"value\":"<<w.value<<"}";
    }
    out<<"]}\n";
}
} // namespace

int main(int argc,char** argv) {
    using namespace shadow::boot;
    try {
        Require(argc==2 || (argc==4 && std::string(argv[2])=="--output"),
                "PAL DOL sibling directory required; optional --output research JSON");
        const std::filesystem::path path(argv[1]);
        const auto app=Read(path.parent_path()/"apploader.img");
        const auto boot=Read(path.parent_path()/"boot.bin"), bi2=Read(path.parent_path()/"bi2.bin");
        const auto profile=ConditionalFreshGcPalEntryResearchProfile42();
        auto Fresh=[&] { return ApploaderEntryFramesResearch(app,boot,bi2,profile); };
        auto pristine=Fresh();
        const auto initial=pristine.State();
        Require(initial.cpu.pc==0x81200258u && initial.cpu.npc==0u && initial.cpu.lr==0u,
                "RunFunction/Reset source production differs");
        Require(initial.cpu.gpr[1]==0x815edca8u && initial.cpu.gpr[2]==0x814b5b20u
                && initial.cpu.gpr[13]==0x814b4fc0u && initial.cpu.gpr[3]==0x80003100u
                && initial.cpu.gpr[4]==0x80003104u && initial.cpu.gpr[5]==0x80003108u,
                "PAL source/argument producer differs");
        Require(pristine.HandlerWord()==0x4c000064u && pristine.ReportWord()==0u,
                "handler/report installation order differs");
        pristine.ProduceFirstThree();
        const auto first=pristine.State();
        Require(first.cpu.pc==0x81200264u && first.cpu.npc==0x81200264u
                && first.cpu.gpr[1]==0x815edca0u && pristine.Stores().size()==2,
                "first-three independent original parity differs");
        pristine.ProduceNestedPrologue();
        const auto& final=pristine.State();
        Require(final.cpu.pc==0x812003b8u && final.cpu.npc==0x812003b8u
                && final.cpu.lr==0x81200268u && final.cpu.gpr[0]==0x81200268u
                && final.cpu.gpr[1]==0x815edc78u && pristine.CompletedSourceInstructions()==11u,
                "nested independent original parity differs");
        Require(pristine.Instructions().size()==11 && pristine.Stores().size()==10
                && pristine.Boundary()==ApploaderEntryFrameBoundary::Before812003B8,
                "finite topology/stop differs");
        const std::array<std::uint32_t,10> addresses{{0x815edca0u,0x815edcacu,0x815edca4u,
            0x815edc78u,0x815edc94u,0x815edc98u,0x815edc9cu,0x815edc80u,0x815edc84u,0x815edc88u}};
        const std::array<std::uint32_t,10> values{{0x815edca8u,0u,0x81200268u,0x815edca0u,
            0u,0u,0u,0x80003100u,0x80003104u,0x80003108u}};
        for (unsigned i=0;i<10;++i) {
            const auto& w=pristine.Stores()[i];
            Require(w.logical_address==addresses[i] && w.physical_address==(addresses[i]&0x01ffffffu)
                    && w.value==values[i],"captured ordered/zero store differs");
            const auto offset=addresses[i]-final.stack.logical_base;
            for (unsigned b=0;b<4;++b)
                Require(final.stack.bytes[offset+b]==std::uint8_t(values[i]>>(24u-8u*b)),
                        "guest big-endian bytes differ");
        }

        unsigned negatives=0;
        for (unsigned f=0;f<6;++f) {
            auto p=profile;
            switch (f) {
                case 0:p.registers=ApploaderEntryResearchProfile::Registers::Unknown;break;
                case 1:p.memory=ApploaderEntryResearchProfile::Memory::Unknown;break;
                case 2:p.mapping=ApploaderEntryResearchProfile::Mapping::Unknown;break;
                case 3:p.cache=ApploaderEntryResearchProfile::Cache::Unknown;break;
                case 4:p.lease=ApploaderEntryResearchProfile::Lease::Unknown;break;
                case 5:p.first_advance=ApploaderEntryResearchProfile::FirstAdvance::Unknown;break;
            }
            Reject([&]{ApploaderEntryFramesResearch x(app,boot,bi2,p);},"unprovided profile admitted"); ++negatives;
        }
        for (const auto pc:std::array<std::uint32_t,12>{{0x81200258u,0x8120025cu,0x81200260u,
            0x81200264u,0x8120039cu,0x812003a0u,0x812003a4u,0x812003a8u,0x812003acu,
            0x812003b0u,0x812003b4u,0x812003b8u}}) {
            auto bad=app; bad[32u+pc-0x81200000u+3u]^=1;
            Reject([&]{ApploaderEntryFramesResearch x(bad,boot,bi2,profile);},"changed raw instruction admitted"); ++negatives;
        }
        auto bad_app=app; bad_app[0x13]^=4;
        Reject([&]{ApploaderEntryFramesResearch x(bad_app,boot,bi2,profile);},"changed header entry admitted"); ++negatives;
        bad_app=app; bad_app.pop_back();
        Reject([&]{ApploaderEntryFramesResearch x(bad_app,boot,bi2,profile);},"truncated code admitted"); ++negatives;
        auto bad_bi2=bi2; bad_bi2[0x1b]=1;
        Reject([&]{ApploaderEntryFramesResearch x(app,boot,bad_bi2,profile);},"non-PAL producer admitted"); ++negatives;
        auto bad_boot=boot; bad_boot[3]='E';
        Reject([&]{ApploaderEntryFramesResearch x(app,bad_boot,bi2,profile);},"different disc admitted"); ++negatives;
        for (auto bad_sp:std::array<std::uint32_t,5>{{0u,0x80000000u,0xffffffffu,0x815edca9u,0x815edc78u}}) {
            auto x=Fresh(); x.WriteResearchGprForFalsification(1,bad_sp);
            Reject([&]{x.ProduceFirstThree();},"wrapped/unaligned/unowned stack admitted");
            Require(x.Stores().empty() && x.CompletedSourceInstructions()==0, "unsupported branch emitted effects"); ++negatives;
        }
        for (const auto f:std::array<EntryResearchRetainedField,14>{{
            EntryResearchRetainedField::Msr,EntryResearchRetainedField::Exceptions,
            EntryResearchRetainedField::Hid0,EntryResearchRetainedField::Hid1,EntryResearchRetainedField::Hid2,
            EntryResearchRetainedField::Ibat0U,EntryResearchRetainedField::Ibat0L,
            EntryResearchRetainedField::Dbat0U,EntryResearchRetainedField::Dbat0L,
            EntryResearchRetainedField::Dbat1U,EntryResearchRetainedField::Dbat1L,
            EntryResearchRetainedField::RamReal,EntryResearchRetainedField::RamMask,
            EntryResearchRetainedField::EffectiveDcache}}) {
            auto x=Fresh(); x.WriteResearchRetainedForFalsification(f,1);
            Reject([&]{x.ProduceFirstThree();},"unknown mapping/cache/fault branch admitted"); ++negatives;
        }
        auto unknown=Fresh(); unknown.ForgetResearchStackByteForFalsification(0x815edc79u);
        Reject([&]{unknown.ProduceFirstThree();},"unknown unrelated preserved stack byte admitted"); ++negatives;
        auto no_first=Fresh();
        Reject([&]{no_first.ProduceNestedPrologue();},"nested path before caller admitted"); ++negatives;
        Reject([&]{pristine.ProduceFirstThree();},"repeated caller admitted"); ++negatives;
        Reject([&]{pristine.ProduceNestedPrologue();},"continued beyond owned boundary"); ++negatives;
        auto nested_fault=Fresh(); nested_fault.ProduceFirstThree();
        nested_fault.WriteResearchGprForFalsification(1,0xffffffffu);
        Reject([&]{nested_fault.ProduceNestedPrologue();},"unsupported nested wrapped EA admitted");
        Require(nested_fault.Stores().size()==2 && nested_fault.CompletedSourceInstructions()==3,
                "nested preflight changed already-owned prefix"); ++negatives;

        // Fresh zero saved registers come from Reset, not ABI. Perturb live
        // owned native values to falsify constants and numeric/endian mistakes.
        auto live=Fresh();
        live.WriteResearchRetainedForFalsification(EntryResearchRetainedField::Lr,0x89abcdefu);
        live.WriteResearchRetainedForFalsification(EntryResearchRetainedField::Cr,0x10203040u);
        live.WriteResearchRetainedForFalsification(EntryResearchRetainedField::Xer,0x55667788u);
        live.WriteResearchRetainedForFalsification(EntryResearchRetainedField::Ctr,0xffeeddccu);
        live.WriteResearchRetainedForFalsification(EntryResearchRetainedField::Fpscr,0x11223344u);
        live.WriteResearchGprForFalsification(3,0x80004000u);
        live.WriteResearchGprForFalsification(4,0x80004004u);
        live.WriteResearchGprForFalsification(5,0x80004008u);
        live.WriteResearchGprForFalsification(7,0xdeadbeefu);
        live.WriteResearchGprForFalsification(29,0x01020304u);
        live.WriteResearchGprForFalsification(30,0x89abcdefu);
        live.WriteResearchGprForFalsification(31,0xfedcba98u);
        live.WriteResearchStackByteForFalsification(0x815edc7cu,0xa5u);
        live.ProduceFirstThree(); live.ProduceNestedPrologue();
        Require(live.IsFalsificationRun() && live.Stores()[1].value==0x89abcdefu
                && live.Stores()[2].value==0x81200268u && live.Stores()[4].value==0x01020304u
                && live.Stores()[5].value==0x89abcdefu && live.Stores()[6].value==0xfedcba98u
                && live.Stores()[7].value==0x80004000u && live.Stores()[8].value==0x80004004u
                && live.Stores()[9].value==0x80004008u, "live owned LR/register spill replaced by a constant");
        Require(live.State().cpu.cr==0x10203040u && live.State().cpu.xer==0x55667788u
                && live.State().cpu.ctr==0xffeeddccu && live.State().cpu.fpscr==0x11223344u
                && live.State().cpu.gpr[7]==0xdeadbeefu && live.State().stack.bytes[4]==0xa5u,
                "unrelated retained CPU/stack state overwritten");

        InitialBootEventOwner unadvanced(FreshInitialBootSourceConfig42());
        Reject([&]{RequireNativeApploaderEntryFrameOwnership(unadvanced);},"unadvanced native owner admitted"); ++negatives;
        InitialBootEventOwner partial(FreshInitialBootSourceConfig42());
        partial.FirstAdvance({true,true,true});
        Reject([&]{RequireNativeApploaderEntryFrameOwnership(partial);},"partial native owner admitted"); ++negatives;
        InitialBootBranchBindings bindings;
        bindings.movie_lifecycle=InitialBootBranchBindings::MovieLifecycle::FreshInactive;
        bindings.dtk_audio_logging=false; bindings.gpu_allow_sleep=[]{};
        bindings.frame_step=false; bindings.achievement_client_present=false; bindings.achievement_dll_found=false;
        InitialBootEventOwner declared(FreshInitialBootSourceConfig42(),bindings);
        Require(declared.FirstAdvance({true,true,true})==InitialBootStop::FirstAdvanceComplete,
                "test complete conditional event path unavailable");
        Reject([&]{RequireNativeApploaderEntryFrameOwnership(declared);},"declared-only Complete native owner admitted"); ++negatives;

        if (argc==4) {
            std::ofstream output(argv[3],std::ios::binary);
            Require(bool(output),"research output unavailable"); Emit(output,pristine);
        }
        std::cout<<"Apploader Entry conditional C++ reconstruction:11 words,10 ordered stores; "
                 <<negatives<<" negative gates passed; live native first-Advance ownership blocked\n";
        return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
