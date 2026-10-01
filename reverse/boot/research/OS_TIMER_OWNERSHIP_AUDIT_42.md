# Independent next OS ownership and timer audit, checkpoint 42

Authority is the original read-only PAL DOL, SHA256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
`tools/agent_os_timer_42.py` independently scans all1,216,776 aligned text words
for raw direct edges, DEC SPR forms and lexical pointer candidates. Its bounded
word ledgers include file offsets, BE words and display-only local decoding.
The report is `build/agent-os-timer-42.json`; `--check` passes the pinned raw
ledger, complete DEC inventory, direct counts and dispatch provenance.

This is **static evidence**. It assumes neither a proven live clock nor a
native OS implementation, and promotes no connected frontier. Dynamic pointer
ownership, aliases, interrupts, device responses and callback targets remain
explicit dependencies. No GekkoForge execution occurs in this audit.

## The immediate pointer/copy boundary

After the first clock pair's low/high stores, the raw chain is:

```text
80370EB0:4800526D  call8037611C, disable EE again
80370EB4:387F0050  destination=r31+50=80586C90
80370EB8:48002BFD  call80373AB4, LR=80370EBC
80373AB4:7C0802A6  save actual incoming LR in r0
80373AB8:90010004  spill LR at oldSP+4
80373ABC:9421FFF8  eight-byte frame/back-chain
80373AC0:3CA08000  r5=80000000
80373AC4:808530F0  r4=P=BE32[800030F0]
80373AC8:7C052040  UNSIGNED comparison of r5 and r4
80373ACC:41810010  if80000000>P, branch80373ADC
```

The comparison permits **every numeric P>=80000000**, including80000000,
unaligned values, values outside RAM, uncached/MMIO-looking values and wrapped
28-byte extents. It does not validate a region, alignment, ownership or an
extent. The native boundary must preserve this branch predicate and decline at
the first unsupported memory consumer rather than invent guest RAM.

For P<80000000, `80373ADC:38000000` and `80373AE0:90030000` write only
destination word0. Bytes destination+4..+1B remain unchanged. Their earlier
CRT zeros, if actually produced and still owned, are separate provenance from
this helper. A 28-byte memset is not this branch's effect.

For P>=80000000, `80373AD0:38A0001C` supplies count28 and
`80373AD4:4BC91A21` calls **800054F4**. The source's complete28-byte extent
must be readable at the actual reads. The helper's return reloads LR from its
own saved slot and unwinds the eight-byte frame. There is one DOL direct caller
of80373AB4:80370EB8.

### Ordered byte copy and alias falsifier

800054F4 compares source r4 against destination r3 **unsigned**:

| Raw region | Admitted branch and exact operation |
|---|---|
|800054F4:7C041840 /800054F8:41800028|Source<destination selects descending branch80005520; otherwise ascending branch.|
|800054FC..5518|Initialize source-1 and destination-1; increment via `lbzu` at550C and `stbu` at5510. For count28, read/write offsets0 through27 in that order.|
|80005520..553C|Initialize source+28 and destination+28; decrement via `lbzu` at5530 and `stbu` at5534. For count28, read/write offsets27 through0 in that order.|

Each byte is read immediately before its byte store. In the ordinary single
address-space case this is memmove-like overlap handling. The source need not
be word aligned. r3 remains the original destination. At normal completion r5
is0, and CR0 is equality plus the retained SO from the final `addic.`; XER.CA
is1. Final r4/r6 are source+27/destination+27 on the ascending arm and the
original source/destination on the descending arm. The outer helper subsequently
overwrites r0 with its saved LR; it does not preserve the final copied byte.

**Do not canonicalize before choosing order.** If an independently validated
memory service aliases cached80586C90 with uncachedC0586C90, sourceC0586C8C
is numerically greater than destination80586C90 while its physical bytes begin
four bytes below the destination. The original therefore chooses ascending
accesses and can read bytes overwritten by earlier stores. Host memmove over
canonical physical buffers could instead choose descending order and diverge.
This is a concrete conditional falsifier; it does not assert that the current
reference backend or guest MSR/BAT mapping admits that alias.

A scalar P value supplies no ownership proof. Addresses800030F0,000030F0 and
C00030F0, or80586C90,00586C90 andC0586C90, cannot be treated as the same
storage until the selected backend and guest translation/cache state prove it.
The whole DOL aligned pointer-word scan finds none of these six constants.
Lexical30F0 candidates include three unrelated r13 accesses at80099D2C/74/7C;
the actual low-memory references below have explicit lis8000 producers.

## Later DOL production of800030F0

The pointer does have later DOL producers. `80373BB8..80374038` is a larger
transfer/reboot path; its one direct caller is803741B8. It is not in the first
OS startup call before80373AC4. Therefore discovering its stores does not prove
the first startup P came from them.

