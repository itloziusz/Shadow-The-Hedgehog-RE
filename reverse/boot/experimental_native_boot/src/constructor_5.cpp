#include "constructor_5.h"

namespace {
using W=GuestWord32;
using A=GuestAddress32;
void Require(bool ok,const char* why) { if(!ok) throw BootError(why); }
W Imm(W w) { const W low=w&65535u; return low&32768u ? low|0xFFFF0000u : low; }
std::string Words(const auto& values) {
    std::string text;
    for(auto value:values) text+=Constructor1Hex(value,4);
    return text;
}
void Emit(Constructor5BodyResult& r,std::string_view kind,A pc,A address,unsigned size,W value) {
    r.events.push_back({kind,pc,address,size,Constructor1Hex(value,size ? size : 4)});
}
}

Constructor5BodyResult RunConstructor5Body(const BootImage& image,std::array<W,4> arguments,
    W sda,W stack,W return_pc) {
    image.RequirePalFixtureDigest();
    Constructor5BodyResult r;
    r.arguments=arguments;
    const A local=stack+Imm(image.ReadWord(0x80039B88u));
    Emit(r,"write",0x80039B88u,local,4,stack);
    r.destination=sda+Imm(image.ReadWord(0x80039B90u));
    Emit(r,"write",0x80039B98u,local+Imm(image.ReadWord(0x80039B98u)),4,return_pc);
    W delta=image.ReadWord(0x80039BA8u)&0x03FFFFFCu;
    if(delta&0x02000000u) delta|=0xFC000000u;
    const A helper=0x80039BA8u+delta;
    Require(helper==0x8040E328u,"unproven byte initializer");
    Emit(r,"call",0x80039BA8u,helper,0,0x80039BACu);
    for(unsigned i=0;i<4;++i) {
        const A pc=helper+4*i;
        // stb takes only the low eight bits; do not combine into a host word store.
        r.bytes[i]=static_cast<std::uint8_t>(arguments[i]&255u);
        Emit(r,"write",pc,r.destination+Imm(image.ReadWord(pc)),1,r.bytes[i]);
    }
    Emit(r,"return",helper+16,0x80039BACu,0,0);
    Emit(r,"read",0x80039BACu,local+Imm(image.ReadWord(0x80039BACu)),4,return_pc);
    r.r1=local+Imm(image.ReadWord(0x80039BB4u));
    Require(r.r1==stack,"constructor 5 stack not restored");
    r.return_pc=return_pc;
    Emit(r,"return",0x80039BB8u,return_pc,0,0);
    return r;
}

