#include "shadow/boot/NativeBootPrefix.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class F> void Reject(F action, const char* message) {
    try { action(); } catch (const std::exception&) { return; }
    throw std::runtime_error(message);
}
const shadow::boot::NativeBootCheckpoint& At(const shadow::boot::NativeBootRun& run, std::uint32_t pc) {
    for (const auto& cp : run.checkpoints) if (cp.state.machine.cpu.pc == pc) return cp;
    throw std::runtime_error("checkpoint not reached");
}
void Put(std::array<std::uint8_t, 16>& bytes, unsigned offset, std::uint32_t word) {
    for (unsigned n = 0; n < 4; ++n) bytes[offset+n] = static_cast<std::uint8_t>(word >> (24-8*n));
}
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("PAL DOL argument required");
        std::ifstream file(argv[1], std::ios::binary);
        const std::vector<std::uint8_t> original(std::istreambuf_iterator<char>{file}, {});
        const auto image = shadow::boot::LoadValidatedFixture(original);
        shadow::boot::NativeBootInputs input{};
        input.msr = 0x32u; input.hid0 = 0x0011C064u;
        input.hid2 = 0x40000000u; // PSE/LSQE initially clear: producers must enable them.
        input.cr = 0xABCDE123u; input.xer = 0xA000007Fu;
        input.ctr = 0x12345679u; input.fpscr = 0x01000080u;
        for (unsigned n = 0; n < 32; ++n) input.fpr[n] = {0x100000000ull+n, 0x200000000ull+n};
        for (unsigned n = 0; n < 8; ++n) input.gqr[n] = 0x07040704u+n;
        input.source_address = 0x805F1F30u;
        Put(input.source_bytes, 0, 0x400A0000u);
        Put(input.source_bytes, 8, 0x3FC00000u); Put(input.source_bytes, 12, 0xC0100000u);
        const auto run = shadow::boot::RunImmutableNativeBootPrefix(image, input);
        const auto& before = At(run, 0x80371730u);
        const auto& after = At(run, 0x80371734u);
        Require(before.hid0 == input.hid0 && before.state.machine.cpu.gpr[3] == (input.hid0 | 0x800u),
                "ICFI operand conflated with self-cleared HID0");
        Require(before.hid2 == 0xE0000000u, "paired enable not derived from write");
        Require(before.gqr == input.gqr && after.gqr == input.gqr, "GQR changed before actual writes");
        Require(before.state.machine.cpu.gpr == after.state.machine.cpu.gpr &&
                before.state.cr == after.state.cr && before.state.xer == after.state.xer &&
                before.state.fpscr == after.state.fpscr && before.ctr == after.ctr,
                "sync changed architectural registers");
        Require(after.paired_stack.LoadBE32(0x8060C5F4u) == 0x80003414u &&
                after.paired_stack.LoadBE32(0x8060C5E8u) == 0x8060C5F0u,
                "sync did not preserve actual committed stack bytes");
        Reject([&] { (void)after.paired_stack.LoadBE32(0x8060C5ECu); }, "unwritten memory fabricated");
        const auto& consumer = At(run, 0x80370CDCu);
        Require(std::all_of(consumer.gqr.begin(), consumer.gqr.end(), [](auto q) { return q == 0u; }),
                "GQR outputs did not come from eight writes");
        for (unsigned n = 0; n < 32; ++n)
            Require(consumer.state.fpr[n].ps0 == input.fpr[n].ps0 && consumer.state.fpr[n].ps1 == input.fpr[n].ps1,
                    "incoming FPR state lost before consumer");
        Require(At(run, 0x80370D00u).state.fpr[0].ps1 == 0xC002000000000000ull,
                "paired consumer did not consume connected source");
        const auto& last = run.checkpoints.back();
        Require(last.state.machine.cpu.pc == 0x80372894u && last.state.machine.cpu.lr == 0x80372878u &&
                last.state.machine.cpu.gpr[0] == 0x4000u && last.state.machine.cpu.gpr[3] == input.hid0,
                "downstream HID0 branch/readback differs");
        Require(last.state.cr == 0x5BCDE123u && last.state.xer == input.xer && last.ctr == input.ctr,
                "CR0.SO/other fields/CTR changed incorrectly");
        Require(last.paired_stack.LoadBE32(0x8060C5F4u) == 0x8000341Cu &&
                last.paired_stack.LoadBE32(0x8060C5ECu) == 0x8000315Cu &&
                last.paired_stack.LoadBE32(0x8060C5E8u) == 0u,
                "later frame did not update overlapping stack addresses");
        Require(run.cache_stack[0].LoadBE32(0x8060C5E0u) == 0x8060C5F0u,
                "cache frame backchain not committed");

        auto changed = input;
        for (auto flag : {0x40000u, 0x4000u, 0x8000u, 0x800u, 0x100u, 0x400u, 0x200u, 1u}) {
            changed = input; changed.msr |= flag;
            Reject([&] { (void)shadow::boot::RunImmutableNativeBootPrefix(image, changed); }, "unsafe MSR accepted");
        }
        for (auto flag : {0x400u, 0x800u, 8u, 0x2000u, 0x1000u, 0x80000000u}) {
            changed = input; changed.hid0 |= flag;
            Reject([&] { (void)shadow::boot::RunImmutableNativeBootPrefix(image, changed); }, "unproven HID0 accepted");
        }
        for (auto enable : {0x8000u, 0x4000u}) {
            changed = input; changed.hid0 &= ~enable;
            Reject([&] { (void)shadow::boot::RunImmutableNativeBootPrefix(image, changed); }, "unimplemented cache mode accepted");
        }
        for (auto flag : {0x10000000u, 0x01000000u, 0x00010000u, 1u}) {
            changed = input; changed.hid2 |= flag;
            Reject([&] { (void)shadow::boot::RunImmutableNativeBootPrefix(image, changed); }, "DMA/locked/reserved HID2 accepted");
        }
        changed = input; changed.source_address += 4u;
        Reject([&] { (void)shadow::boot::RunImmutableNativeBootPrefix(image, changed); }, "source pointer provenance lost");
        changed = input; Put(changed.source_bytes, 8, 1u);
        Reject([&] { (void)shadow::boot::RunImmutableNativeBootPrefix(image, changed); }, "unknown subnormal consumed");
        changed = input; changed.hid0 = 0xC000u; changed.hid2 = 0u;
        Put(changed.source_bytes, 8, 0x80000000u); Put(changed.source_bytes, 12, 0u);
        const auto variant = shadow::boot::RunImmutableNativeBootPrefix(image, changed);
        Require(variant.checkpoints.back().hid0 == changed.hid0 && variant.checkpoints.back().hid2 == 0xA0000000u &&
                At(variant, 0x80370D00u).state.fpr[0].ps0 == 0x8000000000000000ull,
                "variant was coerced to reference constants");
        const auto repeat = shadow::boot::RunImmutableNativeBootPrefix(image, input);
        Require(repeat.checkpoints.back().hid0 == run.checkpoints.back().hid0 &&
                repeat.checkpoints.back().state.fpr[0].ps1 == run.checkpoints.back().state.fpr[0].ps1,
                "mutable stale state leaked between runs");

        // Every reached raw instruction plus the next stop word, not just
        // selected prologues. Inject even a legal nop: it must not be ignored.
        unsigned mutations = 0;
        const std::array<std::array<std::uint32_t, 2>, 10> ranges{{
            {{0x80003154u, 0x80003158u}}, {{0x800032B0u, 0x8000333Cu}},
            {{0x80003400u, 0x80003418u}}, {{0x80371714u, 0x80371764u}},
            {{0x80370BA8u, 0x80370BB4u}}, {{0x803725F4u, 0x80372600u}},
            {{0x80370CDCu, 0x80370E00u}}, {{0x80372838u, 0x80372860u}},
            {{0x80372874u, 0x80372880u}}, {{0x80370AECu, 0x80370AF0u}},
        }};
        const auto mutate = [&](std::uint32_t pc) {
            auto dol = original;
            const auto offset = pc < 0x80005600u ? 0x100u + pc - 0x80003100u : 0x2600u + pc - 0x80008D40u;
            dol[offset] ^= 1u;
            Reject([&] { (void)shadow::boot::RunImmutableNativeBootPrefix(shadow::boot::LoadValidatedFixture(dol), input); },
                   "changed connected instruction accepted");
            ++mutations;
        };
        for (const auto& r : ranges) for (auto pc = r[0]; pc <= r[1]; pc += 4) mutate(pc);
        mutate(0x80372894u);
        std::cout << "PAL connected native sync/FPR/cache-consumer gates passed; " << mutations << " raw-word mutations rejected\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
