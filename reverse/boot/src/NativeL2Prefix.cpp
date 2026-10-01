#include "shadow/boot/NativeL2Prefix.hpp"

#include <stdexcept>

namespace shadow::boot {
namespace {
constexpr std::uint32_t kEnable = 0x80000000u;
constexpr std::uint32_t kInvalidate = 0x00200000u;
constexpr std::uint32_t kBusy = 1u;
constexpr std::uint32_t kPassive = 0x40480000u; // CE, DO, WT; not TS.

unsigned Offset(std::uint32_t address) {
    if ((address & 3u) || address < L2StackBytes::base ||
        std::uint64_t(address) + 4u > std::uint64_t(L2StackBytes::base) + 0x90u)
        throw std::runtime_error("L2 stack address is unknown, unaligned or outside owned bytes");
    return address - L2StackBytes::base;
}
template<std::size_t N>
void CheckWords(const BootImage& image, std::uint32_t pc,
                const std::array<std::uint32_t, N>& words) {
    for (auto word : words) {
        if (image.ReadWord(pc) != word)
            throw std::runtime_error("native L2 instruction fingerprint mismatch");
        pc += 4u;
    }
}

// No L2 tag array/data shadow, guest interpreter, device callback, queued
// store or caller acknowledgment exists in this owner. The only writes are
// synchronously committed to private byte storage. Cache residency cannot
// affect these bytes. The configuration word remains visible for consumers.
class OwnedL2 {
public:
    OwnedL2(std::uint32_t entry, const L2StackBytes& memory,
            std::vector<L2Effect>& trace) : control_(entry), passive_(entry & kPassive),
                                           memory_(memory), trace_(trace) {
        if (entry & kBusy) throw std::runtime_error("incoming L2IP producer/completion is unresolved");
        if (entry & kInvalidate) throw std::runtime_error("incoming L2I command history is unresolved");
        if (entry & ~(kEnable | kPassive))
            throw std::runtime_error("L2 test/reserved state is outside the native contract");
    }
    std::uint32_t Read(std::uint32_t pc) {
        const auto value = Value();
        trace_.push_back({L2EffectKind::Read, pc, value});
        return value;
    }
    std::uint32_t Value() const {
        // Status is owned work, never echoed from an mtspr operand.
        return control_ | (requested_ != completed_ ? kBusy : 0u);
    }
    void CompleteStores(std::uint32_t pc) {
        // Reads actual bytes for each initialized word. No write-event log or
        // host fence is accepted as completion of an outstanding guest store.
        for (unsigned n = 0; n < memory_.bytes.size(); n += 4u) {
            bool any = false, all = true;
            for (unsigned b = 0; b < 4; ++b) {
                any |= memory_.valid[n+b]; all &= memory_.valid[n+b];
            }
            if (any && !all) throw std::runtime_error("partly committed L2 stack word");
            if (all) (void)memory_.LoadBE32(memory_.base + n);
        }
        stores_complete_ = true;
        trace_.push_back({L2EffectKind::StoreCompletion, pc, control_});
    }
    void Write(std::uint32_t pc, std::uint32_t operand) {
        if ((operand & ~(kEnable | kPassive | kInvalidate)) ||
            (operand & kPassive) != passive_)
            throw std::runtime_error("L2 write has unproven configuration/status effects");
        if ((operand & (kEnable | kInvalidate)) == (kEnable | kInvalidate))
            throw std::runtime_error("cannot invalidate enabled L2");
        const bool starts = !(control_ & kInvalidate) && (operand & kInvalidate);
        control_ = operand;
        trace_.push_back({L2EffectKind::Write, pc, operand});
        if (starts) {
            if (!stores_complete_ || requested_ != completed_)
                throw std::runtime_error("L2 invalidation lacks owned store completion");
            ++requested_;
            // IBM 9.1.4 clears tags/status/LRU, not authoritative RAM. This
            // backend has no cache-owned data or tags to retain/discard. All
            // prior writes are materialized; no later observer can see a
            // different generation. Completion is synchronous here, not a
            // prediction of the console's ~32K-cycle hardware state machine.
            CompleteStores(pc);
            completed_ = requested_;
            trace_.push_back({L2EffectKind::InvalidateCompleted, pc, completed_});
        }
        if ((operand & kEnable) && requested_ != completed_)
            throw std::runtime_error("L2 enabled with unresolved work");
    }
private:
    std::uint32_t control_, passive_;
    const L2StackBytes& memory_;
    std::vector<L2Effect>& trace_;
    std::uint32_t requested_ = 0, completed_ = 0;
    bool stores_complete_ = false;
};
}

void L2StackBytes::StoreBE32(std::uint32_t address, std::uint32_t value) {
    auto offset = Offset(address);
    for (unsigned n = 0; n < 4; ++n) {
        bytes[offset+n] = static_cast<std::uint8_t>(value >> (24u-8u*n));
        valid[offset+n] = true;
    }
}
std::uint32_t L2StackBytes::LoadBE32(std::uint32_t address) const {
    auto offset = Offset(address);
    std::uint32_t result = 0;
    for (unsigned n = 0; n < 4; ++n) {
        if (!valid[offset+n]) throw std::runtime_error("L2 load consumes unwritten bytes");
        result = (result << 8u) | bytes[offset+n];
    }
    return result;
}

std::uint32_t L2BranchSuccessor(std::uint32_t pc, std::uint32_t value) {
    switch (pc) {
    case 0x803728A0u: return value & kEnable ? 0x803728F8u : 0x803728A4u;
    case 0x80372684u: return value & kBusy ? 0x80372678u : 0x80372688u;
    case 0x803726C0u: return value & kBusy ? 0x803726A8u : 0x803726C4u;
    default: throw std::runtime_error("unknown L2 branch site");
    }
}

NativeL2Run RunImmutableNativeL2Prefix(const BootImage& image, const NativeL2Inputs& inputs) {
    NativeL2Run run{};
    // Preserve checkpoint 37 exactly. No mid-chain machine state is an input.
    run.prefix = RunImmutableNativeBootPrefix(image, inputs.entry);
    auto live = run.prefix.checkpoints.back();
    L2StackBytes memory;
    const auto import = [&memory](const PairedSetupStackMemory& source) {
        for (unsigned n = 0; n < 16; n += 4u) {
            bool any = false, all = true;
            for (unsigned b = 0; b < 4; ++b) { any |= source.valid[n+b]; all &= source.valid[n+b]; }
            if (any && !all) throw std::runtime_error("partial prefix word cannot be imported");
            if (all) memory.StoreBE32(source.base+n, source.LoadBE32(source.base+n));
        }
    };
    import(live.paired_stack);
    for (const auto& window : run.prefix.cache_stack) import(window);
    OwnedL2 effects(inputs.l2cr, memory, run.effects);
    auto& cpu = live.state.machine.cpu;
    if (cpu.pc != 0x80372894u || cpu.gpr[1] != 0x8060C5E0u)
        throw std::runtime_error("L2 entry provenance differs");
    const auto save = [&](std::uint32_t pc) {
        cpu.pc = pc;
        run.checkpoints.push_back({live, effects.Value(), memory});
    };
    const auto store = [&](std::uint32_t address, std::uint32_t value) {
        memory.StoreBE32(address, value);
        run.ordered_stack_writes.push_back({address, value});
    };
    const auto compare = [&](std::uint32_t value) {
        live.state.cr = (live.state.cr & 0x0FFFFFFFu) |
            (value ? 0x40000000u : 0x20000000u) |
            ((live.state.xer & 0x80000000u) ? 0x10000000u : 0u);
    };
    // Whole relevant bodies, including rejected busy edges, are byte-pinned.
    CheckWords(image, 0x80372894u, std::array<std::uint32_t, 29>{{
        0x4BFFE269,0x54600000,0x28000000,0x40820058,0x4BFFE239,0x7C7E1B78,
        0x7C0004AC,0x38600030,0x4BFFE231,0x7C0004AC,0x7C0004AC,0x4BFFE23D,
        0x5463007E,0x4BFFE23D,0x7C0004AC,0x4BFFFD71,0x7FC3F378,0x4BFFE20D,
        0x4BFFE221,0x64608000,0x540302D2,0x4BFFE21D,0x387F01E4,0x4CC63182,
        0x4BFFE399,0x3C608037,0x388326D8,0x38600001,0x48000A75}});
    CheckWords(image, 0x80372640u, std::array<std::uint32_t, 38>{{
        0x7C0802A6,0x90010004,0x9421FFF0,0x93E1000C,0x7C0004AC,0x4BFFE4A9,
        0x5463007E,0x4BFFE4A9,0x7C0004AC,0x4BFFE499,0x64630020,0x4BFFE499,
        0x48000004,0x48000004,0x4BFFE485,0x546007FE,0x28000000,0x4082FFF4,
        0x4BFFE475,0x546302D2,0x4BFFE475,0x48000004,0x3C608056,0x3BE31380,
        0x48000004,0x48000010,0x7FE3FB78,0x4CC63182,0x4BFFE5DD,0x4BFFE449,
        0x546007FE,0x28000000,0x4082FFE8,0x80010014,0x83E1000C,0x38210010,
        0x7C0803A6,0x4E800020}});
    CheckWords(image, 0x80370ADCu, std::array<std::uint32_t, 4>{{
        0x7C6000A6,0x4E800020,0x7C600124,0x4E800020}});
    CheckWords(image, 0x80370AFCu, std::array<std::uint32_t, 4>{{
        0x7C79FAA6,0x4E800020,0x7C79FBA6,0x4E800020}});
    CheckWords(image, 0x80370C8Cu, std::array<std::uint32_t, 20>{{
        0x9421FF90,0x40860024,0xD8210028,0xD8410030,0xD8610038,0xD8810040,
        0xD8A10048,0xD8C10050,0xD8E10058,0xD9010060,0x90610008,0x9081000C,
        0x90A10010,0x90C10014,0x90E10018,0x9101001C,0x91210020,0x91410024,
        0x38210070,0x4E800020}});
    const auto read = [&](std::uint32_t call) { cpu.lr = call+4u; cpu.gpr[3] = effects.Read(call); };
    const auto write = [&](std::uint32_t call) { cpu.lr = call+4u; effects.Write(call, cpu.gpr[3]); };
    read(0x80372894u); save(0x80372898u);
    cpu.gpr[0] = cpu.gpr[3] & kEnable; compare(cpu.gpr[0]); save(0x803728A0u);
    if (L2BranchSuccessor(cpu.pc, cpu.gpr[3]) == 0x803728A4u) {
        cpu.lr = 0x803728A8u; cpu.gpr[3] = live.state.machine.msr;
        cpu.gpr[30] = cpu.gpr[3]; effects.CompleteStores(0x803728ACu);
        cpu.gpr[3] = 0x30u; cpu.lr = 0x803728B8u; live.state.machine.msr = cpu.gpr[3];
        effects.CompleteStores(0x803728B8u); effects.CompleteStores(0x803728BCu);
        read(0x803728C0u); cpu.gpr[3] &= ~kEnable; write(0x803728C8u);
        effects.CompleteStores(0x803728CCu);
        cpu.lr = 0x803728D4u; save(0x80372640u);
        // The helper's saved LR/r31 are read from real stores on return.
        cpu.gpr[0] = cpu.lr; store(cpu.gpr[1]+4u, cpu.gpr[0]);
        const auto parent_sp = cpu.gpr[1]; cpu.gpr[1] -= 16u;
        store(cpu.gpr[1], parent_sp); store(cpu.gpr[1]+12u, cpu.gpr[31]);
        effects.CompleteStores(0x80372650u);
        read(0x80372654u); cpu.gpr[3] &= ~kEnable; write(0x8037265Cu);
        effects.CompleteStores(0x80372660u);
        read(0x80372664u); cpu.gpr[3] |= kInvalidate; save(0x8037266Cu);
        write(0x8037266Cu); save(0x80372678u);
        read(0x80372678u); cpu.gpr[0] = cpu.gpr[3] & kBusy;
        compare(cpu.gpr[0]); save(0x80372684u);
        if (L2BranchSuccessor(cpu.pc, cpu.gpr[3]) != 0x80372688u)
            throw std::runtime_error("first L2 poll has unexplained pending work");
        save(0x80372688u); read(0x80372688u); cpu.gpr[3] &= ~kInvalidate;
        save(0x80372690u); write(0x80372690u);
        cpu.gpr[3] = 0x80560000u; cpu.gpr[31] = cpu.gpr[3]+0x1380u;
        save(0x803726B4u); read(0x803726B4u); cpu.gpr[0] = cpu.gpr[3] & kBusy;
        compare(cpu.gpr[0]); save(0x803726C0u);
        if (L2BranchSuccessor(cpu.pc, cpu.gpr[3]) != 0x803726C4u)
            throw std::runtime_error("second L2 poll has unexplained pending work; logger retry required");
        save(0x803726C4u);
        cpu.gpr[0] = memory.LoadBE32(cpu.gpr[1]+20u);
        cpu.gpr[31] = memory.LoadBE32(cpu.gpr[1]+12u);
        cpu.gpr[1] += 16u; cpu.lr = cpu.gpr[0];
        if ((cpu.lr & ~3u) != 0x803728D4u) throw std::runtime_error("L2 helper return provenance differs");
        save(0x803728D4u);
        cpu.gpr[3] = cpu.gpr[30]; cpu.lr = 0x803728DCu; live.state.machine.msr = cpu.gpr[3];
        save(0x803728DCu); read(0x803728DCu);
        cpu.gpr[0] = cpu.gpr[3] | kEnable; cpu.gpr[3] = cpu.gpr[0] & ~kInvalidate;
        write(0x803728E8u); save(0x803728ECu);
        cpu.gpr[3] = cpu.gpr[31]+0x1E4u; live.state.cr &= ~0x02000000u;
        cpu.lr = 0x803728F8u; save(0x80370C8Cu);
        const auto logger_parent = cpu.gpr[1]; cpu.gpr[1] -= 0x70u;
        store(cpu.gpr[1], logger_parent);
        if (live.state.cr & 0x02000000u) throw std::runtime_error("unproven logger FPR-save path");
        save(0x80370CB4u);
        for (unsigned reg = 3; reg <= 10; ++reg) store(cpu.gpr[1]+8u+4u*(reg-3u), cpu.gpr[reg]);
        save(0x80370CD4u); cpu.gpr[1] += 0x70u;
    }
    save(0x803728F8u);
    cpu.gpr[3] = 0x80370000u; cpu.gpr[4] = cpu.gpr[3]+0x26D8u; cpu.gpr[3] = 1u;
    save(0x80372904u); // Next call needs live handler slot 80586CB4.
    return run;
}
}  // namespace shadow::boot
