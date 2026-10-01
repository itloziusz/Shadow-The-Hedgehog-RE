# Passive original apploader Entry receipts

2026-10-01. Original PAL GC HLE boot, reference evidence only. The original
source/library/game, old timing captures and the two initial Entry receipts
remain read-only. This closes the missing original first-three observation
identified inAPPLOADER_ENTRY_OWNER_42.md. It does not construct a native
RAM/event owner or admit a production clock. The finite conditional C++
reconstruction below remains disconnected from those live owners.

## Separate observer and admitted42b artifact

Only a copied original Interpreter translation unit is changed. It observes
live state before the original firstAdvance, after the existing opcode fetch,
and after ordinary execution insideSingleStepInner. The original
SingleStep/Advance order, instruction execution, PC update, cost and outer
slice reset remain intact. No new guest MMU read, opcode fetch, clock getter,
timing producer or writer is added. The stack/handler/report snapshots read
the already allocated RAM backing bytes directly. Existing physical writer
hooks carry separate source provenance and logical/translated addresses.

The original run stops naturally at its existing startup GDB stop80003154,
after all callbacks. There is no pre-entry breakpoint, RSP write, initial
register control, guest patch or clock reseeding. Original startup disc SHA
is a8a526ef7006a2d2a8d41f536ef30b208ff197c185c05e9b2276e6e93cb41a8e.

The admitted provenance-complete receipt is
build/agent-entry-receipt-original-42b.json and its.states/.writes/.events.tsv.
The earlier42 artifact is preserved too; its raw state/store receipt passed,
but its initial checker misread the writer-header syntax. No raw observation
was changed to repair the parser. A fresh42b oracle/run added the exact
build-time helper snapshot and stronger provenance checks.

| Bound artifact | SHA256 |
|---|---|
| Original Interpreter.cpp |1993e7a3976be7aa7d276f1e0b25e04bbdcbe1a4555fac837a4794f6bc18128d|
|42b copied Interpreter.cpp |bc6f477bf76d6c606cf8449877ff2e89d027f0be932b0a73dee479745c697b0a|
|42b executable |ef208d1a55fc635aa8ac1d47aff2b44e6b669df6d05c9ddeb67632d0cbdc2933|
|42b build-time helper snapshot |bb25e0ecd2eaee9641f8f1fc36a2d52aeba82d06d53af605af3855cea61c26ba|
| Both runs' identical eight state rows |dac1f8630d97c28e457cf3edd48de21a38bf64a34f89845f237936ff46d88d08|
|42b physical writer log |6eb7447591bd695cd42f8a7639f8a0dc87609824ea5bae44fdb8a60fabd814c7|
|42b event log |97c0f281bbe5993f68a6f1544d5c53cd41e1376b0f85299c532eff7a25e69006|

The checker rereads the copied source/object/executable, saved build helper,
compiler, compile/link response files, GDB object,14 producer source pins and
all inherited original link inputs. It also reproduces the exact copied TU
from the pinned original source. Build-time helper identity and later
validation-helper identity are separately named; a later checker change is
not claimed to have built an older executable.

## All eight independently observed state rows

| Stage | Original PC | Live PC/NPC | r0 | r1 | Global timer | Slice/downcount |
|---|---|---|---|---|---:|---:|
|beforeAdvance|81200258|81200258/00000000|00000000|815EDCA8|0|20000/0|
|fetched enter|81200258|81200258/8120025C|00000000|815EDCA8|20000|10888/10888|
|inner exit|81200258|8120025C/8120025C|00000000|815EDCA8|20000|10888/10888|
|fetched enter|8120025C|8120025C/81200260|00000000|815EDCA8|20001|10887/10887|
|inner exit|8120025C|81200260/81200260|00000000|815EDCA0|20001|10887/10887|
|fetched enter|81200260|81200260/81200264|00000000|815EDCA0|20002|10886/10886|
|inner exit|81200260|81200264/81200264|00000000|815EDCA0|20002|10886/10886|
|next fetched boundary|81200264|81200264/81200268|00000000|815EDCA0|20003|10885/10885|

