#include "shadow/boot/NativeBootPrefix.hpp"

#include <stdexcept>

namespace shadow::boot {
namespace {

// These private operations cannot accept a forged 'sync completed' flag or a
// mutable external memory backend. The only producer is the checked entry run.
class LocalCompletion {
public:
    explicit LocalCompletion(const NativeBootInputs& inputs)
        : hid0_(inputs.hid0), hid2_(inputs.hid2), gqr_(inputs.gqr) {
        if ((inputs.msr & 0x4CF01u) != 0u)
            throw std::runtime_error("native entry has unsupported MSR mode");
        // DPM/NHR, ICE/DCE, DCFA/BTIC/BHT are the only admitted HID0 bits.
        // The full-word mtspr also reissues DCFI if present: do NOT mask it.
        if ((hid0_ & 0x400u) != 0u)
            throw std::runtime_error("HID0 DCFI may discard dirty data; provenance unresolved");
        if ((hid0_ & ~0x0011C064u) != 0u || (hid0_ & 0xC000u) != 0xC000u)
            throw std::runtime_error("native cache profile requires ICE/DCE and validated passive HID0 bits");
        // No locked cache, DMA queue/status/errors or reserved HID2 bits.
        // WPE is retained, but this closed prefix never writes the gather port.
        if ((hid2_ & ~0xE0000000u) != 0u)
            throw std::runtime_error("native prefix has unvalidated HID2 DMA/locked/reserved state");
    }

    void AcceptPairedEnable(const SprWriteRequest& request) {
        if (request.spr != 920u || request.value != (hid2_ | 0xA0000000u))
            throw std::runtime_error("paired enable producer mismatch");
        hid2_ = request.value;
        paired_code_visible_ = false;
    }

    void CompleteLocalSync(const Hid0IcfiBoundary& boundary) {
        if (boundary.request.spr != 1008u || boundary.request.value != (hid0_ | 0x800u))
            throw std::runtime_error("ICFI producer mismatch");
        const auto& stack = boundary.prefix.prefix;
        // Store completion is actual same-owner memory, not a write-event log.
        // Neither store aliases any DOL section, MMIO, code or source bytes.
        for (const auto& write : stack.ordered_writes) {
            if (write.address < 0x8060C5E8u || write.address > 0x8060C5F4u ||
                stack.stack_memory.LoadBE32(write.address) != write.value)
                throw std::runtime_error("sync has an uncommitted/out-of-scope stack write");
        }
        // IBM HID0 Table 2-4 + 3.4.1.4: ICE=1 flash invalidate completes and
        // command self-clears. Immutable C++ bodies have no stale pre-enable
        // instruction decode/cache. Preserve all noncommand control bits.
        // No MMIO, DMA, callback, shared observer or code write exists in this
        // owned finite prefix; ABE=0. Thus there is no external work to await.
        // A host fence would not prove these facts and is not substituted.
        hid0_ = boundary.request.value & ~0x800u;
        paired_code_visible_ = true;
    }

    void ApplyGqr(const SprWriteRequest& request) {
        if (!paired_code_visible_ || request.spr < 912u || request.spr > 919u)
            throw std::runtime_error("GQR write lacks completed instruction visibility");
        gqr_[request.spr - 912u] = request.value;
    }

    void RequirePairedConsumer() const {
        if (!paired_code_visible_ || (hid2_ & 0xA0000000u) != 0xA0000000u || gqr_[0] != 0u)
            throw std::runtime_error("paired consumer prerequisites incomplete");
    }

