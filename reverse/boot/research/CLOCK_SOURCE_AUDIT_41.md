# Clock source audit after checkpoint 40

Audit scope: the exact locally linked private HLE producer and its scheduling
contract, followed by the independent hardware specification. This audit does
not promote reference source semantics to retail execution parity. Original
oracle source and game inputs remain unchanged. Ordinary observations use pure
getters; the separately labelled experiment below changes only a private clock
source input before DOL entry. No native clock value is supplied here.

## Pinned local authority

Source root is
`<matching-oracle-source>/`.
The following paths are relative to that root. The read-only helper
`reverse/boot/tools/agent_clock_source.py` fails closed if any pinned bytes
change. It pins the linked library too. The private oracle manifest identifies
that same core.lib hash, so the copied GDB translation unit may add getters
without rebuilding the clock producer.

| Source path | SHA256 |
|---|---|
| Source/Core/Core/HW/SystemTimers.cpp | 578abca48d1ab5273d44bdf337431ff66817352d6862e6c28c79890969962bf7 |
| Source/Core/Core/HW/SystemTimers.h | f81c9aae61e58c7cf069630f162a66d87ea8342111e2c7db34e833c6d206c69d |
| Source/Core/Core/Boot/Boot_BS2Emu.cpp | 8c204a36bd7fe7171d6c8e03e7f07739c11297e483dd6180c6ff7391973ec06d |
| Source/Core/Core/PowerPC/Interpreter/Interpreter_SystemRegisters.cpp | 698f153ad3a9e457c7d51f9a84358b86140c29b879f35dc39fe70412f23f8556 |
| Source/Core/Core/PowerPC/Interpreter/Interpreter.cpp | 1993e7a3976be7aa7d276f1e0b25e04bbdcbe1a4555fac837a4794f6bc18128d |
| Source/Core/Core/CoreTiming.cpp | 742a2eb94f3db96d85f9ba1637ac1eec77e815b9e0bac1a7b0f574e096d6fac1 |
| Source/Core/Core/HW/EXI/EXI_DeviceIPL.cpp | 08bb42b7c1cac9343561053fde838716fbc11621817cdb11cfc5e99085a32aad |
| Source/Core/Core/HW/HW.cpp | 173015f302d96ba801efe7c47dbf9bcb572955d6ed116ff61d51215325c151a0 |
| Source/Core/Core/HW/CPU.cpp | d11e6d1a09586559ed871adb6b9a71a2c07fdf2714b7ee7ee1d1293eea59d7dd |
| Source/Core/Core/PowerPC/PowerPC.cpp | cd0de27b7b7359674723a18796fa1032ffd7728a1b177a77d6fefd9376463341 |
| Source/Core/Core/PowerPC/MMU.cpp | ffc04c07b33b2ff77f1d72f9d6e09dca65c042d94a970e0e570755103aef9d78 |
| Source/Core/Common/Timer.cpp | cc82523205b4dc13fc865e1d2469db67f5a0217c622d3ab2a14343887d7c5c94 |
| Source/Core/Core/OraclePhysicalWrites.cpp | baee215768579ae4173b7fbb00364d6193e26ee8eb4012fa891640e12eef1a9d |
| Source/Core/Core/OracleEventAudit.cpp | bf4665f53049fd4621aea6fa9502cf765902c357bf60062de6ba6ba14662b1c7 |

Linked library:
`<matching-oracle-build>/Source/Core/Core/Release/core.lib`,
SHA256 `eb0e5041b253072d735ddaac5fba9079ed2ca4916f26312845c706bd132b2939`.
Library write time is 2026-09-28 08:55:38 local. Relevant producer source write
times precede it; timestamps support provenance but are not a compiler proof.
Same-run independent exported inputs and actual instruction outputs are still
required to validate that this producer contract governs the captured executable.

## Exact HLE producer

