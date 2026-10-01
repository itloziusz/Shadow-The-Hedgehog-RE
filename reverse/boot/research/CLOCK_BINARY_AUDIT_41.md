# Independent PAL clock binary audit, checkpoint 41

This is an independent **static** audit of the original read-only
`<read-only-PAL-main.dol>`, SHA256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
It does not advance connected native execution or assert runtime parity.
Checkpoint40's stop before `80379628` remains valid until producer and same-run
state evidence supplies the missing live inputs.

The tool `tools/agent_clock_binary.py` scans all **1,216,776 aligned text words**.
TB and direct branch decoding use independent raw bitfields. `gameplay/tools/ppc.py`
is used for readable display only; no `q.py`, symbol or dataflow cache is needed.
The DOL section mapper reads its actual header and refuses unmapped reads.
The pinned inventory and branch-sign/destination/reserved-bit falsifications pass
with `--check`. Those are scanner checks, not hardware or clock parity tests.

## Complete raw TB opcode inventory inside DOL text

There are exactly **11 read forms and 2 write forms**. Every row has primary
opcode31 and Rc0. Read XO371 uses TBR268/269; write XO467 uses SPR284/285.
The field number is `((word>>16)&31) | (((word>>11)&31)<<5)`, rather than the
unreversed ten-bit instruction slice.

| VA | File offset | Raw BE word | Independent effect |
|---|---|---|---|
|80376EBC|37077C|7CAC42E6|Read TBL into r5|
|80376EC0|370780|7CCC42E6|Read TBL into r6|
|80379628|372EE8|7C6D42E6|Read TBU into r3|
|8037962C|372EEC|7C8C42E6|Read TBL into r4|
|80379630|372EF0|7CAD42E6|Read TBU into r5|
|80379640|372F00|7C6C42E6|Read TBL into r3|
|8039F5BC|398E7C|7D4C42E6|Read TBL into r10|
|8039F5C0|398E80|7D6D42E6|Read TBU into r11|
|8039F75C|39901C|7F1C43A6|Write r24 to TBL through SPR284|
|8039F760|399020|7F3D43A6|Write r25 to TBU through SPR285|
|804035D4|3FCE94|7C6D42E6|Read TBU into r3|
|804035D8|3FCE98|7C8C42E6|Read TBL into r4|
|804035DC|3FCE9C|7C0D42E6|Read TBU into r0|

This scope establishes all these encoded forms in loaded **DOL text**, including
aligned words that might be unreachable or inline data. It does not include IPL,
apploader, subsequently loaded code, self-modified code or arbitrary indirect
aliases. It does not prove every dynamic timing consumer has been reached.

## Ordering, rollover and interrupt consequences

`80379628..3C` returns the low sample from `2C` and its matching first high sample
from `28` only when the second high sample at `30` equals it. `80379634:7C032800`
compares r3/r5; `80379638:4082FFF0` retries at28 on inequality. This is an
unbounded high/low/high retry loop. It does not mask interrupts itself, apply an
offset, scale a tick, or read any clock frequency. The caller at `80379648`
disables EE before calling it; disabling EE alone is not evidence that TB stops.

On a monotonically increasing TB, a single ordinary low-word rollover between
the two high reads causes a retry. A rollover entirely before or after the
sampling window leaves a consistent pair. Equal high samples alone cannot rule
out an intervening TB rewrite that restores the same high word, or an elapsed
full upper-word cycle. Those cases require an explicit producer/elapsed-time
contract. Raw code has no retry budget and supplies no fixed tick value.

The second stable loop at `804035D4..E8` has the same order but uses r0 for the
second high and compares r0/r3. Its complete public entry is **804035A8**:
signed count `[805DE7AC] <= 0`, or selector `[805DE7A8] == FFFFFFFF`, returns
the zero pair before any TB access. Other admitted inputs sample raw TB.
Native code that begins directly at35D4 would omit those gates.