Constructor5Result ExecuteSixthConstructor(const BootImage& image,const PreEntryOracle& pre,
    const Constructor4Result& previous,const Constructor4Oracle& prior_evidence,const Constructor5Oracle& oracle) {
    image.RequirePalFixtureDigest();
    Require(pre.present && prior_evidence.present && oracle.present,"constructor 5 requires dependency evidence");
    const std::array<std::string,9> startup{pre.memory_0x805f1f30,pre.memory_0x805f1f38,
        pre.lowmem_0x80000030,pre.lowmem_0x80000034,pre.lowmem_0x80000044,pre.lowmem_0x800000f4,
        pre.lowmem_0x800030e4,pre.lowmem_0x800030e6,pre.bi2_bytes};
    Require(oracle.startup_inputs==startup && prior_evidence.startup_inputs==startup,"constructor 5 startup lineage differs");
    Require(previous.constructors_executed==5 && previous.pc==0x803796F8u && previous.cursor==0x804AAC74u &&
        previous.next_constructor==0x80039B88u && previous.lr==previous.next_constructor &&
        image.ReadWord(previous.cursor)==previous.next_constructor,"constructor 5 executed out of order");
    const auto get=[&](const std::string& key)->const std::string& {
        const auto it=oracle.fields.find(key);
        if(it==oracle.fields.end()) throw BootError("missing constructor 5 field: "+key);
        return it->second;
    };
    const auto compare=[&](const std::string& key,std::uint64_t value,unsigned size=4) {
        if(get(key)!=Constructor1Hex(value,size)) throw BootError("constructor 5 mismatch: "+key);
    };
    for(const auto& [key,value]:prior_evidence.fields) {
        if(!key.starts_with("final_") || key=="final_pc" || key=="final_lr") continue;
        const std::string suffix=key=="final_destination" ? "c4_object" : key.substr(6);
        Require(get("entry_"+suffix)==value,"constructor 4->5 observable lineage differs");
    }
    compare("entry_pc",previous.next_constructor); compare("entry_lr",previous.pc+4);
    compare("entry_r31",previous.cursor); compare("entry_r1",previous.body.r1);
    compare("entry_r13",NativeBootManifest::sdata_base);
    const auto& old=previous.body;
    const std::array<W,3> node{old.registration.next,old.registration.destructor,old.registration.object};
    Require(get("entry_node")==Words(node) && get("entry_c4_object")==Words(old.object),
        "native registration/object lineage differs");
    compare("entry_source",old.head);
    const auto& c3=previous.preserved;
    compare("entry_f0",c3.body.fpr01[0],8); compare("entry_f1",c3.body.fpr01[1],8);
    Require(get("entry_c3_vector")==Words(c3.body.vector),"native C3 lineage differs");
    // Inputs are the four original li immediates; no prior destination byte is read.
    const std::array<W,4> arguments{Imm(image.ReadWord(0x80039B94u)),Imm(image.ReadWord(0x80039B9Cu)),
        Imm(image.ReadWord(0x80039BA0u)),Imm(image.ReadWord(0x80039BA4u))};
    auto body=RunConstructor5Body(image,arguments,NativeBootManifest::sdata_base,old.r1,previous.pc+4);
    const A return_pc=body.return_pc;
    const A cursor=previous.cursor+Imm(image.ReadWord(return_pc));
    const A next=image.ReadWord(cursor);
    Emit(body,"read",return_pc+4,cursor,4,next);
    const bool more=next!=0;
    const A dispatch=more ? return_pc+12+Imm(image.ReadWord(return_pc+12)&0xFFFCu) : return_pc+16;
    Emit(body,"branch",return_pc+12,dispatch,0,more);
    Require(more,"unexpected constructor table end");
    Constructor5Result r{previous,std::move(body),dispatch+4,cursor,next,next,previous.constructors_executed+1};
    std::string output;
    for(auto byte:r.body.bytes) output+=Constructor1Hex(byte,1);
    Require(get("final_destination")==output,"constructor 5 byte output mismatch");
    compare("final_pc",r.pc); compare("final_lr",r.lr); compare("final_r31",r.cursor); compare("final_r12",r.next_constructor);
    compare("final_r0",return_pc); compare("final_r1",r.body.r1); compare("final_r3",r.body.destination);
    for(unsigned i=0;i<4;++i) compare("final_r"+std::to_string(i+4),r.body.arguments[i]);
    for(unsigned i=0;i<32;++i) {
        if(i!=0 && i!=1 && (i<3 || i>7) && i!=12 && i!=31)
            Require(get("entry_r"+std::to_string(i))==get("final_r"+std::to_string(i)),"unexplained GPR change");
        Require(get("entry_f"+std::to_string(i))==get("final_f"+std::to_string(i)),"unexplained FPR change");
    }
    for(const auto key:{"fpscr","msr","ctr","xer","context","owner","fragment_id","fragment_slot",
        "prior_vectors","prior_angles","c2_vectors","c3_vector","c4_object","node","source","neighbors"})
        Require(get(std::string("entry_")+key)==get(std::string("final_")+key),"unexplained surviving effect");
    const W cr=static_cast<W>(std::stoul(get("entry_cr"),nullptr,16));
    const W xer=static_cast<W>(std::stoul(get("entry_xer"),nullptr,16));
    compare("final_cr",ConstructorWalkerCompareCr(cr,xer,next));
    return r;
}
