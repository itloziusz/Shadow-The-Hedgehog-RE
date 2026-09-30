#include "constructor_8.h"
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
        Require(argc==12,"usage: Constructor8ContractTests DOL pre C0 C1 C2 C3 C4 C5 C6 C7 C8");
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
        const auto c7=ExecuteEighthConstructor(im,pre,c6,proof,p7);
        const auto p8=LoadConstructor8Oracle(argv[11]);
        const auto run=[&](const Constructor8Oracle& p){return ExecuteNinthConstructor(im,pre,c7,p7,p);};
        const auto r=run(p8);
        Require(r.pc==0x803796F8u && r.cursor==0x804AAC84u && r.next_constructor==0x80047710u &&
            r.constructors_executed==9 && r.lr==r.next_constructor,"wrong C9 boundary");
        Require(r.body.events.size()==22 && r.body.pairs[0]==std::array<GuestWord32,2>{0,340} &&
            r.body.pairs[1]==std::array<GuestWord32,2>{640,120},"wrong pair values/effect count");
        Require(r.preserved.body.head==c7.body.head && r.preserved.body.objects==c7.body.objects,"prior state damaged");
        std::vector<GuestAddress32> calls,stores;
        for(const auto& e:r.body.events){
            if(e.kind=="call")calls.push_back(e.address);
            Require(e.kind!="call_indirect","fake callback");
            if(e.kind=="write" && e.address<0x80600000u)stores.push_back(e.address);
        }
        Require(calls==std::vector<GuestAddress32>{0x80045FB8u,0x80045FACu} &&
            stores==std::vector<GuestAddress32>{0x805EF1F8u,0x805EF1FCu,0x805EF200u,0x805EF204u},"wrong ordered calls/stores");
        Reject([&]{run({});},"missing proof accepted");
        for(const auto& [key,value]:p8.fields){
            if(key=="entry_destination")continue;
            auto bad=p8;auto& v=bad.fields[key];v.back()=v.back()=='0'?'1':'0';
            Reject([&]{run(bad);},"altered evidence accepted");
        }
        for(unsigned i=0;i<9;++i){auto bad=p8;bad.startup_inputs[i]+='0';Reject([&]{run(bad);},"startup mismatch");}
        auto wrong=c7;wrong.constructors_executed=9;
        Reject([&]{ExecuteNinthConstructor(im,pre,wrong,p7,p8);},"wrong order accepted");
        wrong=c7;wrong.body.head=0;
        Reject([&]{ExecuteNinthConstructor(im,pre,wrong,p7,p8);},"lost list head accepted");
        for(const auto address:{0x80045F50u,0x80045F54u,0x80045F60u,0x80045F74u,0x80045F80u,
            0x80045F84u,0x80045FACu,0x80045FB8u,0x80045FC0u,0x804AAC84u}){
            auto changed=bytes;changed[Offset(address)]^=1;
            Reject([&]{ExecuteNinthConstructor(LoadValidatedFixture(changed),pre,c7,p7,p8);},"fixture mutation accepted");
        }
        const std::array<std::array<GuestWord32,2>,2> alternate{{{0xDEADBEEFu,0x80000001u},{7,0xFFFFFFFFu}}};
        const auto different=RunConstructor8Body(im,alternate,NativeBootManifest::sdata_base,c7.body.r1,c7.pc+4);
        Require(different.pairs==alternate,"hardcoded pair output");
        for(auto helper:{0x80045FACu,0x80045FB8u}){
            std::vector<Constructor1Event> events;
            Require(InitializeWordPair(im,helper,0x8060C5B8u,alternate[0],0x12345678u,events)==alternate[0] &&
                events.size()==3 && events[2].address==0x12345678u,"helper contract/return broken");
        }
        std::vector<Constructor1Event> none;
        Reject([&]{InitializeWordPair(im,0x80028E90u,0x8060C5B8u,alternate[0],0,none);},"similar helper silently accepted");
        Reject([&]{InitializeWordPair(im,0x80045FACu,0x8060C5B9u,alternate[0],0,none);},"unaligned object accepted");
        Require(none.empty(),"failed contract emitted partial effects");
        auto dead=p8;dead.fields["entry_destination"]=std::string(32,'a');
        Require(run(dead).body.pairs==r.body.pairs,"dead destination used");
        std::ifstream j(argv[11]);const std::string text{std::istreambuf_iterator<char>(j),{}};
        Reject([&]{ParseConstructor8Oracle(text.substr(0,text.find_last_of('}')));},"truncated oracle accepted");
        auto advanced=text;const std::string flag="\"constructor_9_executed\": false";const auto pos=advanced.find(flag);
        Require(pos!=std::string::npos,"missing stop flag");advanced.replace(pos,flag.size(),"\"constructor_9_executed\": true");
        Reject([&]{ParseConstructor8Oracle(advanced);},"C9 capture accepted");
        std::cout<<"PASS constructor8 pair-summary reuse, provenance, state, ordered effects and boundary\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
