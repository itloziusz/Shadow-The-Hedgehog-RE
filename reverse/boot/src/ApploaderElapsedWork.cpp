#include "shadow/boot/ApploaderElapsedWork.hpp"

#include <array>
#include <cstddef>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>

namespace shadow::boot {
namespace {

void Require(bool condition, const char* reason) {
    if (!condition) throw std::runtime_error(reason);
}

std::uint32_t Word(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    Require(offset <= bytes.size() && bytes.size() - offset >= 4,
            "apploader research byte extent unavailable");
    return (std::uint32_t(bytes[offset]) << 24u) |
           (std::uint32_t(bytes[offset + 1]) << 16u) |
           (std::uint32_t(bytes[offset + 2]) << 8u) | bytes[offset + 3];
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

std::uint64_t Span(std::uint32_t first, std::uint32_t last) {
    Require(first <= last && (first & 3u) == 0 && (last & 3u) == 0,
            "invalid analytical instruction block");
    return (last - first) / 4u + 1u;
}

enum class CacheKind { DataInvalidate, DataFlush, DataStore, InstructionInvalidate };
std::uint64_t Cache(std::uint32_t address, std::uint32_t bytes, CacheKind kind) {
    if (bytes == 0) return 2;
    Require(std::uint64_t(address) + bytes <= 0x100000000ull,
            "unowned wrapped cache-work extent");
    const auto lines = (std::uint64_t(bytes) + (address & 31u) + 31u) / 32u;
    // 7 entry instructions; dcbi has blr, dcbf/dcbst have sc/RFI/blr,
    // icbi has sync/isync/blr. Default one-word RFI is a profile premise.
    return 3u * lines + (kind == CacheKind::DataInvalidate ? 8u : 10u);
}

std::uint64_t Copy(std::uint32_t bytes) { return 4ull * bytes + 9u; }

std::uint64_t Fill(std::uint32_t address, std::uint32_t bytes, std::uint8_t value) {
    Require(std::uint64_t(address) + bytes <= 0x100000000ull,
            "unowned wrapped fill-work extent");
    // 12 wrapper instructions. Short path: five initial instructions, two
    // tests, then byte replication, three per byte and final blr if nonzero.
    if (bytes < 32) return 12u + 7u + (bytes ? 2ull + 3ull * bytes : 0ull);
    const auto leading = (0u - address) & 3u;
    const auto left = bytes - leading;
    return 12u + 20u + (leading ? 2ull + 3ull * leading : 0ull)
           + (value ? 6u : 0u) + 10ull * (left / 32u)
           + 3ull * ((left / 4u) & 7u)
           + ((left & 3u) ? 2ull + 3ull * (left & 3u) : 0ull);
}

struct Section { std::uint32_t offset, address, size; };
std::vector<Section> Sections(const std::vector<std::uint8_t>& header, bool text) {
    std::vector<Section> result;
    for (unsigned i = 0; i < (text ? 7u : 11u); ++i) {
        const auto offset = Word(header, (text ? 0u : 0x1cu) + 4u * i);
        const auto address = Word(header, (text ? 0x48u : 0x64u) + 4u * i);
        const auto size = Word(header, (text ? 0x90u : 0xacu) + 4u * i);
        if (offset == 0) { Require(address == 0 && size == 0, "partial empty descriptor"); continue; }
        Require(address < 0x81100000u && std::uint64_t(address) + size <= 0x80700000ull
                && (address & 31u) == 0 && (size & 31u) == 0,
                "unsupported apploader section range or alignment");
        result.push_back({offset,address,size});
    }
    return result;
}

void GateProfile(const ApploaderSdkInitialization& p) {
    Require(p.memory_owner == ApploaderSdkInitialization::MemoryOwner::FreshClearNoRestoreOrForeignWrites
            && p.step_policy == ApploaderSdkInitialization::StepPolicy::GcBootSingleStepSource42
            && p.physical_ram_bytes == 0x01800000u && p.console_type == 0x10000006u
            && p.si_poll == (492u << 16u) && p.syscall_handler_word == 0x4c000064u
            && p.report_stub_word == 0x4e800020u && p.original_disc_without_patches
            && p.no_additional_guest_paths_or_exceptions,
            "unresolved or unsupported apploader SDK initialization premise");
}

}  // namespace

ApploaderSdkInitialization FreshGcHleApploaderSourceInitialization() {
    ApploaderSdkInitialization p;
    p.memory_owner=ApploaderSdkInitialization::MemoryOwner::FreshClearNoRestoreOrForeignWrites;
    p.step_policy=ApploaderSdkInitialization::StepPolicy::GcBootSingleStepSource42;
    p.physical_ram_bytes=0x01800000u;
    p.console_type=0x10000006u;
    p.si_poll=492u<<16u;
    p.syscall_handler_word=0x4c000064u;
    p.report_stub_word=0x4e800020u;
    p.original_disc_without_patches=true;
    p.no_additional_guest_paths_or_exceptions=true;
    return p;
}

ApploaderElapsedWork DeriveApploaderElapsedWork(
    const BootImage& image, const std::vector<std::uint8_t>& app,
    const std::vector<std::uint8_t>& boot, const std::vector<std::uint8_t>& bi2,
    const std::vector<std::uint8_t>& header, const ApploaderSdkInitialization& p) {
    GateProfile(p);
    Pin(app,0x1de58u,"8a1fe9487b029b2518a6633b1e0eb9ec0922f5f5eb5afdd7eba1267a85f096fe",
        "original apploader byte identity mismatch");
    Pin(boot,0x440u,"7d387ce9162342a93a3becac1d2df6db1cccf6a2439f3fc97b29b2f7047d8e88",
        "original boot-header byte identity mismatch");
    Pin(bi2,0x2000u,"8bbadffc2a7f79eda5982aa2fe30831969c2702aa9206f621b930aae0b70af6b",
        "original BI2 byte identity mismatch");
    Pin(header,0x100u,"4a820e037e1d8d2118dea36c0e8f75a5c02caef13e897480c75778501cb29354",
        "original DOL-header byte identity mismatch");
    Require(Word(header,0xe0u) == 0x80003154u && image.ReadWord(0x80003154u) == 0x4800015du,
            "BootImage does not match admitted PAL entry");
    Require(Word(app,0x10u) == 0x81200258u && Word(app,0x14u) == 0x1a98u
            && Word(app,0x18u) == 0x1c3a0u, "apploader header mismatch");
    const auto text = Sections(header,true), data = Sections(header,false);
    const auto nt = text.size(), nd = data.size();
    Require(nt == 2 && nd == 8, "unsupported callback descriptor topology");
    const auto bss = Word(header,0xd8u), bss_size = Word(header,0xdcu);
    Require(bss < 0x81100000u && std::uint64_t(bss) + bss_size <= 0x80700000ull,
            "unsupported BSS range-predicate branch");
    const auto dol_offset = Word(boot,0x420u), fst_offset = Word(boot,0x424u);
    const auto fst_size = Word(boot,0x428u), fst_max = Word(boot,0x42cu);
    const auto sim_memory = Word(bi2,4u);
    Require(dol_offset < fst_offset && fst_size <= fst_max && sim_memory == p.physical_ram_bytes
            && Word(bi2,0u) == 0 && Word(bi2,0x28u) == 0,
            "unsupported loaded-header or layout branch");
    const auto fst_base = (0x80000000u + sim_memory - fst_max) & ~31u;
    const auto fst_read = (fst_size + 31u) & ~31u;
    Require(fst_base >= 0x81700000u && fst_read <= 0x20000u,
            "apploader fatal or chunk path is unresolved");

    ApploaderElapsedWork r;
    // Count explicit raw blocks. No PPC instruction is evaluated here.
    r.return_thunk_bytes = 0x81200254u - 0x81200208u;
    r.return_thunk_steps = Span(0x81200208u,0x8120022cu) + 3u * (0x100000u / 32u)
                          + Span(0x8120023cu,0x81200250u);
    const auto outer = Span(0x81200258u,0x81200274u) + r.return_thunk_steps;
    const auto prologue = Span(0x812007c4u,0x8120081cu) + 2u; // getter8120115C
    const auto main_base = outer + prologue + Span(0x81201120u,0x81201134u);
    r.footprint_steps = 7u+2u+nt*14u+(7u-nt)*8u+2u+2u+nd*14u+(11u-nd)*8u+4u;
    r.each_bounds_check_steps = 6u+2u+nt*23u+(7u-nt)*8u+2u+2u+nd*23u+(11u-nd)*8u+13u;
    const auto entry_body = Span(0x8120039cu,0x81200414u) + 1u
                          + Span(0x8120044cu,0x81200464u) + Span(0x81200468u,0x81200484u);
    r.entry_steps = outer + entry_body + Copy(r.return_thunk_bytes)
                   + Cache(0x812fff80u,r.return_thunk_bytes,CacheKind::DataStore)
                   + Cache(0x812fff80u,r.return_thunk_bytes,CacheKind::InstructionInvalidate);
    r.init_steps = outer + 44u + Fill(0x81201920u,0x20u,0) + Fill(0x81201940u,0x100u,0)
                   + 13u + 2u; // F0 fallback; two one-step Start report hooks
    const auto state6_fixed = Span(0x81200c30u,0x81200c48u) + Span(0x81200c9cu,0x81200ca4u)
        + Span(0x81200ca8u,0x81200cc0u) + Span(0x81200cccu,0x81200ce4u)
        + Span(0x81200d2cu,0x81200d38u) + Span(0x81200d54u,0x81200d64u)
        + 1u + Span(0x81200d94u,0x81200db8u);
    r.bss_fill_steps = Fill(bss,bss_size,0);
    r.bss_flush_steps = Cache(bss,bss_size,CacheKind::DataFlush);
    // Source console word10000006 selects two range checks. One-check arm
    // changes work and is rejected by GateProfile; no fitted residual exists.
    r.state6_steps = state6_fixed + r.footprint_steps + 15u + 2u*r.each_bounds_check_steps
                     + r.bss_fill_steps + r.bss_flush_steps;
    r.main_steps.push_back(main_base + 12u + Cache(0x81201920u,0x20u,CacheKind::DataInvalidate));
    r.main_steps.push_back(main_base + 19u + 1u + Cache(0x81201a80u,0x20u,CacheKind::DataInvalidate));
    r.main_steps.push_back(main_base + 65u + Cache(fst_base-0x2000u,0x2000u,CacheKind::DataInvalidate));
    r.main_steps.push_back(main_base + 20u + 12u + Cache(0x81201940u,0x100u,CacheKind::DataInvalidate));
    for (std::size_t i=0;i<text.size();++i)
        r.main_steps.push_back(main_base + (i==0?r.state6_steps:0u) + 46u
                              + Cache(text[i].address,text[i].size,CacheKind::DataInvalidate));
    const auto text_exit = 12u*(7u-nt)+14u; // sentinel reads first data offset
    for (std::size_t i=0;i<data.size();++i)
        r.main_steps.push_back(main_base + (i==0?text_exit:0u) + 46u
                              + Cache(data[i].address,data[i].size,CacheKind::DataInvalidate));
    const auto data_exit = 12u*(11u-nd)+14u; // sentinel reads text0 RAM address
    r.main_steps.push_back(main_base+data_exit+28u+Cache(fst_base,fst_read,CacheKind::DataInvalidate)+14u);
    r.main_steps.push_back(outer+prologue+Span(0x81201124u,0x81201134u)+7u+8u+20u+12u);
    r.close_steps = outer + 4u;
    r.callback_count = static_cast<std::uint32_t>(r.main_steps.size()) + 3u;
    r.total_steps = r.entry_steps+r.init_steps+r.close_steps
                    + std::accumulate(r.main_steps.begin(),r.main_steps.end(),std::uint64_t{0});
    return r;
}

}  // namespace shadow::boot
