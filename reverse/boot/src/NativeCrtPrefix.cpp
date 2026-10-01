#include "shadow/boot/NativeCrtPrefix.hpp"
#include <algorithm>
#include <initializer_list>
#include <stdexcept>

namespace shadow::boot {
namespace {
void CheckWords(const BootImage& image, std::uint32_t pc, std::initializer_list<std::uint32_t> words) {
    for (auto word : words) {
        if (image.ReadWord(pc) != word) throw std::runtime_error("native CRT raw word mismatch");
        pc += 4u;
    }
}
std::uint32_t ReadBytes(const NativeZeroRange& range, std::uint32_t address) {
    if ((address & 3u) || address < range.address ||
        std::uint64_t(address)+4u > std::uint64_t(range.address)+range.bytes.size())
        throw std::runtime_error("zero-range read outside produced bytes");
    std::uint32_t value=0;
    for (unsigned b=0;b<4;++b) value=(value<<8u)|range.bytes[address-range.address+b];
    return value;
}
}
std::uint32_t WithoutExternalInterrupts(std::uint32_t msr) { return msr & ~0x8000u; }
std::uint32_t NativeHandlerSlot::Load(std::uint32_t address) const {
    if(address!=0x80586CB4u||!word_)throw std::runtime_error("handler read address/state unproven");
    return *word_;
}
void NativeHandlerSlot::Store(std::uint32_t address,std::uint32_t word) {
    if(address!=0x80586CB4u)throw std::runtime_error("handler store address unproven");
    word_=word;
}
std::uint32_t RestoreExternalInterrupts(std::uint32_t msr, bool prior) {
    return prior ? msr|0x8000u : msr&~0x8000u;
}

NativeCrtRun RunImmutableNativeCrtPrefix(const BootImage& image, const NativeCrtInputs& input) {
    // Raw DOL authority, including untaken instruction variants and all table
    // words. These are fingerprints, never reference post-state inputs.
    CheckWords(image, 0x80372904u, {0x48000A75u,0x387F01FCu,0x4CC63182u,0x4BFFE37Du,0x80010014u,0x83E1000Cu,0x83C10008u,0x38210010u,0x7C0803A6u,0x4E800020u});
    CheckWords(image, 0x80373378u, {0x7C0802A6u,0x90010004u,0x9421FFD0u,0x93E1002Cu,0x93C10028u,0x93A10024u,0x3BA30000u,0x93810020u,0x3B840000u,0x48002D81u,0x3C808058u,0x57A513BAu,0x38046CB0u,0x57A6043Eu,0x7C802A14u,0x83C40000u,0x28060010u,0x7C7D1B78u,0x93840000u,0x408201A0u});
    CheckWords(image, 0x80373564u, {0x7FA3EB78u,0x48002BDDu,0x7FC3F378u,0x80010034u,0x83E1002Cu,0x83C10028u,0x83A10024u,0x83810020u,0x38210030u,0x7C0803A6u,0x4E800020u});
    CheckWords(image, 0x8037611Cu, {0x7C6000A6u,0x5464045Eu,0x7C800124u,0x54638FFEu,0x4E800020u});
    CheckWords(image, 0x80376144u, {0x2C030000u,0x7C8000A6u,0x4182000Cu,0x60858000u,0x48000008u,0x5485045Eu,0x7CA00124u,0x54838FFEu,0x4E800020u});
    CheckWords(image, 0x8000341Cu, {0x7FE803A6u,0x4E800020u});
    CheckWords(image, 0x8000315Cu, {0x3800FFFFu,0x9421FFF8u,0x90010004u,0x90010000u,0x480001D5u,0x38000000u,0x3CC08000u,0x38C60044u,0x90060000u,0x3CC08000u,0x38C600F4u,0x80C60000u});
    CheckWords(image, 0x80003340u, {0x7C0802A6u,0x90010004u,0x9421FFE8u,0x93E10014u,0x93C10010u,0x93A1000Cu,0x3C608000u,0x38035544u,0x7C1D0378u,0x48000004u,0x48000004u,0x83DD0008u,0x281E0000u,0x41820038u,0x809D0000u,0x83FD0004u,0x41820024u,0x7C1F2040u,0x4182001Cu,0x7FE3FB78u,0x7FC5F378u,0x48002161u,0x7FE3FB78u,0x7FC4F378u,0x48000085u,0x3BBD000Cu,0x4BFFFFC4u,0x3C608000u,0x380355C8u,0x7C1D0378u,0x48000004u,0x48000004u,0x80BD0004u,0x28050000u,0x4182001Cu,0x807D0000u,0x4182000Cu,0x38800000u,0x48002035u,0x3BBD0008u,0x4BFFFFE0u,0x8001001Cu,0x83E10014u,0x83C10010u,0x83A1000Cu,0x38210018u,0x7C0803A6u,0x4E800020u});
    CheckWords(image, 0x8000540Cu, {0x9421FFF0u,0x7C0802A6u,0x90010014u,0x93E1000Cu,0x7C7F1B78u,0x4800001Du,0x80010014u,0x7FE3FB78u,0x83E1000Cu,0x7C0803A6u,0x38210010u,0x4E800020u,0x28050020u,0x5484063Eu,0x38C3FFFFu,0x7C872378u,0x41800090u,0x7CC030F8u,0x540307BFu,0x41820014u,0x7CA32850u,0x3463FFFFu,0x9CE60001u,0x4082FFF8u,0x28070000u,0x4182001Cu,0x54E3C00Eu,0x54E0801Eu,0x54E4402Eu,0x7C600378u,0x7C800378u,0x7CE70378u,0x54A3D97Fu,0x3886FFFDu,0x4182002Cu,0x90E40004u,0x3463FFFFu,0x90E40008u,0x90E4000Cu,0x90E40010u,0x90E40014u,0x90E40018u,0x90E4001Cu,0x94E40020u,0x4082FFDCu,0x54A3F77Fu,0x41820010u,0x3463FFFFu,0x94E40004u,0x4082FFF8u,0x38C40003u,0x54A507BEu,0x28050000u,0x4D820020u,0x34A5FFFFu,0x9CE60001u,0x4082FFF8u,0x4E800020u});
    CheckWords(image, 0x80005544u, {0x80003100u,0x80003100u,0x24E8u,0x80005600u,0x80005600u,0x1F08u,0x80007520u,0x80007520u,0x1814u,0x80008D40u,0x80008D40u,0x4A1F08u,0x804AAC60u,0x804AAC60u,0x46Cu,0x804AB0E0u,0x804AB0E0u,0xCu,0x804AB100u,0x804AB100u,0x72418u,0x8051D520u,0x8051D520u,0x528C8u,0x805E4500u,0x805E4500u,0xAB20u,0x805F2780u,0x805F2780u,0x9DB8u,0x0u,0x0u,0x0u,0x8056FE00u,0x74700u,0x805EF020u,0x375Cu,0x805FC540u,0xACu,0x0u,0x0u});
    NativeCrtRun run{};
    run.prefix=RunImmutableNativeL2Prefix(image,input.l2);
    auto live=run.prefix.checkpoints.back();
    auto& cpu=live.native.state.machine.cpu;
    auto& state=live.native.state;
    NativeHandlerSlot registry(input.old_handler);
    std::optional<std::uint32_t> lowmem44;
    const auto save=[&](std::uint32_t pc) {
        cpu.pc=pc;
        // Preserve stack aliases: paired bytes are a view of the same stores.
        for (unsigned n=0;n<16;++n) {
            const auto off=live.native.paired_stack.base+n-live.stack.base;
            live.native.paired_stack.bytes[n]=live.stack.bytes[off];
            live.native.paired_stack.valid[n]=live.stack.valid[off];
        }
        run.checkpoints.push_back({live,registry.Value(),lowmem44});
    };
    const auto store=[&](std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        live.stack.StoreBE32(address,value);
        run.stores.push_back({pc,address,value,live.stack.LoadBE32(address)});
    };
    const auto compare=[&](std::uint32_t value,std::uint32_t rhs=0u) {
        state.cr=(state.cr&0x0FFFFFFFu)|(value<rhs?0x80000000u:value>rhs?0x40000000u:0x20000000u)
                 |((state.xer&0x80000000u)?0x10000000u:0u);
    };
    const auto call=[&](std::uint32_t site,std::uint32_t target) {
        cpu.lr=site+4u; save(target);
    };
    const auto logger=[&](std::uint32_t site) {
        call(site,0x80370C8Cu);
        const auto parent=cpu.gpr[1];cpu.gpr[1]-=0x70u;
        store(0x80370C8Cu,cpu.gpr[1],parent);
        if (state.cr&0x02000000u) throw std::runtime_error("unproven logger FPR stores");
        save(0x80370CB4u);
        for (unsigned r=3;r<=10;++r) store(0x80370CB4u+4u*(r-3u),cpu.gpr[1]+8u+4u*(r-3u),cpu.gpr[r]);
        save(0x80370CD4u);cpu.gpr[1]+=0x70u;
    };
    // Selector1 only. Both scalar arguments have connected DOL producers.
    if (cpu.pc!=0x80372904u || cpu.gpr[3]!=1u || cpu.gpr[4]!=0x803726D8u)
        throw std::runtime_error("handler arguments lack connected provenance");
    call(0x80372904u,0x80373378u);
    cpu.gpr[0]=cpu.lr;
    store(0x8037337Cu,cpu.gpr[1]+4u,cpu.gpr[0]);
    const auto parent=cpu.gpr[1];cpu.gpr[1]-=0x30u;
    store(0x80373380u,cpu.gpr[1],parent);
    store(0x80373384u,cpu.gpr[1]+0x2Cu,cpu.gpr[31]);
    store(0x80373388u,cpu.gpr[1]+0x28u,cpu.gpr[30]);
    store(0x8037338Cu,cpu.gpr[1]+0x24u,cpu.gpr[29]);cpu.gpr[29]=cpu.gpr[3];
    store(0x80373394u,cpu.gpr[1]+0x20u,cpu.gpr[28]);cpu.gpr[28]=cpu.gpr[4];
    call(0x8037339Cu,0x8037611Cu);
    cpu.gpr[3]=live.native.state.machine.msr;cpu.gpr[4]=WithoutExternalInterrupts(cpu.gpr[3]);
    save(0x80376124u);live.native.state.machine.msr=cpu.gpr[4];
    cpu.gpr[3]=(cpu.gpr[3]>>15u)&1u;save(0x803733A0u);
    cpu.gpr[4]=0x80580000u;
    cpu.gpr[5]=(cpu.gpr[29]&0xFFFFu)*4u;cpu.gpr[0]=cpu.gpr[4]+0x6CB0u;
    cpu.gpr[6]=cpu.gpr[29]&0xFFFFu;cpu.gpr[4]=cpu.gpr[0]+cpu.gpr[5];
    save(0x803733B4u);
    if (!registry.Value()) return run; // Unknown never becomes a pointer/word.
    cpu.gpr[30]=registry.Load(cpu.gpr[4]);compare(cpu.gpr[6],16u);cpu.gpr[29]=cpu.gpr[3];
    save(0x803733C0u);registry.Store(cpu.gpr[4],cpu.gpr[28]);
    run.stores.push_back({0x803733C0u,cpu.gpr[4],cpu.gpr[28],registry.Load(cpu.gpr[4])});
    save(0x803733C4u);
    if (cpu.gpr[6]==16u) throw std::runtime_error("unproven selector16 path");
    save(0x80373564u);cpu.gpr[3]=cpu.gpr[29];call(0x80373568u,0x80376144u);
    compare(cpu.gpr[3]); // Input is proven 0/1: signed cmpwi equals unsigned.
    cpu.gpr[4]=live.native.state.machine.msr;
    cpu.gpr[5]=RestoreExternalInterrupts(cpu.gpr[4],cpu.gpr[3]!=0u);
    save(0x8037615Cu);live.native.state.machine.msr=cpu.gpr[5];
    cpu.gpr[3]=(cpu.gpr[4]>>15u)&1u;save(0x8037356Cu);
    cpu.gpr[3]=cpu.gpr[30];
    cpu.gpr[0]=live.stack.LoadBE32(cpu.gpr[1]+0x34u);
    cpu.gpr[31]=live.stack.LoadBE32(cpu.gpr[1]+0x2Cu);
    cpu.gpr[30]=live.stack.LoadBE32(cpu.gpr[1]+0x28u);
    cpu.gpr[29]=live.stack.LoadBE32(cpu.gpr[1]+0x24u);
    cpu.gpr[28]=live.stack.LoadBE32(cpu.gpr[1]+0x20u);
    cpu.gpr[1]+=0x30u;cpu.lr=cpu.gpr[0];save(cpu.lr&~3u);
    if (cpu.pc!=0x80372908u) throw std::runtime_error("handler saved return differs");
    cpu.gpr[3]=cpu.gpr[31]+0x1FCu;state.cr&=~0x02000000u;
    logger(0x80372910u);save(0x80372914u);
    cpu.gpr[0]=live.stack.LoadBE32(cpu.gpr[1]+0x14u);
    cpu.gpr[31]=live.stack.LoadBE32(cpu.gpr[1]+0xCu);
    cpu.gpr[30]=live.stack.LoadBE32(cpu.gpr[1]+8u);
    cpu.gpr[1]+=16u;cpu.lr=cpu.gpr[0];save(cpu.lr&~3u);
    if (cpu.pc!=0x8000341Cu) throw std::runtime_error("cache saved return differs");
    cpu.lr=cpu.gpr[31];save(cpu.lr&~3u);
    if (cpu.pc!=0x8000315Cu) throw std::runtime_error("hardware return differs");
    cpu.gpr[0]=0xFFFFFFFFu;
    const auto sentinel_parent=cpu.gpr[1];cpu.gpr[1]-=8u;
    store(0x80003160u,cpu.gpr[1],sentinel_parent);
    store(0x80003164u,cpu.gpr[1]+4u,cpu.gpr[0]);
    store(0x80003168u,cpu.gpr[1],cpu.gpr[0]);save(0x8000316Cu);
    call(0x8000316Cu,0x80003340u);cpu.gpr[0]=cpu.lr;
    store(0x80003344u,cpu.gpr[1]+4u,cpu.gpr[0]);
    const auto walker_parent=cpu.gpr[1];cpu.gpr[1]-=24u;
    store(0x80003348u,cpu.gpr[1],walker_parent);
    store(0x8000334Cu,cpu.gpr[1]+20u,cpu.gpr[31]);
    store(0x80003350u,cpu.gpr[1]+16u,cpu.gpr[30]);
    store(0x80003354u,cpu.gpr[1]+12u,cpu.gpr[29]);
    cpu.gpr[3]=0x80000000u;cpu.gpr[0]=cpu.gpr[3]+0x5544u;cpu.gpr[29]=cpu.gpr[0];
    // Private immutable loaded text owns descriptors. Every intervening write
    // was to proven stack/global addresses disjoint from this table.
    for (unsigned n=0;n<=10;++n) {
        save(0x8000336Cu);cpu.gpr[30]=image.ReadWord(cpu.gpr[29]+8u);compare(cpu.gpr[30]);
        if (!cpu.gpr[30]) break;
        cpu.gpr[4]=image.ReadWord(cpu.gpr[29]);cpu.gpr[31]=image.ReadWord(cpu.gpr[29]+4u);
        compare(cpu.gpr[31],cpu.gpr[4]);
        if (cpu.gpr[31]!=cpu.gpr[4]) throw std::runtime_error("CRT relocation/cache path unproven");
        cpu.gpr[29]+=12u;
    }
    save(0x800033ACu);cpu.gpr[3]=0x80000000u;cpu.gpr[0]=cpu.gpr[3]+0x55C8u;
    cpu.gpr[29]=cpu.gpr[0];
    for (unsigned n=0;n<3;++n) {
        save(0x800033C0u);cpu.gpr[5]=image.ReadWord(cpu.gpr[29]+4u);compare(cpu.gpr[5]);
        cpu.gpr[3]=image.ReadWord(cpu.gpr[29]);cpu.gpr[4]=0u;
        call(0x800033D8u,0x8000540Cu);
        const auto fill_parent=cpu.gpr[1];cpu.gpr[1]-=16u;
        store(0x8000540Cu,cpu.gpr[1],fill_parent);cpu.gpr[0]=cpu.lr;
        store(0x80005414u,cpu.gpr[1]+20u,cpu.gpr[0]);
        store(0x80005418u,cpu.gpr[1]+12u,cpu.gpr[31]);cpu.gpr[31]=cpu.gpr[3];
        call(0x80005420u,0x8000543Cu);
        const auto address=cpu.gpr[3],size=cpu.gpr[5];
        if ((address&3u)||(size&3u)||size<32u || std::uint64_t(address)+size>0x100000000ull)
            throw std::runtime_error("unproven byte-alignment/short-fill path");
        compare(size,32u);cpu.gpr[4]&=0xFFu;cpu.gpr[6]=address-1u;cpu.gpr[7]=cpu.gpr[4];
        cpu.gpr[0]=~cpu.gpr[6];cpu.gpr[3]=cpu.gpr[0]&3u;compare(cpu.gpr[3]);
        if (cpu.gpr[3] || cpu.gpr[7]) throw std::runtime_error("unproven leading/nonzero fill");
        compare(cpu.gpr[7]);const auto groups=size>>5u,remaining=(size>>2u)&7u;
        cpu.gpr[3]=groups;compare(groups);cpu.gpr[4]=cpu.gpr[6]-3u;
        NativeZeroRange range{address,std::vector<std::uint8_t>(size)};
        // Proven ascending eight-word group stores then remaining words.
        // A newly allocated byte is not an input; every exposed byte is written.
        for (std::uint32_t group=0;group<groups;++group) {
            std::fill_n(range.bytes.begin()+32u*group,32u,0u);
            --cpu.gpr[3];state.xer|=0x20000000u;compare(cpu.gpr[3]);cpu.gpr[4]+=32u;
        }
        cpu.gpr[3]=remaining;compare(remaining);
        for (std::uint32_t word=0;word<remaining;++word) {
            std::fill_n(range.bytes.begin()+32u*groups+4u*word,4u,0u);
            --cpu.gpr[3];state.xer|=0x20000000u;compare(cpu.gpr[3]);cpu.gpr[4]+=4u;
        }
        cpu.gpr[6]=cpu.gpr[4]+3u;cpu.gpr[5]&=3u;compare(cpu.gpr[5]);
        run.zero_ranges.push_back(std::move(range));
        const auto& completed=run.zero_ranges.back();
        if (0x80586CB4u>=address && std::uint64_t(0x80586CB4u)+4u<=std::uint64_t(address)+size)
            registry.Store(0x80586CB4u,ReadBytes(completed,0x80586CB4u));
        save(cpu.lr&~3u);
        if (cpu.pc!=0x80005424u) throw std::runtime_error("fill leaf return differs");
        cpu.gpr[0]=live.stack.LoadBE32(cpu.gpr[1]+20u);cpu.gpr[3]=cpu.gpr[31];
        cpu.gpr[31]=live.stack.LoadBE32(cpu.gpr[1]+12u);cpu.lr=cpu.gpr[0];cpu.gpr[1]+=16u;
        save(cpu.lr&~3u);if (cpu.pc!=0x800033DCu) throw std::runtime_error("fill wrapper return differs");
        cpu.gpr[29]+=8u;
    }
    save(0x800033C0u);cpu.gpr[5]=image.ReadWord(cpu.gpr[29]+4u);compare(cpu.gpr[5]);
    if (cpu.gpr[5]) throw std::runtime_error("CRT zero terminator differs");
    save(0x800033E4u);cpu.gpr[0]=live.stack.LoadBE32(cpu.gpr[1]+28u);
    cpu.gpr[31]=live.stack.LoadBE32(cpu.gpr[1]+20u);
    cpu.gpr[30]=live.stack.LoadBE32(cpu.gpr[1]+16u);
    cpu.gpr[29]=live.stack.LoadBE32(cpu.gpr[1]+12u);cpu.gpr[1]+=24u;cpu.lr=cpu.gpr[0];
    save(cpu.lr&~3u);if (cpu.pc!=0x80003170u) throw std::runtime_error("CRT saved return differs");
    cpu.gpr[0]=0u;cpu.gpr[6]=0x80000000u;cpu.gpr[6]+=0x44u;lowmem44=cpu.gpr[0];
    run.stores.push_back({0x8000317Cu,cpu.gpr[6],*lowmem44,*lowmem44});
    cpu.gpr[6]=0x80000000u;cpu.gpr[6]+=0xF4u;save(0x80003188u);
    return run;
}
}
