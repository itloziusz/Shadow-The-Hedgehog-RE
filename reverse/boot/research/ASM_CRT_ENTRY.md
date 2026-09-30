# PAL GUPP8P assembly audit: CRT tables and entry continuation

Scope: `0x8000315C..0x80003268`, including the called CRT table walker
`0x80003340..0x800033FC`, its reached `memset` implementation
`0x8000540C..0x800054F0`, and the small `0x80370BF0` predecessor of OS
startup. The preceding hardware call must return before any instruction here
executes. Evidence is the read-only PAL `main.dol` with SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`;
addresses and disassembly were independently queried through `q.py`, and both
descriptor tables were reread as big-endian DOL words. **PROVEN** below means
an exact instruction or immutable table consequence, **STRONG** a semantic
classification supported by those effects, and **UNKNOWN** an unobserved
runtime input or callee outcome. Static proof does not assert the path ran on
retail hardware. The available Dolphin entry capture is a synthetic startup
disc/HLE path only; it is not retail IPL evidence.

## Boundary and stack ledger

On return from `0x80003400`, entry PC is `0x8000315C` and, if the earlier
register helper remains valid, `r1=0x8060C5F0`, `r2=0x805FA780`,
`r13=0x805EC500`. The hardware callee must independently establish its own
return and observable state; this document does not supply it.

| PC | Exact instruction effect |
|---|---|
| `0x8000315C` | `li r0,-1`: `r0=0xFFFFFFFF`. |
| `0x80003160` | `stwu r1,-8(r1)`: `r1=0x8060C5E8`; `[0x8060C5E8]` receives the old stack pointer `0x8060C5F0`. |
| `0x80003164` | `stw r0,4(r1)`: `[0x8060C5EC]=0xFFFFFFFF`. |
| `0x80003168` | `stw r0,0(r1)`: overwrites the back-chain word, `[0x8060C5E8]=0xFFFFFFFF`. |
| `0x8000316C` | `bl 0x80003340`: `LR=0x80003170`; enters the CRT walker. |
| `0x80003340` | `mflr r0`: `r0=0x80003170`. |
| `0x80003344` | `stw r0,4(r1)`: **overwrites** the upper `-1` sentinel at `0x8060C5EC` with `0x80003170`. |
| `0x80003348` | `stwu r1,-0x18(r1)`: CRT `r1=0x8060C5D0`, back-chain `[D0]=0x8060C5E8`. |
| `0x8000334C/50/54` | Save incoming `r31/r30/r29` at `0x8060C5E4/E0/DC`. |
| `0x800033E4..0x800033FC` | Reload LR from `0x8060C5EC`, restore `r31/r30/r29`, add `0x18` to `r1`, set LR, return to `0x80003170`. `r1=0x8060C5E8`; the upper stack word stays `0x80003170`, lower stays `-1`. |

The return address store is an observable ordered guest-memory effect. A
projection that retains both `-1` stack words after the CRT call is incorrect.
The `memset` wrapper uses its own 16-byte frame below the CRT frame, saves
LR at its frame `+0x14`, and restores it. No CRT instruction reads SDA or
SDA2; `r2/r13` are input bases carried across this region, not recomputed.
The integer CRT/fill path does not modify FPRs or FPSCR. Integer comparisons
and `addic.` instructions do modify CR0 and XER; their exact final values
should be checked at a runtime boundary, not inferred from ABI convention.

## CRT copy CFG and data flow (`0x80003358..0x800033A8`)

| PC | Instruction-level interpretation |
|---|---|
| `0x80003358/5C/60` | Build `r29=0x80005544`, the copy-table cursor (`lis; addi; mr`). |
| `0x80003364/68` | Two unconditional branches into loop head `0x8000336C`. |
| `0x8000336C` | `r30=[r29+8]`, the descriptor size. |
| `0x80003370/74` | Unsigned compare to zero, then branch to zero-table setup `0x800033AC` if size is zero. This test alone terminates the loop; source/destination terminator words are not consulted. |
| `0x80003378/7C` | `r4=[r29]` source; `r31=[r29+4]` destination. |
| `0x80003380` | A second `beq 0x800033A4` reuses the size comparison's CR0. On the reached nonzero-size path it is necessarily false; the intervening loads do not set CR0. |
| `0x80003384/88` | Compare destination and source as 32-bit words; equal skips copying and cache work. |
| `0x8000338C/90/94` | Set `(r3,r4,r5)=(destination,source,size)`; call `memmove` only for unequal addresses. |
| `0x80003398/9C/A0` | Set `(r3,r4)=(destination,size)`; call `0x80003424` for D-cache writeback/I-cache invalidate only after the copy returns. |
| `0x800033A4/A8` | Advance cursor by 12 bytes and repeat from `0x8000336C`. |

The ten DOL descriptors have nonzero sizes and **all** have `source ==
destination`; the eleventh at `0x800055BC` is `(0,0,0)`. Therefore the
direct path makes zero `memmove` and zero `0x80003424` calls. This is an
immutable-image reachability proof, not permission to discard the descriptor
contract if the DOL changes. For a different descriptor, `0x80003424`
aligns its cache address with mask `0xFFFFFFF1`, performs `dcbst; sync; icbi`
in an eight-byte-stepped loop with `addic.`/`bge`, then `isync`; this branch
is unreachable for the ten checked PAL entries. Its exact cache range
depends on its input address and length and must not be silently replaced by
an unrelated host operation.

The ten checked `(source=destination,size)` pairs are:

```text
80003100:000024E8  80005600:00001F08  80007520:00001814
80008D40:004A1F08  804AAC60:0000046C  804AB0E0:0000000C
804AB100:00072418  8051D520:000528C8  805E4500:0000AB20
805F2780:00009DB8
```

## CRT zero CFG and reached fill implementation

| PC | Instruction-level interpretation |
|---|---|
| `0x800033AC/B0/B4` | Build `r29=0x800055C8`, the zero-table cursor. |
| `0x800033B8/BC` | Two unconditional branches into loop head `0x800033C0`. |
| `0x800033C0/C4/C8` | `r5=[r29+4]` size; compare to zero; branch to epilogue at `0x800033E4` if zero. Destination terminator word is not inspected. |
| `0x800033CC/D0` | `r3=[r29]` destination; repeated `beq` uses the previous size CR0 and is false on the reached nonzero-size path. |
| `0x800033D4/D8` | `r4=0`; call `memset(r3,0,r5)` at `0x8000540C`. |
| `0x800033DC/E0` | Advance cursor by eight bytes and repeat. |
| `0x800033E4/E8/EC/F0/F4/F8/FC` | Reload return LR at caller-frame `+0x1C`, restore nonvolatile registers, pop CRT frame, `mtlr`, `blr`. |

The three reached pairs are:

| Destination | Bytes | Exclusive end | Exact effect |
|---:|---:|---:|---|
| `0x8056FE00` | `0x74700` | `0x805E4500` | Clears BSS region including `0x80586CB4` installed earlier by the hardware handler call and any `0x805A4FB8` debug context. The earlier install is transient across this CRT call. |
| `0x805EF020` | `0x375C` | `0x805F277C` | Clears later OS globals, including FPR *source* bytes `0x805F1F30..3F` **after** hardware has read them; also `0x805F1F18/+1C`, and debug byte `0x805F1FF0`. |
| `0x805FC540` | `0xAC` | `0x805FC5EC` | Clears final range; the four-byte gap `0x805F277C..0x805F2780` is not cleared by this table. |

At `0x8000540C..38`, `memset` creates/restores a 16-byte frame, preserves
incoming `r31`, calls `0x8000543C`, then returns the original destination
in `r3`. At `0x8000543C..F0`, the fill routine converts byte `r4` to
eight bits (`0x80005440`), computes destination alignment (`0x80005444..68`),
replicates a nonzero fill byte when needed (`0x8000546C..88`), writes groups
of 32 bytes with eight `stw` instructions (`0x8000548C..BC`), then any
remaining four-byte words (`0x800054C0..D0`) and tail bytes
(`0x800054D4..F0`). All three actual destinations are four-byte aligned,
their sizes are multiples of four, and the input byte is zero, so there is
no leading/tail byte write and each of their bytes is zeroed. The fill uses
integer registers only. No `memmove`, MMIO, SDA, FPR, paired-single or
indirect call occurs on this checked CRT path.

The DOL header's broad BSS envelope is not an equivalent replacement:
it overlaps loaded data and does not express these three timed calls.

## Entry CFG after the CRT return (`0x80003170..0x80003268`)

| PCs | Exact register, branch and memory effect |
|---|---|
| `0x80003170..7C` | Build `r6=0x80000044`, set `r0=0`, store zero to `[0x80000044]`. |
| `0x80003180..90` | Load BI2 pointer `B=[0x800000F4]`; if `B==0`, branch to `0x8000319C`; otherwise continue at `0x80003194`. Unknown `B` is unsafe to dereference. |
| `0x80003194/98` | For `B!=0`, load debug word `D=[B+0x0C]`, then branch to `0x800031BC`. |
| `0x8000319C..AC` | For `B==0`, load arena-high `A=[0x80000034]`; if `A==0`, skip debug handling to `0x800031F8`. |
| `0x800031B0..B8` | If `B==0 && A!=0`, load debug word `D=[0x800030E8]` (a full **word**, not a byte). |
| `0x800031BC..D8` | Build debug argument `r5`: `D==2` -> `r5=0` and branch `0x800031E8`; `D==3` -> `r5=1` and branch `0x800031E8`; `D==4` -> `r5=2`, continue `0x800031E0`; other `D` -> branch `0x800031F8`. The comparisons are unsigned word comparisons. |
| `0x800031E0/E4` | For `D==4`, call `0x80003140`; it executes `li r0,1; stb r0,0x5AF0(r13)` so `[0x805F1FF0]=1`, then return and branch `0x800031F8`. |
| `0x800031E8..F4` | For `D==2/3`, materialize `r6=0x8039F8E0`, `mtlr r6`, then `blrl`. The *apparent indirect target is a proven constant* at this call site; `blrl` sets return LR to `0x800031F8`. Its callee's exceptional/context-transfer outcome is **UNKNOWN**. It writes a register context at `0x805A4FB8` onward, touches MSR/SRR1 and branches through other helpers; a normal return cannot be assumed merely because `blrl` has a following instruction. |
| `0x800031F8..0x80003208` | Reload `B=[0x800000F4]` (do not reuse the first load); if zero branch to `0x80003258`. This reread matters if a debug callee changed memory. |
| `0x8000320C..14` | If `B!=0`, load `O=[B+8]`; if `O==0`, branch to `0x80003258`. Unknown `O` is not an address until checked against mapped memory. |
| `0x80003218..24` | Set `P=B+O`; load count `N=[P]` into `r14`; if zero branch `0x80003258`. Integer wraparound and unmapped `P` must be rejected in a native representation. |
| `0x80003228..30` | Set `r15=P+4`, `CTR=N`, and `r6=P+4`. |
| `0x80003234..40` | Do/while `CTR`: load `[r6]`, add base `B`, store result back to `[r6]`, branch `bdnz 0x80003230` after decrementing CTR. Exactly `N` consecutive words starting at `P+4` are relocated, with 32-bit arithmetic. The loop's address range must be proven valid before native writes. |
| `0x80003244..54` | Build pointer `0x80000034`; mask `r15` with `0xFFFFFFE0` (`rlwinm` bits 0..26); store this **aligned start of relocation array**, not its end, to `[0x80000034]`; branch to `0x80003260`. `r14=N`, `r15=P+4` remain live. |
| `0x80003258/5C` | On any skipped relocation path set `r14=0`, `r15=0`. These registers later become application arguments only if intervening callees preserve them. |
| `0x80003260` | Direct call to `0x80370BF0`, detailed below. |
| `0x80003264` | Direct call to OS startup `0x80370E68`. This is the next large runtime/device boundary; this audit does not claim its return. |

The `0x80370BF0..0x80370C14` leaf uses the post-CRT SDA base `r13`:

| PCs | Proven store |
|---|---|
| `0x80370BF0..0x80370BFC` | Build `0x80000040` and write it to `[r13+0x5A18]=[0x805F1F18]`. |
| `0x80370C00..0x80370C08` | Build `0x80370C60`, subtract `0x80000000` using `addis`, write `0x00370C60` to `[0x80000048]`. |
| `0x80370C0C..0x80370C14` | Write one to `[r13+0x5A1C]=[0x805F1F1C]`, return to `0x80003264`. |

No virtual dispatch or vtable access occurs in this audited region. The only
indirect call, `0x800031F4`, has a statically constructed exact target.
Neither the debug path nor relocation may be dropped as obsolete without
proving their branch inputs and consumer effects. The low-memory/BI2 route
is CRT/runtime handoff; the cache flush is GameCube-specific but unreachable
for this image; OS device initialization starts after this region.

## Portable C++ representation of *proved behavior*

This pseudocode specifies an interface/algorithm, **not** a claim that the
current connected native boot has passed these boundaries. Every read/write
uses checked guest addresses and big-endian words; `DebugTransfer` and
`EnterOsStartup` decline until their effects are separately proven.

```cpp
void CrtTables(CheckedGuestMemory& m) {
    for (uint32_t p = 0x80005544;; p += 12) {
        uint32_t n = m.u32(p + 8);
        if (n == 0) break;
        uint32_t src = m.u32(p), dst = m.u32(p + 4);
        if (src != dst) {
            m.memmove_checked(dst, src, n);
            platform.cache_after_code_copy(dst, n); // exact contract pending
        }
    }
    for (uint32_t p = 0x800055C8;; p += 8) {
        uint32_t n = m.u32(p + 4);
        if (n == 0) break;
        m.memset_checked(m.u32(p), 0, n);
    }
}

