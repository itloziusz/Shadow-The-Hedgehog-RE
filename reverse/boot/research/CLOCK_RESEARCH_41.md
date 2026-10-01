# Checkpoint 41 research: exact TB semantics; native producer still unresolved

Target: Shadow the Hedgehog, GameCube PAL GUPP8P. Binary authority is the
read-only DOL, SHA256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
Read this together with `BINARY_CLOCK_PREFIX_41.md`, `CLOCK_BINARY_AUDIT_41.md`,
`CLOCK_SOURCE_AUDIT_41.md` and `CLOCK_ADVERSARIAL_41.md`.

## Actual progress and boundary

| Scope | Last established state | First unresolved dependency |
|---|---|---|
| Connected native production | Checkpoint40 from80003154, unchanged sync/L2/handler/CRT/BI2 and OS frames, **before80379628: mftbu r3** | Native elapsed CPU-domain time/event/epoch provider; no host timer is admitted |
| Executed research projection | Four full-entry profiles reproduce the clock sampler, offset arithmetic, EE restoration, stack return, timestamp stores and next OS frame | **Before80373AC4: lwz r4,30F0(r5)**; source clock schedule is still an explicit research input |
| Reference continuous sampler | Unchanged original sampler through8037963C, without intermediate TB breakpoints | Its block-retirement schedule differs from SingleStep; not admitted by the projection |

**PROVEN conditional evidence**, not connected production advancement. The
research projection calls the existing native prefix and consumes producer
inputs rather than expected TB/GPR results. No interpreter, host clock,
per-read replacement, forced branch or injected object was introduced.
The recognizer database frontier remains checkpoint40. Unknowns and earlier
declines remain active; no first-frame, retail timing or pixel claim is made.

## Raw code and flow

`BINARY_CLOCK_PREFIX_41.md` records71 exact instruction words, file offsets,
encoding fields, independent/Capstone decodes,12 direct branches and4 checked
SPR encodings. Every word is reread and gated by the C++ projection and CTest;
71 single-word mutations decline. Some rows are unexecuted successor/poll
words, not claims of implemented paths.

```text
80379628 TBU -> 8037962C TBL -> 80379630 TBU
   ^                              |
   |                   80379634 signed cmpw + XER.SO
   +------ unequal 80379638 -------+
                    equal -> 8037963C return
 ->8037966C..7C offset loads/addc/adde
 ->80376144..64 restore prior EE using current MSR
 ->80379688..A8 saved registers, SP/LR and return
 ->80370EA8 low store; 80370EAC high store
 ->8037611C EE disable; 80370EB4 object-relative address
 ->80373AB4..C0 next frame
 ->STOP BEFORE80373AC4, pointer producer800030F0
```

**PROVEN ISA**, `80379628/2C/30`: XO371, TBR269/268/269, Rc0. Each read samples
the live counter; it is not a memory fence or serializer. The first high and
low pair returns only when the second high agrees. Equality alone does not
exclude an intervening rewrite/full upper-word cycle; bounded elapsed time
and no rebase are explicit projection conditions. CR0 uses signed high-word
comparison and XER.SO; the retry tests only equality.

## Production of the reference clock

**PROVEN for the pinned private reference**, `SystemTimers.cpp207..213`,
`CoreTiming.cpp247..256`, `Interpreter_SystemRegisters.cpp237..269`:

```cpp
// Unsigned64 subtraction wraps BEFORE division; addition wraps AFTER it.
uint64_t CounterTimeBaseAt(uint64_t origin, uint64_t origin_cycles,
                          uint64_t current_cycles) {
    return origin + (current_cycles - origin_cycles) / 12u;
}
```

The source getter is not the cached architectural TU/TL pair. Executing either
TB read refreshes the whole cached pair from this equation, then copies the
selected half. Pure observation of cycles, origin and cached pair does not
sample or advance the clock. No read increments TB by itself.