`80379640..44` returns one TBL read. It has no retry or high word. The inline
poll at `80376EBC..CC` saves TBL once in r5, repeatedly reads TBL in r6, forms
`r7=(r6-r5) mod2^32` and retries while the **unsigned** delta is below `0x1124` (=4388 ticks).
The two low-only polling consumers at `80372340/48` and `803724A4/AC` instead
use **signed** comparisons against892 and2C after wrapped word subtraction.
These distinctions constrain later native polling and cannot be flattened into
an unlimited host duration comparison.

`80379648..A8` samples raw TB with EE disabled, then reads offset **low first**
at `800030DC`, **high second** at `800030D8`. `addc`/`adde` at78/7C produce
`(TB + offset) mod2^64`, including low carry and high carry-out in XER.CA.
The EE restore leaf uses its current MSR and saved old EE bit. The helper then
returns r3 high/r4 low. The first caller at `80370EA8/EAC` publishes low to
`805F1F54` before high to `805F1F50`, after EE restoration. An interrupt could
observe the intermediate pair if enabled and delivered. No atomic 64-bit store
or frozen interrupt/TB behavior follows from these instructions.

## DOL TB writers are conditional context restoration

The DOL is not entirely free of TB writes. `8039F570` sets r2=`805A4FB8`, saves
segment registers, then reads **low at8039F5BC before high at8039F5C0**. There
is no high/low/high retry on this path. `8039F614:BD4201E8` stores r10..r31;
the saved TB low/high words become `805A51A0/805A51A4`.

`8039F728` sets the same context base, reads bytes `80568D30/31`, and clears
both before testing the old first byte. If the old first byte is zero,
`8039F750:41820014` skips the TB writes. Otherwise it loads saved r24/r25
from context+1E8/+1EC and writes **TBL first, TBU second** at75C/760.
Intermediate state and the exact write-time tick progression are therefore
observable dependencies of the producer, rather than an atomic restore pair.

The flag's explicit DOL producer is `8039E998..EB04`, a register-transfer path.
It admits indices through60, calculates a context word interval beginning at
`805A4FB8+1A8+4*start`, and on its r7==0 arm checks whether the selected interval
overlaps context+1E8..+1EC. If so, `8039EA68:98C38D30` writes
`80568D30=1` before the transfer call `8039EAAC -> 8039B39C`. Thus a selected
transfer covering saved TB fields enables subsequent TB restoration. This is
an instruction/data-flow proof of the flag mechanism; transfer I/O and its
dynamic callers remain unvalidated. Direct save callers are `8039DB30`,
`8039F92C`, `8039F9C0`; the one direct restore caller is `8039DCC0`, followed
by architectural context restoration and `rfi` at `8039DCF4`.

The checkpoint40 debug2/3 target `8039F8E0` reaches the save call at8039F92C.
That route is already excluded by checkpoint40's precise pre-call stop. The
conditional writes above are not evidence of an ordinary boot TB initializer.

## Frequency and unit evidence

**PROVEN:** the DOL repeatedly uses `BE32[800000F8] >> 2` as the rate for these
TB consumers. **UNKNOWN from this audit:** the producer/value of800000F8,
its physical unit, the oscillator/bus source, the timebase enable/reset state
and the guest advancement rule. The below division/scaling cannot by itself
prove a numeric hardware frequency.

The tooling's conservative straight-block constant scan finds **41 accesses**
to800000F8, all loads; two accesses to800030D8/DC, both loads; no matching
access to800000FC and no matching stores to the four queried words. This scan
resets knowledge at every branch/call/return/unsupported operation and therefore
has intentionally incomplete alias/CFG coverage. It proves neither that the
words are never changed nor that their only producer must be external.
The whole-text immediate30D8/DC search finds only the two offset loads and
unrelated SDA byte accesses `80096B9C/80096F14`; no DOL text instruction
materializes those literal displacements for a store. All four queried
low-memory words are below the first DOL section, hence are not initialized by
file-backed DOL bytes.

The strongest independent unit evidence is:

| Addressed path | Exact binary consequence |
|---|---|
|80051414..5C|Sample raw TB; left-shift the pair by3 modulo2^64; signed divide by the zero-high pair `floor((F8>>2)/125000)`; return pair.|
|80051460..9C|Sample raw TB; signed divide by the zero-high pair `floor((F8>>2)/1000)`; return low quotient in r3.|
|804036A0..B4|Store high0 at805DE7B0 and low`F8>>2` at805DE7B4. Getter8040354C returns this pair. The guarded raw sampler's clients can therefore receive raw sample plus this rate.|
|80374750..68|Calculate `floor((F8>>2)/1000)*1000` using `mulhwu` with10624DD3 then shift6; compare elapsed full TB against that threshold.|
|80377214..34|Calculate `floor((F8>>2)/125000)*100`, then shift3; use resulting threshold for full adjusted-TB elapsed comparisons.|
|8037A644..68|Calculate `floor((F8>>2)/125000)*12`, then shift3; poll adjusted TB until elapsed comparison permits the next MMIO write.|
|8037A184..A8|Calculate `(F8>>2)*10 mod2^32`, pass as a zero-high duration pair to80371A60.|
|80379D9C..F8|Choose `(F8>>2)*20` or`*10 mod2^32`, pass duration to80371A60.|
|803B433C..6C|Convert raw TB by signed divide with `floor((F8>>2)/1000)`, then signed divide by100; preserve resulting low quotient+1 in per-channel low memory at800030C0+4*channel.|

The divider `803A2040..2174` explicitly checks the sign of each high word,
negates negative operands with low `subfic`/high `subfze`, computes an unsigned
quotient, and negates the quotient when signs differ. Thus replacing its use
with unsigned division would diverge after bit63 changes. Divider zero-input
behavior needs separate reconstruction before admitting unprovided/zero rate.

**Conditional interpretation, not a producer proof:** if F8 is a bus rate in
Hz and raw TB advances at one fourth that rate,80051414 is a microsecond
conversion and80051460 a millisecond conversion, subject to the actual integer
truncations and signed64 wrapping above. For a *separately proven* F8=162000000,
the resulting tick frequency would be40500000Hz; the two denominators would
be324 and40500. No host timer or default nominal frequency was admitted here.

## Exact direct-consumer inventory and remaining coverage

Raw I/B displacement scanning finds **39 BL sites** to80379628, **12 BL sites**
to80379640, **27 BL sites** to80379648, and **3 BL sites** to804035A8. The
raw stable sampler has one further non-link edge, its own retry. The exact
aligned word scan over **all file-backed DOL sections** finds no words equal
to the sampled instruction addresses or80379648. This does not exclude an
address formed by instructions, an unaligned pointer, dynamic target, or
transitive consumers of stored results.

Every direct site is preserved below. Descriptions are the visible local
consequence, not recovered function names or validated whole-function behavior.

| Calls to raw pair80379628 | Visible local consumer |
|---|---|
|80051420,8005146C|Scaled signed64 division described above.|
|803735C0|Retain high/low in r29/r28 during a larger exception/report path.|
|80374748,80374788|Save initial raw pair; subtract later pair; threshold and device polls.|
|803768D8|Store low/high to805F1FA4/805F1FA0 before an indirect interrupt handler call.|
|80379668|Read low-memory offsets; adjusted clock helper described above.|
|80380F10|Retain pair and shift by16 through803A22A4 before bit/word processing.|
|803825B0,803825F4,803826DC|Initial sample, elapsed full-pair arithmetic, repeated raw-pair deadline poll near MMIO+6C00.|
|803857B8,80385808,80385818,80385824,80385844,803858C0|Six low-then-high timestamp pairs at r31+4A50,+4A58,+4A60,+4A68,+4A70,+4A78 around runtime operations and an indirect callback.|
|80385968,80385A38|Store pair at805F2250/54; later take low-word elapsed delta then shift2 before80385798.|
|80391934|Retain pair during larger transfer/device path; complete downstream arithmetic remains unvalidated.|
|8039261C,80392D54,803935DC|Signed64 division; store low quotient to object+28. Two visible paths derive a rate fromF8; complete object behavior remains unvalidated.|
|803937B0,803937CC|Store pair to805F2310/14; further buffer/halfword processing path remains unvalidated.|
|8039614C,80396164,803961D0,803961E8,80396214,80396228,803962B8,803962D0,8039633C,80396354,80396380,80396394|Six initial/later sample pairs; full64 elapsed comparisons against raw thresholds8,32,5,8,32,5(hex).|
|803B4354|Frequency-derived quotient and per-channel deadline state described above.|
|8043F574|Store low/high through `[805F24AC]` pointer at object+1C/+18.|