`Interpreter_SystemRegisters.cpp:237..241` decodes TBR and delegates
`mftb` to `mfspr`. At `:266..269`, either TL or TU read calls
`SystemTimersManager::GetFakeTimeBase()`, writes the entire returned u64 to
the cached TL/TU pair, then copies the selected SPR to its destination GPR
(the final destination assignment follows the switch). No per-read increment
is performed. Both halves are refreshed even when only one half was requested.
`PowerPC.cpp:403..413` reads/writes the full pair with memcpy.

`SystemTimers.cpp:207..213` computes exactly:

```text
delta = u64(CoreTiming.GetTicks() - fake_TB_start_ticks)
TB = u64(fake_TB_start_value + floor(delta / 12))
TBU = u32(TB >> 32)
TBL = u32(TB)
```

Unsigned subtraction wraps before division; unsigned addition wraps afterward.
A native C++ implementation must preserve both operations and phase relative
to the epoch. Shifting cycles, dividing by a rounded period, signed subtraction,
or rounding the quotient produces a different result. Tick reads do not
advance CoreTiming. TBL carries into TBU every 2^32 TB ticks; the full pair
wraps modulo 2^64. TBL rollover spacing at 40.5MHz is about 106.048575 seconds.

`SystemTimers.h:32..41` sets TIMER_RATIO=12. `SystemTimers.cpp:233..241`
selects GC CPU frequency 486000000; Wii selects 729000000. GC TB units are
therefore 40500000 ticks/second, not nanoseconds or microseconds. CPU
overclock configuration affects cycles/downcount conversion rather than
changing this published nominal CPU frequency
(`CoreTiming.cpp:51..65`).

`CoreTiming.cpp:247..256` returns global_timer if the global timer is sane.
Inside a slice it returns global_timer + slice_length -
int(downcount * last_OC_factor_inverted). Conversion uses floating multiply
and truncating C++ conversion. `:376..418` Advance commits the elapsed slice,
dispatches all due events in queue order, chooses a new slice, and resets
downcount. Thus an epoch plus an unexplained elapsed cycle count is not yet a
complete clock reconstruction: the cycle producer must itself be admitted.

## Single stepping, pause, and instruction order

`Interpreter.cpp:204..218` advances CoreTiming first, executes one
instruction, then sets slice_length=1 and downcount=0. It explicitly ignores
the instruction timing value returned by SingleStepInner. Consecutive steps
therefore advance the nominal cycle producer once per step at ordinary
overclock 1, with the first step closing whatever slice existed at the stop.

Continuous interpreter execution differs: `Interpreter.cpp:257..285`
accumulates table instruction cycles within a block and retires them to
downcount only after m_end_block. Consecutive TBU/TBL/TBU reads inside the same
block observe the same cycle count in this model. A single-step capture can
exercise a rollover/retry that ordinary same-block interpreter execution
cannot expose. These are distinct schedules and must be labelled in evidence.

`HW/CPU.cpp:137..145` forces one SingleStep when continuing at a breakpoint,
then enters RunLoop. `:156..197` waits for a GDB step or resume command;
waiting does not call Advance. GDB `s` (`GDBStub.cpp:1024..1026`) leaves the
CPU stepping and causes the CPU loop to service one instruction. Host time
spent reading registers or waiting at a breakpoint does not advance this
cycle producer. The paused cached SPR pair also remains unchanged until a
guest TB read/write updates it.

`Interpreter_SystemRegisters.cpp:349..356` writes the requested cached
TL/TU half then calls TimeBaseSet. `SystemTimers.cpp:199..204` rebases the
epoch to GetTicks and the full cached pair. The opposite half comes from the
cache; this path does not first sample the live full TB. Ordered separate
mttbl/mttbu writes must therefore retain their observed epochs and cached
counterpart halves. There is no atomic-pair-write equivalence by assumption.

## RTC and low-memory offset production

