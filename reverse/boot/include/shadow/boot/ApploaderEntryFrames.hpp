#pragma once

#include "shadow/boot/InitialBootEvents.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace shadow::boot {

// Declared source prerequisites for an OFFLINE conditional research run.
// Selecting this profile does not prove a live original machine's lifecycle.
struct ApploaderEntryResearchProfile {
    enum class Registers { Unknown, FreshPowerPcResetGcBs2RunFunction42 };
    enum class Memory { Unknown, FreshMem1ClearOriginalAppCopy42 };
    enum class Mapping { Unknown, GcBatSource42 };
    enum class Cache { Unknown, DirectRamCacheInterpreterDisabled42 };
    enum class Lease { Unknown, NoRestoreOrForeignWritesConditional42 };
    enum class FirstAdvance { Unknown, UnchangedCpuRamConditional42 };
    Registers registers = Registers::Unknown;
    Memory memory = Memory::Unknown;
    Mapping mapping = Mapping::Unknown;
    Cache cache = Cache::Unknown;
    Lease lease = Lease::Unknown;
    FirstAdvance first_advance = FirstAdvance::Unknown;
};
ApploaderEntryResearchProfile ConditionalFreshGcPalEntryResearchProfile42();

// A finite subset retained by this reconstruction. Timebase, DEC, scheduler,
// cache lines, MMU tables and device/worker state are deliberately not owned.
// A future connector must apply the listed effects to its continuous owners.
struct ApploaderEntryRegisters {
    std::array<std::uint32_t,32> gpr{};
    std::uint32_t pc{}, npc{}, lr{}, cr{}, xer{}, ctr{}, fpscr{}, msr{}, exceptions{};
    std::uint32_t hid0{}, hid1{}, hid2{};
    std::uint32_t ibat0u{}, ibat0l{}, dbat0u{}, dbat0l{}, dbat1u{}, dbat1l{};
    std::uint32_t ram_real{}, ram_mask{};
    bool effective_dcache{};
    bool reserve{};
    std::uint32_t reserve_address{};
    std::array<std::uint32_t,16> segment{};
    std::array<std::uint32_t,8> gqr{};
    std::array<std::array<std::uint64_t,2>,32> paired_single{};
};
struct ApploaderEntryStack {
    std::uint32_t logical_base{}, physical_base{};
    std::array<std::uint8_t,64> bytes{};
    std::array<bool,64> known{};
};
struct ApploaderEntryFrameState {
    ApploaderEntryRegisters cpu;
    ApploaderEntryStack stack;
};
struct ApploaderEntryStore {
    std::uint32_t instruction_pc{}, logical_address{}, physical_address{}, value{};
};
struct ApploaderEntryInstruction {
    std::uint32_t instruction_pc{}, word{};
    ApploaderEntryFrameState enter, inner_exit;
};
enum class ApploaderEntryFrameBoundary { BeforeEntry, BeforeNestedCall, Before812003B8 };

// These fields are owned native values after source construction. Explicit
// fault injection below is for falsification only and never grants admission.
enum class EntryResearchRetainedField {
    Lr, Cr, Xer, Ctr, Fpscr, Msr, Exceptions, Hid0, Hid1, Hid2,
    Ibat0U, Ibat0L, Dbat0U, Dbat0L, Dbat1U, Dbat1L,
    RamReal, RamMask, EffectiveDcache
};

// Owns private bounded storage, not an emulator or an original RAM witness.
// Construction consumes original bytes and emits fresh source-derived values;
// no captured register, memory, clock, count, log or post-state is an argument.
class ApploaderEntryFramesResearch {
public:
    ApploaderEntryFramesResearch(const std::vector<std::uint8_t>& apploader,
                                 const std::vector<std::uint8_t>& boot,
                                 const std::vector<std::uint8_t>& bi2,
                                 const ApploaderEntryResearchProfile&);
    void ProduceFirstThree();
    void ProduceNestedPrologue();
    const ApploaderEntryFrameState& State() const { return state_; }
    const ApploaderEntryFrameState& SourceEntryState() const { return initial_; }
    const ApploaderEntryFrameState& BeforeNestedCall() const;
    const std::vector<ApploaderEntryStore>& Stores() const { return stores_; }
    const std::vector<ApploaderEntryInstruction>& Instructions() const { return instructions_; }
    ApploaderEntryFrameBoundary Boundary() const { return boundary_; }
    std::uint32_t CompletedSourceInstructions() const { return completed_; }
    std::uint32_t HandlerWord() const;
    std::uint32_t ReportWord() const;
    bool IsFalsificationRun() const { return falsification_; }

    // Native writes to the private research owner, solely for negative/live
    // value tests. These are not original-state imports or producer proofs.
    void WriteResearchGprForFalsification(unsigned index, std::uint32_t value);
    void WriteResearchRetainedForFalsification(EntryResearchRetainedField, std::uint32_t);
    void WriteResearchStackByteForFalsification(std::uint32_t address, std::uint8_t value);
    void ForgetResearchStackByteForFalsification(std::uint32_t address);
private:
    void Preflight(const std::vector<std::uint32_t>& addresses) const;
    void Begin(std::uint32_t pc, std::uint32_t word);
    void End();
    void Store(std::uint32_t address, std::uint32_t value);
    std::uint32_t CodeWord(std::uint32_t address) const;
    std::size_t StackOffset(std::uint32_t address) const;
    ApploaderEntryFrameState initial_, state_, nested_;
    std::vector<std::uint8_t> app_body_;
    std::array<std::uint8_t,4> handler_{}, report_{};
    std::vector<ApploaderEntryStore> stores_;
    std::vector<ApploaderEntryInstruction> instructions_;
    ApploaderEntryFrameBoundary boundary_ = ApploaderEntryFrameBoundary::BeforeEntry;
    std::uint32_t completed_{};
    bool falsification_{};
};

// FAIL CLOSED: the current capsule has declared controls/effect delivery,
// even at Complete. No current InitialBootEventOwner can certify live worker,
// Movie, configuration or achievement lifecycle equivalence. Always rejects;
// no caller bool/enum can turn the research profile into native admission.
void RequireNativeApploaderEntryFrameOwnership(const InitialBootEventOwner&);

} // namespace shadow::boot
