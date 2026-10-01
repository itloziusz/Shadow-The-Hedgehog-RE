#include "shadow/boot/NativeBi2Prefix.hpp"
#include <initializer_list>
#include <stdexcept>

namespace shadow::boot {
namespace {
void Check(const BootImage& image,std::uint32_t pc,std::initializer_list<std::uint32_t> words) {
    for (auto word:words) {
        if (image.ReadWord(pc)!=word)throw std::runtime_error("native BI2 raw word mismatch");
        pc+=4u;
    }
}
bool Overlaps(std::uint32_t a,std::size_t n,std::uint32_t b,std::size_t m) {
    return std::uint64_t(a)<std::uint64_t(b)+m && std::uint64_t(b)<std::uint64_t(a)+n;
}
std::uint32_t Word(const std::vector<std::uint8_t>& bytes,std::size_t offset) {
    if ((offset&3u)||offset+4u>bytes.size())throw std::runtime_error("BI2/SDA word outside known owner");
    std::uint32_t value=0;
    for(unsigned n=0;n<4;++n)value=(value<<8u)|bytes[offset+n];
    return value;
}
void Put(std::vector<std::uint8_t>& bytes,std::size_t offset,std::uint32_t value) {
    if ((offset&3u)||offset+4u>bytes.size())throw std::runtime_error("BI2/SDA store outside known owner");
    for(unsigned n=0;n<4;++n)bytes[offset+n]=static_cast<std::uint8_t>(value>>(24u-8u*n));
}
}
NativeBi2Run RunImmutableNativeBi2Prefix(const BootImage& image,const NativeBi2Inputs& input) {
    Check(image,0x80003188u,{0x80C60000u,0x28060000u,0x4182000Cu,0x80E6000Cu,0x48000024u,0x3CA08000u,0x38A50034u,0x80A50000u,0x28050000u,0x4182004Cu,0x3CE08000u,0x38E730E8u,0x80E70000u,0x38A00000u,0x28070002u,0x41820024u,0x28070003u,0x38A00001u,0x41820018u,0x28070004u,0x40820020u,0x38A00002u,0x4BFFFF61u,0x48000014u,0x3CC0803Au,0x38C6F8E0u,0x7CC803A6u,0x4E800021u,0x3CC08000u,0x38C600F4u,0x80A60000u,0x28050000u,0x41A20050u,0x80C50008u,0x28060000u,0x41A20044u,0x7CC53214u,0x81C60000u,0x280E0000u,0x41820034u,0x39E60004u,0x7DC903A6u,0x38C60004u,0x80E60000u,0x7CE72A14u,0x90E60000u,0x4200FFF0u,0x3CA08000u,0x38A50034u,0x55E70034u,0x90E50000u,0x4800000Cu,0x39C00000u,0x39E00000u,0x4836D991u,0x4836DC05u});
    Check(image,0x80003140u,{0x38000001u,0x980D5AF0u,0x4E800020u});
    Check(image,0x80370BF0u,{0x3C808000u,0x38040040u,0x3C608037u,0x900D5A18u,0x38630C60u,0x3C038000u,0x90040048u,0x38000001u,0x900D5A1Cu,0x4E800020u});
    Check(image,0x80370E68u,{0x7C0802A6u,0x90010004u,0x9421FFE8u,0x93E10014u,0x93C10010u,0x93A1000Cu,0x800D5A40u,0x3C608058u,0x3BE36C40u,0x2C000000u,0x3C608056u,0x3BC310F8u,0x40820494u,0x38000001u,0x900D5A40u,0x480087A5u});
    Check(image,0x80379648u,{0x7C0802A6u,0x90010004u,0x9421FFE0u,0x93E1001Cu,0x93C10018u,0x93A10014u,0x4BFFCABDu,0x7C7F1B78u,0x4BFFFFC1u});
    Check(image,0x80379628u,{0x7C6D42E6u}); // Stop before consuming TB, not after.
    NativeBi2Run run{};run.prefix=RunImmutableNativeCrtPrefix(image,input.crt);
    if(run.prefix.checkpoints.back().boot.native.state.machine.cpu.pc!=0x80003188u)return run;
    auto live=run.prefix.checkpoints.back();
    auto& boot=live.boot;auto& state=boot.native.state;auto& cpu=state.machine.cpu;
    run.globals=run.prefix.zero_ranges;run.bi2=input.loaded_bi2;
    std::optional<std::uint32_t> arena,low48;
    const auto global=[&](std::uint32_t address)->NativeZeroRange& {
        for(auto& range:run.globals)
            if(address>=range.address && std::uint64_t(address)+4u<=std::uint64_t(range.address)+range.bytes.size())return range;
        throw std::runtime_error("SDA read/write has no connected CRT producer");
    };
    const auto load_global=[&](std::uint32_t address) { auto& range=global(address);return Word(range.bytes,address-range.address); };
    const auto save=[&](std::uint32_t pc) {
        cpu.pc=pc;
        for(unsigned n=0;n<16;++n) {
            const auto off=boot.native.paired_stack.base+n-boot.stack.base;
            boot.native.paired_stack.bytes[n]=boot.stack.bytes[off];
            boot.native.paired_stack.valid[n]=boot.stack.valid[off];
        }
        run.checkpoints.push_back({live,run.bi2,{load_global(0x805F1F18u),load_global(0x805F1F1Cu),load_global(0x805F1F40u),load_global(0x805F1FF0u)},arena,low48});
    };
    const auto compare=[&](std::uint32_t value,std::uint32_t rhs=0u) {
        state.cr=(state.cr&0x0FFFFFFFu)|(value<rhs?0x80000000u:value>rhs?0x40000000u:0x20000000u)
            |((state.xer&0x80000000u)?0x10000000u:0u);
    };
    const auto stack_store=[&](std::uint32_t pc,std::uint32_t a,std::uint32_t value) {
        boot.stack.StoreBE32(a,value);run.stores.push_back({pc,a,value,boot.stack.LoadBE32(a)});
    };
    const auto sda_store=[&](std::uint32_t pc,std::uint32_t a,std::uint32_t value) {
        auto& range=global(a);Put(range.bytes,a-range.address,value);
        run.stores.push_back({pc,a,value,load_global(a)});
    };
    const auto call=[&](std::uint32_t pc,std::uint32_t target) {cpu.lr=pc+4u;save(target);};
    if(!input.pointer) {save(0x80003188u);return run;}
    // Low-memory pointer is a separate closed input owner. Neither admitted
    // debug4 nor any later owned store writes it; still perform both reads.
    const auto read_pointer=[&](std::uint32_t address) {
        if(address!=0x800000F4u)throw std::runtime_error("unproven pointer location");
        return *input.pointer;
    };
    cpu.gpr[6]=read_pointer(cpu.gpr[6]);compare(cpu.gpr[6]);
    if(!cpu.gpr[6]) {
        if(!run.bi2.empty())throw std::runtime_error("blob supplied for null BI2 pointer");
        cpu.gpr[5]=0x80000034u;save(0x800031A4u);return run;
    }
    const auto base=cpu.gpr[6];
    if((base&3u)||base<0x80000000u||std::uint64_t(base)+0x2000u>0x81800000ull)
        throw std::runtime_error("BI2 mapping outside proven MEM1 bounds/alignment");
    // Loaded DOL sections and current prefix owners must be disjoint. These
    // extents are the pinned PAL descriptor copies, not guessed object sizes.
    for(unsigned n=0;n<10;++n) {
        const auto a=image.ReadWord(0x80005544u+12u*n),size=image.ReadWord(0x8000554Cu+12u*n);
        if(Overlaps(base,0x2000u,a,size))throw std::runtime_error("BI2 aliases immutable DOL owner");
    }
    for(const auto& range:run.globals)
        if(Overlaps(base,0x2000u,range.address,range.bytes.size()))throw std::runtime_error("BI2 aliases produced CRT owner");
    if(Overlaps(base,0x2000u,boot.stack.base,boot.stack.bytes.size())||Overlaps(base,0x2000u,0x80000000u,0x3100u))
        throw std::runtime_error("BI2 aliases stack or low-memory owner");
    save(0x80003194u);
    if(run.bi2.empty())return run;
    if(run.bi2.size()!=0x2000u)throw std::runtime_error("partial BI2 input rejected");
    const auto read=[&](std::uint32_t a) {
        if(a<base || std::uint64_t(a)+4u>std::uint64_t(base)+run.bi2.size())throw std::runtime_error("BI2 read outside explicit owner");
        return Word(run.bi2,a-base);
    };
    cpu.gpr[7]=read(cpu.gpr[6]+12u);save(0x800031BCu);
    cpu.gpr[5]=0u;compare(cpu.gpr[7],2u);save(0x800031C4u);
    if(cpu.gpr[7]!=2u) {
        compare(cpu.gpr[7],3u);cpu.gpr[5]=1u;save(0x800031D0u);
    }
    if(cpu.gpr[7]==2u||cpu.gpr[7]==3u) {
        save(0x800031E8u);cpu.gpr[6]=0x8039F8E0u;cpu.lr=cpu.gpr[6];
        save(0x800031F4u);return run; // Resolved target, unvalidated context effects.
    }
    compare(cpu.gpr[7],4u);save(0x800031D8u);
    if(cpu.gpr[7]==4u) {
        cpu.gpr[5]=2u;call(0x800031E0u,0x80003140u);cpu.gpr[0]=1u;
        const auto address=cpu.gpr[13]+0x5AF0u;auto& range=global(address);
        range.bytes[address-range.address]=static_cast<std::uint8_t>(cpu.gpr[0]);
        run.byte_stores.push_back({0x80003144u,address,static_cast<std::uint8_t>(cpu.gpr[0]),range.bytes[address-range.address]});
        save(0x80003148u);save(cpu.lr&~3u);
    }
    save(0x800031F8u);cpu.gpr[6]=0x800000F4u;save(0x80003200u);
    cpu.gpr[5]=read_pointer(cpu.gpr[6]);compare(cpu.gpr[5]);save(0x8000320Cu);
    cpu.gpr[6]=read(cpu.gpr[5]+8u);compare(cpu.gpr[6]);save(0x80003214u);
    if(cpu.gpr[6]) {
        // Reject wrap and unknown extents before allowing a pointer consumer.
        if(std::uint64_t(cpu.gpr[5])+cpu.gpr[6]>0xFFFFFFFFull)throw std::runtime_error("BI2 table address wraps");
        cpu.gpr[6]+=cpu.gpr[5];save(0x8000321Cu);cpu.gpr[14]=read(cpu.gpr[6]);
        compare(cpu.gpr[14]);save(0x80003224u);
        if(cpu.gpr[14]) {
            if(std::uint64_t(cpu.gpr[6])+4u+4ull*cpu.gpr[14]>std::uint64_t(base)+run.bi2.size())
                throw std::runtime_error("BI2 relocation extent unproven");
            cpu.gpr[15]=cpu.gpr[6]+4u;boot.native.ctr=cpu.gpr[14];
            save(0x80003230u);
            // Opaque relocated words are not promoted into callable pointers.
            // Ordered loads/stores retain aliases within this same BI2 owner.
            for(std::uint32_t n=0;n<cpu.gpr[14];++n) {
                cpu.gpr[6]+=4u;save(0x80003234u);cpu.gpr[7]=read(cpu.gpr[6]);
                cpu.gpr[7]+=cpu.gpr[5];save(0x8000323Cu);
                Put(run.bi2,cpu.gpr[6]-base,cpu.gpr[7]);
                run.stores.push_back({0x8000323Cu,cpu.gpr[6],cpu.gpr[7],read(cpu.gpr[6])});
                save(0x80003240u);--boot.native.ctr;
                if(boot.native.ctr)save(0x80003230u);
            }
            cpu.gpr[5]=0x80000034u;cpu.gpr[7]=cpu.gpr[15]&0xFFFFFFE0u;save(0x80003250u);
            arena=cpu.gpr[7];run.stores.push_back({0x80003250u,cpu.gpr[5],*arena,*arena});
        } else {save(0x80003258u);cpu.gpr[14]=0u;cpu.gpr[15]=0u;}
    } else {save(0x80003258u);cpu.gpr[14]=0u;cpu.gpr[15]=0u;}
    save(0x80003260u);call(0x80003260u,0x80370BF0u);
    cpu.gpr[4]=0x80000000u;cpu.gpr[0]=cpu.gpr[4]+0x40u;cpu.gpr[3]=0x80370000u;
    save(0x80370BFCu);sda_store(0x80370BFCu,cpu.gpr[13]+0x5A18u,cpu.gpr[0]);
    cpu.gpr[3]+=0xC60u;cpu.gpr[0]=cpu.gpr[3]+0x80000000u;save(0x80370C08u);
    low48=cpu.gpr[0];run.stores.push_back({0x80370C08u,cpu.gpr[4]+0x48u,*low48,*low48});
    cpu.gpr[0]=1u;save(0x80370C10u);sda_store(0x80370C10u,cpu.gpr[13]+0x5A1Cu,cpu.gpr[0]);
    save(cpu.lr&~3u);if(cpu.pc!=0x80003264u)throw std::runtime_error("metadata return unproven");
    call(0x80003264u,0x80370E68u);cpu.gpr[0]=cpu.lr;
    stack_store(0x80370E6Cu,cpu.gpr[1]+4u,cpu.gpr[0]);
    const auto os_parent=cpu.gpr[1];cpu.gpr[1]-=24u;stack_store(0x80370E70u,cpu.gpr[1],os_parent);
    stack_store(0x80370E74u,cpu.gpr[1]+20u,cpu.gpr[31]);
    stack_store(0x80370E78u,cpu.gpr[1]+16u,cpu.gpr[30]);
    stack_store(0x80370E7Cu,cpu.gpr[1]+12u,cpu.gpr[29]);save(0x80370E80u);
    cpu.gpr[0]=load_global(cpu.gpr[13]+0x5A40u);cpu.gpr[3]=0x80580000u;cpu.gpr[31]=cpu.gpr[3]+0x6C40u;
    // The connected producer supplies 0. Retain a rejecting guard if ownership
    // changes: no unproven signed/second-init branch may be silently skipped.
    if(cpu.gpr[0])throw std::runtime_error("OS guard lacks first-init producer");
    compare(cpu.gpr[0]);cpu.gpr[3]=0x80560000u;cpu.gpr[30]=cpu.gpr[3]+0x10F8u;save(0x80370E98u);
    cpu.gpr[0]=1u;save(0x80370EA0u);sda_store(0x80370EA0u,cpu.gpr[13]+0x5A40u,cpu.gpr[0]);
    call(0x80370EA4u,0x80379648u);cpu.gpr[0]=cpu.lr;
    stack_store(0x8037964Cu,cpu.gpr[1]+4u,cpu.gpr[0]);
    const auto clock_parent=cpu.gpr[1];cpu.gpr[1]-=32u;stack_store(0x80379650u,cpu.gpr[1],clock_parent);
    stack_store(0x80379654u,cpu.gpr[1]+28u,cpu.gpr[31]);
    stack_store(0x80379658u,cpu.gpr[1]+24u,cpu.gpr[30]);
    stack_store(0x8037965Cu,cpu.gpr[1]+20u,cpu.gpr[29]);save(0x80379660u);
    call(0x80379660u,0x8037611Cu);cpu.gpr[3]=state.machine.msr;cpu.gpr[4]=WithoutExternalInterrupts(cpu.gpr[3]);
    save(0x80376124u);state.machine.msr=cpu.gpr[4];cpu.gpr[3]=(cpu.gpr[3]>>15u)&1u;save(0x80379664u);
    cpu.gpr[31]=cpu.gpr[3];call(0x80379668u,0x80379628u);
    return run;
}
}