All eight directly observe LR0, MSR2032, exceptions0, CR0,
HID00011C464/HID180000000/HID2E0000000 and effective m_enable_dcache0.
DBAT0U80001FFF/L00000002 and DBAT1UC0001FFF/L0000002A are live values,
not profile defaults. Real RAM01800000 and allocated RAM mask01FFFFFF are
also observed. HID0.DCE=1 alone would not prove physical byte visibility;
the actual emulator cache flag0 and original MMU write records establish it.

The inner-exit rows occur before the outerSingleStep writes slice1/downcount0.
They must not be called a post-SingleStep pending clock. The independent event
journal then observes step-reset(global20000,slice1,downcount0,next5C),
step-reset(global20001,slice1,downcount0,next60), and
step-reset(global20002,slice1,downcount0,next64). Thus all before/after phases
retain their actual meaning; no endpoint projection repairs an earlier row.

## Ordered physical production

Watch configuration is exclusively
015EDC98:32,01200258:12,00000C00:4,01300000:4, from initialization onward.
All watched stack bytes are directly observed zero before firstAdvance and
still zero after mflr. The physical log independently records:

| Writer sequence | Producer | Watched physical address | Bytes | Existing getter ticks |
|---:|---|---|---|---:|
|2|MemoryManager::Clear|015EDC98..B7|32 zero bytes|0|
|4|MemoryManager::Clear|00000C00|00000000|0|
|5|MemoryManager::Clear|01300000|00000000|0|
|12|GC SetupGCMemory HostWrite→MMU|00000C00|4C000064|0|
|15|DVDRead→CopyToEmu01200000 length122424|01200258..63|7C0802A69421FFF89001000C|0|
|17|PC8120025C MMU Write|015EDCA0|815EDCA8|20001|
|19|PC81200260 MMU Write|015EDCAC|00000000|20002|
|28|RunApploader host report-word installation|01300000|4E800020|118726|

Actual Clear size is33554432 allocated bytes; real RAM remains25165824.
The range check therefore uses observed real-size/mapping gates, not an
assumption that allocated size and real size coincide. Record17 also gives
logical815EDCA0→translated015EDCA0 and record19 logical815EDCAC→
translated015EDCAC. Both use actual cache flag0 and original
WriteToHardware<Write,false> atMMU.cpp496. A zero LR store is proved by its
write receipt even though before/after backing bytes are both zero.

The reader-only leases are distinct: CopyFromEmu atPC81200258 reads the
existing opcode-cache line01200240..5F and atPC81200260 reads01200260..7F.
They are source-pinned reads, not unexplained mutable RAM leases. Before the
first store, the checker rejects unexplained overlapping writers/leases;
only the exact Clear, handler installation, app copy and its known bounded
read/copy leases are admitted. This is a finite instrumented producer proof,
not a certificate that every foreign writer anywhere in the process is absent.

Report word is directly zero in all eight Entry rows. The sole observed
BLR installation is118726 = fresh20000+Entry98726, with hostPC0; the
source event brackets are Entry enter→Entry exitPC0→Init enter81200278.
The final original DOL snapshot reads BLR and syscallRFI. The live HLE hook
map is not part of this observer; its registration atBS2Emu190 remains a
separate source-proven effect, rather than being inferred from BLR bytes.

The source's PAL SP/LR assignments are now independently receipted at the
actual firstAdvance boundary. The app byte producer, handler and exact two
stack stores are directly receipted. Other GPRs and unrelated RAM are retained
unknown; saved registers must not be invented from ABI conventions.

Original DOL pending entry cycles remain2676493 (hex28D70D), agreeing with
the independent analytical2656493 work plus fresh20000. The count module's31
negative gates are unchanged and remain separate from this memory receipt.

## Precise next nested frame

