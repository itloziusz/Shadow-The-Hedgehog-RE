# PAL GUPP8P constructor and destructor assembly frontier

Scope: the static path after `__start` calls `0x803796AC` at `0x8000329C`,
through the first constructor targets. This is an instruction and data-flow
audit of the read-only `sys/main.dol` with SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
It is **not** a claim that the OS path returned, that all constructors ran in a
reference execution, or that native C++ has executed them. At checkpoint 24,
the connected native prefix reaches `0x80371714` only with an observed MSR;
the ordinary probe stops at `0x80003400` (see `../PROGRESS.md`). `PROVEN` below means
the words and their PPC data flow were checked; runtime branch inputs and
downstream effects retain their own confidence.

## Entry, exact walker CFG and register contract

`0x803796AC..0x803796C8` is an eight-instruction wrapper. At `AC/B0/B4`
it saves LR at old `r1+4`, creates an 8-byte frame, and `bl` calls
`0x803796CC` at `B8`. At `BC/C0/C4/C8` it reloads LR from current
`r1+12`, restores r1, and returns to `0x800032A0`. It does not itself read
the constructor table.

| Block | Instructions and exact data flow | Successor |
|---|---|---|
| W0 `0x803796CC..E8` | Save LR at old `r1+4`, `stwu r1,-16`, save r31 at new `r1+12`; `lis/addi` makes `0x804AAC60` in r0 via r3; set r31 to that table address. | W1 through two literal `b` instructions at `E8/EC` |
| W1 `0x803796F0`, `0x80379700..08` | `b` to `0x80379700`; load `r12=[r31]`; `cmplwi cr0,r12,0`. | Nonzero: W2 at `0x803796F4`; zero: W3 at `0x8037970C` |
| W2 `0x803796F4..FC` | `mtlr r12; blrl` invokes the loaded pointer (return PC `0x803796FC`); after a normal return, add four to r31. The `blrl` changes LR again; the wrapper's saved LR survives on stack. | W1 at `0x80379700` |
| W3 `0x8037970C..1C` | Reload saved LR from new `r1+20`, restore r31 from `r1+12`, pop 16-byte frame, `mtlr`, `blr`. | Wrapper return at `0x803796BC` |

The walker has no SDA/SDA2 access, FPR instruction, hardware register access,
or vtable lookup. It does change CR0 through the null comparison, LR at each
call, r0/r3/r12 as temporaries, and temporary stack memory. A constructor's
input GPR/FPR state other than the entry ABI is not fixed by the walker. The
call target is unsafe until the table value is checked against the executable
image. If a constructor fails to return, the next entry is not reached.

Independent raw checks: text1 maps virtual `0x80008D40` to file offset
`0x2600`. At `0x803796B8`, the word is `48000015` (wrapper call). At
`0x803796DC/E0` the words `3C60804B 3803AC60` derive the table base. At
`0x803796F4/F8/FC` they are `7D8803A6 4E800021 3BFF0004`; at
`0x80379700/04/08` they are `819F0000 280C0000 4082FFEC`. These words
agree with `q.py dis` and the CFG above. The `blrl` indirect edge comes from
RAM contents at the table address; it is not a direct branch encoded in text.

## Pointer table and call order

Data2 maps virtual `0x804AAC60` to file offset `0x4A7C60`. A fresh big-endian
parse of 283 words, including the terminator, gives SHA-256
`655d42743b66361082a98665dfc0fbad894a689407e977e288652153dba492b3`.
There are **282 nonzero, distinct, four-byte-aligned** targets, each inside
the two DOL text intervals (`0x80003100..0x80005600` or
`0x80008D40..0x804AAC60`). Word 282 at `0x804AB0C8` is zero. Thus, if all
targets return and nothing mutates the table, the loop performs 282 calls in
table order and stops there. The table itself is a writable DOL data section;
the static parse alone cannot prove that no earlier code changes a pointer.

