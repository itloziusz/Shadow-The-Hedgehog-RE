#include "constructor_3.h"

namespace {
using W = GuestWord32;
using A = GuestAddress32;
void Require(bool ok, const char* why) { if (!ok) throw BootError(why); }
W Imm(W w) { const W v=w&65535u; return v&32768u ? v|0xFFFF0000u : v; }
std::string Words(const auto& values) {
    std::string text;
    for (auto v : values) text += Constructor1Hex(v,4);
    return text;
}
void Emit(Constructor3BodyResult& r, std::string_view kind, A pc, A address, unsigned size, std::uint64_t value) {
    r.events.push_back({kind,pc,address,size,Constructor1Hex(value,size ? size : 4)});
}
}

Constructor3BodyResult RunConstructor3Body(const BootImage& image, std::array<W,2> inputs,
    W sda2, W return_pc, bool fpu_enabled) {
    image.RequirePalFixtureDigest();
    Require(fpu_enabled,"constructor 3 requires completed FPU initialization");
    Constructor3BodyResult r;
    const auto load = [&](A pc, unsigned index, unsigned fpr) {
        Emit(r,"read",pc,sda2+Imm(image.ReadWord(pc)),4,inputs[index]);
        r.fpr01[fpr]=WidenConstructor2Single(inputs[index]);
        Emit(r,"fpr",pc,fpr,8,r.fpr01[fpr]);
        return inputs[index];
    };
    const W x=load(0x8002218Cu,0,1);
    W r3=(image.ReadWord(0x80022190u)&65535u)<<16;
    const W y=load(0x80022194u,1,0);
    // stfsu stores using the old base plus displacement, then updates r3.
    r3+=Imm(image.ReadWord(0x80022198u));
    r.destination=r3;
    r.vector[0]=x;
    Emit(r,"write",0x80022198u,r3,4,x);
    r.vector[1]=y;
    Emit(r,"write",0x8002219Cu,r3+Imm(image.ReadWord(0x8002219Cu)),4,y);
    r.vector[2]=x;
    Emit(r,"write",0x800221A0u,r3+Imm(image.ReadWord(0x800221A0u)),4,x);
    Emit(r,"return",0x800221A4u,return_pc,0,0);
    return r;
}