The path calls arena allocation80372234 with count1C/alignment1 at80373BF0,
then records these exact words at the returned object r31:

| Offset | Raw store | Proven source |
|---|---|---|
|+00|80373BFC:901F0000|1|
|+04|80373C04:931F0004|Entry r4 retained in r24|
|+08|80373E5C:937F0008|Entry r3 retained/conditionally recomputed in r27, on the later admitted branch|
|+0C|80373C08:939F000C|Entry r5 retained in r28|
|+10|80373C0C:93BF0010|Entry r6 retained in r29|
|+14|80373C10:933F0014|Entry r7 retained in r25|
|+18|80373C24:907F0018|Only entry-r7==0 arm: allocation of2000 bytes with alignment1|

The nonzero-r7 arm skips the +18 store. +08 is not stored in the initial
allocation prefix. No whole-object zeroing occurs there. These offsets give
an exact **seven-word extent**, but do not justify semantic names for all fields
or assume the entire extent has been initialized on every path.

On the later branch, the path loads a header into81200000 and obtains an
indirect target from the loaded header's +10 word at80373E68. The call at
80373E74 receives pointers to stack wordsC0/C4/C8 and supplies later callback
targets. Their runtime values are not DOL-static targets.

Both publication arms make a fresh count1C/alignment1 allocation, copy the
28-byte object through800054F4, and publish the result with explicit
`lis r4,8000` preceding:

| Producer | Source/allocation/copy and successor dependencies |
|---|---|
|80373EA4:936430F0|Allocate80373E80; copy80373E94; publish r27; call the target loaded from stackC0 at80373EB0; set arena low back to r27 through8037222C at80373EB8.|
|80373F6C:936430F0|After calls through stackC4 at80373F28 and stackC8 at80373F3C, allocate80373F4C; copy80373F60; publish r27; write7 toCC003024; disable EE; transfer to the target returned by the last indirect call through80373A00.|

80373A00 puts its input into LR after cache operations/sync/isync, then executes
blr at80373A24. It transfers to the supplied address; it cannot be treated as
an ordinary returning no-op. Another branch loads a different image at81300000,
uses8130DFEC/8130DFF0 as metadata, and transfers to81300000. These high-address
images and their relocation/callback targets require loaded-byte provenance.

Thus a preserved block from an earlier transfer is a plausible pointer source,
but this audit does not prove previous execution, physical retention across
reset, initial IPL publication, or reachability of the producer arms. A first
startup snapshot alone cannot identify its historical writer.

### Earliest producer in the selected fresh GC HLE profile

The private oracle's selected fresh GC path has a stronger source owner:
`Core.cpp585` callsHW::Init beforeCBoot::BootUp at644; `HW.cpp46` calls
MemoryManager::Init. `Memmap.cpp177` then callsClear after establishing its
physical RAM region at0. Clear at611..618 performs
`memset(m_ram,0,GetRamSize())`, including physical30F0..30F3. The source
observer records that whole `memory-clear` extent. Under the installed GC
DBAT0 mapping, the later800030F0 read addresses that initialized storage.
The source-derived zero and selected capture's zero at entry/80373AC4 agree.

**An explicit store was initially attributed to the wrong platform.**
Boot_BS2Emu.cpp502 does contain `Write_U32(0,30f0)`, but it is inside
SetupWiiMemory, which starts376. SetupGCMemory starts253 and does not contain
that store; the GC dispatcher callsSetupGCMemory thenRunApploader. The
explicit Wii store cannot establish GC ownership. The tool pins source,
checks those function boundaries and records the fresh GC owner asClear.

The original apploader's first1A98-byte header-code region has exactly one
lexical30F0/B0F0 occurrence:81201234:808530F0, a read. Its admitted normal
callback graph uses this read, the first1280 code bytes, the copied return
thunk and the one-word report/syscall handlers. It performs no30F0 write.
Scanning **the complete image including its loaded trailer** finds three
additional candidates:81205810:808530F0 and stores81205BF0/81205CB8:
936430F0. These match the SDK helper/transfer pattern at its high-address
loaded location. They are not called by the admitted callback graph; they
must not be silently erased from the static inventory or declared executed.

On this exact path the byte-writing extents exclude physical30F0: disc ID
ends at20, callback output words start3100, actual DOL sections start3100,
the apploader and its state/thunk/output buffers are at8120/812F/8130, PAL
stack writes remain above815EDC38, BSS starts8056FE00, and BI2/FST are at
817E54E0/817E74E0. Low-memory handoff fields includeF0 at800000F0,
**not**30F0; their actual writes are distinct. The source-produced zero
survives until the first DOL read when the validated connected DOL prefix
also excludes its later transfer producers. Reuse, savestate restoration,
external memory modification or alternate loaded callbacks revoke this
ownership. The corresponding passive physical-write ledger is the
falsification check; an entry snapshot alone is insufficient.