| Index | Table word | Call target | Bounded classification |
|---:|---:|---:|---|
| 0 | `0x804AAC60` | `0x803A2520` | Conditional exception metadata registration |
| 1 | `0x804AAC64` | `0x8000AAF4` | Float/vector constants and two angle conversions |
| 2 | `0x804AAC68` | `0x8001C484` | Two 12-byte vector copies |
| 3 | `0x804AAC6C` | `0x8002218C` | Three float stores |
| 4 | `0x804AAC70` | `0x80035D4C` | Eight-byte object zero and destructor registration |
| 5 | `0x804AAC74` | `0x80039B88` | Four `0xFF` bytes |
| 6 | `0x804AAC78` | `0x8003AFB8` | Four element constructions and destructor registration |
| 7 | `0x804AAC7C` | `0x8004336C` | Four null-initialized handles and destructor registrations |
| 8 | `0x804AAC80` | `0x80045F48` | Two integer pairs |
| 9 | `0x804AAC84` | `0x80047710` | Fifteen element constructions and destructor registration |
| 10 | `0x804AAC88` | `0x8004DF20` | Integer pairs and byte quads |
| 11 | `0x804AAC8C` | `0x80054534` | `CannonTable_StaticInit`, scalar data/SDA stores |
| 281 | `0x804AB0C4` | `0x804216F4` | Last nonzero entry; body not yet audited here |
| 282 | `0x804AB0C8` | `0x00000000` | Loop terminator, never called |

`0x804AB0CC..0x804AB0DF` is outside the 283-word loop prefix. No inferred
constructor is assigned to those bytes. The bodies at indices 12..281 are
**UNKNOWN in this audit**. Their addresses are recoverable from the pinned
table but their effects and nested calls must be investigated individually
before claiming the post-constructor state or reaching `0x800510C0` natively.

## First-target instruction/data-flow slices

### C0: `0x803A2520..0x803A2558`

`stwu r1,-16`, save LR at current `r1+20`. `lwz r0,0x2850(r13)` reads
`0x805EED50`; the DOL data6 word is `0xFFFFFFFE` initially. `cmpwi r0,-2`
selects the direct return at `0x803A254C` if the word differs. When equal,
`lis/addi` sets r3=`0x80008D14` (extabindex), r4=`r2` (validated register
helper sets `0x805FA780`), then direct `bl 0x803A3954` and stores its r3
return to `0x805EED50` at `0x803A2548`.

The callee `0x803A3954..0x803A3984` reads `0x805A61F8`. If zero, it writes
`[0x805A61F0]=r3`, `[0x805A61F4]=r4`, `[0x805A61F8]=1` in that order and
returns r3=0; otherwise it returns r3=`0xFFFFFFFF` without those writes.
The CRT zero table covers `0x805A61F0..F8`, but **UNKNOWN** intervening OS
writes must be excluded by a runtime trace before asserting which branch ran.
SDA is read and conditionally written; no FPR, paired-single or indirect call
occurs in this bounded call chain. It likely registers exception metadata,
but that semantic label is **STRONG**, while the above instruction effects are
**PROVEN**.

### C1: `0x8000AAF4..0x8000AB8C`

The 32-byte frame stores LR at new `r1+36`, f31 both by `stfd` at `+16` and
`psq_st` at `+24`, and r31 at `+12`. `r2` supplies six scalar words. Verified
raw data7 words: `0x805F27AC=00000000`, `0x805F27B8=41A00000`,
`0x805F27BC=42548538`, `0x805F27C0=427D0027`,
`0x805F27C4=3F800000`, `0x805F27C8=43340000`, and
`0x805FBDB0=40490FDB`. `r31=0x8056FEE8`; stores at `0x8000AB20..3C`
initialize six floats across `0x8056FEE8..0x8056FEFC`. Direct calls to
`0x8000AB90` compute `f1 = f1 * (float(pi) / 180.0f)` by `lfs; lfs; fdivs;
fmuls` twice, first for the `0x805F27BC` input and then for
`0x805F27C0`. The first result is held in f31. Stores at
`0x8000AB60..70` write two converted floats to SDA
`0x805EF048/+4` and three more floats to `0x8056FF00/+4/+8`.
The return restores f31 with `psq_l` and `lfd`, LR and r31. Exact floating
result bits, FPSCR effects and paired PS1 restore require a Gekko reference
trace; a host `float` expression alone is **not** validated. No indirect call.