**PROVEN reference units**, `SystemTimers.h32..41` and
`SystemTimers.cpp233..241`: GC frequency486000000 CPU-domain cycles/second,
ratio12, nominal TB40500000 ticks/second. Lowmem800000F8=162000000 busHz,
800000FC=486000000 CPUHz in the original startup captures; binary consumers
use `BE32[800000F8] >> 2`. TBL carries into TBU every2^32 ticks (about106.048575
seconds in this profile); full TB addition wraps modulo2^64.

**PROVEN reference origins**, `SystemTimers.cpp268..275`:
initial RTC seconds times40500000, with an independently sampled cycle origin.
`Boot_BS2Emu.cpp52..60` separately samples RTC and writes an unsigned64 offset
at800030D8/DC, high then low. These two samples can differ; an original capture
observed a one-second difference. Do not equate them, subtract an assumed
boot time, or assume zero. RTC default/local-time, custom RTC, movie and
netplay producers are separately documented in `CLOCK_SOURCE_AUDIT_41.md`.

**PROVEN conditional writes**, original8039F75C/760: the debugger restore can
write TBL then TBU and rebase the source. Its guard80568D30 is set when debug
register transfers modify the saved TB fields805A51A0/A4. These are not
ordinary startup writes. They retain the opposite cached half, not a freshly
sampled counter, and must not be collapsed into an assumed atomic write.

**PROVEN manual semantics / UNKNOWN retail state**: IBM Gekko v1.2 specifies
busclk/4 and distinguishes sleep (TB disabled) from doze/nap. Nominal162MHz
bus implies40.5MHz while that bus/power state is established. Actual retail
startup origin, phase, enable state and instruction/device timing were not
observed. The primary hardware sources and exact scope are linked in the
source audit. Generic750 TBEN details do not prove a Gekko pin state.

## Ordering: an observed falsification

**PROVEN reference SingleStep contract**, `Interpreter.cpp204..218`: Advance,
execute the instruction, then set slice_length1/downcount0. The post-step
getter includes the next pending cycle. At the original zero-profile frontier
cycles=2B30DF, the three reads sample **DF, E0, E1**, while post-read checkpoints
show **E0, E1, E2**. Sampling after increment is wrong and changes the low tick.
Tests include phase11 and phase10 boundaries to prevent this regression.

**PROVEN reference continuous contract**, `Interpreter.cpp257..285`: source
cost is accumulated within a block and debited at block end. The separate
continuous capture returns lowD5A92427 at8037963C, while the SingleStep schedule
with the same epoch/frontier inputs predictsD5A92428. At the return CP the
cycle getter has already advanced to2B30E4. The source explains this difference;
three separate stepping costs are not a substitute for the continuous model.
`validate_clock_research.py` explicitly rejects that capture as a SingleStep
projection input. Host pauses of250ms produced identical getter banks, proving
pause in this reference freezes the source; this is not a retail power-state
experiment.

## Reconstructed externally visible effects

**PROVEN for admitted profiles**,80379670/74: read low offset800030DC then high
800030D8. `80379678 addc` creates low sum and CA; `8037967C adde` consumes CA,
creates high sum and replaces CA. Both preserve other XER bits, with no OE/Rc.
`AddClockOffset` preserves the two separate architectural carry states;9,604
independent word/carry cases cover signed boundaries and wrap.

**PROVEN**,80376144..64: restore only EE from the saved bit, retaining other
current MSR bits. CR0/SO and the return value reflect the leaf instructions.
`80379690..A8` reloads actual produced frame bytes and restores r29..31/SP/LR.
The stale clock frame is not copied as an invented pristine object.

**PROVEN stores**,80370EA8 ->805F1F54 low, then80370EAC ->805F1F50 high, using
r13. The pair is not an atomic64-bit store. Three prior CRT owners remain
coherent; all491272 current global bytes per profile are compared after these
stores. The earlier BI2/SDA bytes remain unchanged by this tail.

**PROVEN next consumer setup**,80370EB4: r3=r31+50=80586C90. At80373AB8 the
next saved LR80370EBC overwrites8060C5D4, after the old LR was reloaded. At
80373ABC the8-byte frame backchain overwrites8060C5C8, after the old r30 reload.
The next r5=80000000 is produced by80373AC0. No unprovided pointer is read.
Static successors compare80000000 unsigned with800030F0, then either copy28
bytes or clear one word; their live state/ownership remains UNKNOWN here.