This closes the initial pointer's zero ownership for the stated fresh GC
HLE profile. It establishes neither a retail IPL value nor previous reboot
retention, and a controlled pre-entry pointer perturbation has its own
explicitly labelled owner rather than this fresh-clear provenance.

## Arena selection and later copied-block consumers

80372214 returns high H from805F1F70;8037221C returns low L from805EEBE8.
80372224/2C store those words. For inputs count N in r3 and alignment A in r4,
the allocation80372234..5C computes, with word wrapping:

```text
mask = ~(A-1)
B    = (L + (A-1)) & mask
L'   = (B + N + (A-1)) & mask
return B; store L'
```

It reads neither H nor a memory size and has no explicit power-of-two,
nonzero-alignment, overflow or H-bound check. Pointer ownership and safe
arithmetic extents must be tracked separately from faithfully reconstructing
these register/store consequences.

The first OS initializer selects L from80000030 or8060E600. Only when the
original low word is0, a debug-field pointer exists, and its unsigned word<2,
it replaces L with8060C600. H comes from80000034 or817A0000. Original low/high
boot-info words and the actual debug pointer/word are therefore needed. BI2
nonzero produces the debug pointer BI2+C and derives bytes800030E8/E9 from
BI2+C/+24; BI2 zero has another conditional fallback. Current owners must
include every actual word/byte, not an assumed coherent generic boot block.

The copied-block reader803771B8 tests `[80586C90+0]`. Nonzero returns
`[80586C90+4] | 80000000`. Zero readsCC003024, masks its bottom three bits
and shifts right3. The first OS initializer then examines the returned high
bit. Its arena-clear branches use copied-block **+0C and+10**:

| Conditional state at803711D4 onward | Exact requested clear ranges |
|---|---|
|Returned high bit0|Clear [L,H), using wrapped H-L as count.|
|Returned high bit1 and block+0C==0|Clear [L,H).|
|High bit1, block+0C!=0, L>=block+0C|No arena clear in this branch.|
|High bit1, L<block+0C and H<=block+0C|Clear [L,H).|
|High bit1, L<block+0C<H|Clear [L,block+0C); if H>block+10, also clear [block+10,H).|

These are raw comparisons and requests to8000540C, not a proof that either
endpoint is a valid arena pointer or that the two preserved endpoints form
an ordered interval. Invalid bounds or wrap must not become large native host
allocations/stores. If block+10<block+0C, requested clears can overlap; if
block+10<L, the second request can extend below the nominal arena low.

## Timer, DEC and context dispatch dependencies

There are exactly **three raw DEC SPR22 forms** in all DOL text:

| VA | Word | Effect |
|---|---|---|
|80370B0C|7C7603A6|Write r3 to DEC; next word is blr.|
|8039F71C|7FF602A6|Read DEC into r31 during debugger context save.|
|8039F7F8|7F5603A6|Write r26 to DEC in a conditional alternate debugger restore island.|

Reachability is separate from this complete raw inventory. From the ordinary
8039F728 entry, unconditional branch8039F7E8 goes to8039F81C, skipping the
DEC island8039F7EC..FC. The island itself tests the old flag80568D31 retained
in r6 and loads saved DEC from805A4FB8+278=805A5230 before the write. A live
code patch or alternate incoming edge is required to admit its execution;
this audit has not proved either. Likewise ordinary save8039F6F8 returns
before the adjacent DEC save island8039F6FC..724. Raw presence therefore
cannot be used to declare DEC save/restore reachable on the current prefix.

The setter80370B0C has **15 BL sites**, all inside alarm insertion/cancellation/
dispatch:71968/71990/7199C,71A14/71A3C/71A48,71B80/71BA8/71BB4,
71C84/71CAC/71CB8,71D74/71D9C/71DA8 (all addresses prefixed8037).
These instructions prove the programmed values, not DEC's live countdown rate,
underflow timing, exception eligibility, pending state or delivery ordering.
Those need the same backend/hardware source proof as TB.

The exception offset table at8056121C has15 words:
`100,200,300,400,500,600,700,800,900,C00,D00,F00,1300,1400,1700`.
Slot8 therefore maps to vector offset900. 80371348's installation loop builds
cached destinations80000000+offset, copies a template, flushes/invalidates
caches, and temporarily patches the template's instruction at80371688 with
the exception index before restoring its original word at803715A4.
The original file is unaffected; the live guest text is modified during this
operation. Raw file pinning alone cannot validate that temporal template state.

At8037156C the initializer publishes handler-table base80003000 to805F1F44,
then initializes15 slots to803716BC through803715F0.803717A8 queries slot8;
unless it already equals80371E14, it clears head/tail805F1F58/5C, installs
80371E14 in slot8, and registers another callback via80376D6C. A matching
preexisting slot bypasses those clears. A copied vector is not sufficient:
the actual slot/table words must be observed and retained as owned state.