| Calls to low80379640 | Visible local consumer |
|---|---|
|80372340,80372348|Signed wrapped-low delta poll against892.|
|803724A4,803724AC|Signed wrapped-low delta poll against2C.|
|8038E500,8038E530,8038E5E4|Seed/reseed32-bit state at805EECD0; multiply by41C64E6D and add3039; shifted values feed subsequent decisions. Host timing substitution would affect these non-duration outcomes.|
|803CDD9C,803CDDDC,803CE494,803CE4D4|Unsigned low-only division by frequency-derived ticks/ms; compare modular quotient elapsed against7D0 around indirect callbacks.|
|8040BC2C|Store single raw low word into object+4 and clear object+0.|

| Calls to adjusted80379648 | Visible local consumer scope |
|---|---|
|80370EA4|First OS clock stores at805F1F54 then805F1F50.|
|80371858,80371930,803719E0,80371A94,80371B48,80371C04,80371C4C,80371D3C|Alarm/deadline family; helper80371A60 takes caller-provided duration pairs and callback addresses. Scheduling, decrementer and interrupt delivery remain unvalidated.|
|80377208,80377240,80377304|Stored adjusted-pair elapsed comparisons aroundCC003000 reset/button state and indirect callback.|
|80379964,80379A18,80379D50,8037A020,8037A63C,8037A670,8037A6A8|Device-state timestamps, elapsed thresholds andCC006000 MMIO paths; final raw pointer/state meanings remain unvalidated.|
|803B5AF4,803B5D38,803B5E28,803B6AA8,803B6BE8,803B6E68,803B6EB0,803B6F88|Per-channel adjusted timestamps, rate-derived elapsed limits and alarm/callback setup. Complete serial/device behavior remains unvalidated.|

The three direct guarded-sampler callers are `803EEAE8`, `803FFA08`, and
`803FFAD8`. `803EEAE8` writes the sample pair to stack+20/+24, then calls
the rate getter8040354C. The other two branches preserve a pair through larger
control flow. All transitive uses, pointer aliases, callback target sets and
loaded-module consumers are **UNKNOWN** until separately traced/reconstructed.
This inventory must not be relabelled “every downstream consumer proven.”

## Minimal next experiments and falsifiers

1. Observe800000F8/FC and800030D8/DC before entry and at their actual consumer
   reads; trace their initialization in the selected reference backend/IPL/
   apploader path. DOL bytes cannot initialize these words. An all-zero snapshot
   is insufficient to prove the writer or a retail reset value.
2. At80379628/2C/30 capture ordered architectural full TB and GPR effects plus
   the backend tick/cycle scheduler state. Vary the initial TB low word so the
   first high and second high straddle a rollover; retain every failed attempt.
   Vary host delays between debugging operations to falsify a host-wall-clock
   dependence. A raw instruction count alone is insufficient when the backend
   advances by scheduler blocks.
3. Perturb both offset words **before entry**, then check low carry/high
   wrapping, architectural XER.CA, EE restore, stack unwind and the caller's two
   separate writes. Retry input is not a substitute for measuring the producer.
4. Admit the captured producer rule in C++ before timing substitutions; then
   test low-only polling, signed/unsigned quotients and random-seed consumers.
   A clock-result-only equality test misses their distinct observable effects.

Reproduce the raw audit:

```powershell
python -B reverse/boot/tools/agent_clock_binary.py <read-only-PAL-main.dol> --check
python -B reverse/boot/tools/agent_clock_binary.py <read-only-PAL-main.dol> --lowmem
python -B reverse/boot/tools/agent_clock_binary.py <read-only-PAL-main.dol> --callers 80379628 --caller-context 8
```

No GekkoForge execution, native timing replacement, connected parity assertion,
full-repository regression assertion or gameplay regression assertion is made
by this static sub-audit. Those mandatory stages belong to the coordinator's
connected reconstruction and must run after the missing producer is validated.
