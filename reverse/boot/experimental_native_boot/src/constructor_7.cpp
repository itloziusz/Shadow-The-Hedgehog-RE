#include "constructor_7.h"

namespace {
using W=GuestWord32; using A=GuestAddress32;
void Require(bool ok,const char* why) { if(!ok)throw BootError(why); }
W Imm(W w) { const W v=w&65535u;return v&32768u ? v|0xFFFF0000u:v; }
A Address(const BootImage& im,A hi,A lo) {return ((im.ReadWord(hi)&65535u)<<16)+Imm(im.ReadWord(lo));}
A Target(const BootImage& im,A pc) {W d=im.ReadWord(pc)&0x3FFFFFCu;if(d&0x2000000u)d|=0xFC000000u;return pc+d;}
std::string Words(const auto& values) {std::string s;for(auto v:values)s+=Constructor1Hex(v,4);return s;}
void Emit(Constructor7BodyResult& r,std::string_view kind,A pc,A address,unsigned size,W value) {
    r.events.push_back({kind,pc,address,size,Constructor1Hex(value,size ? size:4)});
}
void Token(Constructor7BodyResult& r,std::string_view kind,A pc,A address,unsigned reg) {
    r.events.push_back({kind,pc,address,4,"ENTRY_R"+std::to_string(reg)});
}
// This is the proved null-initialization specialization, not a general pointer
// replacement/release service. Branches are evaluated from actual native state.
A NullBranch(const BootImage& im,Constructor7BodyResult& r,A pc,W value) {
    const bool null=value==0;
    const A next=null ? pc+Imm(im.ReadWord(pc)&0xFFFCu):pc+4;
    Emit(r,"branch",pc,next,0,null);
    Require(null,"unproved non-null release/allocation dependency");
    return next;
}
}

Constructor7BodyResult RunConstructor7Body(const BootImage& im,A old_head,W sda,W stack,W cursor,W ret) {
    im.RequirePalFixtureDigest();
    Constructor7BodyResult r;
    r.head=old_head;
    const A outer=stack+Imm(im.ReadWord(0x8004336Cu));
    const A nodes=Address(im,0x80043374u,0x80043384u);
    Emit(r,"write",0x8004336Cu,outer,4,stack);
    Emit(r,"write",0x8004337Cu,outer+20,4,ret);
    Emit(r,"write",0x80043380u,outer+12,4,cursor);
    for(unsigned i=0;i<4;++i) {
        const A call=0x80043388u+24*i, regcall=call+16;
        const A argpc=i ? call-4:0x80043378u;
        const A object=sda+Imm(im.ReadWord(argpc));
        r.object_addresses[i]=object;
        const A init=Target(im,call),local=outer+Imm(im.ReadWord(init));
        Emit(r,"call",call,init,0,call+4);
        Emit(r,"write",init,local,4,outer);
        const W incoming=Imm(im.ReadWord(init+8)); // li r4,0, before helper call
        Emit(r,"write",init+12,local+20,4,call+4);
        Emit(r,"write",init+20,local+12,4,nodes);
        r.objects[i]=Imm(im.ReadWord(init+16)); // required original store, no old read
        Emit(r,"write",init+28,object+Imm(im.ReadWord(init+28)),4,r.objects[i]);
        const A helper=Target(im,init+32),frame=local+Imm(im.ReadWord(helper));
        Emit(r,"call",init+32,helper,0,init+36);
        Emit(r,"write",helper,frame,4,local);
        const bool compact=i==1; // distinct, pinned second helper saves only r30/r31
        const unsigned saved=compact ? 30:29;
        const W saveoffset=Imm(im.ReadWord(helper+12));
        Emit(r,"write",helper+8,frame+Imm(im.ReadWord(helper+8)),4,init+36);
        for(unsigned reg=saved;reg<=31;++reg) {
            const A slot=frame+saveoffset+4*(reg-saved);
            if(reg==31)Emit(r,"write",helper+12,slot,4,object);
            else Token(r,"write",helper+12,slot,reg);
        }
        Emit(r,"read",helper+24,object,4,r.objects[i]);
        const A input_test=NullBranch(im,r,helper+32,r.objects[i]);
        const A epilogue=NullBranch(im,r,input_test+4,incoming);
        for(unsigned reg=saved;reg<=31;++reg) {
            const A slot=frame+Imm(im.ReadWord(epilogue))+4*(reg-saved);
            if(reg==31)Emit(r,"read",epilogue,slot,4,object);
            else Token(r,"read",epilogue,slot,reg);
        }
        Emit(r,"read",epilogue+4,frame+Imm(im.ReadWord(epilogue+4)),4,init+36);
        Require(frame+Imm(im.ReadWord(epilogue+12))==local,"reset helper stack not restored");
        Emit(r,"return",epilogue+16,init+36,0,0);
        Emit(r,"read",init+36,local+20,4,call+4);
        Emit(r,"read",init+44,local+12,4,nodes);
        Require(local+Imm(im.ReadWord(init+52))==outer,"initializer stack not restored");
        Emit(r,"return",init+56,call+4,0,0);
        const A destructor=Address(im,call+4,call+12);
        const A node=nodes+Imm(im.ReadWord(call+8));
        r.node_addresses[i]=node;
        const A link=Target(im,regcall);
        Require(link==0x803A1210u,"unproved registration helper");
        Emit(r,"call",regcall,link,0,regcall+4);
        r.head_address=sda+Imm(im.ReadWord(link));
        Emit(r,"read",link,r.head_address,4,r.head);
        r.registrations[i]={r.head,destructor,object};
        const std::array<W,3> values{r.head,destructor,object};
        for(unsigned j=0;j<3;++j)Emit(r,"write",link+4+4*j,node+4*j,4,values[j]);
        r.head=node;
        Emit(r,"write",link+16,r.head_address,4,r.head);
        Emit(r,"return",link+20,regcall+4,0,0);
    }
    Emit(r,"read",0x800433E4u,outer+20,4,ret);
    Emit(r,"read",0x800433E8u,outer+12,4,cursor);
    r.r1=outer+Imm(im.ReadWord(0x800433F0u));r.return_pc=ret;
    Require(r.r1==stack,"constructor 7 stack not restored");
    Emit(r,"return",0x800433F4u,ret,0,0);
    return r;
}