    std::uint32_t Hid0() const { return hid0_; }
    std::uint32_t Hid2() const { return hid2_; }
    const auto& Gqr() const { return gqr_; }

private:
    std::uint32_t hid0_;
    std::uint32_t hid2_;
    std::array<std::uint32_t, 8> gqr_;
    bool paired_code_visible_ = false;
};

void CheckWord(const BootImage& image, std::uint32_t pc, std::uint32_t word) {
    if (image.ReadWord(pc) != word)
        throw std::runtime_error("native connected prefix instruction fingerprint mismatch");
}

}  // namespace

NativeBootRun RunImmutableNativeBootPrefix(const BootImage& image, const NativeBootInputs& inputs) {
    LocalCompletion effects(inputs);
    NativeBootRun run{};
    FprSeedState live{};
    live.cr = inputs.cr; live.xer = inputs.xer;
    live.fpscr = inputs.fpscr; live.fpr = inputs.fpr;
    live.machine.msr = inputs.msr;
    const auto save = [&] {
        run.checkpoints.push_back({live, inputs.ctr, effects.Hid0(), effects.Hid2(), effects.Gqr(),
                                   run.paired_stack});
    };
    live.machine.cpu = EnterRegisterStartup(image); save();
    live.machine.cpu = EnterHardwareCall(image, live.machine.cpu); save();
    live.machine = EnterPairedSetupCall(image, live.machine.cpu, inputs.msr); save();
    const auto stack = EnterHid2ReadCall(image, live.machine);
    run.paired_stack = stack.stack_memory;
    live.machine = stack.machine; save();
    const auto read = ReturnFromHid2Read(image, stack, effects.Hid2());
    live.machine = read.machine; save();
    const auto enabled = IssueHid2Write(image, read);
    effects.AcceptPairedEnable(enabled.request);
    live.machine = enabled.prefix.machine; save();
    const auto icfi = IssueHid0IcfiRequest(image, enabled, effects.Hid0());
    effects.CompleteLocalSync(icfi);
    live.machine = icfi.prefix.prefix.machine; save();
    // CompleteLocalSync consumed every prior effect. sync changes no registers.
    live.machine.cpu.pc = 0x80371734u; save();
    const auto tail = PredictPostSyncGqrTail(image, icfi);
    for (const auto& write : tail.ordered_gqr_writes) effects.ApplyGqr(write);
    live.machine = tail.before_saved_lr_load; save();
    live.machine = tail.after_return; save();
    run.paired_stack = icfi.prefix.prefix.stack_memory;

    CheckWord(image, 0x80003414u, 0x4836D8C9u); // bl 80370CDC
    if (live.machine.cpu.pc != 0x80003414u)
        throw std::runtime_error("saved LR did not return to FPR caller");
    live.machine.cpu.lr = 0x80003418u;
    live.machine.cpu.pc = 0x80370CDCu; save();
    effects.RequirePairedConsumer();
    const auto fpr = PredictFprSeed(image, {live, effects.Hid2(), effects.Gqr()[0],
                                         inputs.source_address, inputs.source_bytes});
    for (const auto& checkpoint : fpr.checkpoints) { live = checkpoint; save(); }

    CheckWord(image, 0x80003418u, 0x4836F421u); // bl 80372838
    live.machine.cpu.lr = 0x8000341Cu;
    live.machine.cpu.pc = 0x80372838u; save();
    // PROVEN bounded cache-consumer path. Only enabled ICE/DCE is admitted;
    // optional helpers/logging and the next L2CR read remain outside this run.
    constexpr std::array<std::uint32_t, 11> cache_prefix{{
        0x7C0802A6u, 0x90010004u, 0x9421FFF0u, 0x93E1000Cu,
        0x93C10008u, 0x3C608056u, 0x3BE31380u, 0x4BFFE299u,
        0x54600420u, 0x28000000u, 0x40820014u,
    }};
    for (unsigned n = 0; n < cache_prefix.size(); ++n)
        CheckWord(image, 0x80372838u + 4u*n, cache_prefix[n]);
    CheckWord(image, 0x80370AECu, 0x7C70FAA6u);
    CheckWord(image, 0x80370AF0u, 0x4E800020u);
    constexpr std::array<std::uint32_t, 4> dce_words{{
        0x4BFFE279u, 0x54600462u, 0x28000000u, 0x40820014u,
    }};
    for (unsigned n = 0; n < dce_words.size(); ++n)
        CheckWord(image, 0x80372874u + 4u*n, dce_words[n]);
    CheckWord(image, 0x80372894u, 0x4BFFE269u); // next unexecuted L2CR call
    auto& cpu = live.machine.cpu;
    const auto sp = cpu.gpr[1];
    cpu.gpr[0] = cpu.lr;
    run.cache_stack_writes = {{{sp + 4u, cpu.gpr[0]}, {sp - 16u, sp},
                              {sp - 4u, cpu.gpr[31]}, {sp - 8u, cpu.gpr[30]}}};
    // A private byte window applies these stores now; it is not a cache model.
    // Four words span 20 bytes, with one untouched word in the middle.
    auto& cache_bytes = run.cache_stack;
    cache_bytes[0].base = sp - 16u; cache_bytes[1].base = sp;
    for (const auto& w : run.cache_stack_writes)
        cache_bytes[w.address < sp ? 0u : 1u].StoreBE32(w.address, w.value);
    for (const auto& w : run.cache_stack_writes)
        if (w.address >= run.paired_stack.base &&
            std::uint64_t(w.address) + 4 <= std::uint64_t(run.paired_stack.base) + 16)
            run.paired_stack.StoreBE32(w.address, w.value);
    for (const auto& w : run.cache_stack_writes)
        if (cache_bytes[w.address < sp ? 0u : 1u].LoadBE32(w.address) != w.value)
            throw std::runtime_error("cache frame store not committed");
    cpu.gpr[1] = sp - 16u;
    cpu.gpr[31] = 0x80561380u; // lis/addi in the pinned prologue
    const auto compare = [&live](std::uint32_t value) {
        live.cr = (live.cr & 0x0FFFFFFFu) | (value ? 0x40000000u : 0x20000000u) |
                  ((live.xer & 0x80000000u) ? 0x10000000u : 0u);
    };
    cpu.gpr[3] = effects.Hid0(); cpu.gpr[0] = cpu.gpr[3] & 0x8000u;
    compare(cpu.gpr[0]); cpu.lr = 0x80372858u; cpu.pc = 0x80372860u; save();
    if (cpu.gpr[0] == 0u) throw std::runtime_error("unimplemented optional ICE branch");
    cpu.gpr[3] = effects.Hid0(); cpu.gpr[0] = cpu.gpr[3] & 0x4000u;
    compare(cpu.gpr[0]); cpu.lr = 0x80372878u; cpu.pc = 0x80372880u; save();
    if (cpu.gpr[0] == 0u) throw std::runtime_error("unimplemented optional DCE branch");
    cpu.pc = 0x80372894u; save();
    return run;
}

}  // namespace shadow::boot
