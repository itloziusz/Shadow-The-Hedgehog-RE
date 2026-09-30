#include "constructor_5.h"
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
        Require(argc==9,"usage: Constructor5ContractTests DOL pre C0 C1 C2 C3 C4 C5");
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
        const auto p3=LoadConstructor3Oracle(argv[6]);
        const auto c3=ExecuteFourthConstructor(image,pre,c2,p2,p3);
        const auto prior=LoadConstructor4Oracle(argv[7]);
        const auto c4=ExecuteFifthConstructor(image,crt,pre,c3,p3,prior);
        const auto proof=LoadConstructor5Oracle(argv[8]);
        const auto run=[&](const Constructor5Oracle& p) { return ExecuteSixthConstructor(image,pre,c4,prior,p); };
        const auto result=run(proof);
        Require(result.pc==0x803796F8u && result.cursor==0x804AAC78u && result.lr==0x8003AFB8u &&
            result.next_constructor==result.lr && result.constructors_executed==6,"wrong constructor 6 boundary");
        const auto& b=result.body;
        Require(b.bytes==std::array<std::uint8_t,4>{255,255,255,255} && b.destination==0x805EF190u &&
            b.arguments==std::array<GuestWord32,4>{255,255,255,255},"incorrect byte initialization");
        Require(b.events.size()==12 && b.r1==c4.body.r1 && b.return_pc==0x803796FCu,"missing effects/stack restore");
        std::vector<GuestAddress32> calls,stores;
        for(const auto& e:b.events) {
            if(e.kind=="call") calls.push_back(e.address);
            if(e.kind=="write" && e.address>=b.destination && e.address<b.destination+4) {
                Require(e.size==1,"byte initialization widened to word store");
                stores.push_back(e.address);
            }
        }
        Require(calls==std::vector<GuestAddress32>{0x8040E328u},"unexpected registration/callback call");
        Require(stores==std::vector<GuestAddress32>{0x805EF190u,0x805EF191u,0x805EF192u,0x805EF193u},"byte order wrong");
        const auto& kept=result.preserved.body;
        Require(kept.head==c4.body.head && kept.object==c4.body.object &&
            kept.registration.next==c4.body.registration.next &&
            kept.registration.destructor==c4.body.registration.destructor &&
            kept.registration.object==c4.body.registration.object &&
            result.preserved.preserved.body.fpr01==c3.body.fpr01,"registration/earlier state damaged");
        Reject([&]{run({});},"missing evidence accepted");
        for(const auto key:{"entry_pc","entry_lr","entry_r13","entry_r1","entry_r31","entry_f0","entry_f1",
            "entry_source","entry_node","entry_c4_object","entry_c3_vector","entry_context",
            "final_pc","final_lr","final_r0","final_r1","final_r3","final_r4","final_r5","final_r6","final_r7",
            "final_r12","final_r31","final_destination","final_source","final_node","final_neighbors",
            "final_f0","final_f31","final_r30","final_cr","final_fpscr","final_msr","final_context","final_c4_object"}) {
            auto bad=proof; auto& f=bad.fields.at(key); f.back()=f.back()=='0'?'1':'0';
            Reject([&]{run(bad);},"tampered observation accepted");
        }
        for(unsigned i=0;i<9;++i) {
            auto bad=proof; bad.startup_inputs[i]+='0';
            Reject([&]{run(bad);},"different startup accepted");
        }
        auto wrong=c4; wrong.constructors_executed=6;
        Reject([&]{ExecuteSixthConstructor(image,pre,wrong,prior,proof);},"duplicate execution accepted");
        wrong=c4; wrong.body.registration.destructor^=4;
        Reject([&]{ExecuteSixthConstructor(image,pre,wrong,prior,proof);},"native registration corruption accepted");
        wrong=c4; wrong.body.head=0;
        Reject([&]{ExecuteSixthConstructor(image,pre,wrong,prior,proof);},"lost list head accepted");
        for(const auto address:{0x80039B88u,0x80039B90u,0x80039B94u,0x80039B9Cu,0x80039BA0u,0x80039BA4u,
            0x80039BA8u,0x8040E328u,0x8040E334u,0x804AAC78u}) {
            auto changed=bytes; changed[Offset(address)]^=1;
            Reject([&]{ExecuteSixthConstructor(LoadValidatedFixture(changed),pre,c4,prior,proof);},"tampered fixture accepted");
        }
        // Distinct components and high bits expose both hardcoded FF and missing stb truncation.
        const std::array<GuestWord32,4> alternate{0x12345612u,0xABCDEF34u,0x80000056u,0xFFFFFF78u};
        const auto body=RunConstructor5Body(image,alternate,NativeBootManifest::sdata_base,c4.body.r1,0x803796FCu);
        Require(body.bytes==std::array<std::uint8_t,4>{0x12,0x34,0x56,0x78} && body.arguments==alternate,
            "helper failed low-byte truncation/order or changed arguments");
        const auto zero=RunConstructor5Body(image,{0x100,0x200,0x80000000u,0},NativeBootManifest::sdata_base,c4.body.r1,0x803796FCu);
        Require(zero.bytes==std::array<std::uint8_t,4>{0,0,0,0},"helper hardcoded white output");
        auto dead=proof; dead.fields["entry_destination"]="12345678";
        Require(run(dead).body.bytes==b.bytes,"dead destination consumed");
        std::ifstream json(argv[8]);
        const std::string text{std::istreambuf_iterator<char>(json),{}};
        Reject([&]{ParseConstructor5Oracle(text.substr(0,text.find_last_of('}')));},"truncated evidence accepted");
        auto advanced=text;
        const std::string flag="\"constructor_6_executed\": false";
        const auto pos=advanced.find(flag);
        Require(pos!=std::string::npos,"missing boundary flag");
        advanced.replace(pos,flag.size(),"\"constructor_6_executed\": true");
        Reject([&]{ParseConstructor5Oracle(advanced);},"constructor 6 capture accepted");
        std::cout<<"PASS constructor5: byte stores/truncation, input provenance, registration preservation, boundary\n";
        return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