The common vector template reads the physical context pointer from000000C0,
saves r3/r4/r5, CR/LR/CTR/XER, SRR0/SRR1, and dispatches through physical
3000+4*index after an rfi. It uses virtual current-context pointer800000D4 as
the handler argument.80372B80 publishes the virtual pointer atD4 and its
`&3FFFFFFF` value atC0, while consulting current FPU-context pointerD8 and
altering saved/current MSR.FP. This explicit virtual/physical pairing still
needs validated memory-service and MSR translation semantics.

80371E14 saves r0/r1/r2, r6..r31 and GQR1..7 in the supplied context and
branches to80371BE4. Its continuation creates a2F0-byte frame, reads adjusted
time, consults the alarm head, and uses80372C68 when no alarm or no due alarm
should execute. That helper ends with **rfi at80372D3C**, restoring the supplied
saved PC/MSR and machine registers; it does not return to the next instruction
as an ordinary C++ call.

### Alarm fields established by actual reads/stores

The shown code requires an extent of at least28(hex)=40 bytes; no complete
larger structure or unobserved trailing field is claimed.

| Offset(s) | Established local semantics |
|---|---|
|00|Callback pointer; zero is tested as inactive by cancellation; cleared when dequeued.|
|04|Create80371800 clears it; wider semantic role unproven here.|
|08/0C|Deadline high/low words; written low then high at803718D8/DC.|
|10/14|Previous/next pointers used by insertion, cancellation and dequeue.|
|18/1C|Signed64 period high/low, positive enabling periodic calculations.|
|20/24|Signed64 base/start pair used for periodic deadline calculation.|

80371800 creates an alarm by clearing only00/04.80371A60 saves EE, clears
period low/high1C/18, reads adjusted time, adds its duration pair r5/r6 modulo
2^64 and calls80371810 with the absolute deadline and callback r7; then restores
prior EE. A fully zeroed40-byte alarm is not the create helper's effect.

80371810 inserts by signed64 deadline order. It uses strict earlier-than,
so equal deadlines are scanned past and the new item follows existing equal
items. For positive period P with base S, it uses S when S>=now; otherwise
computes `S + (signed64((now-S) mod2^64)/P + 1)*P mod2^64` through the signed
divider803A2040. It consequently advances to the next period after now.
Period/base writers and complete reachable caller set require separate audit.

When the head changes, the code programs DEC from signed64 wrapped delta
`deadline-now`: delta<0 writes0;0<=delta<80000000 writes its low word;
delta>=80000000 writes7FFFFFFF. Exactly80000000 saturates. This rule occurs
in insertion, cancellation and interrupt dispatch. It is not a host sleep.

On a due alarm,80371BE4 dequeues one item, retains and clears its callback,
requeues a positive-period item **before** calling the callback, and programs
DEC for the next head. It saves/installs a temporary context, calls the retained
pointer at80371DD0 with alarm r3 and interrupted context r4, restores context,
resumes scheduling, and transfers back through80372C68. Recursive scheduling,
callback reentrancy and delivery chronology cannot be flattened into a single
timer return value. Exact context fields beyond the shown saves/restores remain
unprovided until captured; allocating a2F0 frame does not initialize its bytes.

## Other immediate indirect provenance

Timer initialization's call803717EC registers descriptor805612F0 through
80376D6C. Its original four words are80371E64,FFFFFFFF,0,0: callback,
unsigned sort key and two links, as established by the registration reads
and stores. Registration walks head805F1FB0, scanning past existing keys
less than or equal to the new key; it maintains next+08/previous+0C and
head/tail. Its keyFFFFFFFF places this descriptor after existing keys on the
admitted unsigned-order path. This is an ordered callback list, not another
alarm deadline.

80376DF0 walks this list and calls each descriptor+00 target at80376E3C with
its input flag r3. The static timer target80371E64 returns1; with flag zero
it performs no queue cancellation, while with nonzero flag it walks the
alarm queue, retains next+14 before calling8037DA50, and cancels through
80371AC8 only when that predicate returns zero. The list caller also calls
80377C6C and accumulates zero callback returns by unsigned priority group.
Six direct callers of80376DF0 are8037477C/B8 and80377054/7C/A4/CC. Actual
list pointers and targets remain runtime ownership requirements. Replacing
registration with a no-op would remove these downstream effects.

Thread-like initialization80378324 builds state at80587038, derives its main
context at80587450, sets controls/stack pointers, and publishes it at800000D8.
It calls80372D78 to clear two halfword context controls, then80372B80 to install
the current context. These are field writes; they do not clear an entire object.

