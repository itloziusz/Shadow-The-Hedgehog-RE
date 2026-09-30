#include "constructor_4.h"

namespace {
using W=GuestWord32;
using A=GuestAddress32;
void Require(bool ok,const char* why) { if(!ok) throw BootError(why); }
W Imm(W w) { const W low=w&65535u; return low&32768u ? low|0xFFFF0000u : low; }
A Address(const BootImage& image,A lis,A addi) {
    return ((image.ReadWord(lis)&65535u)<<16)+Imm(image.ReadWord(addi));
}
A Target(const BootImage& image,A pc) {
    W d=image.ReadWord(pc)&0x03FFFFFCu;
    if(d&0x02000000u) d|=0xFC000000u;
    return pc+d;
}
std::string Words(const auto& values) {
    std::string text;
    for(auto value:values) text+=Constructor1Hex(value,4);
    return text;
}
void Emit(Constructor4BodyResult& r,std::string_view kind,A pc,A address,unsigned size,W value) {
    r.events.push_back({kind,pc,address,size,Constructor1Hex(value,size ? size : 4)});
}
W CrtHead(const BootImage& image,const CrtSemantics& crt,A address) {
    Require(crt.pc==0x80003170u && crt.zero_ranges==3 && crt.identity_copies==10 &&
        !crt.copies_executed && !crt.guest_image_zeroed,"constructor 4 requires verified CRT");
    for(unsigned i=0;i<static_cast<unsigned>(crt.zero_ranges);++i) {
        const A begin=image.ReadWord(NativeBootManifest::zero_table.raw+8*i);
        const W size=image.ReadWord(NativeBootManifest::zero_table.raw+8*i+4);
        if(address>=begin && HostByteCount64{address}+4<=HostByteCount64{begin}+size) return 0;
    }
    throw BootError("destructor-list head has no proven CRT initializer");
}
}

Constructor4BodyResult RunConstructor4Body(const BootImage& image,A old_head,W sda,W stack,W return_pc) {
    image.RequirePalFixtureDigest();
    Constructor4BodyResult r;
    const A local=stack+Imm(image.ReadWord(0x80035D4Cu));
    Emit(r,"write",0x80035D4Cu,local,4,stack);
    r.object_address=sda+Imm(image.ReadWord(0x80035D54u));
    Emit(r,"write",0x80035D58u,local+Imm(image.ReadWord(0x80035D58u)),4,return_pc);
    const A initialize=Target(image,0x80035D5Cu);
    Require(initialize==0x80028E90u,"unproven object initializer");
    Emit(r,"call",0x80035D5Cu,initialize,0,0x80035D60u);
    // The original li immediate initializes both words; old contents are not read.
    const W value=Imm(image.ReadWord(initialize));
    for(unsigned i=0;i<2;++i) {
        r.object[i]=value;
        const A pc=initialize+4+i*4;
        Emit(r,"write",pc,r.object_address+Imm(image.ReadWord(pc)),4,value);
    }
    Emit(r,"return",0x80028E9Cu,0x80035D60u,0,0);
    const A destructor=Address(image,0x80035D60u,0x80035D68u);
    r.node_address=Address(image,0x80035D64u,0x80035D6Cu);
    const A link=Target(image,0x80035D70u);
    Require(link==0x803A1210u,"unproven destructor registration helper");
    Emit(r,"call",0x80035D70u,link,0,0x80035D74u);
    r.head_address=sda+Imm(image.ReadWord(link));
    Emit(r,"read",link,r.head_address,4,old_head);
    r.registration.next=old_head;
    Emit(r,"write",link+4,r.node_address+Imm(image.ReadWord(link+4)),4,r.registration.next);
    r.registration.destructor=destructor;
    Emit(r,"write",link+8,r.node_address+Imm(image.ReadWord(link+8)),4,r.registration.destructor);
    r.registration.object=r.object_address;
    Emit(r,"write",link+12,r.node_address+Imm(image.ReadWord(link+12)),4,r.registration.object);
    Require(sda+Imm(image.ReadWord(link+16))==r.head_address,"list-head producer/consumer mismatch");
    r.head=r.node_address;
    Emit(r,"write",link+16,r.head_address,4,r.head);
    Emit(r,"return",link+20,0x80035D74u,0,0);
    Emit(r,"read",0x80035D74u,local+Imm(image.ReadWord(0x80035D74u)),4,return_pc);
    r.r1=local+Imm(image.ReadWord(0x80035D7Cu));
    Require(r.r1==stack,"constructor 4 stack not restored");
    r.return_pc=return_pc;
    Emit(r,"return",0x80035D80u,return_pc,0,0);
    return r;
}

