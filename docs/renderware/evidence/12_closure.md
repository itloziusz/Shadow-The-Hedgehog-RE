# Role 12 — input and xref closure (static, blur-only)

## Evidence contract
- Only this report is written. Original DOL, prior reports, exports, and `GPT_SOL_analysis/` remain read-only; no optional source fragment or runtime claim.
- `O/` = `analysis/motion_blur/evidence/original/`. `D[a,b)` = fresh, hash-checked original instructions obtained with `python -B analysis/motion_blur/tools/dol_evidence.py disasm 0xa 0xb`; all ranges are local VAs, end-exclusive.
- Fresh `verify`: **PASS**, SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`, 81 ranges / 7,249 instruction words / 3,626 data bytes. Fresh direct-branch enumeration independently confirms all **12** references below (8 setters, 4 getters).
- Names below describe verified operations, not map-derived gameplay intent. Containing functions also perform unrelated work: the documented blur branches are **not complete replacements** for those functions.

## 1. Literal gaps now closed
`data_manifest.json:670-706,839-850` and fresh `data` commands agree; float operations below retain PPC single-precision order.

| VA | Bits | Exact binary32 value / use |
|---|---|---|
| `805F3BCC` | `3F800000` | 1; ramp upper bound / temporary default scale |
| `805F3BD0` | `3F866666` | 1.0499999523162842; installed scale (nominal 1.05) |
| `805F3BD4` | `3F000000` | 0.5; ramp step divisor |
| `805F3BDC` | `00000000` | 0; lower bound / enable threshold |
| `805F3BE0` | `42800000` | 64; ramp alpha multiplier |
| `8051CB58` | `3F7FF972` | 0.9998999834060669; interpolation snap threshold |
| `8051CB5C` | `3F800000` | 1; interpolation complement baseline |
| `8051BFA4` | `38D1B717` | 0.00009999999747378752; progress epsilon |
| `8051BFA8` | `3F800000` | 1; progress fallback / upper clamp |

`8051CB60` holds **binary64** `4330000000000000` (= 2^52), the unsigned-byte conversion bias, not an f32 parameter. Snap threshold is a separate literal from the complement baseline 1; do not replace it with 1. Adjacent `805F3BD8=-0.5` is not loaded by the blur ramp slices.

## 2. ABI, exact direct references, and producer scope
Let `P={u8 enabled@0,u8 alpha@1,f32 scale@4,u32 aux08@8,u32 aux0c@C}`; extent 0x10, padding +2/+3 not transferred. `B=ptr32[M+40]` is the blur object.
- Getter `8042F084(out=r3,M=r4)` copies B's fields. If B is absent, it writes enabled=alpha=0, scale=1, but copies **uninitialized local stack words** into aux08/aux0c (`O/8042F084_MotionBlur_GetParameters.asm:5-29`).
- Setters use `(M=r3,source=r4)`, return without writing if B is absent; F114 additionally accepts null source as a no-op, F0E8 does not. No manager-null guard is supplied.
- Helpers copy only the five fields; no history validity write, allocation, float/byte clamping, or reset. Independently checked `O/8042E9C4_MotionBlur_CopyParameters.asm:5-15`, `8042E9F0_MotionBlur_AssignParameters.asm:5-15`, `8042EA1C_MotionBlur_AssignParametersIfPresent.asm:5-17`, and both setter-wrapper exports:5-15.

| Call instruction | Target | Exact immediate input/output |
|---|---|---|
| `800A356C` | `8042F114` | SP+10: `(0,128,1.05,?,?)` |
| `800A375C` | `8042F114` | SP+10: `(1,128,1.05,?,?)` |
| `800A481C` | `8042F114` | SP+8: ramp-derived enabled/alpha, same scale, unknown aux words |
| `80204720` / `8020475C` | `8042F084` / `8042F0E8` | getter SP+30; copy to SP+40, change enabled only to 0 |
| `802055E8` / `80205624` | `8042F084` / `8042F0E8` | getter SP+20; copy to SP+48, change enabled only to 0 |
| `80206320` / `8020635C` | `8042F084` / `8042F0E8` | getter SP+30; copy to SP+40, change enabled only to 0 |
| `8043A908` | `8042F084` | getter SP+1C copied into controller E+14C (saved prefix) |
| `80439FD8` | `8042F0E8` | SP+14: controller's target or interpolated prefix |
| `8043A544` | `8042F0E8` | **E+15C target**, not E+14C saved prefix |

`?` means actual machine-stack bytes are not initialized by that producer, not zero and not a recovered coordinate. Direct-setter enumeration does not enumerate arbitrary computed/virtual calls. The blur constructor separately copies an optional prefix through `8042EA1C` at `8042F048`.

## 3. Fixed enable/disable and their immediate dispatch
- Complete containers: `[800A3528,800A35D8)` and `[800A3714,800A37A4)`. Setter inputs are unconditional on entry; see corresponding `O/800A3528_MotionBlur_DisableFixedParameters.asm:5-22` and `O/800A3714_MotionBlur_EnableFixedParameters.asm:5-23`. Later non-blur branches cannot retroactively gate those writes.
- Wrappers `[800A2E18,800A2E3C)` / `[800A2E3C,800A2E60)` load their object's +28 pointer and call the disable/enable containers at `800A2E28/800A2E4C`; they introduce no condition.
- Enable's immediate caller is `8007BC8C`. In `D[8007BB8C,8007BCA0)`, let N be incoming self: the path requires `(u32[N+20]&8)!=0` **or** `800ABF30(N+24)==5`, then `s32[N+1C]==1`, then `(u32[N+20]&4)!=0`. It passes `ptr32[ptr32[N+4]+23C]` to the enable wrapper. `80014E34-80014E50` independently proves these indexed-bit tests.
- In `[8007C0AC,8007C184)`, the branch at `8007C140` selects **fixed disable** (`8007C15C -> 800A2E18`) when N+20 bit 3 is clear. If set, it instead calls `800A2DF0 -> 800A34C8` to construct a new ramp. This is a control-word distinction, not proof of a named gameplay mode.

## 4. Ramp: strength, direction input, and immediate actor conditions
- Let T be the ramp task and C the wrapper's +28 controller. `[800A34B0,800A34C8)` sets `byte[ptr32[C+30]+34]=1` if that task exists. `[800A34C8,800A3528)` first marks any old task likewise, allocates 0x40, then constructs a **new** task through `800A4AB4` (`800A3504-3510`). Allocation failure supplies no new ramp.
- Constructor `[800A4AB4,800A4BB8)` installs vptr `80522A78` at T+18; raw table word `80522A84=800A4830` proves its +0xC update slot. It saves T in the supplied owner slot, writes **T+34=0, T+38=0** (`800A4B08-4B18`), not the current blur alpha. `8006D83C-8006D87C` copies the actor reference's first word into T+28; other constructor effects are outside this blur slice.
- `[800A4830,800A492C)` receives delta in **f1**; no velocity/camera value is read to calculate the ramp. For x=f32[T+38], step=`FDIVS(f1,0.5)`: if T+34 !=0, store `FSUBS(x,step)`, then clamp only LT to 0 (`4850-4878`); otherwise store `FADDS(x,step)`, then clamp only GT to 1 (`487C-48A0`).
- **Correction to `11_decompiler.md:494`:** `800A489C` is `40810008` (`ble`, branch when CR0.GT is clear), so an unordered comparison **skips** the upper clamp. The lower clamp also leaves unordered unchanged. These statements assume comparison execution returns normally, not enabled FP traps.
- It calls `[800A47B0,800A4830)` at `800A48C4` after updating x. Enabled is `!(CR0.LT || CR0.EQ)` for `FCMPO(x,0)` (`47D8-47EC`): finite x>0 enables; NaN selects 1. Alpha is `low8(FCTIWZ(FMULS(64,x)))` (`47FC-4810`); scale is the fixed 1.05 bits.
- No byte saturation: `fctiwz` then `stfd/lwz/stb` preserves PPC conversion/low-byte behavior, including its non-C invalid-conversion contract. For finite x in [0,1], alpha is trunc(64*x), **0..64**, unlike fixed enable's **128**. For 0<x<1/64, enabled=1 but alpha=0 still suppresses blur. Do not describe this task as interpolating from the current prefix.
- For finite nonnegative deltas from the initialized interval, T+34=0 strengthens up to alpha 64; T+34!=0 weakens to zero. Negative/nonfinite deltas are not rejected. At x<=0, the later ordered comparison sets bit 0 of u16[T+4] (`800A48F8-4914`); retirement timing is not proved.
- Immediate decrease requests: both `800AF68C` and `800AF988` call `800A2DCC -> 800A34B0`. `8006CEA0-8006CEC4` proves actor flag indexing: flag 0x15 = actor+A8 mask `00200000`; flag 0x22 = actor+AC mask `00000004`.
- In `[800AF60C,800AF6A4)`, actor=`ptr32[self+28]`; the decrease wrapper is called iff actor flag **0x15 is set** and either `f32(s32[8007993C()+70C]) <= 0` or actor flag **0x22 is clear**. The signed conversion bias at `805F3D20=4330000080000000` and threshold `805F3D18=00000000` were freshly dumped. No meaning for that counter beyond the instruction-derived test is asserted.
- In the message arm `D[800AF924,800AF998)`, `u16[message+4]==0x106` plus actor flag 0x15 set requests decrease and ORs 2 into message+7. The ramp constructor itself sets actor flag 0x15 via `800A4B24 -> 80076DDC`; the latter's indexed OR/store was independently checked through `80076DF8`. No broader event semantics are inferred.
- Source anchors: `O/800A4830_MotionBlur_RampTaskUpdate.asm:5-42,55-62`, `O/800A47B0_MotionBlur_ApplyRamp.asm:5-32`; the D ranges above independently close their immediate callers.

## 5. Three enable-only disable blocks: real containing ranges
Each block was independently disassembled, including the enclosing entry's dispatch and terminal epilogue. These are **interior blocks**, not three standalone blur functions.

| Containing function / boundary proof | Blur-only block | Immediate path predicate |
|---|---|---|
| `[802046BC,80204968)`; `stwu -60` at entry, `blr 80204964`, next prologue `80204968` | `[80204714,80204760)` | incoming self+4 ==0; `80044E38()==1` |
| `[8020554C,80205E3C)`; `stwu -70`, `blr 80205E38`, next prologue `80205E3C` | `[802055DC,80205628)` | self+1C ==0; `80044E38()==1`; after wrapping subtraction `self+4 -= u32[8058E76C]`, signed self+4 <=0 |
| `[802062AC,80206534)`; `stwu -60`, `blr 80206530`, next prologue `80206534` | `[80206314,80206360)` | incoming self+4 ==0; `80044E38()==1` |

Second dispatch is not guessed: `80205560-5580` indexes table `80544E74`, whose freshly dumped entry 0 is `80205584` (no other entry points there). First/third predicates follow their explicit signed compare trees. `80044E38-80044E5C` returns `u32[8004427C()+14]`; its gameplay meaning is not assigned.
All three invoke `8042659C` **before** the getter. Their local guarantee is to preserve alpha, scale, and aux words **as returned after that call**, not necessarily the state at enclosing-function entry. That helper walks manager entries/calls `80426440`; arbitrary indirect side effects are not collapsed into the prefix-only guarantee.

## 6. Resource/controller construction: input fields and type separation
- **Actual constructor range `[8043A5A4,8043AB68)` (0x5C4)**: `stwu -90` at A5A4; common epilogue AB54-AB64; next unrelated function prologue AB68. Getter/target block `[8043A900,8043A97C)` belongs to this constructor, not a function starting at A900.
- Constructor calls `[8042B734,8042B7C0)` with descriptor D in r4 and manager in r6. Its base `[80426F8C,804270C0)` stores r6 at E+1C, D at E+44, initializes **age E+C=0, flags E+18=1** (`BA4C=0`, independently dumped), and does not establish timer-advance bit 1.
- `8042B790-7A8`: **E+108=ptr32[D+34]**, **duration E+104=f32[D+30]**, **source flags E+FC=u32[D+2C]** (only on active path). These are distinct from E+18's execution flags and from incoming delta f1. No duration unit is established.
- Constructor checks E+18 bit 0, successfully casts E+1C through `803A1AFC`, then dispatches on **s32[ptr32[E+108]+0]** at `8043A774-7A0`. Exactly **kind 1** reaches A900.
- A908 obtains current prefix into E+14C. At A934-A974, target **enabled E+15C = (E+FC & 0x00010000)!=0**; target **alpha E+15D = u8[R+8]**; **scale E+160=f32[R+C]**; **aux E+164/E+168=f32[R+10]/f32[R+14]**, where R=ptr32[E+108]. Resource+4 does not determine blur enabled. The two aux floats are transported/interpolated upstream, not interpreted as draw coordinates here.
- Outer factory type is separate: `80433798-37B8` indexes **s16[D+20]**; raw `8056BE28=804338D4` proves type **8** allocates 0x1F4 and calls A5A4 at `80433900`. Another direct call, `801FA8F8`, is a derived constructor forwarding to it. Neither outer type nor a map name substitutes for resource **kind 1**.

## 7. Common timer/progress update, independently decoded
Whole helper `[8042B63C,8042B6CC)` is `O/8042B63C_EffectParameter_UpdateProgress.asm:5-40`. Define `LT/GT` as the corresponding CR bit from `FCMPO`, and EPS=bits `38D1B717`:

```text
F = u32[E+18]
if !(F & 1) or (F & 4): return                // B63C-B64C; no timer/progress change
if F & 2: E+C = FADDS(f32[E+C], incoming_f1) // B650-B660, only age-advance condition
age = f32[E+C]; duration = f32[E+104]
if GT(age, FADDS(EPS,duration)):              // B674-B67C, strictly GT (not >=)
    u32[E+18] &= ~1; return                  // B680-B68C; old E+100 retained