At803783D0 it reads callback pointer805EEC10 and atD4 reads the old thread-like
pointer800000E4. The file-backed .sdata word805EEC10 is exactly80378320, a
one-word blr leaf. The runtime call at803783DC receives old pointer r3 and new
context r4, then publishes the new pointer at800000E4. A observed r12==80378320
would admit that leaf conditionally; a static default alone does not prove no
writer or earlier callback changed the target. Additional callback consumers
of the same SDA word occur at803788D0 and80378998. All alternate target values
are unresolved until their actual bytes/call effects are validated.

## Smallest actionable capture and falsification plan

1. Preserve the already validated prefix and unresolved clock gate. In the
   same original run, captureP at80373AC4, destination28 bytes before/after,
   and, only for P>=80000000, the complete owned source28 bytes and each actual
   copy load/store order. Checkpoints at3AC8,3AD4 or3ADC,3AE4 and70EBC retain
   branch, stack aliases, LR and return effects. Missing P stops at3AC4;
   unknown source stops at550C or5530 before the first byte read.
2. Use labelled **pre-entry** controls forP=0,7FFFFFFF,80000000, an explicitly
   owned loaded BI2 byte range and an unaligned location inside that range.
   Observe rather than assume rejected mapping/high-address/wrap behavior.
   If cached/uncached aliases are independently established, include the
   direction falsifier above. Distinct overlap bytes need an admitted producer;
   pre-entry BSS canaries will be cleared by CRT and cannot establish overlap
   correctness by themselves. Keep isolated helper experiments separately
   labelled from connected-prefix evidence.
3. Before arena reads, capture80000030/34, BI2 pointer/full bytes, debug pointer
   and word,800030E8/E9, currentL/H and the copied block. At803771B8 include
   actualCC003024 only when the flag arm consumes it. Falsify whole-block
   fallback clearing, wrong signedness, atomic pair publication and incorrect
   preserved intervals with raw/effect ledgers and controlled owner state.
4. For timer initialization capture vector900 bytes, handler slot80003020,
   head/tail, virtual/physical current-context pointers and context controls.
   For a later timer experiment capture each adjusted-TB read, DEC write and
   delivery event, old/new MSR, SRR0/1, the complete owned alarm/queue/context
   extents and actual callback target. Test equal deadlines, past deadline,
   delta7FFFFFFF/80000000, period0/negative/positive, callback requeue and saved
   context return. Guest debugger polling must not be substituted for CPU time.

The first pointer/copy slice can be validated without solving the entire timer
chain, provided the clock's predecessor gate and precise memory owners pass.
The subsequent six performance SPR writes, HID0 read/OR200/write and FPSCR
`mtfsb1 29` are separate machine-state gates at80370EBC..EF0. No first OS
return, device readiness, timer completion or gameplay/whole-repository parity
follows from this static audit.

```powershell
python -B reverse/boot/tools/agent_os_timer_42.py <original-sys>/main.dol --check
python -B reverse/boot/tools/agent_os_timer_42.py <original-sys>/main.dol --check --report build/agent-os-timer-42.json
```

## Separate extension: analytical HLE apploader step production

The original read-only apploader is122456 bytes, SHA256
`8a1fe9487b029b2518a6633b1e0eb9ec0922f5f5eb5afdd7eba1267a85f096fe`.
Its32-byte header has entry81200258, code size1A98 and trailer size1C3A0.
The first code word maps file+20 to81200000. This section is separate from
the DOL F0/alarm audit and does not promote the native clock gate.

`agent_os_timer_42.py --apploader PATH --source-root PATH` now produces a
**conditional analytical count**. It uses no PPC instruction execution,
register state, generic instruction dispatch, captured count input or event
log. It counts the explicitly admitted raw basic blocks and reduces bounded
copy/fill/cache/descriptor loops to arithmetic. The source and all original
inputs are read only. A native C++ reconstruction still requires its own
source-derived implementation, perturbation and connected parity gates.

### Callback return thunk explains the dominant count

Every callback wrapper81200258/278/298/2B8 has8 instructions. Each calls its
body, then puts812FFF80 into LR and transfers there. Entry body8120039C..484
copies original source81200208..253 to
812FFF80, exactly76 bytes, flushes it and invalidates its instruction cache.
The source endpoint81200254 is a nop excluded from the copied extent.

The copied return thunk is a concrete producer, not a zero-cost host return:

```text
81200208..22C: 10 fixed instructions
81200210:3CA00010         length=00100000 (one MiB)
81200218..22C            line alignment/count and mtctr
81200230:7C0037AC        icbi using r6
81200234:38C60020        addi r6,r6,20
81200238:4200FFF8        bdnz to230
8120023C..250: 6 fixed instructions, including actual savedLR and SP return
```