### C2 and C3: `0x8001C484..0x8001C4B4`, `0x8002218C..0x800221A4`

C2 creates a 16-byte frame and passes destination `0x80571C10`, source
`0x805E1860` to `0x8001C4B8`. That helper saves r30/r31,
calls `Vec3_Copy2` (`0x800091EC..0x80009204`) for destination offsets 0 and
12, then returns destination in r3. `Vec3_Copy2` performs three ordered
`lfs/stfs` copies per vector. C2 therefore copies 24 source bytes to
`0x80571C10..0x80571C27`; no arithmetic or indirect call. The source lies
inside the earlier CRT zero range, subject to any intervening writes.
C3 loads floats from SDA2 `0x805F29A8` (DOL word `0x00000000`) and
`0x805F29BC` (DOL word `0x3F800000`), then stores the
first to `0x80571CC0`, the second to `0x80571CC4`, and the first again to
`0x80571CC8`. Its `stfsu` updates r3 to `0x80571CC0`. No stack frame, call,
or indirect target. C2/C3 are **PROVEN instruction effects**; their runtime
source values are **UNKNOWN** until the pre-call memory image is checked.

### C4 and C5: `0x80035D4C..0x80035D80`, `0x80039B88..0x80039BB8`

C4 passes SDA address `0x805EF140` to `0x80028E90`, which writes zero to
the first two words. It then calls `__register_global_object`
(`0x803A1210..0x803A1224`) with r3=the object, r4=`0x80028D60`
(destructor), r5=`0x80572238` (12-byte record). The registration helper reads
old head at SDA `0x805F2370`, writes record `+0=old head`,
`+4=0x80028D60`, `+8=0x805EF140`, then writes the record address to the
head. It neither calls the destructor nor allocates memory. C5 passes
`0x805EF190` and four `0xFF` arguments to `0x8040E328`; four `stb`
instructions write `0xFF` to `0x805EF190..93`. The two constructors' 16-byte
frames save and restore LR. The pointer chain contents after C4 depend on the
old head, though the new record's addresses and store order are **PROVEN**.

### C6, C7 and C9: array and handle constructors

C6 (`0x8003AFB8..0x8003B00C`) passes base `0x80572544`, element constructor
`0x8003B048`, element destructor `0x80021C44`, stride 4 and count 4 to
`0x803A1500`. C9 (`0x80047710..0x80047764`) uses the same callbacks and
stride, base `0x8057433C`, count 15. The array helper's `0x803A154C..78`
loop sets r3 to each element, r4=1, r12 to the *argument* constructor
pointer, `mtctr; bctrl`, advances r31 by stride and increments a stack
counter. For these two call sites the constructor target is **resolved** to
`0x8003B048`, whose four instructions write halfwords `0` and `0xFFFF` at
element offsets 0 and 2. On normal return from all calls the counter equals
the count, so `0x803A1584` branches around the helper's destructor cleanup
region. That region has a second indirect call at `0x803A15C0` through the
destructor argument; it is not taken on the normal count-complete path, and
would resolve to `0x80021C44` for these two call sites if reached. An
exception/non-return is **UNKNOWN** and cannot be modeled as normal success.
C6 registers record `0x80572538` with destructor `0x8003B010`; C9 registers
record `0x80574330` with destructor `0x80047768`, both through
`__register_global_object` with object argument zero. These destructor
callbacks are stored for later; neither is called by registration.