The next fetched word81200264:48000139 calls8120039C and sets
LR81200268. The existing writer watch additionally records812003A0 saving
81200268 at815EDCA4, at ticks20005. Raw812003A4:9421FFD8 creates the
next40-byte frame at815EDC78, outside the old stack watch. The following
stmw r29 at812003A8 includes815EDC94, also outside that old watch. The
three argument spills at812003AC/B0/B4 are at815EDC80/C84/C88.

The independent review toolagent_owner_receipt_adversarial_42.py verifies
the exact42b transform inverse/link recipe,265 named original objects and
libraries plus the bound GDB object, all8 rows/224 fields and11 early
records. Its237 mutation rejections separate source inputs from captured
output injection. This passes bounded reference parity; it does not certify
complete foreign-writer or native-owner coverage.

## Separate nested call/prologue receipt

The new private prefix isbuild/agent-entry-nested-original-42.json and its
three TSV logs; oraclebuild/agent-entry-nested-oracle-42. Original42/42b
receipts remain immutable. Nested scope adds live r3/r4/r5/r29/r30/r31 and
64 physical stack bytes[015EDC78,015EDCB8). It observes beforeAdvance
at81200264, both fetched-enter/inner-exit for the call and seven prologue
words, then the fetched boundary812003B8. The original run continues to its
same natural DOL stop; the observer alone stops logging.

| Nested artifact | SHA256 |
|---|---|
| Copied Interpreter.cpp |0d9acf65e509a06f76d26bc93715c0b05fec7965bf20c2a8624eab9978d45a18|
| Executable |016b4a460f2b5cb7dd92bb4529e805641732af9cb16b7b73709b9a60c500366b|
| Build-time helper snapshot |4a1e0aae55b5d7a129551ecee05417d941aafcdce2017011b37d9f8baa1be8c8|
|18 state rows |9e7f8a6e9908f976dcc000553edc143f98a239b6563966d01ab1c655aa0a23fb|
| Physical writer log |947536df3e9282a56e54f929e99be79a18f17c4da4e74771836b6d9ba9b586dc|
| Event log |8f12ce266efb57a2bfc88e7176116c5c34ab89c3357a16d0d37343a4a8ab9ebf|

All18 rows retain the same observed mapping/cache/MSR/CR/HID/exception
gates. Source PCs and actual fetched words:

| PC | Word | Directly receipted effect |
|---|---|---|
|81200264|48000139|PC/NPC→8120039C, LR→81200268; r0 remains0|
|8120039C|7C0802A6|r0→81200268|
|812003A0|90010004|store nested incomingLR at old frameSP+4|
|812003A4|9421FFD8|store oldSP815EDCA0, then r1→815EDC78|
|812003A8|BFA1001C|three ordered writes of live r29/r30/r31 at newSP+1C/+20/+24|
|812003AC|90610008|store live r3 at newSP+8|
|812003B0|9081000C|store live r4 at newSP+C|
|812003B4|90A10010|store live r5 at newSP+10|
|812003B8|3C608120|next boundary fetched, before execution|

The args are directly observed80003100/80003104/80003108; saved registers
are directly observed0/0/0. The verifier uses the before-call live saved
registers to check their spills and preservation. It does not infer them
from an ABI or use their capture values to initialize a candidate.

| Writer sequence | Original PC | Logical address | Physical address | Actual value | Getter ticks |
|---:|---|---|---|---|---:|
|25|812003A0|815EDCA4|015EDCA4|81200268|20005|
|26|812003A4|815EDC78|015EDC78|815EDCA0|20006|
|27|812003A8|815EDC94|015EDC94|00000000|20007|
|28|812003A8|815EDC98|015EDC98|00000000|20007|
|29|812003A8|815EDC9C|015EDC9C|00000000|20007|
|30|812003AC|815EDC80|015EDC80|80003100|20008|
|31|812003B0|815EDC84|015EDC84|80003104|20009|
|32|812003B4|815EDC88|015EDC88|80003108|20010|