## Complete bounded DOL consumer inventory

**PROVEN aligned loaded-text scope**: scan1216776 words;11 TB reads and2 TB
writes. Full VA/offset/raw-word inventory and every direct call site are in
`CLOCK_BINARY_AUDIT_41.md`. Direct calls:39 to stable80379628,12 to low-only
80379640,27 to adjusted80379648,3 to guarded804035A8. No aligned initial pointer
word to those exact entries was found. That does not exclude indirect calls,
computed pointers, external writes, runtime-loaded code or transitive consumers.

- **PROVEN**,80376EBC..CC: low elapsed delta modulo32, unsigned comparison
  against **0x1124=4388 ticks**, retry atC0 retaining the original start tick.
  At40.5MHz this is about108.346 microseconds, not1124 ticks or nanoseconds.
- **PROVEN**,80051414: signed64((TB<<3) modulo64) divided by
  floor((F8>>2)/125000), default divisor324. Magic431BDE83/shift47 implements
  divisor125000, not100000. Sign, truncation and wrap matter.
- **PROVEN**,80051460: signed64(TB) divided by floor((F8>>2)/1000), default40500;
  the returned low quotient is observably narrower than the full result.
- **PROVEN**,804035A8..E8: count805DE7AC and selector805DE7A8 gate the sample;
  zero count/selectorFFFFFFFF returns zero pair. Its rate pair805DE7B0/B4 is
  initialized to0 andF8>>2 at804036A0..B4. Starting at35D4 omits these gates.
- **PROVEN static dependencies**, caller tables in the binary audit: absolute
  RNG seeds, callback timestamps, signed conversions, OS/device deadlines,
  saved debugger state and audio-related guarded calls consume ticks.
  **UNKNOWN**: complete dynamic/transitive coverage and their runtime deltas.

These consumers prevent declaring arbitrary host units, a constant sample,
independent half reads or unspecified event progress equivalent.

## Multi-pass validation and perturbation

Reference observers add pure producer/exception getters in one copied GDB TU;
original instruction code/source and linked producer library remain read-only.
The optional, separate experiment oracle permits epoch low/high/commit controls
only at original entry80003154. Exact PC/packet width/hex/commit guards reject44
malformed packets without source/cache mutation; ordinary writer/getters are
unchanged. Each capture records all three ordered controls and immediate
readbacks. The validator independently reconstructs their effects and requires
an unchanged epoch through every later checkpoint. No midchain/per-read source
control is admitted.

Natural zero-profile frontier minus entry is154066 cycles=12838 ticks+phase10.
OriginlowFFFFCDD9 therefore crosses the rollover during the original three
reads. This upstream input chooses an adversarial clock phase; it is not a TB
output inserted at the frontier. Original branch and register instructions
execute and determine the retry.

| Executed profile | New CP | Raw state fields | Known stack bytes | Ordered stores | Current global bytes |
|---|---:|---:|---:|---:|---:|
| Original RTC and lowmem offset, L2 disabled |43|4988|4472|4|491272|
| Original RTC, offsetFFFFFFFFFFFFFFFF, debug4/finite source/L2 enabled |43|4988|4472|4|491272|
| Pre-entry epochFFFFFFFFFFFFCDD9, offsetFFFFFFFFFFFFFFFF |48|5568|4992|4|491272|
| Pre-entry epoch7FFFFFFFFFFFCDD9, offset0 |48|5568|4992|4|491272|
| **Total research tail** |**182**|**21112**|**18928**|**16**|**1965088**|

All previous connected checkpoints are rerun from80003154 in every profile.
No entry PC/GPR or intermediate expected branch result is supplied. The full
wrap failed compare is signedLT (FFFFFFFF versus0); the sign-boundary failed
compare is GT (7FFFFFFF versus80000000). Both take the original backward retry,
then replace the failed low sample with the consistent sample.

