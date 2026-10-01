# First apploader Entry frame: source producers and missing original receipts

2026-10-01. Scope: the original PAL GC HLE boot profile and only the first
three words at81200258/5C/60. This audit does not admit a production clock,
replay the whole apploader, or modify the validated DOL prefix. Original
game/oracle inputs remain read-only. The source and raw-byte proof is separate
from original observations and from a native owner that actually emits state.

## DVD callback name reconciliation

`DTKClock` is the coordinator's native semantic owner label. The original
source event name is `FinishExecutingCommand`, registered with
FinishExecutingCommandCallback atDVDInterface.cpp287. Init at289..290
packs ReplyType::DTK and DIInterruptType::TCINT, then enqueues due0.
PackFinishExecutingCommandUserdata at1283..1285 shifts reply_type by32;
the callback at1288..1293 decodes the two halves. Userdata
0000000300000001 therefore selects DTK3/TCINT1. FinishExecutingCommand's
DTK case at1347..1349 callsDTKStreamingCallback. They identify the same
original event/arm; there is no original RegisterEvent("DTKClock"). The
first-Advance device/queue effects have a separate native research capsule;
its live inputs and completion remain proof obligations. They cannot be
inferred from Entry's instruction count.

## Raw authority and source pins

The first code byte of apploader.img maps file20 to81200000. Original
header entry81200258, code1A98 and trailer1C3A0 imply the host load interval
physical[01200000,0121DE38), excluding the stack and report-stub intervals.
The full file SHA is
8a1fe9487b029b2518a6633b1e0eb9ec0922f5f5eb5afdd7eba1267a85f096fe.

| PC | Original word | Exact architectural statement |
|---|---|---|
|81200258|7C0802A6|mflr r0: r0=SPR8, the incoming LR|
|8120025C|9421FFF8|stwu r1,-8(r1): store old r1 at old r1-8; update r1 only if no DSI|
|81200260|9001000C|stw r0,12(r1): store r0 at the updated r1+12|

`tools/agent_apploader_entry_42.py` independently checks these words,
the complete app/boot/BI2 SHA gates and14 exact original source pins.
Its build/agent-apploader-entry-42.json report records all source hashes.
This checker contains three explicit consequences, no generic opcode
dispatcher, interpreter, timing getter or observed-count input. It reports
late original stack bytes separately and returns ownershipUNKNOWN.
Its eight in-memory negative gates reject mutations of each Entry word,
truncation, each complete original input and the BS2 source pin. They do not
modify original files and are separate from the31 elapsed-work gates.

Key original source SHA pins:

| Source | SHA256 |
|---|---|
|Boot_BS2Emu.cpp|8c204a36bd7fe7171d6c8e03e7f07739c11297e483dd6180c6ff7391973ec06d|
|Boot.cpp|2245b1ab41fa4ab092d26416016c05d52b5591c9035f2ed4b1e3ffe8d256daaa|
|Memmap.cpp|c8b5b5ea180c73f54736b6835e20d8027d0203cda7641349254fbce99bb2747f|
|Interpreter_LoadStore.cpp|dbfc463df0e83aa989e7728bec097d3d95f05ec2e0fa2a409291e3b985234553|
|Interpreter_SystemRegisters.cpp|698f153ad3a9e457c7d51f9a84358b86140c29b879f35dc39fe70412f23f8556|
|MMU.cpp|ffc04c07b33b2ff77f1d72f9d6e09dca65c042d94a970e0e570755103aef9d78|

## Actual input and memory producers

The SP branch is source/data-derived. VolumeGC::GetRegion at108 reads the
big-endian word at disc458, which is originalbi2.bin+18, value2. It is not
derived from the game ID's PAL-looking letter. VolumeDisc.cpp102..108
admits this region value. SConfig's Disc metadata visitor at375 assigns
volume.GetRegion to m_region; IsNTSC atEnums.cpp150..152 accepts only
NTSC_J/U/K. Non-Triforce PAL therefore selects the false arm at
Boot_BS2Emu.cpp338..347: r1=815EDCA8, r2=814B5B20, r13=814B4FC0.
The input SHA gates include the BI2 region producer, rather than merely
asserting a PAL-profile enum.

