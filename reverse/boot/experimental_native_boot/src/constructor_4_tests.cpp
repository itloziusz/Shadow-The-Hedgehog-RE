#include "constructor_4.h"
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
        Require(argc==8,"usage: Constructor4ContractTests DOL pre C0 C1 C2 C3 C4");
        std::ifstream file(argv[1],std::ios::binary);
        const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file),{}};
        const auto image=LoadValidatedFixture(bytes);
        const auto pre=LoadPreEntryOracle(argv[2]);
        const auto hardware=InitializeNativeHardwareSemantics(pre);
        const auto crt=InitializeNativeCrt(image,hardware);
        const auto route=ResolveRuntimeRoute(image,ResolveLowMemoryPrelude(image,crt,pre));
        const auto c0=ExecuteFirstConstructor(image,crt,hardware,route,pre,LoadConstructor0Oracle(argv[3]));
        const auto c1=ExecuteSecondConstructor(image,hardware,pre,c0,LoadConstructor1Oracle(argv[4]));
        const auto p2=LoadConstructor2Oracle(argv[5]);
        const auto c2=ExecuteThirdConstructor(image,crt,pre,c1,p2);
        const auto prior=LoadConstructor3Oracle(argv[6]);
        const auto c3=ExecuteFourthConstructor(image,pre,c2,p2,prior);
        const auto proof=LoadConstructor4Oracle(argv[7]);
        const auto run=[&](const Constructor4Oracle& p) { return ExecuteFifthConstructor(image,crt,pre,c3,prior,p); };
        const auto result=run(proof);
        Require(result.pc==0x803796F8u && result.cursor==0x804AAC74u && result.lr==0x80039B88u &&
            result.next_constructor==result.lr && result.constructors_executed==5,"wrong constructor 5 boundary");
        const auto& b=result.body;
        Require(b.object==std::array<GuestWord32,2>{0,0} && b.object_address==0x805EF140u &&
            b.head_address==0x805F2370u && b.head==b.node_address && b.head==0x80572238u &&
            b.registration.next==0 && b.registration.destructor==0x80028D60u &&
            b.registration.object==b.object_address,"incorrect list/object state");
        Require(b.events.size()==17 && b.r1==c2.body.r1 && b.return_pc==0x803796FCu,"missing effects/stack restore");
        std::vector<GuestAddress32> calls,writes;
        for(const auto& e:b.events) {
            if(e.kind=="call") calls.push_back(e.address);
            if(e.kind=="write") writes.push_back(e.address);
        }
        Require(calls==std::vector<GuestAddress32>{0x80028E90u,0x803A1210u},"fake destructor/callback call");
        Require(writes==std::vector<GuestAddress32>{0x8060C5C0u,0x8060C5D4u,0x805EF140u,0x805EF144u,
            0x80572238u,0x8057223Cu,0x80572240u,0x805F2370u},"wrong initialization/publication order");
        Require(result.preserved.body.vector==c3.body.vector && result.preserved.body.fpr01==c3.body.fpr01 &&
            result.preserved.preserved.body.vectors==c2.body.vectors &&
            result.preserved.preserved.preserved.vectors==c1.vectors,"earlier state damaged");
        Reject([&]{run({});},"missing evidence accepted");
        for(const auto key:{"entry_pc","entry_lr","entry_r13","entry_r1","entry_r31","entry_f0","entry_f1",
            "entry_owner","entry_context","entry_c3_vector","entry_c2_vectors","entry_prior_vectors",
            "after_crt_source","before_walker_source","constructor_4_entry_source","entry_source","final_source",
            "final_pc","final_lr","final_r3","final_r4","final_r5","final_r12","final_r31","final_destination",
            "final_node","final_f0","final_f1","final_f2","final_f31","final_r30","final_r0","final_r1",
            "final_cr","final_fpscr","final_msr","final_context","final_fragment_slot","final_c3_vector"}) {
            auto bad=proof; auto& f=bad.fields.at(key); f.back()=f.back()=='0'?'1':'0';
            Reject([&]{run(bad);},"tampered observation accepted");
        }
        for(unsigned i=0;i<9;++i) {
            auto bad=proof; bad.startup_inputs[i]+='0';
            Reject([&]{run(bad);},"different startup accepted");
        }
        auto wrong=c3; wrong.constructors_executed=5;
        Reject([&]{ExecuteFifthConstructor(image,crt,pre,wrong,prior,proof);},"duplicate execution accepted");
        auto no_crt=crt; no_crt.zero_ranges=0;
        Reject([&]{ExecuteFifthConstructor(image,no_crt,pre,c3,prior,proof);},"unproven head silently zeroed");
        for(const auto address:{0x80035D4Cu,0x80035D54u,0x80035D68u,0x80028E90u,0x803A1210u,
            0x803A121Cu,0x803A1220u,0x800055D0u,0x804AAC74u}) {
            auto changed=bytes; changed[Offset(address)]^=1;
            Reject([&]{ExecuteFifthConstructor(LoadValidatedFixture(changed),crt,pre,c3,prior,proof);},"tampered fixture accepted");
        }
        // Unit-level old-head identities are linked, not dereferenced, dropped or guessed.
        for(const auto head:{0u,0x80572200u,0x12345678u}) {
            const auto body=RunConstructor4Body(image,head,NativeBootManifest::sdata_base,c2.body.r1,0x803796FCu);
            Require(body.registration.next==head && body.head==body.node_address,"list predecessor hardcoded/lost");
            Require(body.events[7].kind=="read" && body.events[7].value==Constructor1Hex(head,4),"head read not traced");
        }
        // Old object/node contents are dead, not unknown inputs: every word is overwritten.
        auto dead=proof; dead.fields["entry_destination"]="1234567887654321";
        dead.fields["entry_node"]="123456788765432112345678";
        Require(run(dead).body.registration.next==0,"dead storage incorrectly consumed");
        std::ifstream json(argv[7]);
        const std::string text{std::istreambuf_iterator<char>(json),{}};
        Reject([&]{ParseConstructor4Oracle(text.substr(0,text.find_last_of('}')));},"truncated evidence accepted");
        auto advanced=text;
        const std::string flag="\"constructor_5_executed\": false";
        const auto pos=advanced.find(flag);
        Require(pos!=std::string::npos,"missing boundary flag");
        advanced.replace(pos,flag.size(),"\"constructor_5_executed\": true");
        Reject([&]{ParseConstructor4Oracle(advanced);},"constructor 5 capture accepted");
        std::cout<<"PASS constructor4 contracts: provenance, list linking, ordered publication, no callback, boundary\n";
        return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