Thus every callback return has `10+3*(100000/20)+6=98320` executed
instructions. On the selected normal path15 DVD requests produce16 Main
calls; Entry, Init and Close make **19 thunk executions**, or1868080 steps.
These request counts come from the original state graph and ten populated
DOL section descriptors. The large text section is returned as one request;
state7/8 do not call the128KiB chunk helper. FST length18B20 is below20000,
so this selected path does not use state12. Host DVDRead performs the copy
outside SingleStep and contributes no guest instruction count here.

### Exact reduced loops and fixed branches

For an admitted, nonwrapped readable/writable byte extent, raw copier
812000F4..140 executes `4*N+9` instructions, including the final failed loop
test and blr. It picks ascending/descending order by unsigned guest addresses,
just as the DOL copier; this count does not remove its memory effects.

For positive N let `Q=ceil((N+(address&31))/32)`. Raw cache helper counts are:

| Helper | Steps including return / conditional exception handler |
|---|---:|
|81201164 dcbi|8+3Q|
|81201190 dcbf|10+3Q, including the installed one-word RFI after sc|
|812011C0 dcbst|10+3Q, including that RFI|
|812011F0 icbi|10+3Q|

N=0 returns after2 instructions. The GC HLE source installs word4C000064
at80000C00 before apploader entry. Interpreter sc transfers to physicalC00;
the next SingleStep executes that RFI and returns. Counting it as8 fetched
instructions would contradict both source and the per-function validation.
The report hook is HookType::Start at81300000, where the host installs blr;
its report runs inside that one guest step and ordinary blr still executes.

The fill wrapper81200000 has12 fixed instructions. For its zero, aligned,
N>=32 case the inner81200030..F0 yields:

```text
32 + 10*floor(N/32) + 3*((N>>2)&7)
   + (N&3 ? 2+3*(N&3) : 0)       // wrapper included
```

The additional nonzero-value replication and leading alignment bytes are
handled by the tool's separate arithmetic branches. For the exact DOL BSS
8056FE00+8C7EC, this produces179871 steps. The following cache flush has
53962 steps. Init's two smaller aligned fills of20 and100 bytes take42 and
112 steps respectively.

The footprint routine81200538 has221 steps for2 populated text and8 data
descriptors. Each descriptor's nonempty arm has14 instructions; empty has8.
The range predicate812005DC has319 steps for these extents when their ends
are at most the tested limit and they start below81100000: each populated
descriptor arm has23 instructions; empty has8.

**The decisive state6 branch is source-produced.** SetupGCMemory publishes
ConsoleType::LatestDevkit, whose exact source constant is10000006. The
apploader getter81200368 reads8000002C. Its high-nibble test at81200CDC/E0/E4
therefore selects81200D2C, performing both range checks with limits80700000
and81200000. The zero-high-nibble arm atCE8 performs one check and cannot
stand in for this branch. Both pass for the supplied header. State6 totals:

```text
44 selected fixed instructions
+221 footprint routine
+15 console-type getter
+2*319 range predicates
+179871 BSS fill
+53962 BSS flush
=234751 steps
```

The bound-loop failures select the error print and infinite halt at81201148;
they must fail closed, not retain the completed-path count. The FST path also
requires its selected base>=81700000. A hypothetical smaller memory size
can produce a symbolic FST address while failing this later loader condition.

### Derived totals, independent captured falsification, remaining premises

The produced counts are:

| Callback | Analytically derived steps |
|---|---:|
|Entry|98726|
|Init|98541|
|Main0..15|98382,98390,99200,98423,334052,553816,99234,98992,98521,98416,142288,130114,102520,102199,107942,98405|
|Close|98332|
|Sum|**2656493**|

The coordinator's separately captured per-RunFunction SingleStep totals
match every row. Those values were used to falsify the initial ledger, not
accepted as count inputs. The initial mismatch localized entirely to state6;
raw source then established the devkit branch and two explicit arithmetic
hand-count errors. No timing residual was chosen or added to fit the result.

The tool pins the existing Boot_BS2Emu/Interpreter/branch/HLE sources plus:

| Source | SHA256 |
|---|---|
|Source/Core/Core/Core.h|5bb9d817552da8f7dc04ae29dd9b29149edf6a6470d3db724d2f398b352390eb|
|Source/Core/Core/HLE/HLE.cpp|81f052fb94a0d3daebcb1d4fa956d71c0f7ba090de61ed4903999036189b45bc|
|Source/Core/Core/HLE/HLE_OS.cpp|8da9a89200e5f6c443c32bb7f872e0622d621bb289f9ffd183d5ead4c561e6e3|
|Source/Core/Core/HW/SI/SI.cpp|4dd9a3b22f3610d7ffe7704b6ee5b4142f29ad6e932c391edc094b22ac6abae7|
|Source/Core/Core/HW/SI/SI.h|00c23198d5541f76e010eb3119442bb4a37cc8c2d8db2ebff143956860783205|
|Source/Core/Core/HW/Memmap.cpp|c8b5b5ea180c73f54736b6835e20d8027d0203cda7641349254fbce99bb2747f|
|Source/Core/Core/HW/HW.cpp|173015f302d96ba801efe7c47dbf9bcb572955d6ed116ff61d51215325c151a0|
|Source/Core/Core/Core.cpp|8bf9e90fc56b5b4d7a1e42410ea2e2291f44b5d86fb4e982b3aa68912edf5944|