Before callAdvance, globals are20002/slice1/downcount0. Call enter/exit
is20003/10885/10885. Each subsequent ordinary unit advances the actual
global by1; the final boundary is20011/10877/10877. Actual PC/NPC before
call execution is81200264/81200268, then8120039C/8120039C. No checker
mistakes the pre-execution NPC for the branch target. Register snapshots
and all64 backing bytes agree before/after each effect; the three zero
stmw writes have independent ordered write receipts.

The expanded watch rejects unknown overlapping writers, cached writes or
leases through the last spill. Exact initialClear and source CopyToEmu
code production are present for both code watch intervals. Known opcode
cache-line CopyFromEmu reads remain distinct from a mutable memory lease.
Original DOL pending cycles still2676493. Independent nested review accepted18rows/612fields,8actual spills and628
mutation rejects; native ownership remains separate from this raw receipt.

## Earlier connected-owner proposal (historical)

This earlier integration sketch requires a native step owner that is not
implemented by the current finite research class below. It is not a present
clock ownership claim. Use a source-derived startup constructor and owned
live state, rather than
calling a constructor with any captured register, byte, tick or journal:

```cpp
struct FrameOwners {
  NativeStartupRegisters& cpu; // emitted by native fresh source producers
  NativeMem1Ranges& memory;    // emitted Clear/copy/mapping; known ranges
  NativeBootStepOwner& steps;  // exact original SingleStep/Advance policy
};
// Source/code/initialization gates must succeed before effects are consumed.
Result ProduceApploaderEntryPrologue(FrameOwners&, const OriginalBootInputs&);
```

Constructor obligations are concrete: sourcePowerPC::Init280→Reset295→
ResetRegisters146 fills the GPR array0; the GC SetupMSR/HID/BAT effects,
fresh allocatedMemoryClear, exact app CopyToEmu and owned firstAdvance
effects must actually be emitted/retained. PAL region2 comes from original
BI2+18 through VolumeGC/SConfig/IsNTSC; GC BS2 assigns SP/SDA at343..347.
RunApploader assigns r3/r4/r5=80003100/+4/+8; RunFunction assigns PC from
the raw header and LR0. Fresh r29/r30/r31 zeros are conditional on that
Reset producer plus the uninterrupted owned pre-entry path, not observed
values or ABI rules. An alternate/restore/native callback path cannot inherit
those zeros without its own producer proof. General unbound live registers
must remain in the supplied owned CPU state and be spilled as they stand.

For the admitted raw/no-DSI/no-alignment/no-LE/cache0 branch, explicit
statements may implement only these eleven words, with a unit-owner step
before each according to the original source policy:

```cpp
cpu.r0 = cpu.lr;                               //81200258
const auto entry_sp = cpu.r1;
memory.store_be32(entry_sp - 8, entry_sp);      //8120025C
cpu.r1 = entry_sp - 8;                         //after successful store
memory.store_be32(cpu.r1 + 12, cpu.r0);        //81200260
cpu.lr = 0x81200268; cpu.pc = 0x8120039C;       //81200264
cpu.r0 = cpu.lr;                               //8120039C
memory.store_be32(cpu.r1 + 4, cpu.r0);         //812003A0
const auto body_sp = cpu.r1;
memory.store_be32(body_sp - 40, body_sp);      //812003A4
cpu.r1 = body_sp - 40;                         //after successful store
memory.store_be32(cpu.r1 + 28, cpu.r29);       //812003A8, ascending order
memory.store_be32(cpu.r1 + 32, cpu.r30);
memory.store_be32(cpu.r1 + 36, cpu.r31);
memory.store_be32(cpu.r1 + 8, cpu.r3);         //812003AC
memory.store_be32(cpu.r1 + 12, cpu.r4);        //812003B0
memory.store_be32(cpu.r1 + 16, cpu.r5);        //812003B4
```

These are semantic statements with fixed gated source words, not a generic
PPC dispatch loop. Update PC/NPC in the original per-word order; leave outer
slice reset to the step owner. No observed200xx phase may be constructor
input. Memory.store_be32 must preserve big-endian byte order, numeric guest
addresses/mapping and actual write order. Reject an unknown range/cache or
fault premise before admitting this branch. If a fault arm is later modeled,
stw/stwu/stmw ordering and partial writes must follow original source;
stmw checks alignedEA/LE before writes, then ascending r29..31 with DSI
checks after each write atInterpreter_LoadStore.cpp297..316.

