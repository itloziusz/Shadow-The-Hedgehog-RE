#include "constructor_6.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <algorithm>
namespace {
void Require(bool ok,const char* why) { if(!ok)throw BootError(why); }
void Reject(const auto& fn,const char* why) { bool rejected=false; try{fn();}catch(const BootError&){rejected=true;} Require(rejected,why); }
std::size_t Offset(GuestAddress32 a) {
    for(const auto& s:NativeBootManifest::sections)if(a>=s.address && a<s.address+s.size)return s.file_offset+a-s.address;
    throw BootError("test address missing");
}
}
int main(int argc,char** argv) {
    try {
        Require(argc==10,"usage: Constructor6ContractTests DOL pre C0 C1 C2 C3 C4 C5 C6");
        std::ifstream f(argv[1],std::ios::binary);
        const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(f),{}};
        const auto im=LoadValidatedFixture(bytes);
        const auto pre=LoadPreEntryOracle(argv[2]);
        const auto hw=InitializeNativeHardwareSemantics(pre);
        const auto crt=InitializeNativeCrt(im,hw);
        const auto route=ResolveRuntimeRoute(im,ResolveLowMemoryPrelude(im,crt,pre));
        const auto c0=ExecuteFirstConstructor(im,crt,hw,route,pre,LoadConstructor0Oracle(argv[3]));
        const auto c1=ExecuteSecondConstructor(im,hw,pre,c0,LoadConstructor1Oracle(argv[4]));
        const auto p2=LoadConstructor2Oracle(argv[5]);
        const auto c2=ExecuteThirdConstructor(im,crt,pre,c1,p2);
        const auto p3=LoadConstructor3Oracle(argv[6]);
        const auto c3=ExecuteFourthConstructor(im,pre,c2,p2,p3);
        const auto p4=LoadConstructor4Oracle(argv[7]);
        const auto c4=ExecuteFifthConstructor(im,crt,pre,c3,p3,p4);
        const auto prior=LoadConstructor5Oracle(argv[8]);
        const auto c5=ExecuteSixthConstructor(im,pre,c4,p4,prior);
        const auto proof=LoadConstructor6Oracle(argv[9]);
        const auto run=[&](const Constructor6Oracle& p){return ExecuteSeventhConstructor(im,pre,c5,prior,p);};
        const auto result=run(proof); const auto& b=result.body;
        Require(result.pc==0x803796F8u && result.cursor==0x804AAC7Cu && result.next_constructor==0x8004336Cu &&
            result.lr==result.next_constructor && result.constructors_executed==7,"wrong C7 boundary");
        Require(b.events.size()==69 && b.elements.size()==4 && b.count==4 && b.stride==4,"wrong loop/effect counts");
        for(const auto& e:b.elements)Require(e==std::array<std::uint16_t,2>{0,65535},"wrong element values");
        Require(b.head==0x80572538u && b.registration.next==c4.body.head && b.registration.destructor==0x8003B010u &&
            b.registration.object==0 && b.cleanup_destructor==0x80021C44u,"registration/cleanup identities wrong");
        unsigned indirect=0; std::vector<GuestAddress32> stores;
        for(const auto& e:b.events) {
            if(e.kind=="call_indirect") { ++indirect; Require(e.address==0x8003B048u,"fake callback"); }
            if(e.kind=="call")Require(e.address==0x803A1500u || e.address==0x803A1210u,"unexpected direct call");
            if(e.kind=="write" && e.address>=b.array_address && e.address<b.array_address+16) {
                Require(e.size==2,"halfword store widened"); stores.push_back(e.address);
            }
        }
        Require(indirect==4 && stores==std::vector<GuestAddress32>{0x80572544u,0x80572546u,0x80572548u,0x8057254Au,
            0x8057254Cu,0x8057254Eu,0x80572550u,0x80572552u},"element order/call count wrong");
        Require(result.preserved.body.bytes==c5.body.bytes && result.preserved.preserved.body.head==c4.body.head &&
            result.preserved.preserved.body.registration.destructor==c4.body.registration.destructor,"lost previous state");
        Reject([&]{run({});},"missing evidence accepted");
        for(const auto key:{"entry_pc","entry_lr","entry_r1","entry_r13","entry_r31","entry_source","entry_c4_node",
            "entry_c5_bytes","entry_context","final_pc","final_lr","final_r0","final_r1","final_r3","final_r4",
            "final_r5","final_r6","final_r7","final_r12","final_r28","final_r29","final_r30","final_r31","final_ctr",
            "final_source","final_node","final_destination","final_c4_node","final_f0","final_cr","final_context"}) {
            auto bad=proof;auto& v=bad.fields.at(key);v.back()=v.back()=='0'?'1':'0';
            Reject([&]{run(bad);},"changed dependency/effect accepted");
        }
        for(unsigned i=0;i<9;++i) {auto bad=proof;bad.startup_inputs[i]+='0';Reject([&]{run(bad);},"startup mismatch accepted");}
        auto wrong=c5; wrong.constructors_executed=7;
        Reject([&]{ExecuteSeventhConstructor(im,pre,wrong,prior,proof);},"duplicate execution");
        wrong=c5;wrong.preserved.body.head=0;
        Reject([&]{ExecuteSeventhConstructor(im,pre,wrong,prior,proof);},"lost previous head accepted");
        for(const auto a:{0x8003AFD0u,0x8003AFD8u,0x8003AFDCu,0x8003AFE0u,0x803A155Cu,0x803A1578u,0x803A1584u,
            0x8003B050u,0x8003AFF4u,0x803A1220u,0x804AAC7Cu}) {
            auto changed=bytes;changed[Offset(a)]^=1;
            Reject([&]{ExecuteSeventhConstructor(LoadValidatedFixture(changed),pre,c5,prior,proof);},"fixture mutation accepted");
        }
        for(unsigned n:{0u,1u,4u}) {
            const auto body=RunConstructor6Body(im,0x12345678u,NativeBootManifest::sdata_base,c5.body.r1,c5.cursor,c5.pc+4,n,{0x12345678u,0xABCDEF01u});
            Require(body.elements.size()==n && body.registration.next==0x12345678u,"forced count/head");
            for(const auto& e:body.elements)Require(e==std::array<std::uint16_t,2>{0x5678,0xEF01},"sth truncation wrong");
            Require(static_cast<unsigned>(std::count_if(body.events.begin(),body.events.end(),[](const auto& e){return e.kind=="call_indirect";}))==n,"loop forced calls");
        }
        Reject([&]{RunConstructor6Body(im,0,NativeBootManifest::sdata_base,c5.body.r1,c5.cursor,c5.pc+4,5,{0,0});},"unknown array extent accepted");
        auto dead=proof;dead.fields["entry_destination"]=std::string(32,'a');dead.fields["entry_node"]=std::string(24,'b');
        Require(run(dead).body.elements==b.elements,"old storage read");
        std::ifstream j(argv[9]); const std::string text{std::istreambuf_iterator<char>(j),{}};
        Reject([&]{ParseConstructor6Oracle(text.substr(0,text.find_last_of('}')));},"truncated evidence accepted");
        auto advanced=text;const std::string flag="\"constructor_7_executed\": false";const auto pos=advanced.find(flag);
        Require(pos!=std::string::npos,"boundary flag missing");advanced.replace(pos,flag.size(),"\"constructor_7_executed\": true");
        Reject([&]{ParseConstructor6Oracle(advanced);},"C7 capture accepted");
        std::cout<<"PASS constructor6 loop, halfwords, registration chaining, provenance, boundary\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