The selected path additionally requires fresh GC HLE24MiB RAM, original
unpatched disc inputs, initial800030F0<80000000, no unexpected exception/
hook/foreign mutation, and SI poll state retaining EN3=0. SI Init assigns
poll zero then X492 at bit16, so the apploader's port3 mask10 takes the
12-step absent arm and publishes a zero halfword at800030E4. The only later
SI poll assignment found is its MMIO write callback; this apploader path
does not perform that write. Current runtime ownership/lifecycle and event
callbacks must still prove those predicates for a native admission.

Small count falsifiers now exist: selecting the zero-high-nibble console arm
removes322 steps; taking the pointer-copy arm adds122 steps for its28 bytes;
changing cache-line alignment alters Q; a populated or empty descriptor
changes the corresponding branch count; changing SI EN3 opens additional
type/data reads. A wrong fallback count must not accidentally retain the
same final time merely because its clock phase modulo12 agrees.

```powershell
python -B reverse/boot/tools/agent_os_timer_42.py <original-sys>/main.dol --check --apploader <original-sys>/apploader.img --source-root <matching-oracle-source> --report build/agent-os-timer-42.json
```

The PASS raw audit and exact analytical count are an independent research
result. They establish no connected native boot, timing-event equivalence,
checkpoint promotion or whole-repository/gameplay regression by themselves.

### Portable C++17 elapsed-work candidate

The analytical path is now reconstructed as C++17 in
`include/shadow/boot/ApploaderElapsedWork.hpp` and
`src/ApploaderElapsedWork.cpp`. It produces work counts and does not execute
PPC, guest stores, callbacks or devices. It accepts no observed cycle total,
timing residual, step log, trace or host-clock argument. The final total is
an arithmetic sum of the independently derived callback counts.

Its four raw-byte gates are complete original apploader, boot.bin and BI2
SHA256 identities plus the first256 DOL bytes, whose digest is
`4a820e037e1d8d2118dea36c0e8f75a5c02caef13e897480c75778501cb29354`.
It also requires a validated BootImage and checks its entry instruction.
The caller must separately validate the complete DOL SHA identity; the work
model consumes its header descriptors rather than game body instructions.
The API rejects unknown or altered source initialization premises before
deriving a count. The explicit FreshGcHleApploaderSourceInitialization
factory **selects a conditional source profile**, not a runtime owner proof.

```cpp
const auto image = LoadValidatedFixture(original_dol);
const std::vector<std::uint8_t> header(original_dol.begin(),
                                      original_dol.begin() + 256);
const auto profile = FreshGcHleApploaderSourceInitialization();
const auto work = DeriveApploaderElapsedWork(image, original_apploader,
                                             original_boot, original_bi2,
                                             header, profile);
```

Fresh CoreTiming's20000-cycle commit and pending phase remain separate
scheduler-source arithmetic. Applying `20000 + work.total_steps` requires
that separate fresh-object/SingleStep proof; it does not prove native event
effects, physical memory ownership or eligibility to cross the clock gate.

`tests/test_apploader_elapsed_work.cpp` accepts the original main.dol path
and reads the three sibling originals without writing them. It compares all
19 callback totals and the helper breakdown against independent validation
outputs. Its31 negative gates reject altered copy/fill/thunk/dispatch/range/
SC/F0/trailer instruction bytes, descriptor/BSS/boot-header/BI2 changes,
truncated input and unsupported memory, console, SI, handler, report, patch
or exception premises. The source candidate contains no literal2656493;
that value appears only in the validation test.

The standalone C++17 build with clang++-O2/-Wall/-Wextra passes and prints:

```text
Entry=98726 Init=98541
Main=98382,98390,99200,98423,334052,553816,99234,98992,98521,98416,142288,130114,102520,102199,107942,98405
Close=98332 total=2656493; 31 negative gates passed; event ownership unresolved
```

This verifies the C++ work reconstruction and falsification slice. Root
owns integration and the independent full repository/gameplay regression;
this candidate does not advance the connected boot frontier.

### Factory audit: first missing native owner and decisive next slice

