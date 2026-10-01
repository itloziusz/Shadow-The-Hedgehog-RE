#include "shadow/boot/ApploaderEntryFrames.hpp"

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace shadow::boot {
namespace {
void Require(bool condition, const char* reason) {
    if (!condition) throw std::runtime_error(reason);
}
std::uint32_t Word(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    Require(offset <= bytes.size() && bytes.size() - offset >= 4, "original byte extent unavailable");
    return (std::uint32_t(bytes[offset]) << 24u) | (std::uint32_t(bytes[offset+1]) << 16u)
         | (std::uint32_t(bytes[offset+2]) << 8u) | bytes[offset+3];
}
std::uint32_t AddSigned(std::uint32_t base, std::int32_t displacement) {
    return base + static_cast<std::uint32_t>(displacement); // PPC modulo32 EA
}
void WriteWord(std::array<std::uint8_t,4>& bytes, std::uint32_t value) {
    for (unsigned i=0; i<4; ++i) bytes[i]=static_cast<std::uint8_t>(value >> (24u-8u*i));
}
std::uint32_t ReadWord(const std::array<std::uint8_t,4>& bytes) {
    return (std::uint32_t(bytes[0])<<24u) | (std::uint32_t(bytes[1])<<16u)
         | (std::uint32_t(bytes[2])<<8u) | bytes[3];
}
std::uint32_t Rotate(std::uint32_t value, unsigned bits) {
    return (value >> bits) | (value << (32u - bits));
}

// Portable byte identity gate; never a guest operation or timing input.
std::string Sha256(const std::vector<std::uint8_t>& bytes) {
    constexpr std::array<std::uint32_t, 64> k{{
        0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,
        0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
        0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,
        0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
        0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,
        0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
        0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,
        0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
        0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,
        0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
        0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,
        0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
        0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,
        0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
        0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,
        0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
    }};
    std::array<std::uint32_t, 8> h{{0x6a09e667u,0xbb67ae85u,0x3c6ef372u,
        0xa54ff53au,0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u}};
    auto padded = bytes;
    padded.push_back(0x80u);
    while ((padded.size() & 63u) != 56u) padded.push_back(0);
    const auto bits = std::uint64_t(bytes.size()) * 8u;
    for (unsigned i = 0; i < 8; ++i)
        padded.push_back(static_cast<std::uint8_t>(bits >> (56u - 8u * i)));
    for (std::size_t offset = 0; offset < padded.size(); offset += 64) {
        std::array<std::uint32_t, 64> w{};
        for (unsigned i = 0; i < 16; ++i) w[i] = Word(padded, offset + 4u * i);
        for (unsigned i = 16; i < 64; ++i) {
            const auto s0 = Rotate(w[i - 15], 7) ^ Rotate(w[i - 15], 18) ^ (w[i - 15] >> 3u);
            const auto s1 = Rotate(w[i - 2], 17) ^ Rotate(w[i - 2], 19) ^ (w[i - 2] >> 10u);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        auto a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],j=h[7];
        for (unsigned i = 0; i < 64; ++i) {
            const auto s1 = Rotate(e, 6) ^ Rotate(e, 11) ^ Rotate(e, 25);
            const auto choice = (e & f) ^ (~e & g);
            const auto t1 = j + s1 + choice + k[i] + w[i];
            const auto s0 = Rotate(a, 2) ^ Rotate(a, 13) ^ Rotate(a, 22);
            const auto majority = (a & b) ^ (a & c) ^ (b & c);
            const auto t2 = s0 + majority;
            j=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
        }
        h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=j;
    }
    std::ostringstream digest;
    digest << std::hex << std::setfill('0');
    for (const auto value : h) digest << std::setw(8) << value;
    return digest.str();
}

void Pin(const std::vector<std::uint8_t>& bytes, std::size_t size,
         const char* expected, const char* reason) {
    Require(bytes.size() == size, reason);
    Require(Sha256(bytes) == expected, reason);
}


void Gate(const ApploaderEntryResearchProfile& p) {
    Require(p.registers==ApploaderEntryResearchProfile::Registers::FreshPowerPcResetGcBs2RunFunction42,
            "unprovided initial-register producer");
    Require(p.memory==ApploaderEntryResearchProfile::Memory::FreshMem1ClearOriginalAppCopy42,
            "unprovided fresh RAM/code producer");
    Require(p.mapping==ApploaderEntryResearchProfile::Mapping::GcBatSource42, "unprovided BAT producer");
    Require(p.cache==ApploaderEntryResearchProfile::Cache::DirectRamCacheInterpreterDisabled42,
            "unprovided effective cache configuration");
    Require(p.lease==ApploaderEntryResearchProfile::Lease::NoRestoreOrForeignWritesConditional42,
            "unprovided original memory lease/path prerequisite");
    Require(p.first_advance==ApploaderEntryResearchProfile::FirstAdvance::UnchangedCpuRamConditional42,
            "unprovided conditional first-Advance CPU/RAM premise");
}
} // namespace

ApploaderEntryResearchProfile ConditionalFreshGcPalEntryResearchProfile42() {
    ApploaderEntryResearchProfile p;
    p.registers=ApploaderEntryResearchProfile::Registers::FreshPowerPcResetGcBs2RunFunction42;
    p.memory=ApploaderEntryResearchProfile::Memory::FreshMem1ClearOriginalAppCopy42;
    p.mapping=ApploaderEntryResearchProfile::Mapping::GcBatSource42;
    p.cache=ApploaderEntryResearchProfile::Cache::DirectRamCacheInterpreterDisabled42;
    p.lease=ApploaderEntryResearchProfile::Lease::NoRestoreOrForeignWritesConditional42;
    p.first_advance=ApploaderEntryResearchProfile::FirstAdvance::UnchangedCpuRamConditional42;
    return p;
}

ApploaderEntryFramesResearch::ApploaderEntryFramesResearch(
    const std::vector<std::uint8_t>& app, const std::vector<std::uint8_t>& boot,
    const std::vector<std::uint8_t>& bi2, const ApploaderEntryResearchProfile& profile) {
    Gate(profile);
    // Full identities, not an instruction decoder, callback count or trace.
    Pin(app,122456u,"8a1fe9487b029b2518a6633b1e0eb9ec0922f5f5eb5afdd7eba1267a85f096fe",
        "unsupported original apploader bytes");
    Pin(boot,1088u,"7d387ce9162342a93a3becac1d2df6db1cccf6a2439f3fc97b29b2f7047d8e88",
        "unsupported original boot bytes");
    Pin(bi2,8192u,"8bbadffc2a7f79eda5982aa2fe30831969c2702aa9206f621b930aae0b70af6b",
        "unsupported original BI2 bytes");
    Require(Word(bi2,0x18u)==2u, "unsupported non-PAL region producer");
    const auto entry=Word(app,0x10u);
    const auto body_size=Word(app,0x14u)+Word(app,0x18u);
    Require(entry==0x81200258u && body_size==app.size()-32u, "unsupported app body/entry extent");

    // PowerPC::ResetRegisters142..185 emits these finite retained fields.
    // No timebase/DEC or other unrepresented CPU state is reconstructed.
    auto& c=state_.cpu;
    c.gpr.fill(0); c.segment.fill(0); c.gqr.fill(0);
    for (auto& ps:c.paired_single) ps.fill(0);
    c.cr=0; c.xer=0; c.ctr=0; c.fpscr=0; c.exceptions=0;
    c.reserve=false; c.reserve_address=0; c.pc=0; c.npc=0; c.lr=0;
    c.hid1=0x80000000u;
    // BS2 SetupMSR/HID/BAT78..147 on the reset GC source state.
    c.msr=0x2032u; c.hid0=0x0011c464u; c.hid2=0xe0000000u;
    c.ibat0u=c.dbat0u=0x80001fffu; c.ibat0l=c.dbat0l=0x00000002u;
    c.dbat1u=0xc0001fffu; c.dbat1l=0x0000002au;
    c.ram_real=0x01800000u; c.ram_mask=0x01ffffffu;
    // Effective direct-RAM access depends on the config gate, not HID0.DCE.
    c.effective_dcache=false;
    // Bounded private native storage is really cleared/copied here. This is
    // conditional source reconstruction, not proof of all original RAM writers.
    state_.stack.logical_base=0x815edc78u; state_.stack.physical_base=0x015edc78u;
    state_.stack.bytes.fill(0); state_.stack.known.fill(true);
    app_body_.assign(app.begin()+32,app.end());
    handler_.fill(0); report_.fill(0);
    WriteWord(handler_,0x4c000064u); // GC default SC handler at80000C00
    // Report BLR/HLE patch occurs AFTER Entry returns; it is zero here.
    // Original BI2 region2 -> PAL -> BS2 source343..347.
    c.gpr[1]=0x815edca8u; c.gpr[2]=0x814b5b20u; c.gpr[13]=0x814b4fc0u;
    // RunApploader178..182 then RunFunction69..70.
    c.gpr[3]=0x80003100u; c.gpr[4]=0x80003104u; c.gpr[5]=0x80003108u;
    c.pc=entry; c.lr=0; // NPC remains the fresh Reset producer's0
    initial_=state_;
}

std::uint32_t ApploaderEntryFramesResearch::CodeWord(std::uint32_t address) const {
    Require(address>=0x81200000u, "unowned code range");
    return Word(app_body_,static_cast<std::size_t>(address-0x81200000u));
}
std::size_t ApploaderEntryFramesResearch::StackOffset(std::uint32_t address) const {
    const auto base=state_.stack.logical_base;
    Require(address>=base && std::uint64_t(address)<std::uint64_t(base)+state_.stack.bytes.size(),
            "unowned stack range or wrapped EA");
    return static_cast<std::size_t>(address-base);
}
void ApploaderEntryFramesResearch::Preflight(const std::vector<std::uint32_t>& addresses) const {
    const auto& c=state_.cpu;
    Require(c.msr==0x2032u && c.exceptions==0, "unsupported MSR/LE/DSI/exception branch");
    Require(c.hid0==0x0011c464u && c.hid1==0x80000000u && c.hid2==0xe0000000u
            && !c.effective_dcache, "unprovided original HID/effective-cache route");
    Require(c.ibat0u==0x80001fffu && c.ibat0l==2u && c.dbat0u==0x80001fffu && c.dbat0l==2u
            && c.dbat1u==0xc0001fffu && c.dbat1l==0x2au, "unprovided code/data BAT mapping");
    Require(c.ram_real==0x01800000u && c.ram_mask==0x01ffffffu
            && state_.stack.logical_base==0x815edc78u && state_.stack.physical_base==0x015edc78u,
            "unprovided bounded MEM1 allocation/mapping");
    Require(std::all_of(state_.stack.known.begin(),state_.stack.known.end(),[](bool v){return v;}),
            "unknown stack byte/lease in bounded source path");
    for (const auto address:addresses) {
        Require((address&3u)==0u, "unsupported unaligned stmw/store path");
        const auto offset=StackOffset(address);
        Require(offset+4u<=state_.stack.bytes.size(), "unowned store extent");
        const auto physical=state_.stack.physical_base+static_cast<std::uint32_t>(offset);
        Require(std::uint64_t(physical)+4u<=c.ram_real
                && physical==(address&c.ram_mask), "unprovided physical store alias");
    }
}
void ApploaderEntryFramesResearch::Begin(std::uint32_t pc, std::uint32_t word) {
    Require(state_.cpu.pc==pc && CodeWord(pc)==word, "unsupported instruction path/source word");
    state_.cpu.npc=pc+4u; // original source fetched-word ordering, before semantic effect
    instructions_.push_back({pc,word,state_,{}});
}
void ApploaderEntryFramesResearch::End() {
    state_.cpu.pc=state_.cpu.npc; // original UpdatePC
    instructions_.back().inner_exit=state_;
    ++completed_; // one SingleStep for stmw as for every other gated word
}
void ApploaderEntryFramesResearch::Store(std::uint32_t address, std::uint32_t value) {
    const auto offset=StackOffset(address);
    for (unsigned i=0; i<4; ++i)
        state_.stack.bytes[offset+i]=static_cast<std::uint8_t>(value>>(24u-8u*i));
    stores_.push_back({state_.cpu.pc,address,
        state_.stack.physical_base+static_cast<std::uint32_t>(offset),value});
}
void ApploaderEntryFramesResearch::ProduceFirstThree() {
    Require(boundary_==ApploaderEntryFrameBoundary::BeforeEntry && completed_==0,
            "unsupported/repeated Entry path");
    const auto sp=state_.cpu.gpr[1], next_sp=AddSigned(sp,-8);
    Preflight({next_sp,AddSigned(next_sp,12)});
    Begin(0x81200258u,0x7c0802a6u); state_.cpu.gpr[0]=state_.cpu.lr; End();
    Begin(0x8120025cu,0x9421fff8u);
    Store(next_sp,sp); state_.cpu.gpr[1]=next_sp; End(); // stwu updates only after successful write
    Begin(0x81200260u,0x9001000cu); Store(AddSigned(state_.cpu.gpr[1],12),state_.cpu.gpr[0]); End();
    nested_=state_;
    boundary_=ApploaderEntryFrameBoundary::BeforeNestedCall;
}
const ApploaderEntryFrameState& ApploaderEntryFramesResearch::BeforeNestedCall() const {
    Require(boundary_!=ApploaderEntryFrameBoundary::BeforeEntry, "nested boundary not yet produced");
    return nested_;
}
void ApploaderEntryFramesResearch::ProduceNestedPrologue() {
    Require(boundary_==ApploaderEntryFrameBoundary::BeforeNestedCall && completed_==3,
            "unsupported/repeated nested call path");
    const auto sp=state_.cpu.gpr[1], next_sp=AddSigned(sp,-40);
    // Unsupported/fault arms reject before this stage; no guessed rollback.
    Preflight({AddSigned(sp,4),next_sp,AddSigned(next_sp,28),AddSigned(next_sp,32),
        AddSigned(next_sp,36),AddSigned(next_sp,8),AddSigned(next_sp,12),AddSigned(next_sp,16)});
    Begin(0x81200264u,0x48000139u);
    state_.cpu.lr=state_.cpu.pc+4u; state_.cpu.npc=state_.cpu.pc+0x138u; End();
    Begin(0x8120039cu,0x7c0802a6u); state_.cpu.gpr[0]=state_.cpu.lr; End();
    Begin(0x812003a0u,0x90010004u); Store(AddSigned(state_.cpu.gpr[1],4),state_.cpu.gpr[0]); End();
    Begin(0x812003a4u,0x9421ffd8u);
    Store(next_sp,sp); state_.cpu.gpr[1]=next_sp; End();
    Begin(0x812003a8u,0xbfa1001cu);
    // Source297..316: aligned/non-LE arm, ascending registers, no-DSI mapping.
    Store(AddSigned(state_.cpu.gpr[1],28),state_.cpu.gpr[29]);
    Store(AddSigned(state_.cpu.gpr[1],32),state_.cpu.gpr[30]);
    Store(AddSigned(state_.cpu.gpr[1],36),state_.cpu.gpr[31]); End();
    Begin(0x812003acu,0x90610008u); Store(AddSigned(state_.cpu.gpr[1],8),state_.cpu.gpr[3]); End();
    Begin(0x812003b0u,0x9081000cu); Store(AddSigned(state_.cpu.gpr[1],12),state_.cpu.gpr[4]); End();
    Begin(0x812003b4u,0x90a10010u); Store(AddSigned(state_.cpu.gpr[1],16),state_.cpu.gpr[5]); End();
    boundary_=ApploaderEntryFrameBoundary::Before812003B8;
    // Stop BEFORE lis/publication. No callback-table range or step/clock owner.
}
std::uint32_t ApploaderEntryFramesResearch::HandlerWord() const { return ReadWord(handler_); }
std::uint32_t ApploaderEntryFramesResearch::ReportWord() const { return ReadWord(report_); }

void ApploaderEntryFramesResearch::WriteResearchGprForFalsification(unsigned i, std::uint32_t value) {
    Require(i<state_.cpu.gpr.size(), "research GPR outside finite state");
    state_.cpu.gpr[i]=value; falsification_=true;
}
void ApploaderEntryFramesResearch::WriteResearchRetainedForFalsification(
    EntryResearchRetainedField field, std::uint32_t value) {
    auto& c=state_.cpu;
    switch (field) {
        case EntryResearchRetainedField::Lr:c.lr=value;break;
        case EntryResearchRetainedField::Cr:c.cr=value;break;
        case EntryResearchRetainedField::Xer:c.xer=value;break;
        case EntryResearchRetainedField::Ctr:c.ctr=value;break;
        case EntryResearchRetainedField::Fpscr:c.fpscr=value;break;
        case EntryResearchRetainedField::Msr:c.msr=value;break;
        case EntryResearchRetainedField::Exceptions:c.exceptions=value;break;
        case EntryResearchRetainedField::Hid0:c.hid0=value;break;
        case EntryResearchRetainedField::Hid1:c.hid1=value;break;
        case EntryResearchRetainedField::Hid2:c.hid2=value;break;
        case EntryResearchRetainedField::Ibat0U:c.ibat0u=value;break;
        case EntryResearchRetainedField::Ibat0L:c.ibat0l=value;break;
        case EntryResearchRetainedField::Dbat0U:c.dbat0u=value;break;
        case EntryResearchRetainedField::Dbat0L:c.dbat0l=value;break;
        case EntryResearchRetainedField::Dbat1U:c.dbat1u=value;break;
        case EntryResearchRetainedField::Dbat1L:c.dbat1l=value;break;
        case EntryResearchRetainedField::RamReal:c.ram_real=value;break;
        case EntryResearchRetainedField::RamMask:c.ram_mask=value;break;
        case EntryResearchRetainedField::EffectiveDcache:c.effective_dcache=value!=0;break;
        default:throw std::runtime_error("unknown research retained field");
    }
    falsification_=true;
}
void ApploaderEntryFramesResearch::WriteResearchStackByteForFalsification(
    std::uint32_t address, std::uint8_t value) {
    state_.stack.bytes[StackOffset(address)]=value; falsification_=true;
}
void ApploaderEntryFramesResearch::ForgetResearchStackByteForFalsification(std::uint32_t address) {
    state_.stack.known[StackOffset(address)]=false; falsification_=true;
}

void RequireNativeApploaderEntryFrameOwnership(const InitialBootEventOwner& first) {
    if (first.Stop()!=InitialBootStop::FirstAdvanceComplete)
        throw std::runtime_error(std::string("native Entry blocked: first-Advance incomplete at ")+
                                 InitialBootStopName(first.Stop()));
    throw std::runtime_error("native Entry blocked: Complete has declared-only controls/effect delivery; "
        "live GPU/Movie/configuration/achievement side-state ownership unprovided");
}
} // namespace shadow::boot