The HLE initialization order is `HW/HW.cpp:34..53`: CoreTiming.Init,
SystemTimers.PreInit, devices, CPU.Init, SystemTimers.Init.
`HW/CPU.cpp:32..35` invokes PowerPC.Init, whose ResetRegisters
(`PowerPC.cpp:192..194`) zeroes the cached TB and calls TimeBaseSet.
SystemTimers.Init occurs afterward and at `SystemTimers.cpp:268..275` sets
the origin value to GC-epoch emulated seconds times CPU_frequency/12 and
origin cycles to the current GetTicks. Consequently entry TB origin zero
requires a demonstrated later producer such as an apploader mttb sequence.

`Boot_BS2Emu.cpp:52..60` separately obtains GC-epoch emulated seconds,
multiplies by 40500000ULL, and publishes the resulting u64 at 800030D8.
`MMU.cpp:775..779` implements HostWrite<u64> as two ordered 32-bit
writes: high to 800030D8, then low to 800030DC. The first OS clock call reads
low then high and adds them to the admitted TB sample. This HLE initializer
and the TB initializer perform separate RTC calls; they may straddle a second
boundary, so their equality cannot be inferred from one observed value.

`Boot_BS2Emu.cpp:276..277` publishes 800000F8=09A7EC80 (162000000 bus
Hz) and 800000FC=1CF7C580 (486000000 CPU Hz). These are source-produced boot
parameters. They do not prove that an arbitrary changed low-memory frequency
word changes the HLE counter frequency.

`EXI_DeviceIPL.h:27` defines GC_EPOCH=386D4380, the seconds from 1970 to
2000. `EXI_DeviceIPL.cpp:408..438` selects movie or netplay origin when
active and adds elapsed whole emulated seconds. Otherwise it calls local
civil time and subtracts the boot RTC offset. The final conversion is
u32(local_seconds)-epoch modulo 2^32. On this MSVC build,
`Common/Timer.cpp:69..75` uses current_zone and system_clock truncated to
seconds; this is local civil time rather than UTC seconds. Custom RTC
initialization computes local_now minus configured RTC value
(`SystemTimers.cpp:257..261`). That source sets initial RTC provenance; it
does not replace ongoing TB progression with host timing.

## Existing observation tooling and required proof

The linked source already contains optional observation hooks; none of them
must be enabled to execute the guest:

- `DOLPHIN_SI_WRITE_LOG` enables physical writes and TB markers. Existing
  markers include tb-frequency, tb-init-seconds, tb-origin-value,
  tb-origin-ticks, tb-rebase, tb-spr-write, tb-sample, tb-cache-write and
  clock-offset-seconds. Extra watched ranges use
  `DOLPHIN_SI_WATCH_EXTRA=800030d8:8,800000f8:8` and
  `DOLPHIN_SI_WATCH_ONLY=1`. These settings filter writes, not markers.
- `DOLPHIN_SI_EVENT_LOG` enables CoreTiming and RTC events.
  `DOLPHIN_EVENT_KINDS` filters comma-separated names with a trailing
  wildcard or minus exclusion; `DOLPHIN_EVENT_WINDOW` accepts
  start..stop boundaries, optionally kind@first-value-hex;
  `DOLPHIN_EVENT_LIMIT` caps a sink. A second sink is
  `DOLPHIN_EVENT_SINK2` with equivalent _KINDS/_WINDOW/_LIMIT settings.
  All sinks open with wx and cannot overwrite evidence. Invalid filters
  close the affected sink.
- Getters exporting cycles, epoch cycles, epoch value, CPU frequency,
  RTC offset and cached SPR pair can observe state without calling
  GetFakeTimeBase. A getter that calls the TB read producer would refresh the
  cache and add an unexecuted guest-clock consequence.

Mandatory evidence before native admission: capture the original epochs and
offset bytes before the first mftbu; replay every retired/stepped cycle
transition; establish unchanged source configuration; compare each actual
destination GPR and cached SPR pair after TB reads; repeat paused getter reads;
perturb only labelled pre-entry source controls to force phase/carry/wrap;
show that retries follow the compare and branch; preserve checkpoint40 owned
bytes and all prior regressions.