The coordinator's independent MSVC build/CTest also passes the same31
negative gates. Actual C++ work is2656493 unit boot steps; fresh-source
pending entry phase is `20000+2656493=2676493`, residue1 modulo12. The
coordinator has connected this produced work to the continuous-clock
**research** projection through80373AC4 without an observed frontier-count
argument. This strengthens the semantic work proof; it does not manufacture
the state skipped by the count-only module.

FreshGcHleApploaderSourceInitialization constructs an enum, scalar values
and two booleans. GateProfile compares those values. Neither function
constructs a scheduler queue, performs RAM initialization, installs handler
bytes, executes original loaded requests, emits progressive writes or applies
a device callback. In particular, its memory-owner enum and uninterrupted-
path boolean are explicit conditional premises, not returned ownership
receipts. The raw SHA gates certify original bytes, not a live producer that
loaded them at the requested time.

The first **unowned scheduling input** is exact and precedes the first
apploader instruction: the initial queue item
`FinishExecutingCommand(time0,FIFO0,userdata0000000300000001)`. DVD Init
registers this callback and enqueues it at DVDInterface.cpp287..290, after
ResetDrive(false). HW Init initializes DVD before SystemTimers, so this
item precedes SystemTimers' GPU/VI/DSP/audio/patch enqueues. The original
first SingleStep atPC81200258 callsAdvance before executing mflr. Advance
checks config, imports foreign events, commits20000, and dispatches all due
items. Thus claiming an empty queue or skipping that first callback is already
false before any apploader work is consumed.

The separately validated42d queue journal observes the source-derived order:

| First Advance dispatch | Due | FIFO | Lateness atG20000 |
|---|---:|---:|---:|
|FinishExecutingCommand / DTK, TCINT|0|0|20000|
|GPUSleeper|0|1|20000|
|DSPCallback|0|3|20000|
|VICallback|15444|2|4556|

FIFO breaks equal signed deadlines; VI's earlier FIFO2 does not move it
ahead of DSP's deadline0. DTK FinishExecutingCommand initializes transfer
size0 on its DTK arm, preserving DIMAR/DILENGTH; the initialized nonplaying
path has empty data, sets pending_blocks6 and reschedules deadline1699488.
GPU and DSP reschedule486000. VI reschedules30888. The first VI update
0->1 also clears PI's initial VI cause, giving10100->10000 under initial
PI mask0, and invokes its pre-entry NewField/Movie branches. These are
state effects even though they contribute no additional guest SingleStep
count. Audio and patch remain queued after this Advance. A final clock
match cannot distinguish preserved from omitted callback state.

The first **unowned guest-memory effect** inside this analytical module is
also exact:8120025C:9421FFF8 stores oldPALSP815EDCA8 at815EDCA0 and selects
that newSP. The following81200260:9001000C stores incomingLR0 at815EDCAC
(oldSP+4), not at newSP+4. The counted callback return eventually depends
on its actual spill bytes and copied thunk. The work counter emits none of
these bytes. RAM ownership and entry register production therefore cannot
be inferred from its successful count, even though their original-source
producers are known.

The smallest decisive next proof is a finite **native initialization plus
first-Advance/first-frame capsule**, not a whole timer or apploader runtime:

1. Construct fresh factor1 scheduler state and the six original initial
   queue entries from typed native initialization. Bind the source config
   contract before Advance; reject a config change, foreign import or unknown
   native owner before consuming its effect. Construct the relevant initial
   DVD/AI/DSP/VI/PI fields and retained unknown fields as explicit owners.
2. Reduce and apply only the four due callback branches above in signed
   deadline/FIFO order, retaining their actual memory/device/queue effects.
   Verify G20000, exact successor deadlines/FIFOs, PIcause10000/mask0,
   pending exceptions and callback-visible state against pure original
   before/after observations. The original journal validates this native
   construction; it must not be its runtime queue input.
3. Own PAL SP, LR0, bounded stack bytes and handler/report bytes, then apply
   only the first three Entry words. Verify815EDCA0/815EDCAC and actual LR/SP
   consequences. This links the first native semantic work to an owned
   state without an interpreter or host elapsed timing.
4. Falsify with DTK deadline0/1 (successor deadline changes even when N
   agrees), swapped equal-time FIFO, an omitted first VI effect (PIcause
   would remain10100), altered initialization/config/handler ownership and
   a stack spill alias. Preserve original callback observations separately
   from native inputs. Equal final ticks must not admit a mismatched state.

Passing this capsule closes one concrete dependency and permits advancing
through the next finite work/event slices. It does not yet establish the
entire nineteen-callback progressive write order, first SI poll/controller
ownership, RTC epoch provider or physical Gekko timing. The already observed
SI poll at247104 produces external-input-dependent channel words; keep
those unknown until an owned controller path reaches their first consumer.
Physical CPU cache/bus stalls are outside the source scheduler's unit boot
policy and must not be supplied by host duration as an unproved replacement.