C7 (`0x8004336C..0x800433F4`) calls four constructor helpers with SDA
objects `0x805EF1B0/B4/B8/BC`, then registers four corresponding destructor
records `0x80573D88/94/A0/AC`. Each immediate constructor stores zero to
its object word and passes r4=0 to a nested ref-count assignment helper.
The nested helper sees the just-stored zero and the zero argument, so on the
normal path its conditional allocation and vtable branches are skipped;
this resolves the immediate indirect-call risk **within C7**. The four
record destructors are `0x8003EBF4`, `0x80042E18`, `0x8003C9F4`, and
`0x8003E218`. Their bodies are **not** executed at registration. C7 has a
16-byte frame saving LR and r31; each registration prepends its record to
the same SDA head `0x805F2370`.

### C8, C10, C11: additional direct static writes

C8 (`0x80045F48..0x80045FA8`) calls two-word store helpers
`0x80045FB8` and `0x80045FAC` via stack temporaries, producing
`[0x805EF1F8,0x805EF1FC]=(0,0x154)` and
`[0x805EF200,0x805EF204]=(0x280,0x78)` in that order.
C10 (`0x8004DF20..0x8004DFF0`) likewise writes two integer pairs at
`0x805EF24C..58` and two byte quads at `0x805EF25C..63` via the direct
helper `0x8040E33C`; its literal inputs are visible at the listed PPC
addresses. C11 (`0x80054534..0x800545CC`) writes float data to SDA
`0x805EF2BC..C8` and DOL data `0x8051E9A4..0x8051EAC0`. All are direct
calls/stores in their audited bodies. Exact initial FPR/FPSCR propagation
through these `lfs/stfs` sites still needs a runtime comparison.

## Destructor side of the same mechanism

The head at `0x805F2370` is consumed by `0x803A11C8..0x803A120C`, which
reads a record, advances the head to record `+0`, loads callback from `+4`
and object from `+8`, passes r4=`0xFFFFFFFF`, and calls by `mtctr; bctrl`.
Its sole direct code caller found by `q.py callers` is `0x803A3B30`, on the
post-application exit route at `0x803A3B04`; this cleanup is not part of
normal constructor registration. The `.dtors` data3 prefix at
`0x804AB0E0` is `0x803A11C8, 0x803A24EC, 0`, a separate exit table called
by `0x803A3B44..58`. This explains the late call target provenance without
pretending that registered destructors run before `0x800510C0`.

## C++ representation and validation boundary

The following is a *portable data-flow representation* of the proven walker,
not a completed game implementation. It preserves the runtime table read and
refuses an unverified pointer. Each callback must have its own proven state
projection before the loop may continue natively.

```cpp
void RunCtors(GuestState& s, GuestMemory& m, ConstructorRegistry& verified) {
    std::uint32_t slot = 0x804AAC60;
    for (;;) {
        const std::uint32_t target = m.read_be32(slot); // PPC 0x80379700
        s.compare_unsigned_cr0(target, 0);             // PPC 0x80379704
        if (target == 0) break;
        if (!verified.contains_exact(target))
            throw UnresolvedConstructor{slot, target};
        verified.invoke_exact(target, s, m);           // mtlr/blrl
        slot += 4;                                      // PPC 0x803796FC
    }
}
```

The actual PPC loop does not validate the target or throw; the check is a
native fail-closed requirement. It cannot be used to skip a constructor.
Exact stack/LR/CR effects and all nested writes must be included in the
native state contract if they are observed later. The first runtime parity
gate is a same-run pre/post snapshot for C0, then each next target in order,
including table integrity; only after all 282 and any invoked nested calls
are validated can the application entry at `0x800510C0` be promoted.

**Checks performed:** two independent methods, `q.py dis/xref/callers` and
big-endian DOL byte parsing, agree on walker words, table position, count,
termination, first 12 and last pointer, and initial data constants. Raw
table parsing also checked uniqueness, alignment and executable-section
membership of every nonzero pointer. No C++ code or connected boot checkpoint
was changed in this audit. **Remaining UNKNOWN:** actual constructor entry
memory after OS, whether the table was modified before walking, all nested
effects for indices 12..281, exact FPSCR/paired-single outcomes, and any
runtime exception or non-return path.