Constructor4Result ExecuteFifthConstructor(const BootImage& image,const CrtSemantics& crt,
    const PreEntryOracle& pre,const Constructor3Result& previous,
    const Constructor3Oracle& prior_evidence,const Constructor4Oracle& oracle) {
    image.RequirePalFixtureDigest();
    Require(pre.present && prior_evidence.present && oracle.present,"constructor 4 requires dependency evidence");
    const std::array<std::string,9> startup{pre.memory_0x805f1f30,pre.memory_0x805f1f38,
        pre.lowmem_0x80000030,pre.lowmem_0x80000034,pre.lowmem_0x80000044,pre.lowmem_0x800000f4,
        pre.lowmem_0x800030e4,pre.lowmem_0x800030e6,pre.bi2_bytes};
    Require(oracle.startup_inputs==startup && prior_evidence.startup_inputs==startup,"constructor 4 startup lineage differs");
    Require(previous.constructors_executed==4 && previous.pc==0x803796F8u && previous.cursor==0x804AAC70u &&
        previous.next_constructor==0x80035D4Cu && previous.lr==previous.next_constructor &&
        image.ReadWord(previous.cursor)==previous.next_constructor,"constructor 4 executed out of order");
    const auto get=[&](const std::string& key)->const std::string& {
        const auto it=oracle.fields.find(key);
        if(it==oracle.fields.end()) throw BootError("missing constructor 4 field: "+key);
        return it->second;
    };
    const auto compare=[&](const std::string& key,std::uint64_t value,unsigned size=4) {
        if(get(key)!=Constructor1Hex(value,size)) throw BootError("constructor 4 mismatch: "+key);
    };
    for(const auto& [key,value]:prior_evidence.fields) {
        if(!key.starts_with("final_") || key=="final_pc" || key=="final_lr" || key=="final_source") continue;
        const std::string suffix=key=="final_destination" ? "c3_vector" : key.substr(6);
        Require(get("entry_"+suffix)==value,"constructor 3->4 observable lineage differs");
    }
    const auto& c2=previous.preserved;
    const auto& c1=c2.preserved;
    compare("entry_pc",previous.next_constructor); compare("entry_lr",previous.pc+4);
    compare("entry_r31",previous.cursor); compare("entry_r1",c2.body.r1);
    compare("entry_r13",NativeBootManifest::sdata_base);
    compare("entry_f0",previous.body.fpr01[0],8); compare("entry_f1",previous.body.fpr01[1],8);
    Require(get("entry_c3_vector")==Words(previous.body.vector) &&
        get("entry_c2_vectors")==Words(c2.body.vectors[0])+Words(c2.body.vectors[1]) &&
        get("entry_prior_vectors")==Words(c1.vectors) && get("entry_prior_angles")==Words(c1.angles),
        "earlier native globals differ");
    const W sda=NativeBootManifest::sdata_base;
    const A head_address=sda+Imm(image.ReadWord(0x803A1210u));
    const W head=CrtHead(image,crt,head_address);
    for(const auto key:{"after_crt_source","before_walker_source","constructor_4_entry_source","entry_source"})
        compare(key,head);
    auto body=RunConstructor4Body(image,head,sda,c2.body.r1,previous.pc+4);
    const A return_pc=body.return_pc;
    const A cursor=previous.cursor+Imm(image.ReadWord(return_pc));
    const A next=image.ReadWord(cursor);
    Emit(body,"read",return_pc+4,cursor,4,next);
    const bool more=next!=0;
    const A dispatch=more ? return_pc+12+Imm(image.ReadWord(return_pc+12)&0xFFFCu) : return_pc+16;
    Emit(body,"branch",return_pc+12,dispatch,0,more);
    Require(more,"unexpected constructor table end");
    Constructor4Result r{previous,std::move(body),dispatch+4,cursor,next,next,previous.constructors_executed+1};
    Require(get("final_destination")==Words(r.body.object),"constructor 4 object mismatch");
    const std::array<W,3> node{r.body.registration.next,r.body.registration.destructor,r.body.registration.object};
    Require(get("final_node")==Words(node),"constructor 4 registration mismatch");
    compare("final_source",r.body.head);
    compare("final_pc",r.pc); compare("final_lr",r.lr); compare("final_r31",r.cursor); compare("final_r12",r.next_constructor);
    compare("final_r0",return_pc); compare("final_r1",r.body.r1); compare("final_r3",r.body.object_address);
    compare("final_r4",r.body.registration.destructor); compare("final_r5",r.body.node_address);
    for(unsigned i=0;i<32;++i) {
        if(i!=0 && i!=1 && i!=3 && i!=4 && i!=5 && i!=12 && i!=31)
            Require(get("entry_r"+std::to_string(i))==get("final_r"+std::to_string(i)),"unexplained GPR change");
        Require(get("entry_f"+std::to_string(i))==get("final_f"+std::to_string(i)),"unexplained FPR change");
    }
    for(const auto key:{"fpscr","msr","ctr","xer","context","owner","fragment_id","fragment_slot",
        "prior_vectors","prior_angles","c2_vectors","c3_vector"})
        Require(get(std::string("entry_")+key)==get(std::string("final_")+key),"unexplained surviving effect");
    const W cr=static_cast<W>(std::stoul(get("entry_cr"),nullptr,16));
    const W xer=static_cast<W>(std::stoul(get("entry_xer"),nullptr,16));
    compare("final_cr",ConstructorWalkerCompareCr(cr,xer,next));
    return r;
}
