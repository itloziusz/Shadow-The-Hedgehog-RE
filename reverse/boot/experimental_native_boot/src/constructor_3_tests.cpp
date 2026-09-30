#include "constructor_3.h"
#include <fstream>
#include <iostream>
#include <iterator>

namespace {
void Require(bool ok,const char* why) { if(!ok) throw BootError(why); }
void Reject(const auto& fn,const char* why) {
    bool rejected=false;
    try { fn(); } catch(const BootError&) { rejected=true; }
    Require(rejected,why);
}
std::size_t Offset(GuestAddress32 address) {
    for(const auto& s:NativeBootManifest::sections)
        if(address>=s.address && address<s.address+s.size) return s.file_offset+address-s.address;
    throw BootError("test fixture address missing");
}
}
int main(int argc,char** argv) {
    try {
        Require(argc==7,"usage: Constructor3ContractTests DOL pre C0 C1 C2 C3");
        std::ifstream file(argv[1],std::ios::binary);
        const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file),{}};
        const auto image=LoadValidatedFixture(bytes);
        const auto pre=LoadPreEntryOracle(argv[2]);
        const auto hardware=InitializeNativeHardwareSemantics(pre);
        const auto crt=InitializeNativeCrt(image,hardware);
        const auto route=ResolveRuntimeRoute(image,ResolveLowMemoryPrelude(image,crt,pre));
        const auto c0=ExecuteFirstConstructor(image,crt,hardware,route,pre,LoadConstructor0Oracle(argv[3]));
        const auto c1=ExecuteSecondConstructor(image,hardware,pre,c0,LoadConstructor1Oracle(argv[4]));
        const auto prior=LoadConstructor2Oracle(argv[5]);
        const auto c2=ExecuteThirdConstructor(image,crt,pre,c1,prior);
        const auto proof=LoadConstructor3Oracle(argv[6]);
        const auto run=[&](const Constructor3Oracle& p) { return ExecuteFourthConstructor(image,pre,c2,prior,p); };
        const auto result=run(proof);
        Require(result.pc==0x803796F8u && result.cursor==0x804AAC70u && result.lr==0x80035D4Cu &&
            result.next_constructor==result.lr && result.constructors_executed==4,"wrong constructor 4 boundary");
        Require(result.body.vector==NativeVector3{0,0x3F800000u,0} && result.body.destination==0x80571CC0u &&
            result.body.fpr01==std::array<std::uint64_t,2>{0x3FF0000000000000ULL,0},"vector/FPR semantics differ");
        Require(result.body.events.size()==10,"missing ordered effects");
        std::vector<GuestAddress32> writes;
        for(const auto& e:result.body.events) {
            Require(e.kind!="call","leaf constructor invented a callback");
            if(e.kind=="write") writes.push_back(e.address);
        }
        Require(writes==std::vector<GuestAddress32>{0x80571CC0u,0x80571CC4u,0x80571CC8u},"wrong store/update order");
        Require(result.preserved.body.vectors==c2.body.vectors && result.preserved.source==c2.source &&
            result.preserved.preserved.vectors==c1.vectors && result.preserved.preserved.angles==c1.angles &&
            result.preserved.preserved.fpu_owner==c1.fpu_owner,"damaged earlier effects");
        Reject([&]{run({});},"missing evidence accepted");
        for(const auto key:{"entry_pc","entry_lr","entry_r2","entry_r1","entry_r31","entry_f0","entry_f1",
            "entry_owner","entry_context","entry_c2_vectors","entry_prior_vectors","entry_prior_angles",
            "after_crt_source","before_walker_source","constructor_3_entry_source","entry_source","final_source",
            "final_pc","final_lr","final_r3","final_r12","final_r31","final_destination","final_f0","final_f1",
            "final_f2","final_f31","final_r30","final_r0","final_r1","final_cr","final_fpscr","final_msr",
            "final_xer","final_ctr","final_context","final_fragment_slot","final_c2_vectors"}) {
            auto bad=proof; auto& f=bad.fields.at(key); f.back()=f.back()=='0'?'1':'0';
            Reject([&]{run(bad);},"tampered observation accepted");
        }
        for(unsigned i=0;i<9;++i) {
            auto bad=proof; bad.startup_inputs[i]+='0';
            Reject([&]{run(bad);},"different startup accepted");
        }
        auto wrong=c2; wrong.constructors_executed=4;
        Reject([&]{ExecuteFourthConstructor(image,pre,wrong,prior,proof);},"duplicate execution accepted");
        wrong=c2; wrong.preserved.fpu_enabled=false;
        Reject([&]{ExecuteFourthConstructor(image,pre,wrong,prior,proof);},"missing FP prerequisite ignored");
        for(const auto address:{0x8002218Cu,0x80022198u,0x800221A4u,0x805F29A8u,0x805F29BCu,0x804AAC70u}) {
            auto changed=bytes; changed[Offset(address)]^=1;
            Reject([&]{ExecuteFourthConstructor(LoadValidatedFixture(changed),pre,c2,prior,proof);},"tampered fixture accepted");
        }
        // Alternate values are a unit test, never injected into the reference run.
        for(const auto inputs:std::array<std::array<GuestWord32,2>,5>{{
            {0x40000000u,0xC0400000u},{0x80000000u,1u},{0x7F800001u,0xFF800000u},
            {0x7FC01234u,0x007FFFFFu},{0x3F000000u,0xBF800000u}}}) {
            const auto body=RunConstructor3Body(image,inputs,NativeBootManifest::sdata2_base,0x803796FCu,true);
            Require(body.vector==NativeVector3{inputs[0],inputs[1],inputs[0]},"hardcoded final vector");
            Require(body.fpr01[0]==WidenConstructor2Single(inputs[1]) &&
                body.fpr01[1]==WidenConstructor2Single(inputs[0]),"wrong FPR order");
        }
        Reject([&]{RunConstructor3Body(image,{0,0},NativeBootManifest::sdata2_base,0x803796FCu,false);},"body bypassed FP");
        std::ifstream json(argv[6]);
        const std::string text{std::istreambuf_iterator<char>(json),{}};
        Reject([&]{ParseConstructor3Oracle(text.substr(0,text.find_last_of('}')));},"truncated evidence accepted");
        auto advanced=text;
        const auto pos=advanced.find("\"constructor_4_executed\": false");
        Require(pos!=std::string::npos,"missing boundary flag");
        advanced.replace(pos,std::string("\"constructor_4_executed\": false").size(),"\"constructor_4_executed\": true");
        Reject([&]{ParseConstructor3Oracle(advanced);},"constructor 4 capture accepted");
        std::cout<<"PASS constructor3 contracts: values, provenance, order, preservation, boundary\n";
        return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