Constructor7Result ExecuteEighthConstructor(const BootImage& im,const PreEntryOracle& pre,
    const Constructor6Result& previous,const Constructor6Oracle& prior,const Constructor7Oracle& proof) {
    im.RequirePalFixtureDigest();
    Require(pre.present && prior.present && proof.present,"constructor 7 requires dependency evidence");
    const std::array<std::string,9> startup{pre.memory_0x805f1f30,pre.memory_0x805f1f38,
        pre.lowmem_0x80000030,pre.lowmem_0x80000034,pre.lowmem_0x80000044,pre.lowmem_0x800000f4,
        pre.lowmem_0x800030e4,pre.lowmem_0x800030e6,pre.bi2_bytes};
    Require(prior.startup_inputs==startup && proof.startup_inputs==startup,"constructor 7 startup lineage differs");
    Require(previous.constructors_executed==7 && previous.pc==0x803796F8u && previous.cursor==0x804AAC7Cu &&
        previous.next_constructor==0x8004336Cu && previous.lr==previous.next_constructor &&
        im.ReadWord(previous.cursor)==previous.next_constructor,"constructor 7 executed out of order");
    const auto get=[&](const std::string& k)->const std::string& {
        const auto it=proof.fields.find(k);if(it==proof.fields.end())throw BootError("missing constructor 7 field: "+k);return it->second;
    };
    const auto compare=[&](const std::string& k,std::uint64_t v,unsigned size=4) {
        if(get(k)!=Constructor1Hex(v,size))throw BootError("constructor 7 mismatch: "+k);
    };
    for(const auto& [key,value]:prior.fields) {
        if(!key.starts_with("final_") || key=="final_pc" || key=="final_lr")continue;
        std::string suffix=key.substr(6);
        if(suffix=="destination")suffix="c6_array";
        else if(suffix=="node")suffix="c6_node";
        Require(get("entry_"+suffix)==value,"constructor 6->7 observable lineage differs");
    }
    compare("entry_pc",previous.next_constructor);compare("entry_lr",previous.pc+4);
    compare("entry_r31",previous.cursor);compare("entry_r1",previous.body.r1);
    compare("entry_r13",NativeBootManifest::sdata_base);compare("entry_source",previous.body.head);
    const auto& old=previous.body;
    Require(get("entry_c6_node")==Words(std::array<W,3>{old.registration.next,old.registration.destructor,old.registration.object}),"prior native node damaged");
    std::string array;for(const auto& item:old.elements)for(auto v:item)array+=Constructor1Hex(v,2);
    Require(get("entry_c6_array")==array,"prior native array damaged");
    auto body=RunConstructor7Body(im,old.head,NativeBootManifest::sdata_base,old.r1,previous.cursor,previous.pc+4);
    const A ret=body.return_pc,cursor=previous.cursor+Imm(im.ReadWord(ret)),next=im.ReadWord(cursor);
    Emit(body,"read",ret+4,cursor,4,next);
    const bool more=next!=0;
    const A dispatch=more ? ret+12+Imm(im.ReadWord(ret+12)&0xFFFCu):ret+16;
    Emit(body,"branch",ret+12,dispatch,0,more);Require(more,"unexpected constructor table end");
    Constructor7Result r{previous,std::move(body),dispatch+4,cursor,next,next,previous.constructors_executed+1};
    Require(get("final_destination")==Words(r.body.objects),"constructor 7 object output mismatch");
    std::string nodes;for(const auto& n:r.body.registrations)nodes+=Words(std::array<W,3>{n.next,n.destructor,n.object});
    Require(get("final_node")==nodes,"constructor 7 registration output mismatch");
    compare("final_source",r.body.head);compare("final_pc",r.pc);compare("final_lr",r.lr);
    compare("final_r31",r.cursor);compare("final_r12",r.next_constructor);compare("final_r0",ret);compare("final_r1",r.body.r1);
    compare("final_r3",r.body.registrations[3].object);compare("final_r4",r.body.registrations[3].destructor);
    compare("final_r5",r.body.node_addresses[3]);
    for(unsigned i=0;i<32;++i) {
        if(i!=0 && i!=1 && (i<3 || i>5) && i!=12 && i!=31)
            Require(get("entry_r"+std::to_string(i))==get("final_r"+std::to_string(i)),"unexplained GPR change");
        Require(get("entry_f"+std::to_string(i))==get("final_f"+std::to_string(i)),"unexplained FPR change");
    }
    for(const auto key:{"fpscr","msr","ctr","xer","context","owner","fragment_id","fragment_slot","prior_vectors",
        "prior_angles","c2_vectors","c3_vector","c4_object","c4_node","c5_bytes","c5_neighbors","c6_array","c6_node"})
        Require(get(std::string("entry_")+key)==get(std::string("final_")+key),"unexplained surviving effect");
    const W cr=static_cast<W>(std::stoul(get("entry_cr"),nullptr,16)),xer=static_cast<W>(std::stoul(get("entry_xer"),nullptr,16));
    compare("final_cr",ConstructorWalkerCompareCr(cr,xer,next));
    return r;
}