void EntryAfterCrt(CheckedGuestMemory& m, CpuState& c) {
    m.put_u32(0x80000044, 0);
    uint32_t bi2 = m.u32(0x800000F4);
    if (bi2 != 0 || m.u32(0x80000034) != 0) {
        uint32_t debug = bi2 ? m.u32_checked(bi2 + 12)
                             : m.u32(0x800030E8);
        if (debug == 4) m.put_u8(0x805F1FF0, 1);
        if (debug == 2 || debug == 3)
            DebugTransfer(/*r5=*/debug - 2); // explicit unresolved hook
    }
    bi2 = m.u32(0x800000F4); // second original load
    c.r14 = c.r15 = 0;
    if (bi2 != 0) {
        uint32_t off = m.u32_checked(bi2 + 8);
        if (off != 0) {
            uint32_t p = checked_add(bi2, off);
            uint32_t count = m.u32_checked(p);
            if (count != 0) {
                uint32_t first = checked_add(p, 4);
                for (uint32_t i = 0; i < count; ++i) {
                    uint32_t a = checked_add(first, checked_mul(i, 4));
                    m.put_u32(a, m.u32_checked(a) + bi2);
                }
                m.put_u32(0x80000034, first & 0xFFFFFFE0u);
                c.r14 = count;
                c.r15 = first;
            }
        }
    }
    m.put_u32(0x805F1F18, 0x80000040);
    m.put_u32(0x80000048, 0x00370C60);
    m.put_u32(0x805F1F1C, 1);
    EnterOsStartup(); // 0x80003264 -> 0x80370E68, separately gated
}
```

The stack ledger above must also be applied in a CPU-state-equivalent native
model. A high-level memory-only lowering must state explicitly why those
stack bytes are unobservable before eliding them. For a mutated DOL, the
cache helper and `memmove` semantics need their own instruction-level proof.
The checked address arithmetic is deliberately fail-closed: an unobserved
BI2 pointer, offset or count cannot be promoted into a host pointer.

## Validation and unresolved dependencies

1. **Assembly:** `q.py dis` decoded each cited PPC range; independent DOL
   `u32` reads found exactly ten identity triples plus size-zero terminator
   and the three zero pairs plus size-zero terminator. The static CFG explains
   the repeated `beq` instructions through the unchanged CR0. No class/RTTI
   inference was used.
2. **Data flow:** register and stack addresses were propagated from the
   already validated helper output. The upper sentinel overwrite follows
   directly from the call's LR and `stw r0,4(r1)`. The three fills include
   the first hardware handler-table write; that write must be treated as
   transient unless reinstalled after CRT.
3. **State/execution:** this audit has **not** compared a same-run PPC
   checkpoint at `0x80003170` or `0x80370E68` against the connected native
   prefix. The current `shadow_boot_probe` stops at `0x80003158`; archive
   `experimental_native_boot` projections are not a substitute. Do not
   promote the pseudocode to validated C++ on the strength of static data.
4. **First dependencies:** hardware return state at `0x8000315C`, the
   pre-entry and post-CRT low-memory/BI2 image (`0x80000034/+F4`, BI2
   `+8/+C`, `0x800030E8`), optional debug transfer behavior, valid
   relocation destination span, and then OS call `0x80370E68` with its
   time-base, interrupts and device effects. A synthetic Dolphin HLE entry
   snapshot can conditionally test that path; retail IPL parity remains open.

The next execution gate is a cold reference break at `0x8000315C`, then
`0x80003170` and `0x80370E68`, recording registers/CR/XER/LR, stack words,
all three zero ranges or suitable boundary samples, BI2 branch inputs,
relocation output, and the three stores from `0x80370BF0`. Run the native
prefix from `0x80003154` after each implementation step and compare ordered
effects. Stop at the first mismatch.