Constructor3Result ExecuteFourthConstructor(const BootImage& image, const PreEntryOracle& pre,
    const Constructor2Result& previous, const Constructor2Oracle& prior_evidence,
    const Constructor3Oracle& oracle) {
    image.RequirePalFixtureDigest();
    Require(pre.present && oracle.present && prior_evidence.present,"constructor 3 requires dependency evidence");
    const std::array<std::string,9> startup{pre.memory_0x805f1f30,pre.memory_0x805f1f38,
        pre.lowmem_0x80000030,pre.lowmem_0x80000034,pre.lowmem_0x80000044,pre.lowmem_0x800000f4,
        pre.lowmem_0x800030e4,pre.lowmem_0x800030e6,pre.bi2_bytes};
    Require(oracle.startup_inputs==startup && prior_evidence.startup_inputs==startup,"constructor 3 startup lineage differs");
    Require(previous.constructors_executed==3 && previous.pc==0x803796F8u && previous.cursor==0x804AAC6Cu &&
        previous.next_constructor==0x8002218Cu && previous.lr==previous.next_constructor &&
        image.ReadWord(previous.cursor)==previous.next_constructor,"constructor 3 out of order");
    const auto& c1=previous.preserved;
    Require(c1.fpu_enabled && c1.fpu_owner==c1.context && c1.fragment && c1.exception_save,
        "constructor 3 required prior effects missing");
    const auto get = [&](const std::string& key) -> const std::string& {
        const auto it=oracle.fields.find(key);
        if(it==oracle.fields.end()) throw BootError("missing constructor 3 field: "+key);
        return it->second;
    };
    const auto compare = [&](const std::string& key, std::uint64_t v, unsigned size=4) {
        if(get(key)!=Constructor1Hex(v,size)) throw BootError("constructor 3 mismatch: "+key);
    };
    // Bind the full observable entry to the already validated C2 boundary.
    // Oracle registers/context are comparisons only, never native initializers.
    for (const auto& [key,value] : prior_evidence.fields) {
        if (!key.starts_with("final_") || key=="final_pc" || key=="final_lr" || key=="final_source") continue;
        const std::string suffix=key=="final_destination" ? "c2_vectors" : key.substr(6);
        Require(get("entry_"+suffix)==value,"constructor 2->3 observable lineage differs");
    }
    compare("entry_pc",previous.next_constructor); compare("entry_lr",previous.pc+4);
    compare("entry_r31",previous.cursor); compare("entry_r1",previous.body.r1);
    compare("entry_r2",NativeBootManifest::sdata2_base);
    compare("entry_f0",previous.body.fpr01[0],8); compare("entry_f1",previous.body.fpr01[1],8);
    Require(get("entry_c2_vectors")==Words(previous.body.vectors[0])+Words(previous.body.vectors[1]),
        "constructor 2 native vectors differ");
    Require(get("entry_prior_vectors")==Words(c1.vectors) && get("entry_prior_angles")==Words(c1.angles),
        "constructor 1 native globals differ");
    compare("entry_fpscr",c1.fpscr); compare("entry_owner",c1.fpu_owner);
    Require((std::stoul(get("entry_msr"),nullptr,16)&0x2000u)!=0,"constructor 3 oracle FP unavailable");
    const W sda2=NativeBootManifest::sdata2_base;
    const std::array<W,2> inputs{image.ReadWord(sda2+Imm(image.ReadWord(0x8002218Cu))),
        image.ReadWord(sda2+Imm(image.ReadWord(0x80022194u)))};
    for (const auto key : {"after_crt_source","before_walker_source","constructor_3_entry_source","entry_source","final_source"})
        Require(get(key)==Words(inputs),"constructor 3 DOL constants not preserved");
    auto body=RunConstructor3Body(image,inputs,sda2,previous.pc+4,c1.fpu_enabled);
    const A return_pc=previous.pc+4;
    const A cursor=previous.cursor+Imm(image.ReadWord(return_pc));
    const A next=image.ReadWord(cursor);
    Emit(body,"read",return_pc+4,cursor,4,next);
    const bool more=next!=0;
    const A dispatch=more ? return_pc+12+Imm(image.ReadWord(return_pc+12)&0xFFFCu) : return_pc+16;
    Emit(body,"branch",return_pc+12,dispatch,0,more);
    Require(more,"unexpected constructor table end");
    Constructor3Result r{previous,std::move(body),dispatch+4,cursor,next,next,previous.constructors_executed+1};
    Require(get("final_destination")==Words(r.body.vector),"constructor 3 destination mismatch");
    compare("final_pc",r.pc); compare("final_r31",r.cursor); compare("final_r12",r.next_constructor);
    compare("final_lr",r.lr); compare("final_r3",r.body.destination);
    compare("final_f0",r.body.fpr01[0],8); compare("final_f1",r.body.fpr01[1],8);
    for(unsigned i=0;i<32;++i) {
        if(i!=3 && i!=12 && i!=31)
            Require(get("entry_r"+std::to_string(i))==get("final_r"+std::to_string(i)),"unexpected constructor 3 GPR change");
        if(i>=2) Require(get("entry_f"+std::to_string(i))==get("final_f"+std::to_string(i)),"unexpected constructor 3 FPR change");
    }
    for(const auto key : {"fpscr","msr","ctr","xer","context","owner","fragment_id","fragment_slot",
        "prior_vectors","prior_angles","c2_vectors"})
        Require(get(std::string("entry_")+key)==get(std::string("final_")+key),"constructor 3 unexplained side effect");
    const W entry_cr=static_cast<W>(std::stoul(get("entry_cr"),nullptr,16));
    // cmplwi copies XER.SO into CR0; it does not preserve a potentially stale CR0.SO.
    const W xer=static_cast<W>(std::stoul(get("entry_xer"),nullptr,16));
    compare("final_cr",ConstructorWalkerCompareCr(entry_cr,xer,next));
    return r;
}