Independent adversarial replay imports no coordinator validator. Fresh output
matches the stored compiled outputs; its separate reference ledgers check37674
field/byte comparisons. It rejects128 reference/provenance mutations and524
compiled state/source/owner/validity/order mutations. The root tools reject
unexplained exceptions, domain/rate/epoch changes, bad widths, source reversal,
missing validity, raw mutations, unsupported schedules and false native progress.

Final full MSVC Release **60/60 CTest** (all earlier56 plus4 clock gates);
standalone gameplay **27/27** and exact required output:
`RESULT: Dark mission cleared -> next stage index 6 (stg0200)`.
Recognizer35/35; timing tools7/7. The source guard, private reference and
independent compiled parity experiments are additional local research gates,
not silently skipped public tests. No pixel/runtime equivalence is claimed.

## Evidence pins (private artifacts stay in ignored build/)

| Artifact | SHA256 |
|---|---|
| Read-only timing/exception observer |0b1980eb9f8d02cf9ff5154bc445e0c603db4eb9718393784052a2f587b8bcb5|
| Controlled pre-entry source oracle |d752feab06d55e818427824799facfa6796c2e8880c7ea06c599a3cb376ada1d|
| Unchanged linked core.lib |eb0e5041b253072d735ddaac5fba9079ed2ca4916f26312845c706bd132b2939|
| Compiled research C++ projection |a1963b2cb6f3b7dcff697b41f295930c7b1c7f13102ebed009de57a3b7f65ff3|
| clock-exceptions-zero-41.json |bd276f74e5728a183413a65f10b8d00755a1841961f471a75950399736cceb0b|
| clock-exceptions-offset-41.json |504f18f41b85f0aa0b89df44044b22e76449ebf4874c88e997f8370a3bd7ff85|
| clock-wrap-41.json |1fff10703d43e80b1a781874656d6ed1080e2a8d0be0b7436ca8a21f982f1d2b|
| clock-signed-rollover-41.json |bf064fe077d55196c3604b6d1230e886ceb0bed8f5e47644ecd62f35e1eedd83|
| Separate continuous-clock capture |05163a1af073416d410a1e48e96797b80d704ccdd49321fed0959cd02abbaf42|

## Tooling feedback and genuine fixes

- Gekko decoder incorrectly treated invalid mftb selectors/Rc as a low read.
  It now fails closed. Recognizer keeps each read symbolic and distinct;
  unknown sources cannot become concrete pointers or branch inputs.
- Raw verification checks TBR/Rc and the next unsigned greater branch. The
  detector preserves the unsigned modulo32 low-tick deadline, every register
  dependency, immediate threshold and actual retry target. A full rescan finds
  one deadline, two stable samplers, two fills and one EE leaf, all UNKNOWN.
- Capture and state-diff tools preserve repeated PCs, producer phase, pending
  exceptions, current global owners and ordered low/high stores. A zero-over-zero
  store cannot test ordering by a vacuous swapped value; the independent
  validator uses a real premature-high corruption and explicit store records.
- The research C++ first sampled after the debugger cycle increment. Actual
  source/reference phase falsified that ordering; sampling now precedes the
  increment. Boundary tests and independent phase10/11 checks prevent recurrence.

## Genuine block and smallest next experiments

**UNKNOWN at80379628**: native prefix does not produce elapsed source time since
the RTC/rebase origin. Feeding an observed frontier cycle count and a debugger
schedule reproduces the experiment but does not solve production. Event queue,
exception delivery, block retirement, pre-entry loader time and physical gating
cannot be dropped without downstream proof. Original sync/L2/CRT/BI2 source is
untouched. Fetch-cache validity/replacement observations are explicitly outside
this architectural research projection; no cache metadata is fabricated.

Next: capture the same original clock entry under ordinary runloop scheduling,
with producer cycles/origin/exception flags and all due event boundaries; trace
GetTicks writes back to loader/CRT and queue producers. Identify which consumers
need absolute phase versus relative elapsed intervals, with boundary/rollover
experiments. Prove a native clock ownership/scheduling contract before supplying
host time or advancing the connected frontier. When that closes, trace800030F0
producer and the OS copy/clear ownership at80373AC4. Physical/retail, constructors,
RenderWare/audio/GX, first game frame and existing pixel gates remain required.
