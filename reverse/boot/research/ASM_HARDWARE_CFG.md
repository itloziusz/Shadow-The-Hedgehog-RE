# Assembly CFG: connected entry hardware call (PAL GUPP8P)

Scope: DOL `__start` at `0x80003154`, through the return from its second call at `0x8000315C`. This is an **assembly reconstruction**, not a connected native execution claim. Source: read-only `main.dol`, SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`; `q.py dis` for each target, independently checked against big-endian DOL words. The first text section maps file offset `0x100` to virtual `0x80003100`; the second, containing the hardware routines, maps file offset `0x2600` to virtual `0x80008D40` (DOL header). Relevant raw words include `0x80003158: 480002A9`, `0x80370CF0: 4182008C`, `0x80372860: 40820014`, `0x80372880: 40820014`, `0x803728A0: 40820058`, `0x80372684: 4082FFF4`, `0x803726C0: 4082FFE8`, and `0x803733C4: 408201A0`. The `q.py` instruction listing and these independently addressed words agree on the CFG edges.

## Entry and call graph

**PROVEN (`0x80003154..0x8000315C`):** `bl 0x800032B0` at entry returns to `0x80003158` after the previously pinned register helper. `bl 0x80003400` then sets LR to `0x8000315C`. Its callee has no stack frame: `mfmsr; ori 0x2000; mtmsr; mflr r31; bl 0x80371714; bl 0x80370CDC; bl 0x80372838; mtlr r31; blr` (`0x80003400..0x80003420`). Thus the three calls are ordered; r31 carries the return address through them, and r31 itself is left equal to `0x8000315C` at return. No indirect call occurs in this connected prefix. The subsequent `0x8000315C` instruction (`li r0,-1`) is outside this slice.

| Node / PPC range | Register and stack data flow | Side effects and successors |
|---|---|---|
| H0 `0x80003400..0x80003410` | Input MSR, LR=`0x8000315C`; r0=`MSR | 0x2000`, r31=LR. No frame. | Write MSR; call P0 `0x80371714`; return H1 `0x80003414`. |
| H1 `0x80003414` | r31 retained by P0. | Call F0 `0x80370CDC`; return H2 `0x80003418`. |
| H2 `0x80003418..0x80003420` | r31 retained by F0 and cache routine; LR restored from r31. | Call C0 `0x80372838`; return to entry `0x8000315C`. |

The preserved-r31 statement is verified through the three direct callees and their nested routines below. Initial MSR and all initial SPR state remain **UNKNOWN** without an entry-state oracle. The `0x2000` write itself is **PROVEN** regardless of the input.

## P: paired-single prerequisite, `0x80371714..0x80371764`

**PROVEN.** P0 saves incoming LR at old `r1+4`, decrements r1 by 8, then calls the HID2 read helper `0x80370BA8` (`mfspr r3,hid2; blr`). At `0x80371724`, `oris r3,r3,0xA000` computes `HID2 | 0xA0000000`; `0x80370BB0` writes it (`mtspr hid2,r3`). `0x803725F4..0x80372600` reads HID0, ORs `0x00000800`, writes HID0, and returns. That HID0 bit is the self-clearing ICFI command when instruction caching is enabled; the written word is not necessarily the next readback word (Gekko manual Table 2-4; `GEKKO_PREFIX_SEMANTICS.md`). After `sync`, `li r3,0` followed by eight `mtspr gqr0..gqr7,r3` at `0x80371738..0x80371754` zeros all GQRs. P0 restores LR from current `r1+12` (the old `r1+4`), increments r1 by 8, and returns to H1. It neither accesses SDA nor calls a constructor. Non-stack effects: HID2 OR write, ICFI command/write, GQR0..7 zero; no ordinary RAM writes. Exact HID0/HID2 readback and cache consequences depend on prior hardware state (**UNKNOWN** for retail).

## F: FPR/FPSCR seed, `0x80370CDC..0x80370E00`

**PROVEN CFG and addresses.** F0 reads MSR into r3, ORs `0x2000`, writes MSR, reads HID2, then `rlwinm. r3,r3,3,31,31` at `0x80370CEC` sets CR0 from the selected low bit. `beq 0x80370D7C` at `0x80370CF0` would skip F1 if that bit were zero. P0's preceding OR of `0xA0000000` forces original HID2 numeric bit 29 to one; rotate-left-3 puts it in numeric bit 0. Thus F1 is selected on the connected path assuming the preceding SPR write has its documented effect. The raw `rlwinm.` word is `54631FFF` and branch word `4182008C`.

| Node / PPC range | Data flow and effects | Successor |
|---|---|---|
| F1 `0x80370CF4..0x80370D78` | Construct `r3=0x805F1F38`; `psq_l f0,0(r3),W=0,GQR0` at `0x80370CFC`; `ps_mr f1..f31,f0`. GQR0 was set to zero by P0. No stack access. | F2 `0x80370D7C`. |
| F2 `0x80370D7C..0x80370E00` | `lfd f0,0x5A30(r13)`; with pinned r13=`0x805EC500`, address is `0x805F1F30`; `fmr f1..f31,f0`; `mtfsf 0xFF,f0`; return. | H2 `0x80003418`. |

**UNKNOWN input dependency:** the 16 bytes `0x805F1F30..3F` are not DOL section contents. F1 and F2 therefore cannot be replaced with hard-coded FPR or FPSCR values from static analysis. `psq_l`, `ps_mr`, `lfd`, and `fmr` must be validated at both paired lanes and FPSCR against runtime evidence. F2 is a read through r13, not an immediate address; the address above depends on the already validated register-helper r13. The F1 source address derives from `lis/addi` independent of SDA. No pointer-valued loads, indirect calls, or constructors occur here.

## C: cache and handler registration, `0x80372838..0x80372928`

**PROVEN frame:** save LR to old `r1+4`, subtract 16 from r1, save r31 at new `r1+12` and r30 at new `r1+8`; restore them at `0x80372914..0x80372928`. r31 becomes the string-table base `0x80561380`; it is restored before return. The three branch tests are explicit inputs, not inferred platform policy:

| Node | Test / successors | Ordered effects when test is false |
|---|---|---|
| C0 `0x80372854..0x80372874` | Read HID0 via `0x80370AEC`; test `HID0 & 0x00008000` at `0x80372858..60`; nonzero -> C1 at `0x80372874`. | `0x80372604..14`: `isync`, read HID0, OR `0x8000`, write HID0; call logging stub `0x80370C8C`. |
| C1 `0x80372874..0x80372894` | Read HID0 again; test `HID0 & 0x00004000` at `0x80372878..80`; nonzero -> C2 at `0x80372894`. | `0x803724F4..0x80372504`: `sync`, read HID0, OR `0x4000`, write HID0; logging stub. |
| C2 `0x80372894..0x803728F8` | Read L2CR via `0x80370AFC`; test `L2CR & 0x80000000` at `0x80372898..A0`; nonzero -> C3 at `0x803728F8`. | Save current MSR in r30; `sync`; write MSR=`0x30`; clear L2CR high bit; `sync`; call invalidate I; restore saved MSR; set L2CR high bit while clearing invalidate bit; logging stub. Exact nested sequence below. |
| C3 `0x803728F8..0x80372928` | No conditional branch in this node. | r4=`0x803726D8`, r3=`1`; call handler registration `0x80373378`; logging stub; restore frame and return H2. Handler address is stored as data, not invoked. |

The three branch outcomes are **UNKNOWN** from the DOL image alone: HID0 readback bits 15/14 and L2CR high bit need runtime state. The earlier HID0 ICFI command does not determine either L1 test. The logging stub `0x80370C8C..0x80370CD8` subtracts 0x70 from r1, conditionally saves FPR1..8 based on CR1, saves r3..r10 to that frame, restores r1, and returns; it performs no out-of-frame output operation. The caller's `crxor crb6,crb6,crb6` clears CR bit 6 before each call, making the stub's `bne cr1` skip the FPR stores on this path. The temporary stack bytes are still writes and CR bit 6 remains changed.

### L2 nested path, `0x80372640..0x803726D4`

**PROVEN CFG; termination UNKNOWN.** The routine saves LR and r31 in a 16-byte frame. After `sync`, it reads L2CR and clears bit `0x80000000`, writes it, `sync`s, reads L2CR again, ORs `0x00200000` (invalidate request), writes it. Its first poll (`0x80372678..84`, raw branch `4082FFF4`) repeats while `L2CR & 1` is nonzero. It then clears bit `0x00200000`, writes L2CR, and reaches a second poll at `0x803726B4..C0` (raw branch `4082FFE8`): while `L2CR & 1` is nonzero it calls the local logging stub with string `0x80561380`. On zero it restores frame and returns. The unconditional branches at `0x80372670`, `0x80372674`, `0x80372694`, `0x803726A0`, and `0x803726A4` are explicit CFG edges, not omitted work. The two polls require hardware state evolution; do not simulate a terminating result without a reference trace or a defined platform hook.

### Handler table, `0x80373378..0x8037358C`

**PROVEN for caller selector 1.** A 48-byte frame saves LR and r28..r31. Inputs r3=1, r4=`0x803726D8`. Call `0x8037611C`: read MSR, clear its `0x8000` interrupt-enable bit, write MSR, return old bit in r3. Compute `r5=rlwinm(1,2,14,29)=4`; base `0x80586CB0`, so `r4=0x80586CB4`. Read old handler into r30, store new handler `0x803726D8` there. `cmplwi (1 & 0xFFFF),0x10` is false, so branch `0x803733C4 -> 0x80373564` skips the large vector/process-list modification region `0x803733C8..0x80373560`. At `0x80373564`, call `0x80376144` with the saved old interrupt bit: read current MSR, restore its `0x8000` bit to the saved value, write MSR, and return previous current bit in r3. Return old handler in r3 after restoring the frame. Thus the selected handler is registered in RAM, but **no vtable or indirect branch is executed** in this slice. No claim is made about selector 16 or about the handler's later execution.

## Portable representation boundary

The following is a data-flow specification for a future portable C++ service, **not** an implementation or a parity assertion. All reads and writes remain observable operations; the cache polls must be supplied by an evidence-backed platform service.

```cpp
// Addresses and order match PPC 0x80003400..0x80003420.
void HardwareCall(State& s, Platform& hw) {
    hw.write_msr(hw.read_msr() | 0x2000);
    hw.write_hid2(hw.read_hid2() | 0xA0000000u);
    hw.write_hid0(hw.read_hid0() | 0x00000800u);
    hw.sync();
    for (unsigned i = 0; i != 8; ++i) hw.write_gqr(i, 0);
    hw.write_msr(hw.read_msr() | 0x2000);
    if (rotl32(hw.read_hid2(), 3) & 1) {
        auto paired = hw.psq_load_two(s.read8(0x805F1F38), 0);
        for (unsigned i = 0; i != 32; ++i) s.ps_mr(i, paired);
    }
    auto d = s.load_be_double_bits(0x805F1F30); // r13 + 0x5A30
    for (unsigned i = 0; i != 32; ++i) s.fmr(i, d);
    s.mtfsf_all_from_f0();
    // Continue with C0..C3 above, including conditional cache operations,
    // polls, and the interrupt-guarded handler-table store.
}
```

The pseudocode deliberately does not replace `psq_l` with a host float conversion or fill unknown RAM, and it does not model cache outcomes as constants. It is sufficient to identify the portable boundary and necessary inputs. A connected implementation must validate register/SPR/CR/FPR state and ordered memory effects at `0x8000315C` before advancing.

## Validation and unresolved edges

- **PROVEN structurally:** all direct calls and conditional edges above match `q.py dis` and sampled raw DOL words; DOL segment addressing independently recovered from header fields. Stack frame offsets, SDA read (`r13+0x5A30`), handler-table address, and register-preservation paths were cross-checked instruction by instruction.
- **UNKNOWN dynamically:** exact incoming MSR/HID0/HID2/L2CR; selected C0/C1/C2 paths; L2 poll values and timing; all bytes at the two FPR source ranges at time of use; exact FPR paired-lane/FPSCR state; interrupt/asynchronous side effects. Any Dolphin startup-only disc trace is a synthetic HLE-loader oracle and must be labeled separately from retail boot evidence.
- **Next evidence:** pause original execution at `0x80003400`, `0x80370CFC`, `0x80370D7C`, `0x80372860`, `0x80372880`, `0x803728A0`, and `0x8000315C`; record incoming registers, CR/XER/CTR/LR, SPRs, raw reads, FPR lanes, FPSCR, stack, and handler-table word. Compare native state after re-running from entry, then investigate the first mismatch.