Only r0/r1/LR/PC/NPC and listed memory words change. Preserve r3/r4/r5,
r29/r30/r31, other GPRs, CR/XER/CTR/FPSCR, FPR/PS/GQR state, MSR/HID/BAT
state and unrelated bytes. Fields not observed by this receipt are preserved
owned state, never fabricated zero constructor arguments. The RAM/clock
owner must remain continuous through these effects; the receipts themselves
are validation outputs only.

## First successor requiring another owner/receipt

At812003B8 the next raw instruction is a deterministic lis, so it is not an
unresolved external input. The next new memory range is callback publication:
812003C0 loads the freshly produced saved r3 from815EDC80;
812003C4 stores81200278 through that pointer to80003100. Subsequent
812003D4/3E4 publish Main81200298 and Close812002B8 at80003104/08.
Those source operations are already raw-proven, but the current private
watch does not cover the destination cells and the finite frame owner above
does not claim their range. A native owner must admit those mapped low-memory
writes and their ensuing pointer consumers; it cannot advance on a stack-only
owner or a callback-table fixture. The smallest next passive addition is
physical00003100:12 with the saved-argument loads/publication boundaries.
This is a remaining proof/effect boundary, not an inherently unknowable SDK
input. The later copied return-thunk/cache/SC effects and first queued event
beyond this tiny slice remain their own concrete dependencies.


## Applied finite C++ reconstruction and final validation

The coordinator reviewed and applied build/apploader-entry-native-42.patch
(SHA256 0d97cb6fff51e48d0b2f0148d7112c4941e93d05cc19aeeea37ea61c8b5d955b),
removing only one extra EOF blank line per file. ApploaderEntryFrames.hpp,
ApploaderEntryFrames.cpp and test_apploader_entry_frames.cpp now implement
a finite conditional research reconstruction. They do not own a connected
clock, scheduler, generated MMU table, cache line, device or host worker.

### Concrete semantic contract

ApploaderEntryFramesResearch takes only original apploader.img, boot.bin
and bi2.bin bytes plus an explicit conditional research source profile.
Full size/SHA256 gates admit the exact original inputs. BI2+18 produces
region2/PAL. No captured register, memory, post-state, log, count, pending
phase or host time is a constructor argument. The constructor actually
creates and clears its private 64-byte stack, copies the original app body
to private code storage, installs the SC RFI and retains report bytes zero.
It emits the finite Reset/GC BS2/RunApploader/RunFunction CPU subset.

Six default-Unknown profile fields require explicit source selection:
register producer, fresh RAM/code producer, BAT producer, effective cache,
no-restore/foreign-writer premise and unchanged CPU/RAM through firstAdvance.
These are declared conditional research prerequisites, not live ownership
certificates. RequireNativeApploaderEntryFrameOwnership rejects NotAdvanced,
every partial callback stop, and the current FirstAdvanceComplete capsule.
Declared controls and GPU effect delivery do not certify live GPU/Movie/
configuration/achievement side states. No caller bool/enum grants admission.

ProduceFirstThree performs the three Entry words and stops before the call
at 81200264. ProduceNestedPrologue performs that call and seven callee words,
including ascending r29/r30/r31 stores, and stops before 812003B8. The
earlier eleven-word ordered semantic statements remain the exact contract.
The methods use the current owned LR/GPR values. stwu writes the old SP
first, then updates r1 on the admitted successful branch. EA arithmetic is
signed displacement modulo32; wrapped, misaligned, unknown or unowned
extents reject before the affected stage. Non-LE/no-DSI/direct-RAM is the
admitted branch; unsupported faults do not run guessed rollback.
Zero-valued stores remain explicit effects. This is fixed raw-gated semantic
code, with no opcode interpreter.

