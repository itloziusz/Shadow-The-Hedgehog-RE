#include "constructor_6.h"

namespace {
using W=GuestWord32; using A=GuestAddress32;
void Require(bool ok,const char* why) { if(!ok) throw BootError(why); }
W Imm(W w) { W v=w&65535u; return v&32768u ? v|0xFFFF0000u : v; }
A Address(const BootImage& im,A hi,A lo) { return ((im.ReadWord(hi)&65535u)<<16)+Imm(im.ReadWord(lo)); }
A Target(const BootImage& im,A pc) { W d=im.ReadWord(pc)&0x3FFFFFCu; if(d&0x2000000u)d|=0xFC000000u; return pc+d; }
std::string Words(const auto& values) { std::string s; for(auto v:values)s+=Constructor1Hex(v,4); return s; }
void Emit(Constructor6BodyResult& r,std::string_view kind,A pc,A address,unsigned size,W value) {
    r.events.push_back({kind,pc,address,size,Constructor1Hex(value,size ? size:4)});
}
}

Constructor6BodyResult RunConstructor6Body(const BootImage& im,A old_head,W sda,W stack,W cursor,W ret,
    unsigned count,std::array<W,2> values) {
    im.RequirePalFixtureDigest();
    Require(count<=4,"constructor 6 specialization exceeds proven array extent");
    Constructor6BodyResult r;
    r.count=count; r.stride=Imm(im.ReadWord(0x8003AFD0u));
    r.array_address=Address(im,0x8003AFC0u,0x8003AFD4u);
    r.element_constructor=Address(im,0x8003AFC4u,0x8003AFD8u);
    r.cleanup_destructor=Address(im,0x8003AFC8u,0x8003AFDCu);
    Require(r.element_constructor==0x8003B048u && r.stride==4,"unproven element constructor/stride");
    const A outer=stack+Imm(im.ReadWord(0x8003AFB8u));
    Emit(r,"write",0x8003AFB8u,outer,4,stack);
    Emit(r,"write",0x8003AFCCu,outer+20,4,ret);
    const A helper=Target(im,0x8003AFE4u);
    Require(helper==0x803A1500u,"unproven array helper");
    Emit(r,"call",0x8003AFE4u,helper,0,0x8003AFE8u);
    const A local=outer+Imm(im.ReadWord(helper));
    Emit(r,"write",helper,local,4,outer);
    Emit(r,"write",0x803A1508u,local+52,4,0x8003AFE8u);
    Emit(r,"write",0x803A1510u,local+44,4,cursor);
    r.events.push_back({"write",0x803A1518u,local+40,4,"ENTRY_R30"});
    r.events.push_back({"write",0x803A1520u,local+36,4,"ENTRY_R29"});
    Emit(r,"write",0x803A1528u,local+24,4,count);
    r.events.push_back({"write",0x803A152Cu,local+32,4,"ENTRY_R28"});
    Emit(r,"write",0x803A1534u,local+8,4,r.array_address);
    Emit(r,"write",0x803A1538u,local+12,4,r.stride);
    Emit(r,"write",0x803A153Cu,local+16,4,count);
    Emit(r,"write",0x803A1540u,local+20,4,r.cleanup_destructor);
    W completed=Imm(im.ReadWord(0x803A150Cu));
    Emit(r,"write",0x803A1544u,local+24,4,completed);
    Emit(r,"branch",0x803A1548u,Target(im,0x803A1548u),0,1);
    A element=r.array_address;
    for(;;) {
        Emit(r,"read",0x803A1570u,local+24,4,completed);
        const bool more=completed<count;
        Emit(r,"branch",0x803A1578u,more ? 0x803A154Cu:0x803A157Cu,0,more);
        if(!more) break;
        // The only indirect target is the pinned five-instruction, nonthrowing leaf.
        Emit(r,"call_indirect",0x803A155Cu,r.element_constructor,0,0x803A1560u);
        std::array<std::uint16_t,2> entry;
        for(unsigned i=0;i<2;++i) {
            entry[i]=static_cast<std::uint16_t>(values[i]&65535u);
            const A pc=0x8003B050u+i*4;
            Emit(r,"write",pc,element+Imm(im.ReadWord(pc)),2,entry[i]);
        }
        r.elements.push_back(entry);
        Emit(r,"return",0x8003B058u,0x803A1560u,0,0);
        Emit(r,"read",0x803A1560u,local+24,4,completed);
        element+=r.stride;
        completed+=Imm(im.ReadWord(0x803A1568u));
        Emit(r,"write",0x803A156Cu,local+24,4,completed);
    }
    Emit(r,"read",0x803A157Cu,local+16,4,count);
    const bool complete=completed>=count;
    Emit(r,"branch",0x803A1584u,complete ? 0x803A15DCu:0x803A1588u,0,complete);
    // In this specialization each pinned element returns normally; completed==count.
    // Do not invent a destructor call or treat an incomplete result as success.
    Require(complete,"unproven partial-array cleanup path");
    Emit(r,"read",0x803A15DCu,local+52,4,0x8003AFE8u);
    Emit(r,"read",0x803A15E0u,local+44,4,cursor);
    r.events.push_back({"read",0x803A15E4u,local+40,4,"ENTRY_R30"});
    r.events.push_back({"read",0x803A15E8u,local+36,4,"ENTRY_R29"});
    r.events.push_back({"read",0x803A15ECu,local+32,4,"ENTRY_R28"});
    Require(local+Imm(im.ReadWord(0x803A15F4u))==outer,"array helper stack not restored");
    Emit(r,"return",0x803A15F8u,0x8003AFE8u,0,0);
    const A destructor=Address(im,0x8003AFE8u,0x8003AFF0u);
    r.node_address=Address(im,0x8003AFECu,0x8003AFF8u);
    const W object=Imm(im.ReadWord(0x8003AFF4u)); // original null registration object
    const A link=Target(im,0x8003AFFCu);
    Require(link==0x803A1210u,"unproven registration helper");
    Emit(r,"call",0x8003AFFCu,link,0,0x8003B000u);
    r.head_address=sda+Imm(im.ReadWord(link));
    Emit(r,"read",link,r.head_address,4,old_head);
    r.registration={old_head,destructor,object};
    for(const auto [offset,value]:std::array<std::pair<W,W>,3>{{{0,old_head},{4,destructor},{8,object}}})
        Emit(r,"write",link+4+offset,r.node_address+offset,4,value);
    r.head=r.node_address;
    Emit(r,"write",link+16,r.head_address,4,r.head);
    Emit(r,"return",link+20,0x8003B000u,0,0);
    Emit(r,"read",0x8003B000u,outer+20,4,ret);
    r.r1=outer+Imm(im.ReadWord(0x8003B008u)); r.return_pc=ret;
    Require(r.r1==stack,"constructor 6 stack not restored");
    Emit(r,"return",0x8003B00Cu,ret,0,0);
    return r;
}

