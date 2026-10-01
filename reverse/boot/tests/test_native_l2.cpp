#include "shadow/boot/NativeL2Prefix.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
void Require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
template<class F> void Reject(F action, const char* why) {
    try { action(); } catch (const std::exception&) { return; }
    throw std::runtime_error(why);
}
const shadow::boot::NativeL2Checkpoint& At(const shadow::boot::NativeL2Run& r, std::uint32_t pc) {
    for (const auto& c : r.checkpoints) if (c.native.state.machine.cpu.pc == pc) return c;
    throw std::runtime_error("missing L2 checkpoint");
}
}
int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("PAL DOL required");
        std::ifstream file(argv[1], std::ios::binary);
        std::vector<std::uint8_t> original(std::istreambuf_iterator<char>{file}, {});
        const auto image = shadow::boot::LoadValidatedFixture(original);
        shadow::boot::NativeL2Inputs input{};
        input.entry.hid0 = 0xC000u; input.entry.msr = 0x32u;
        input.entry.cr = 0xABCDEF12u; input.entry.xer = 0xA000007Fu;
        input.entry.ctr = 0x12985671u; input.entry.source_address = 0x805F1F30u;
        unsigned variants = 0;
        for (unsigned config = 0; config < 8; ++config) {
            const auto retained = ((config & 1u) ? 0x40000000u : 0u) |
                                  ((config & 2u) ? 0x00400000u : 0u) |
                                  ((config & 4u) ? 0x00080000u : 0u);
            for (auto enable : {0u, 0x80000000u}) {
                input.l2cr = retained | enable;
                const auto run = shadow::boot::RunImmutableNativeL2Prefix(image, input);
                const auto& final = run.checkpoints.back();
                const auto& state = final.native.state;
                Require(final.l2cr == (retained | 0x80000000u), "L2 config lost or forced");
                Require(state.machine.cpu.pc == 0x80372904u && state.machine.cpu.gpr[3] == 1u &&
                        state.machine.cpu.gpr[4] == 0x803726D8u, "downstream handler inputs differ");
                Require(state.machine.msr == 0x2032u && state.xer == input.entry.xer &&
                        final.native.ctr == input.entry.ctr, "MSR/XER/CTR not preserved");
                Require(state.machine.cpu.gpr[1] == 0x8060C5E0u &&
                        state.machine.cpu.gpr[31] == 0x80561380u, "helper/logger frame restoration differs");
                if (enable) {
                    Require(run.effects.size() == 1u && run.effects[0].kind == shadow::boot::L2EffectKind::Read,
                            "enabled path performed an L2 write/invalidate");
                    Require(run.ordered_stack_writes.empty() && state.machine.cpu.gpr[30] == 0u &&
                            state.machine.cpu.lr == 0x80372898u && state.cr == 0x5BCDEF12u,
                            "enabled path touched saved MSR/logging/CR");
                } else {
                    const auto& poll = At(run, 0x80372684u);
                    Require(poll.l2cr == (retained | 0x00200000u) &&
                            poll.native.state.machine.cpu.gpr[0] == 0u &&
                            poll.native.state.machine.msr == 0x30u, "first poll state/request incorrect");
                    Require(At(run, 0x803726C0u).l2cr == retained &&
                            At(run, 0x803728DCu).native.state.machine.msr == 0x2032u,
                            "fresh second poll/MSR restore differs");
                    Require(state.machine.cpu.lr == 0x803728F8u && state.machine.cpu.gpr[30] == 0x2032u &&
                            state.cr == 0x39CDEF12u, "logger CR bit6 or SO/other CR fields differ");
                    Require(run.ordered_stack_writes.size() == 12u, "helper/logger stores skipped");
                    Require(final.stack.LoadBE32(0x8060C5E4u) == 0x803728D4u &&
                            final.stack.LoadBE32(0x8060C5D0u) == 0x8060C5E0u &&
                            final.stack.LoadBE32(0x8060C5DCu) == 0x80561380u &&
                            final.stack.LoadBE32(0x8060C570u) == 0x8060C5E0u &&
                            final.stack.LoadBE32(0x8060C578u) == 0x80561564u,
                            "actual saved words/logger args differ");
                    unsigned reads = 0, writes = 0, done = 0;
                    for (const auto& e : run.effects) {
                        reads += e.kind == shadow::boot::L2EffectKind::Read;
                        writes += e.kind == shadow::boot::L2EffectKind::Write;
                        done += e.kind == shadow::boot::L2EffectKind::InvalidateCompleted;
                    }
                    Require(reads == 8u && writes == 5u && done == 1u,
                            "fresh reads or command completion missing");
                }
                Require(final.stack.LoadBE32(0x8060C5E0u) == 0x8060C5F0u &&
                        final.stack.LoadBE32(0x8060C5F4u) == 0x8000341Cu,
                        "original committed cache frame lost");
                Reject([&] { (void)final.stack.LoadBE32(0x8060C5B0u); }, "unwritten stack treated as zero");
                ++variants;
            }
        }
        // Every unsupported bit is rejected, even when E=1 would skip polls.
        for (unsigned bit = 0; bit < 32; ++bit) if (!((1u << bit) & 0xC0480000u)) {
            for (auto enable : {0u, 0x80000000u}) {
                input.l2cr = enable | (1u << bit);
                Reject([&] { (void)shadow::boot::RunImmutableNativeL2Prefix(image, input); },
                       "unsupported L2CR bit accepted");
            }
        }
        // Pure branch tests do not make busy states acceptable to the runner.
        for (unsigned bit = 0; bit < 32; ++bit) {
            const auto value = 1u << bit;
            Require(shadow::boot::L2BranchSuccessor(0x803728A0u, value) ==
                    (bit == 31 ? 0x803728F8u : 0x803728A4u), "L2E mask contains other bits");
            Require(shadow::boot::L2BranchSuccessor(0x80372684u, value) ==
                    (bit == 0 ? 0x80372678u : 0x80372688u), "first IP mask contains other bits");
            Require(shadow::boot::L2BranchSuccessor(0x803726C0u, value) ==
                    (bit == 0 ? 0x803726A8u : 0x803726C4u), "second IP mask/target differs");
        }
        Reject([] { (void)shadow::boot::L2BranchSuccessor(0x80372680u, 0u); }, "unknown branch accepted");
        shadow::boot::L2StackBytes memory;
        Reject([&] { (void)memory.LoadBE32(memory.base); }, "uninitialized memory loaded");
        Reject([&] { memory.StoreBE32(memory.base-4u, 0u); }, "lower address accepted");
        Reject([&] { memory.StoreBE32(memory.base+0x90u, 0u); }, "upper address accepted");
        Reject([&] { memory.StoreBE32(memory.base+1u, 0u); }, "unaligned address accepted");
        memory.StoreBE32(memory.base, 0x01234567u); memory.valid[1] = false;
        Reject([&] { (void)memory.LoadBE32(memory.base); }, "partial word accepted");
        input.l2cr = 0u;
        const auto first = shadow::boot::RunImmutableNativeL2Prefix(image, input);
        // MSR restoration must use the live saved word, never 2032 from the
        // baseline capture. RI differs, while admitted execution stays closed.
        auto other = input;
        other.entry.msr = 0x30u; other.entry.cr = 0xFEDCBA98u; other.entry.xer = 0x20000055u;
        const auto restored = shadow::boot::RunImmutableNativeL2Prefix(image, other);
        Require(restored.checkpoints.back().native.state.machine.msr == 0x2030u &&
                restored.checkpoints.back().native.state.machine.cpu.gpr[30] == 0x2030u &&
                restored.checkpoints.back().native.state.cr == 0x2CDCBA98u &&
                restored.checkpoints.back().native.state.xer == 0x20000055u,
                "live MSR/CR/XER variant coerced to reference values");
        Require(At(restored, 0x80370CD4u).stack.LoadBE32(0x8060C5D0u) == 0x8060C5E0u,
                "skipped logger f8 store erased prior helper backchain");
        input.l2cr = 0x80000000u;
        (void)shadow::boot::RunImmutableNativeL2Prefix(image, input);
        input.l2cr = 0u;
        const auto repeat = shadow::boot::RunImmutableNativeL2Prefix(image, input);
        Require(first.effects.size() == repeat.effects.size() &&
                first.checkpoints.back().stack.bytes == repeat.checkpoints.back().stack.bytes,
                "L2 stale state leaked across runs");
        // All checked L2 words, including rejected retry edges and next call.
        unsigned mutations = 0;
        const std::array<std::array<std::uint32_t, 2>, 5> ranges{{
            {{0x80372894u,0x80372904u}}, {{0x80372640u,0x803726D4u}},
            {{0x80370ADCu,0x80370AE8u}}, {{0x80370AFCu,0x80370B08u}},
            {{0x80370C8Cu,0x80370CD8u}}}};
        for (const auto& r : ranges) for (auto pc = r[0]; pc <= r[1]; pc += 4u) {
            auto bytes = original;
            bytes[0x2600u+pc-0x80008D40u] ^= 1u;
            Reject([&] { (void)shadow::boot::RunImmutableNativeL2Prefix(
                        shadow::boot::LoadValidatedFixture(bytes), input); }, "changed L2 word accepted");
            ++mutations;
        }
        std::cout << "PAL native L2 gates passed; " << variants << " input variants; "
                  << mutations << " raw-word mutations rejected\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n'; return 1;
    }
}