The independent helper accepts an optional JSON object with nonempty
`samples`; each sample requires sixteen hexadecimal digits for
`cycles`, `origin_cycles`, `origin_value`, `sampled_full_tb`, plus
`cpu_frequency="1cf7c580"`. It checks the unsigned equation using integer
arithmetic. Nine mathematical vectors cover subperiod phase, carry, full
wrap and unsigned delta underflow; six malformed or altered inputs decline.
Those synthetic vectors validate the checker, not the reference executable.

## Hardware specification and limits

The IBM Gekko manual specifies separate 32-bit TBL/TBU user reads, a 64-bit
time base, bus-clock/4 progression, and no other register changes for mftb.
Its doze and nap modes retain time-base progression; sleep disables it.
These facts constrain hardware reconstruction, while cycle scheduling and
startup values still require observation. [IBM Gekko User's Manual,
pp.2-6,2-66,12-132,10-3..4](https://doc.kodewerx.org/documents/gekko_user_manual.pdf).

The architecture's timer description defines an unsigned 64-bit incrementing
counter that wraps to zero after all ones; its frequency is system dependent.
[IBM PowerPC Architecture, May1993, chapter8](https://www.bitsavers.org/components/ibm/powerpc/SR28-5124-00_PowerPC_Architecture_First_Edition_May93.pdf).

For 32-bit implementations the programming environment prescribes the
high/low/high compare-and-retry sequence because low-word carry can intervene
between separate reads. Reading the counter does not change its incrementing
behavior. This supports the sampler logic, not a hardware instruction-cycle
schedule. [PowerPC Programming Environments, section2.2.1](https://www.nxp.com/docs/en/user-guide/MPCFPE_AD_R1.pdf).

Unresolved dependencies remain the actual same-run cycle schedule and origin
changes, original input provenance, asynchronous device/interrupt effects,
and every game-clock downstream consumer. The HLE code is conditional evidence
for this reference mode; retail IPL initialization and physical execution
timing are not established by it. A native owner must decline missing epochs,
unknown cycle transitions and unproven device states at their consumers.



## Same-run validation and the pending-cycle distinction

The independent audit read `build/clock-zero-41.json`, SHA256
`e8f8abec1f3d255c8021451bdb817589373f15d21950f013ca4b2cc637ff445a`.
Its new getter-only reference executable is
`d51b30c9f5220a7fcad4b46d94e21d0e963e9ac6061bc25b9a50a7bb615b287e`;
the producer core.lib pin above remains identical.

At DOL entry, the exported cycle producer is 000000000028D70D,
epoch cycles 0 and epoch value 007975C63A06FBE0. The loaded offset is
007975C63C70F700, exactly 40500000 larger than the TB epoch value.
This observed one-second difference falsifies the assumption that the two
independent HLE RTC initializer calls always agree.

| Executed instruction | BEFORE producer cycles | Expected and actual AFTER cached TB |
|---|---|---|
|80379628 mftbu r3|00000000002B30DF|007975C63A0A9547|
|8037962C mftb r4|00000000002B30E0|007975C63A0A9548|
|80379630 mftbu r5|00000000002B30E1|007975C63A0A9548|

The after-stop getter reports one additional pending cycle because
SingleStep has reset slice_length=1/downcount=0. It is therefore the BEFORE
getter cycle that produced the cached sample. At the first read, DF mod 12 = 11;
using the AFTER getter E0 would incorrectly produce ...9548 instead of ...9547.
The checker verifies this phase, destination GPRs, unchanged other GPRs,
MSR/CR/LR/CTR/architecturalXER/FPSCR, clock memory and globals, complete
BI2/L2stack invariance, and two identical paused getter observations. Eight
controlled in-memory capture mutations must decline. This validates the
bounded stepped sample producer; it does not reconstruct the elapsed cycles
of the whole prior boot chain.

The capture equation uses independently exported epoch/cycle inputs before
the TB instruction. The TB output under comparison is never fed back as an
epoch, tick source, or runtime provider.

The Gekko timing table 6-4 assigns mftb SRU latency 1 with no listed
serialization, while mttb is execution serialized. A latency entry does not
supply physical issue/retirement times. Its HID0 table has DOZE/NAP/SLEEP
mode controls; no dedicated TB freeze bit is shown. The manual's status
signal overview mentions time-base enable but its detailed signal subsections
do not specify TBEN. [IBM Gekko manual, pp6-29,2-9..10,7-18](https://doc.kodewerx.org/documents/gekko_user_manual.pdf).

The related MPC750 manual explicitly specifies TBEN: asserted permits
clocking, negated stops clocking. Applying that pin state to Gekko requires
Gekko-specific evidence. [MPC750 User's Manual,
section7.2.9.7.4](https://www.nxp.com/docs/en/reference-manual/MPC750UM.pdf).

Physical 40.5MHz follows 162MHz bus/4 only while the actual bus clock and
enable/power state are established. The HLE equation proves its nominal
40.5MHz from its own CPU-domain frequency and ratio. These proofs leave
physical phase, startup epoch and real instruction scheduling unresolved.

## Separate pre-entry rollover experiment

The builder's optional `--clock-perturbations` requires `--timing`.
Only that mode inserts three controls into the copied GDB register writer;
the original source and the linked clock producer remain unchanged. Its
manifest labels the binary a controlled pre-entry clock-source experiment.

| Private write ID | Packet | Effect at PC 80003154 |
|---|---|---|
|250 (fa)|`Pfa=<8 hex digits>`|Replace producer epoch low word, preserving high|
|251 (fb)|`Pfb=<8 hex digits>`|Replace producer epoch high word, preserving low|
|252 (fc)|`Pfc=00000001`|Set epoch cycles to GetTicks and mirror the full epoch value into cached TU/TL|

The copied parser checks the exact command length, separator, eight hexadecimal
digits and entry PC before any mutation. Commit rejects every value other than
one. Controls decline after entry; they cannot supply a midchain clock value.
After all three packets, capture the original cycle getter, committed epoch
cycles, producer value and cached pair before executing the first guest
instruction. The selected seed is a labelled producer input for falsification,
never an admitted runtime provider.

The experiment binary SHA256 is
`d752feab06d55e818427824799facfa6796c2e8880c7ea06c599a3cb376ada1d`;
copied GDB source SHA256 is
`d64632b5f5767f0b848381cd9b9840f5d38282655a4a354c41a8c353e469a606`.
Its original GDB source SHA256 is
`c2e3ae4869d41eb212bf9fb08940958f17939a4a2f19d5354ec4e386d74ba35e`;
the producer library keeps the pin above.

`agent_clock_source_builder.py` verifies source identity, unchanged ordinary
writer, identical pure getter body, and that removing the optional writer
recovers the ordinary timing source exactly. The ordinary timing source's
LF-normalized SHA256 remains
`50961d834f730e5dd837692c66be8cd87f402422dab6cea01cee947231cbc18b`.
Four malformed source/mode cases decline.

It compiles the exact generated parser and inserted writer against observable
fake source/cache state. Three valid packets preserve opposite halves and
commit one independently read cycle value; 44 invalid packets cover wrong PCs,
short and long values, bad separator, every invalid hex position and forbidden
commit values. All 44 decline without a clock read, epoch change or cache write.
The generated harness SHA256 is
`b8012598dd91874eb608baf0ecc7a141777b41f412260837d22cd2c400eb9f41`.
This guard check validates the experiment boundary. It is not an actual
reference rollover capture and cannot advance the native frontier.

The public helpers require explicit `--source-root` and `--library` inputs
(`--library` applies to the clock audit helper). Machine-specific source and
tool paths belong only in ignored local manifests or invocation arguments.