EmulatedBS2_GC first callsSetupMSR/HID/BAT at302..304, then
SetupGCMemory at306, then reads the disc ID at320 and assigns those
registers at343..347. RunApploader reads its header at156..158 and invokes
DVDRead at164, physical01200000, length1DE38. DVDRead atBoot.cpp335..344
reads disc bytes into a temporary buffer and callsMemoryManager::CopyToEmu;
Memmap.cpp663..675 obtains a bounded guest-RAM range and memcpy writes
the complete original app body/trailer. This is the code-byte producer.
Load failures/foreign patches need explicit rejection by a future native
constructor; a SHA match in a research counter does not perform this copy.

RunApploader at178..182 assigns r3=80003100, r4=80003104,
r5=80003108 and callsRunFunction with the raw entry. RunFunction at69..70
assigns PC=81200258 and LR=0 before the first SingleStep. This directly
produces LR0; LR0 is not an inferred ABI convention. The source-loop first
Advance occurs before mflr and requires the separately owned initial queue
and callback state. This audit does not certify registers/exceptions are
unchanged across an omitted or foreign callback.

Fresh RAM is produced by MemoryManager::Init callingClear at177;
Clear at611..618 memset zeros the allocated RAM. With real RAM01800000,
both physical015EDCA0 and015EDCAC are inside the cleared range. The
pre-entry app copy excludes both. The original-source GC memory setup's
low-memory stores and disc-ID read exclude both as well. A native first
frame must construct a bounded fresh RAM owner or carry a proven unchanged
range receipt through firstAdvance; setting
MemoryOwner::FreshClearNoRestoreOrForeignWrites does not create those bytes.
Prior restore, external writes or unproved callback writes remainUNKNOWN.

SetupMSR sets DR/IR/FP/RI at82..85. SetupBAT at125..129 constructs
DBAT0U=80001FFF/DBAT0L=00000002 for the cached80000000 RAM alias;
815EDCA0/CAC translate to015EDCA0/CAC. The source's real RAM boundary
admits both. Physical byte visibility additionally requires the data-cache
profile: PowerPCManager::RefreshConfig at262 sets m_enable_dcache from
MAIN_ACCURATE_CPU_CACHE, whose default isfalse atMainSettings.cpp50.
MMU.cpp487..497 shows why HID0.DCE alone is insufficient: with enabled
emulator data cache and !wi, the store first affects dCache, while disabled
data cache copies the swapped big-endian bytes to physical RAM. Capture the
actual configuration/state and retain cache effects if enabled; do not
silently reinterpret an unproved cached store as a RAM store.

## Derived ordered first-three effects

Under the admitted no-DSI and source-owned register/mapping premises:

1.81200258 assigns r0=00000000 and leaves r1=815EDCA8.
2.8120025C computes EA815EDCA0, writes big-endian815EDCA8 at that
  logical address (physical015EDCA0), then updates r1=815EDCA0.
3.81200260 computes EA815EDCAC and writes big-endian00000000
  (physical015EDCAC). This address is oldSP+4/newSP+12, not newSP+4.

The stwu source atInterpreter_LoadStore.cpp451..460 writes before checking
DSI and updating RA; stw at445..449 uses Helper_Get_EA. Helpers at21..28
perform u32 address arithmetic with signed16 displacement. In the general
fault case, RA update cannot be applied merely because the instruction was
counted. The source's actual exception state must be captured/owned before
admitting the no-DSI arm. CR, other GPRs, SPRs and unrelated stack bytes are
not derived by these three words and must be retained rather than zeroed.

## Handler/report temporal provenance

The syscall handler word4C000064 at80000C00 is emitted by
SetupGCMemory atBoot_BS2Emu.cpp281..282 through HostWrite, before app load
and Entry. The other initial DSI/FPU words are at279..280. The
CopyDefaultExceptionHandlers helper atBoot.cpp490..501 belongs to the
Executable boot arm (call at558), and is not this disc branch's producer.
The first three Entry words do not read/execute the syscall handler; it is
a later cache helper/return-thunk dependency. Their physical watch receipt
should retain its installed bytes without pretending it affects these stores.