Only r0/r1/LR/PC/NPC and the ten listed memory words change. Other represented
CPU fields and unrelated stack bytes are retained. Explicitly named research
falsification setters perform new writes to the private owner; they do not
import original snapshots or grant native connection. Nonzero live saved
register/argument/LR probes verify that constants or ABI rules cannot replace
their producers. A future live bridge must inherit its own owned register
state and continuous pre-entry history.

The completed-source counter records one for stmw too, for eleven in total.
It is not a clock/scheduler producer. Enter follows the existing fetched-word
phase with NPC=PC+4; inner-exit follows UpdatePC. Outer SingleStep slice
reset belongs to a separate step owner. Final PC/NPC are both 812003B8,
matching original inner-exit3B4 before the next fetch. Original enter3B8
has NPC812003BC and is a different phase.

### Source-only retained fields versus dynamic observations

Dynamic receipts cover r0/r1/LR/PC/NPC/MSR/exceptions/CR, HID0/1/2, effective
dcache, RAM bounds/mask, DBAT0/1 and watched stack bytes. The nested receipt
also observes r3/r4/r5/r29/r30/r31. The following extra fields have source
producer proofs; they were not added to the original observer retrospectively:

| Field | Exact original producer/readback |
| --- | --- |
| GPR array, segment16, GQR8, CTR | PowerPC.cpp142..147 fills GPR/SR/SPR0. PowerPC.h355/358 aliases CTR/GQR to SPRs; Gekko.h834/884 identifies the indices. |
| Paired-single32x2 lanes | PowerPC.cpp144 fills PairedSingle{}. PowerPC.h71..104 explicitly initializes ps0/ps1 uint64 lanes0. |
| FPSCR | PowerPC.cpp173 assigns Hex0. |
| Architectural XER | PowerPC.cpp185 calls SetXER{}. PowerPC.h204..218 sets stringctrl/CA/SO/OV0 and reconstructs architectural0. |
| Architectural CR | PowerPC.cpp179..183 sets eight internal8000000000000001 fields. ConditionRegister.h GetField yields LT/SO/EQ/GT clear, hence architectural0; dynamic CR0 also agrees. |
| Reservation | PowerPC.cpp178..179 clears reserve and reserve_address. |
| IBAT0 | Boot_BS2Emu.cpp125..126 assigns80001FFF/00000002. |
| r2/r13 | BI2 region2/PAL selects BS2 values814B5B20/814B4FC0. |

The independent reviewer confirmed these reset/readback rules. Additional
pinned source headers are PowerPC.h
74beb71dd8f8246ace89a463914eb2290faa6b309deb8f22059b8ed92b5fd675,
ConditionRegister.h
72c2aa97de7a70ee6954dab3a5718602ca0ecfc592c7d0155eaf9f690b8edfb2,
and Gekko.h
fad9f1cbb274f63955b44680627bfab542e1940b24b6f6625ee1653436a6020c.
Timebase/DEC/other SPRs and all host/device side states remain outside this
finite reconstruction. Private C++ storage production does not prove the
complete original machine's RAM writer or host lifecycle ownership.

### Mandatory bounded proof stages

1. Reference: immutable original42b first-three and nested receipts; independent
   audits accepted8/224 and18/612 rows/fields, with237 and628 mutation rejects,
   exact source inverse/link265 binding and all eight nested physical spills.
2. Reconstruction: staged strict clang C++17 build and coordinator MSVC build
   pass. The candidate emits11 words and10 ordered stores, with49 negative
   C++ gates. The historical clang proposal remains separate from the applied
   source identities after the coordinator's EOF cleanup.
3. Falsification: six unprovided prerequisites; each raw word/header/region/
   extent; signed, wrapped, alignment and owned-range cases; cache/HID/BAT/
   fault/unknown-byte states; repeated or unsupported paths; partial and
   declared-only native connection. Nonzero live register and unrelated
   CR/XER/CTR/FPSCR/GPR/stack writes verify preservation, ordering and endian
   semantics.
