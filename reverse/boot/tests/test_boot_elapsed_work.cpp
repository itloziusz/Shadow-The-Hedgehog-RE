#include "shadow/boot/BootElapsedWork.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {
void Require(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }
template<class F> void Reject(F action, const char* reason) {
    try { action(); } catch (const std::exception&) { return; }
    throw std::runtime_error(reason);
}
std::uint32_t Word(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    std::uint32_t value = 0;
    for (unsigned n = 0; n < 4; ++n) value = (value << 8u) | bytes.at(offset+n);
    return value;
}
void Put(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t word) {
    for (unsigned n = 0; n < 4; ++n) bytes.at(offset+n) = static_cast<std::uint8_t>(word >> (24u-8u*n));
}
std::size_t FileOffset(const std::vector<std::uint8_t>& raw, std::uint32_t address) {
    for (unsigned n = 0; n < 18; ++n) {
        const auto base = Word(raw, 0x48u+4u*n), size = Word(raw, 0x90u+4u*n);
        if (address >= base && std::uint64_t(address)+4u <= std::uint64_t(base)+size)
            return Word(raw, 4u*n)+address-base;
    }
    throw std::runtime_error("unmapped raw work gate");
}
shadow::boot::NativeBi2Inputs Input(std::uint32_t l2 = 0, std::uint32_t debug = 0,
                                  std::uint32_t offset = 0, std::uint32_t count = 0) {
    shadow::boot::NativeBi2Inputs input{};
    input.crt.l2.entry.msr = 0x2032u;
    input.crt.l2.entry.hid0 = 0x0011C064u;
    input.crt.l2.entry.hid2 = 0xE0000000u;
    input.crt.l2.entry.source_address = 0x805F1F30u;
    input.crt.l2.entry.cr = 0xABCDEF12u;
    input.crt.l2.entry.xer = 0xC000007Fu;
    input.crt.l2.entry.ctr = 0x89312456u;
    input.crt.l2.l2cr = l2;
    input.crt.old_handler = 0xDEADBEEFu;
    input.pointer = 0x817E54E0u;
    input.loaded_bi2.resize(0x2000u);
    Put(input.loaded_bi2, 8, offset); Put(input.loaded_bi2, 12, debug);
    if (offset) Put(input.loaded_bi2, offset, count);
    return input;
}
const shadow::boot::BootWorkUnit& Unit(const shadow::boot::BootElapsedWork& run, const char* name) {
    for (const auto& unit : run.ledger) if (unit.name == name) return unit;
    throw std::runtime_error("missing semantic work unit");
}
std::uint64_t Total(const shadow::boot::BootElapsedWork& run, const char* name, bool work = true) {
    std::uint64_t value = 0;
    for (const auto& unit : run.ledger) if (unit.name == name)
        value += work ? unit.source_operation_work : unit.original_operations;
    return value;
}
void Check(const shadow::boot::BootElapsedWork& run, const shadow::boot::NativeBi2Inputs& input) {
    const auto debug = Word(input.loaded_bi2, 12), offset = Word(input.loaded_bi2, 8);
    const auto count = offset ? Word(input.loaded_bi2, offset) : 0u;
    const auto disabled = !(input.crt.l2.l2cr & 0x80000000u);
    const auto extra = offset ? (count ? 10ull+5ull*count : 4ull) : 0ull;
    const auto extra_ops = offset ? (count ? 9ull+5ull*count : 4ull) : 0ull;
    Require(run.stop_pc == 0x80379628u &&
            run.prefix.checkpoints.back().boot.boot.native.state.machine.cpu.pc == run.stop_pc,
            "work crossed the first unconsumed TB word");
    Require(!run.connected_clock_admitted, "finite work promoted itself into clock ownership");
    Require(run.source_operation_work == 153636ull+494ull+(disabled?116ull:0ull)+(debug==4u?6ull:0ull)+extra,
            "semantic work differs from separately derived closed form");
    Require(run.original_operations == 154108ull+(disabled?98ull:0ull)+(debug==4u?6ull:0ull)+extra_ops,
            "operation cardinality lost SPR-cost distinction");
    Require(Total(run, "zero_group") == 10ull*(14904ull+442ull+5ull), "zero loop groups depend on an observed trace");
    Require(Total(run, "zero_remaining_word") == 3ull*(0ull+7ull+3ull), "zero word tails differ");
    Require(Total(run, "zero_leaf_fixed") == 3ull*19ull && Total(run, "zero_wrapper") == 3ull*13ull,
            "zero leaf/wrapper boundaries differ");
    Require(Unit(run, "identity_copy_descriptor").repetitions == 10u &&
            Unit(run, "identity_copy_descriptor").source_operation_work == 100u, "CRT identity-copy producer changed");
    Require(Unit(run, "register_seed").raw_sha256 ==
            "080cce80236b961d2416b90e72e86f1fd7382441219b83c1ad8f39e2d2c14ff8", "raw unit identity lost");
    std::uint64_t ledger_work = 0, ledger_ops = 0;
    for (const auto& unit : run.ledger) {
        Require(unit.raw_sha256.size() == 64u, "unit hash missing");
        ledger_work += unit.source_operation_work; ledger_ops += unit.original_operations;
    }
    Require(ledger_work == run.source_operation_work && ledger_ops == run.original_operations, "ledger is inconsistent");
}
}

