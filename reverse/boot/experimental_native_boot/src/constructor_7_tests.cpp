#include "constructor_7.h"
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
        Require(argc==11,"usage: Constructor7ContractTests DOL pre C0 C1 C2 C3 C4 C5 C6 C7");
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
        const auto c6=ExecuteSeventhConstructor(im,pre,c5,prior,proof);
        const auto p7=LoadConstructor7Oracle(argv[10]);
        const auto run=[&](const Constructor7Oracle& p){return ExecuteEighthConstructor(im,pre,c6,proof,p);};
        const auto result=run(p7);const auto& b=result.body;
        Require(result.pc==0x803796F8u && result.cursor==0x804AAC80u && result.next_constructor==0x80045F48u &&
            result.lr==result.next_constructor && result.constructors_executed==8,"wrong C8 boundary");
        Require(b.objects==std::array<GuestWord32,4>{0,0,0,0},"wrong initialized pointers");
        Require(b.events.size()==122,"wrong ordered effect count");
        GuestAddress32 head=c6.body.head;
        const std::array<GuestAddress32,4> destructors{0x8003EBF4u,0x80042E18u,0x8003C9F4u,0x8003E218u};
        std::vector<GuestAddress32> stores;
        unsigned calls=0,branches=0;
        for(unsigned i=0;i<4;++i) {
            Require(b.registrations[i].next==head && b.registrations[i].destructor==destructors[i] &&
                b.registrations[i].object==0x805EF1B0u+4*i,"broken ordered registration");
            Require(b.node_addresses[i]==0x80573D88u+12*i,"wrong registration address");
            head=b.node_addresses[i];
        }
        Require(b.head==head && b.head==0x80573DACu,"wrong head");
        for(const auto& e:b.events) {
            Require(e.kind!="call_indirect","fake callback");
            if(e.kind=="call") {
                ++calls;
                Require(std::find(destructors.begin(),destructors.end(),e.address)==destructors.end(),"destructor invoked");
            }
            if(e.kind=="branch")++branches;
            if(e.kind=="write" && e.address<0x80600000u)stores.push_back(e.address);
        }
        std::vector<GuestAddress32> expected;
        for(unsigned i=0;i<4;++i) {
            const auto n=b.node_addresses[i];
            expected.insert(expected.end(),{b.object_addresses[i],n,n+4,n+8,b.head_address});
        }
        Require(stores==expected && calls==12 && branches==9,"ordered stores/calls/guards wrong");
        Require(result.preserved.body.head==c6.body.head && result.preserved.body.elements==c6.body.elements &&
            result.preserved.body.registration.next==c6.body.registration.next,"lost previous state");
        Reject([&]{run({});},"missing evidence accepted");
        for(const auto key:{"entry_pc","entry_lr","entry_r1","entry_r13","entry_r31","entry_source","entry_c6_node",
            "entry_c6_array","entry_c5_bytes","entry_context","final_pc","final_lr","final_r0","final_r1","final_r3","final_r4",
            "final_r5","final_r12","final_r29","final_r30","final_r31","final_ctr","final_source","final_node","final_destination",
            "final_c6_array","final_c6_node","final_c4_node","final_f0","final_cr","final_context"}) {
            auto bad=p7;auto& v=bad.fields.at(key);v.back()=v.back()=='0'?'1':'0';
            Reject([&]{run(bad);},"changed dependency/effect accepted");
        }
        for(unsigned i=0;i<9;++i){auto bad=p7;bad.startup_inputs[i]+='0';Reject([&]{run(bad);},"startup mismatch accepted");}
        auto wrong=c6;wrong.constructors_executed=8;
        Reject([&]{ExecuteEighthConstructor(im,pre,wrong,proof,p7);},"duplicate execution accepted");
        wrong=c6;wrong.body.head=0;
        Reject([&]{ExecuteEighthConstructor(im,pre,wrong,proof,p7);},"previous head lost");
        for(const auto a:{0x80043378u,0x80043388u,0x80043394u,0x8003ED9Cu,0x8003EDA8u,0x8003EDE8u,0x8003EF34u,
            0x80043680u,0x800436A0u,0x8003DDDCu,0x8003DF28u,0x80043454u,0x800435A0u,0x803A1220u,0x804AAC80u}) {
            auto changed=bytes;changed[Offset(a)]^=1;
            Reject([&]{ExecuteEighthConstructor(LoadValidatedFixture(changed),pre,c6,proof,p7);},"fixture mutation accepted");
        }
        for(const auto previous_head:{0u,0x12345678u}) {
            const auto changed=RunConstructor7Body(im,previous_head,NativeBootManifest::sdata_base,c6.body.r1,c6.cursor,c6.pc+4);
            Require(changed.registrations[0].next==previous_head,"hardcoded previous head");
            for(unsigned i=1;i<4;++i)Require(changed.registrations[i].next==changed.node_addresses[i-1],"chain broken");
        }
        auto dead=p7;dead.fields["entry_destination"]=std::string(32,'a');dead.fields["entry_node"]=std::string(96,'b');
        Require(run(dead).body.objects==b.objects,"old storage used");
        std::ifstream j(argv[10]);const std::string text{std::istreambuf_iterator<char>(j),{}};
        Reject([&]{ParseConstructor7Oracle(text.substr(0,text.find_last_of('}')));},"truncated evidence accepted");
        auto advanced=text;const std::string flag="\"constructor_8_executed\": false";const auto pos=advanced.find(flag);
        Require(pos!=std::string::npos,"boundary flag missing");advanced.replace(pos,flag.size(),"\"constructor_8_executed\": true");
        Reject([&]{ParseConstructor7Oracle(advanced);},"C8 capture accepted");
        std::cout<<"PASS constructor7 null initialization, guarded helpers, registration order, provenance, boundary\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