Constructor6Result ExecuteSeventhConstructor(const BootImage& im,const PreEntryOracle& pre,
    const Constructor5Result& previous,const Constructor5Oracle& prior,const Constructor6Oracle& proof) {
    im.RequirePalFixtureDigest();
    Require(pre.present && prior.present && proof.present,"constructor 6 requires dependency evidence");
    const std::array<std::string,9> startup{pre.memory_0x805f1f30,pre.memory_0x805f1f38,
        pre.lowmem_0x80000030,pre.lowmem_0x80000034,pre.lowmem_0x80000044,pre.lowmem_0x800000f4,
        pre.lowmem_0x800030e4,pre.lowmem_0x800030e6,pre.bi2_bytes};
    Require(prior.startup_inputs==startup && proof.startup_inputs==startup,"constructor 6 startup lineage differs");
    Require(previous.constructors_executed==6 && previous.pc==0x803796F8u && previous.cursor==0x804AAC78u &&
        previous.next_constructor==0x8003AFB8u && previous.lr==previous.next_constructor &&
        im.ReadWord(previous.cursor)==previous.next_constructor,"constructor 6 executed out of order");
    const auto get=[&](const std::string& k)->const std::string& {
        const auto it=proof.fields.find(k); if(it==proof.fields.end())throw BootError("missing constructor 6 field: "+k); return it->second;
    };
    const auto compare=[&](const std::string& k,std::uint64_t v,unsigned size=4) {
        if(get(k)!=Constructor1Hex(v,size))throw BootError("constructor 6 mismatch: "+k);
    };
    for(const auto& [key,value]:prior.fields) {
        if(!key.starts_with("final_") || key=="final_pc" || key=="final_lr")continue;
        std::string suffix=key.substr(6);
        if(suffix=="destination")suffix="c5_bytes";
        else if(suffix=="node")suffix="c4_node";
        else if(suffix=="neighbors")suffix="c5_neighbors";
        Require(get("entry_"+suffix)==value,"constructor 5->6 observable lineage differs");
    }
    compare("entry_pc",previous.next_constructor); compare("entry_lr",previous.pc+4);
    compare("entry_r31",previous.cursor); compare("entry_r1",previous.body.r1);
    compare("entry_r13",NativeBootManifest::sdata_base);
    const auto& c4=previous.preserved.body;
    compare("entry_source",c4.head);
    const std::array<W,3> oldnode{c4.registration.next,c4.registration.destructor,c4.registration.object};
    Require(get("entry_c4_node")==Words(oldnode) && get("entry_c4_object")==Words(c4.object),"prior native registration damaged");
    std::string bytes; for(auto byte:previous.body.bytes)bytes+=Constructor1Hex(byte,1);
    Require(get("entry_c5_bytes")==bytes,"prior native bytes damaged");
    const unsigned count=Imm(im.ReadWord(0x8003AFE0u));
    const std::array<W,2> values{Imm(im.ReadWord(0x8003B048u)),Imm(im.ReadWord(0x8003B04Cu))};
    auto body=RunConstructor6Body(im,c4.head,NativeBootManifest::sdata_base,previous.body.r1,previous.cursor,previous.pc+4,count,values);
    const A ret=body.return_pc,cursor=previous.cursor+Imm(im.ReadWord(ret)),next=im.ReadWord(cursor);
    Emit(body,"read",ret+4,cursor,4,next);
    const bool more=next!=0;
    const A dispatch=more ? ret+12+Imm(im.ReadWord(ret+12)&0xFFFCu):ret+16;
    Emit(body,"branch",ret+12,dispatch,0,more); Require(more,"unexpected constructor table end");
    Constructor6Result r{previous,std::move(body),dispatch+4,cursor,next,next,previous.constructors_executed+1};
    std::string output; for(const auto& item:r.body.elements)for(auto v:item)output+=Constructor1Hex(v,2);
    Require(get("final_destination")==output,"constructor 6 array output mismatch");
    const std::array<W,3> node{r.body.registration.next,r.body.registration.destructor,r.body.registration.object};
    Require(get("final_node")==Words(node),"constructor 6 registration mismatch");
    compare("final_source",r.body.head); compare("final_pc",r.pc); compare("final_lr",r.lr);
    compare("final_r31",r.cursor); compare("final_r12",r.next_constructor); compare("final_r0",ret); compare("final_r1",r.body.r1);
    compare("final_r3",r.body.registration.object); compare("final_r4",r.body.registration.destructor);
    compare("final_r5",r.body.node_address); compare("final_r6",r.body.stride); compare("final_r7",r.body.count);
    compare("final_ctr",r.body.element_constructor);
    for(unsigned i=0;i<32;++i) {
        if(i!=0 && i!=1 && (i<3 || i>7) && i!=12 && i!=31)
            Require(get("entry_r"+std::to_string(i))==get("final_r"+std::to_string(i)),"unexplained GPR change");
        Require(get("entry_f"+std::to_string(i))==get("final_f"+std::to_string(i)),"unexplained FPR change");
    }
    for(const auto key:{"fpscr","msr","xer","context","owner","fragment_id","fragment_slot","prior_vectors",
        "prior_angles","c2_vectors","c3_vector","c4_object","c4_node","c5_bytes","c5_neighbors"})
        Require(get(std::string("entry_")+key)==get(std::string("final_")+key),"unexplained surviving effect");
    const W cr=static_cast<W>(std::stoul(get("entry_cr"),nullptr,16)),xer=static_cast<W>(std::stoul(get("entry_xer"),nullptr,16));
    compare("final_cr",ConstructorWalkerCompareCr(cr,xer,next));
    return r;
}