int main(int argc, char** argv) {
    using namespace shadow::boot;
    try {
        if (argc != 2) throw std::runtime_error("PAL DOL fixture required");
        std::ifstream file(argv[1], std::ios::binary);
        const std::vector<std::uint8_t> raw(std::istreambuf_iterator<char>{file}, {});
        const auto image = LoadValidatedFixture(raw);
        unsigned paths = 0, rejected = 0;
        for (const auto l2 : {0u, 0x80000000u, 0x40480000u, 0xC0480000u}) {
            for (const auto debug : {0u, 1u, 4u, 0xFFFFFFFFu}) {
                const auto input = Input(l2, debug);
                Check(ProduceBootElapsedWorkResearch(image, input), input); ++paths;
            }
        }
        for (const auto l2 : {0u, 0x80000000u}) for (const auto count : {0u, 1u, 7u}) {
            auto input = Input(l2, 4, 0x100u, count);
            for (unsigned n = 0; n < count; ++n) Put(input.loaded_bi2, 0x104u+4u*n, n?0xFFFFFFFFu:0x200u);
            const auto run = ProduceBootElapsedWorkResearch(image, input);
            Check(run, input);
            if (count) Require(run.prefix.checkpoints.back().boot.boot.native.ctr == 0u, "relocation work lacks CTR producer");
            ++paths;
        }
        for (const auto offset : {8u, 12u}) {
            const auto input = Input(0x80000000u, 4, offset, offset==8u?8u:4u);
            const auto run = ProduceBootElapsedWorkResearch(image, input);
            Check(run, input);
            Require(run.prefix.byte_stores.size() == 1u, "early aliased debug4 condition was reread");
            ++paths;
        }
        const auto maximum = Input(0x80000000u, 0, 0x100u, 1983u);
        Check(ProduceBootElapsedWorkResearch(image, maximum), maximum); ++paths;
        auto reject_input = [&](NativeBi2Inputs input) {
            Reject([&] { ProduceBootElapsedWorkResearch(image, input); }, "unknown/unsupported work input admitted"); ++rejected;
        };
        for (const auto bad : {0x8000u, 0x40000u, 1u}) { auto input=Input(); input.crt.l2.entry.msr|=bad; reject_input(input); }
        for (const auto bad : {0u, 0x11C464u}) { auto input=Input(); input.crt.l2.entry.hid0=bad; reject_input(input); }
        for (const auto bad : {1u, 0xE1000000u}) { auto input=Input(); input.crt.l2.entry.hid2=bad; reject_input(input); }
        for (const auto bad : {1u, 0x200000u, 0x01000000u}) { auto input=Input(); input.crt.l2.l2cr=bad; reject_input(input); }
        { auto input=Input(); input.crt.old_handler.reset(); reject_input(input); }
        { auto input=Input(); input.pointer.reset(); reject_input(input); }
        for (const auto bad : {0u, 0x817E54E1u, 0x817FF000u, 0x80003100u, 0x8056FE00u, 0x8060C570u, 0x80000000u}) {
            auto input=Input(); input.pointer=bad; reject_input(input);
        }
        { auto input=Input(); input.loaded_bi2.clear(); reject_input(input); }
        { auto input=Input(); input.loaded_bi2.resize(4u); reject_input(input); }
        for (const auto debug : {2u, 3u}) reject_input(Input(0, debug));
        for (const auto offset : {1u, 0x2000u, 0xFFFFFFFCu}) { auto input=Input(); Put(input.loaded_bi2,8,offset); reject_input(input); }
        reject_input(Input(0, 0, 0x1FFCu, 1));
        reject_input(Input(0, 0, 0x100u, 0xFFFFFFFFu));
        { auto input=Input(); input.crt.l2.entry.fpscr=0x800u; reject_input(input); }
        { auto input=Input(); input.crt.l2.entry.source_address=0u; reject_input(input); }
        { auto input=Input(); input.crt.l2.entry.source_bytes[0]=0x7Fu; input.crt.l2.entry.source_bytes[1]=0xF0u; reject_input(input); }
        { auto input=Input(); input.crt.l2.entry.source_bytes[11]=1u; reject_input(input); }
        { auto input=Input(); input.crt.l2.entry.source_bytes[12]=0x7Fu; input.crt.l2.entry.source_bytes[13]=0x80u; reject_input(input); }
        const auto& gates = BootElapsedWorkRawGatesResearch();
        Require(gates.size() == 524u, "raw work mutation coverage changed");
        for (const auto& gate : gates) {
            Require(image.ReadWord(gate.address) == gate.word, "original raw work gate disagrees with fixture");
            auto changed = raw;
            changed.at(FileOffset(raw, gate.address)+3u) ^= 1u;
            Reject([&] { ProduceBootElapsedWorkResearch(LoadValidatedFixture(changed), Input()); }, "raw work mutation admitted");
            ++rejected;
        }
        Require(paths == 25u && rejected == 557u, "native work falsification coverage lost");
        std::cout << "PAL research semantic work passed; " << paths << " paths, " << rejected
                  << " raw/input mutations rejected; no clock admission\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