The report stub is temporally different: RunApploader at189 writes BLR
4E800020 to81300000 and at190 installs the AppLoaderReport HLE patch,
**after Entry has returned** and beforeInit. AtEntry it is still fresh RAM
zero on the admitted branch, and there is no AppLoaderReport patch yet.
The app load interval does not reach81300000. Report bytes and HLE hook
registration are separate effects; BLR alone does not prove the hook exists.
The research work factory's report-word premise describes a later Init
dependency, not a valid installed-word assertion at81200258.

## What42d independently observes

Existing build/passive-timing-zero-42d.json's original_unmodified_entry
is PC80003154, **after all nineteen apploader callbacks**. It observes
r1=815EDCA8 and LR0. Its stack window starts815EDC98:

| Address | Original late DOL-entry word |
|---|---|
|815EDC98|00000000|
|815EDC9C|00000000|
|815EDCA0|815EDCA8|
|815EDCA4|812002A8|
|815EDCA8|00000000|
|815EDCAC|00000000|
|815EDCB0|00000000|
|815EDCB4|00000000|

These bytes corroborate the source's stack area and are compatible with the
derived stores. Every callback wrapper reuses the same SP frame and
RunFunction resets LR to0. The late bytes therefore cannot attribute either
store toEntry, prove its order or certify its initial values. CA4 is not
Entry's LR spill address; it records later nested Main state.

The42d event journal independently has boot-function-enter81200258,
step-enter81200258, firstAdvance and step-reset nextPC8120025C. The next
boot-steps record aggregates instructions from8120025C through the return
thunk. It contains no r0/r1/LR or physical-write bytes. The passive instruction
observer in build_timing_trace_oracle.py activates only atPC80003154, so
its timing trace does not include Entry's first three words. Its MMIO watch
covers SI/VI/PI/AI, excluding the stack, handler and report intervals. No
direct before/after Entry register/store capture is present in42d.

## Minimum decisive passive observation and falsification

Keep the continuous original SingleStep/Advance path and no early breakpoint
or state reseeding. Extend a copied observer only for original PCs
81200258/5C/60, plus next boundary81200264:

- Observe already-fetched original word, before/after r0/r1/LR, PC/NPC,
  MSR/exceptions, DR/BAT mapping, actual m_enable_dcache and retained CR.
  Do not fetch an additional opcode or execute a timing producer to log it.
- Add an exclusive physical-write log with watch ranges
  `015edc98:32,01200258:12,00000c00:4,01300000:4`. Existing
  OraclePhysicalWrites supports DOLPHIN_SI_WRITE_LOG,
  DOLPHIN_SI_WATCH_EXTRA andDOLPHIN_SI_WATCH_ONLY=1. Capture from memory
  initialization, not only afterEntry. This records the initialClear,
  header/body copy, handler write and ordered stack stores with writer PC,
  physical/logical/translated addresses, bytes, cache flag and source stack.
- Retain firstAdvance's before/after register/exceptions state alongside the
  separate event capsule proof, and bracket Entry exit→report installation→
  Init entry. A known physical range's unknown raw-pointer lease or cached
  write must keep ownershipUNKNOWN until its effects are closed.
- Verify original app load bytes and no stack write betweenClear andEntry
  other than admitted producers. Verify reportzero atEntry, BLR/HLE patch
  installed beforeInit, and syscallRFI unchanged across the first frame.

Then reconstruct just these owned producers and three statements in finite
C++ and compare exact state/store order to the independent original receipts.
Falsify region2→0 (SP branch changes), absent/bounded RAM owner, enabled
data cache, invalid BAT/DR/no-DSI premise, altered incoming LR, missing
SP update, storing LR atCA4 instead ofCAC, reversed stores and premature
report installation. These must reject or select the explicitly modeled arm;
equal final ticks/work do not override state disagreement. Entire app-memory
and handler/report production is not proved merely by31 count-module negative
gates; those31 passed gates and N2656493 remain intact and separately scoped.

Audit outcome: raw/source producer path is exact and actionable. Existing42d
captures do not supply the first-three state/store receipt. This is the
smallest missing observation before a finite native Entry-frame admission.
