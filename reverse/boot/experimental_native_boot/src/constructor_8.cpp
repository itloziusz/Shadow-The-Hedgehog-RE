#include "constructor_8.h"
#include "native_replacements.h"

namespace {
using W=GuestWord32;using A=GuestAddress32;
void Require(bool ok,const char* why){if(!ok)throw BootError(why);}
W Imm(W w){W v=w&65535u;return v&32768u ? v|0xFFFF0000u:v;}
std::string Words(const auto& values){std::string s;for(auto v:values)s+=Constructor1Hex(v,4);return s;}
void Emit(Constructor8BodyResult& r,std::string_view kind,A pc,A a,unsigned size,W v){r.events.push_back({kind,pc,a,size,Constructor1Hex(v,size?size:4)});}
}

Constructor8BodyResult RunConstructor8Body(const BootImage& im,std::array<std::array<W,2>,2> args,W sda,W stack,W ret){
    im.RequirePalFixtureDigest();
    // The tool owns the executable semantics. This adapter only maps the
    // generated typed result into the existing checkpoint/lineage API.
    auto generated=Reconstructed::Fn_80045f48({stack,NativeBootManifest::sdata2_base,
        sda,0x804AAC80u,ret,args[0][0],args[0][1],args[1][0],args[1][1]});
    Constructor8BodyResult result;
    result.pairs={{{generated.global_805ef1f8,generated.global_805ef1fc},
        {generated.global_805ef200,generated.global_805ef204}}};
    result.destinations={sda+Imm(im.ReadWord(0x80045F68u)),sda+Imm(im.ReadWord(0x80045F8Cu))};
    result.r1=generated.stack;result.return_pc=generated.return_address;
    result.events=std::move(generated.events);return result;
}

Constructor8Result ExecuteNinthConstructor(const BootImage& im,const PreEntryOracle& pre,
    const Constructor7Result& previous,const Constructor7Oracle& prior,const Constructor8Oracle& proof){
    im.RequirePalFixtureDigest();
    Require(pre.present && prior.present && proof.present,"constructor 8 requires dependency evidence");
    const std::array<std::string,9> startup{pre.memory_0x805f1f30,pre.memory_0x805f1f38,
        pre.lowmem_0x80000030,pre.lowmem_0x80000034,pre.lowmem_0x80000044,pre.lowmem_0x800000f4,
        pre.lowmem_0x800030e4,pre.lowmem_0x800030e6,pre.bi2_bytes};
    Require(prior.startup_inputs==startup && proof.startup_inputs==startup,"constructor 8 startup lineage differs");
    Require(previous.constructors_executed==8 && previous.pc==0x803796F8u && previous.cursor==0x804AAC80u &&
        previous.next_constructor==0x80045F48u && previous.lr==previous.next_constructor &&
        im.ReadWord(previous.cursor)==previous.next_constructor,"constructor 8 out of order");
    const auto get=[&](const std::string& k)->const std::string&{auto it=proof.fields.find(k);if(it==proof.fields.end())throw BootError("missing C8 field: "+k);return it->second;};
    const auto compare=[&](const std::string& k,std::uint64_t v,unsigned size=4){if(get(k)!=Constructor1Hex(v,size))throw BootError("constructor 8 mismatch: "+k);};
    for(const auto& [key,value]:prior.fields){
        if(!key.starts_with("final_") || key=="final_pc" || key=="final_lr")continue;
        std::string suffix=key.substr(6);if(suffix=="destination")suffix="c7_objects";else if(suffix=="node")suffix="c7_nodes";
        Require(get("entry_"+suffix)==value,"constructor 7->8 lineage differs");
    }
    compare("entry_pc",previous.next_constructor);compare("entry_lr",previous.pc+4);
    compare("entry_r31",previous.cursor);compare("entry_r1",previous.body.r1);compare("entry_r13",NativeBootManifest::sdata_base);
    compare("entry_source",previous.body.head);
    Require(get("entry_c7_objects")==Words(previous.body.objects),"prior native objects damaged");
    std::string nodes;for(const auto& n:previous.body.registrations)nodes+=Words(std::array<W,3>{n.next,n.destructor,n.object});
    Require(get("entry_c7_nodes")==nodes,"prior native list damaged");
    const std::array<std::array<W,2>,2> args{{{Imm(im.ReadWord(0x80045F50u)),Imm(im.ReadWord(0x80045F54u))},
        {Imm(im.ReadWord(0x80045F78u)),Imm(im.ReadWord(0x80045F7Cu))}}};
    auto body=RunConstructor8Body(im,args,NativeBootManifest::sdata_base,previous.body.r1,previous.pc+4);
    const A ret=body.return_pc,cursor=previous.cursor+Imm(im.ReadWord(ret)),next=im.ReadWord(cursor);
    Emit(body,"read",ret+4,cursor,4,next);const bool more=next!=0;
    const A dispatch=more ? ret+12+Imm(im.ReadWord(ret+12)&0xFFFCu):ret+16;
    Emit(body,"branch",ret+12,dispatch,0,more);Require(more,"unexpected table end");
    Constructor8Result r{previous,std::move(body),dispatch+4,cursor,next,next,previous.constructors_executed+1};
    Require(get("final_destination")==Words(r.body.pairs[0])+Words(r.body.pairs[1]),"C8 output mismatch");
    compare("final_pc",r.pc);compare("final_lr",r.lr);compare("final_r31",r.cursor);compare("final_r12",r.next_constructor);
    compare("final_r0",ret);compare("final_r1",r.body.r1);compare("final_r3",r.body.destinations[1]);
    compare("final_r4",r.body.pairs[1][0]);compare("final_r5",r.body.pairs[1][1]);compare("final_r6",r.body.destinations[0]);
    for(unsigned i=0;i<32;++i){
        if(i!=0 && i!=1 && (i<3 || i>6) && i!=12 && i!=31)Require(get("entry_r"+std::to_string(i))==get("final_r"+std::to_string(i)),"unexplained GPR effect");
        Require(get("entry_f"+std::to_string(i))==get("final_f"+std::to_string(i)),"unexplained FPR effect");
    }
    for(const auto key:{"fpscr","msr","ctr","xer","context","owner","fragment_id","fragment_slot","prior_vectors","prior_angles",
        "c2_vectors","c3_vector","c4_object","c4_node","c5_bytes","c5_neighbors","c6_array","c6_node","c7_objects","c7_nodes","source"})
        Require(get(std::string("entry_")+key)==get(std::string("final_")+key),"unexplained global/control effect");
    compare("final_cr",ConstructorWalkerCompareCr(static_cast<W>(std::stoul(get("entry_cr"),nullptr,16)),static_cast<W>(std::stoul(get("entry_xer"),nullptr,16)),next));
    return r;
}