4. Checkpoint parity: compare-native in agent_entry_receipt_42.py re-reads the
   original manifests/TUs/objects/libraries/recipes/logs and binds applied C++
   source plus actual MSVC executable. It reruns that executable with original
   fixture locations only, then compares22 enter/inner-exit snapshots and10
   physical stores against original receipts. No timing fields are compared.
   Recursive exact scalar types/dict keys/list arity reject False==0,
   True==1 and float==integer substitutions. All6066 value/type/key/arity/
   omission/mode/raw-input mutations reject through the reusable validator.
   Independent native review reports3410 mutation rejects.
5. Repository/gameplay: coordinator reports final MSVC66/66, standalone27/27
   and route/gameplay regression passed. Those broader gates are coordinator
   evidence, not a consequence inferred from the finite frame unit test.

Actual MSVC executable:
build/reverse/boot/Release/shadow_boot_entry_frames_tests.exe,
SHA07075d9cbe4440e9d78e5da10c0227bb37a893304489f1905cc0840702c9096b.
Original output and freshly rerun output are both
9df98f07bcfe8bd068b48c6a3375602db3899d21a05ac7a4d5848a27d2bb96b0.
Applied source identities:
header346f3a7f6b652b3a1c4d0004abe1c37eddc83ae923faf256bfb0a808f18512a1;
source851301dbb6a9c263f71410109088f42a537488908fe67f3d949c21575b364f8f;
testf2d53ba5120ed1e1d62126eab029e02a4f6587208018aaf34d018c318a29276f.
Executable hash alone is not an independent link/build proof; coordinator/
author compilation provenance remains separate.

### Preserved auditor and reusable comparator

The historical independent validator a1614565... was not archived before
its working file grew; its exact source is unrecoverable here. No reproducible
binding to that old source is claimed. The accepted immutable combined summary
e10970f2427e5275bee87506eb91d8054b57bf23eeb5eb076fdd0b70e1fd6b6a remains.
A fresh exact strengthened validator is preserved at
build/agent-entry-frame-comparison-42/archived_independent_receipt_auditor_42.py,
SHA9ae357277f05b7f1b5156a79617c0fa9ab6fb497f43f340db5902a4fafead53e.
Rerunning the archive reproduces both237/628 receipt audits and source22/
link265 bindings. Its saved archived_receipt_reaudit_42.json is
fa1facaa4e7ed8ce37572a5559ad44ee6658016e8a7abb3c1e189431293d7175.
The native comparator pins that archive and re-audit.

Native parity report:
build/agent-entry-frame-comparison-42/msvc-parity-42.json,
SHA30bca22536980afb5dc1b72c2adc2c2dbdff0a86e22892f288f1a67b824d8326.
Comparator helper SHA462e9034fc1ab9be6c4df915ef3db25d27fa7f4c221c6da32f29c441650a4b95.

Re-run compare-native with --native build/apploader-entry-msvc-42.json,
--entry build/agent-entry-receipt-original-42b.json,
--nested build/agent-entry-nested-original-42.json,
--auditor-summary build/owner-receipt-adversarial-42.json,
--archived-auditor and --reaudit-summary using the archive paths above,
--candidate-root ., --fixtures <original-read-only-sys-directory>,
--executable using the MSVC path above, --self-test and a fresh --output path.
The comparator preserves existing evidence and fails if output already exists.

### Next actual producer/effect

The finite methods stop before deterministic lis812003B8. The next new memory
range is callback publication:812003C4/3D4/3E4 write80003100/04/08, followed
by pointer consumers. The current stack owner has no callback-table lease.
A physical00003100:12 passive watch with saved-argument loads/publication
boundaries is the next small finite proof.

Earlier than Entry, the real connector still fails on firstAdvance side
states. The newer source-owned DTK logging path stops at GPUSleeper. Its
AllowSleep/BlockingLoop worker lifecycle and atomics require their actual
producer/effect proof. Neither a receipted Entry snapshot nor a Complete
enum replaces those owners. This finite success does not promote checkpoint40
or admit a production clock.