E+100 = GT(duration,EPS) ? FDIVS(age,duration) : 1 // B690-B6AC
if GT(f32[E+100],1): E+100 = 1               // B6B0-B6C8; no lower clamp
```

Bit 1 clear freezes age, **not recomputation** of progress. Unordered comparisons do not satisfy GT; a NaN duration takes fallback 1, a NaN quotient can survive the upper clamp. `[80439A8C,8043A4B8)` calls this helper **first** at `80439AA0`, then rechecks active bit 0 / inhibit bit 2, manager cast, and kind 1 (`O/80439A8C_EffectGlobalParam_Apply.asm:5-37`). Overshooting duration+EPS clears active and **skips that invocation's setter**; it does not force target or disable B there.

## 8. Exact interpolation and finalization
`O/80439A8C_EffectGlobalParam_Apply.asm:273-345` covers `[80439EBC,80439FE0)`:
- Copy target E+15C into local Q; interpolate **only if CR0.LT** for `FCMPO(t,0.9998999834060669)` (`80439EF0/EF4`). Otherwise, including unordered absent trapping, submit the target unchanged.
- Interpolating enabled is `(target.enabled!=0 || saved.enabled!=0) ? 1 : 0`. Endpoint alpha is **not** replaced with zero when its enable byte is zero.
- With `c=FSUBS(1,t)`, scale and each aux float use **`FMADDS(target,t,FMULS(saved,c))`**, at `FA4-FCC`. Preserve the fused operation and rounded saved product; do not substitute `saved+(target-saved)*t`.
- Alpha equal at both endpoints is copied unchanged. Otherwise unsigned bytes are exactly converted using the 2^52 bias, mixed in that same single/fused order, then `FCTIWZ`/low byte (`F30-F80`). No saturation or lower/upper weight clamp is added here.
- Thus at ordinary t in [0,snap), alpha blends toward the target (with byte truncation); either endpoint enabled keeps the gate on. If saved enabled!=0 and target enabled==0, disable is deferred until the snap branch; if both are disabled, the gate stays off. Target alpha 0 can separately suppress drawing earlier. Alpha increase/decrease and scale change are independent controls.
- Finalizer/destructor **`[8043A4B8,8043A5A4)`** checks nonnull self, successful manager cast, and resource kind 1, then passes **E+15C** at `8043A540/544` (`O/8043A4B8_EffectGlobalParam_Destroy.asm:5-49`). It does **not** test E+18 active/inhibit or t in this arm and does **not** restore E+14C. This target write occurs only if finalization executes; expiry alone is not proof that it already did.

## 9. Disable versus reset; genuine remaining gaps
- Draw tests enabled and alpha independently (`O/8042E254_MotionBlur_Draw.asm:10-15`). Changing alpha/scale does not reset capture history. Fixed disable writes alpha 128 and scale 1.05; the three snapshot blocks instead leave both unchanged. A resource target/finalizer can write any supplied alpha byte and scale.
- **No setter-side history reset:** copy helpers never touch B+78 or B+74. Constructor initializes B+78=0 at `8042EB90`; each actual SaveScreen invocation writes B+78=0 **before** enabled/alpha checks (`O/8042E838_MotionBlur_SaveScreen.asm:8-17`). Disabling/re-enabling without a save is not an explicit history invalidation or pixel clear.
- Genuine gaps: live delta values/units, writers/scheduling of E+18 timer/inhibit bits, concrete resource instances/flags, ordering/arbitration among these producers, computed calls, callback side effects in enclosing non-blur routines, and finalizer scheduling. No boost/speed/cutscene identity or visual/runtime behavior is inferred from map names or these numeric predicates.
- Aux values left unwritten by fixed/ramp producers or absent-B getter remain machine indeterminates. FPSCR/trap state and out-of-range/nonfinite conversion outcomes require an explicit PPC model or captured state; they are not defined by host C casts.
- Prior literal gaps in `09_dataflow.md:99-101` / `11_decompiler.md:466-527` are closed here; the unordered upper-clamp statement in role 11 is corrected above. Those owned reports were not edited.

## 10. Reproduction / change ledger
- Wrote only `analysis/motion_blur/evidence/12_closure.md`; no DOL/config/source/export edits, builds, emulator runs, or captured traces.
- Read-only commands used: workspace/evidence listings; `python -B analysis/motion_blur/tools/dol_evidence.py verify`; `xrefs` for F084/F0E8/F114 and the immediate wrappers/constructors; `pointers` for 800A4830/800A2E18/800A2E3C; bounded `disasm` for every D range above and the three entry/epilogue windows; `data` for the constants and dispatch/vtables explicitly cited above. No `export`, `all`, or `analyze_dol.main()` was run.
