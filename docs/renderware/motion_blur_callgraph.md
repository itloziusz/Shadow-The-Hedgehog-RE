# Motion blur — original call graph and execution order

## Scope and notation

This is the blur-relevant graph of the original `main.dol`, SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`. Addresses and offsets below are hexadecimal; ranges are end-exclusive. **D** denotes a direct branch/call, **I** an indirect edge resolved through actual installed table words, and **L** a bounded supplementary instruction window documented in a closure report rather than a separate exported function. These labels establish static reachability, not a captured invocation.

The [function index](motion_blur_functions.md) and [address atlas](motion_blur_original_addresses.md) enumerate all **121 exported original ranges**. [03](evidence/03_closure.md) supplies the top-down proof; [04](evidence/04_closure.md) independently works backward from FIFO copy/blend commands; [05](evidence/05_closure.md) follows image ownership; [12](evidence/12_closure.md) closes immediate parameter references. The current manifests verify **10,075 instruction words and 3,908 file-backed data bytes in 80 data records**. Supplemental windows are not silently counted as additional exports.

## 1. Recurring loop to the two effect entries

```text
800510C0 Main_EventLoop
  D 800511EC -> 80051978(event 0x12, argument 0)
    D 800519A8 -> 80048EB8 Application_DispatchEvent
      I table[8051E5E0 + 4*0x12] = [8051E628] = 80049078
        D case 80049078 -> 801E2210 Frame_DriveTasks
          D ordinary 801E23A4 -> 801EB680 TaskManager_Execute
            D 801EB698 -> 8004ECAC(common root K+0)
              group 5 = *(K+0x18), named "Render"
                Start   R+5C
                Div     R+60 -> I Exec 801E4EF0
                  PostGlare: level 11, R+2C
                    DrawBlur -> I Exec 801D27FC
                      D 801D2808 -> 800A35D8, returns manager M
                      D 801D280C -> 8042F16C(M)
                        D 8042F184 -> 8042E254(B=*(M+40))
                Static  R+64 -> I Exec 801E4E40
                  early PostEffect R+58
                    SaveScreen -> I Exec 801D2764
                      D 801D2770 -> 800A35D8, returns M
                      D 801D2774 -> 8042F140(M)
                        D 8042F158 -> 8042E838(B)
                End     R+68
```

`80049078` is a **case inside `[80048EB8,800490B0)`**, not a function. `801D280C` is a **BL inside DrawBlurTask_Exec**, not an entry. The manager getter returns to each task; it does not itself call the manager bridge. No single relocation delta from the reference map is used.

### Loop and scheduler proof

| Original edge/site | Exact condition or operation |
|---|---|
| `800511E4..800511FC` | Repeats event `0x12`, then calls `8032D444`, while `u32[80576DB0+C]==0`. The prefilter `80050FA8` returns 2 for this event. |
| `801E239C -> 8001C4F8` | Obtains task manager **K=80571C6C**; normal scheduler call supplies delta from `805F0A58`. |
| `8001C524 -> 801EB734` | Lazy construction caches K at `805EF0D0`; root stored at K+0. |
| `801EB76C -> 801EB53C` | Appends six groups in raw order `0,1,2,3,4,5` from `8053ED6C`; pointers stored at K+4+4*index. `[8053ED34]=805F7150` names group 5 `Render`. |
| `801EB614` | Returns `*(K+4+4*r4)`. Group 5 is K+18, not the scheduler's common root itself. |
| `801EB680` | Null-checks K+0, then calls the common child walker. Render is reached by recursion, not a separate group-5-only loop. |
| `8004F014` | Task construction tail-appends to parent+14 using sibling+0C and the head's tail link+8. |
| `8004ED98..8004EDA4` | Normal indirect Exec: `task+18 -> vtable+0C -> bctrl`. |
| `8004EDB0`, `8004EDB4` | Recurse into that task's children, then follow next sibling+0C. This proves phase order. |
| `8004ED88 -> 8004EDF0` (L) | Profiling wrapper uses the **same Exec slot** at `8004EE44..50`; profiling does not reverse ordering. |

For task flags `F=u16(task+4)`, low nibble `F&0xF` normally skips the task/subtree. Bit 0 can instead lead to deferred destruction if pause is clear; bit `0x20` changes that deletion path. With the low nibble clear, pause `byte[805EF27C]!=0` suppresses Exec when `F&0x100` is clear **but still recurses into children**. Group, phase, level, DrawBlur and SaveScreen construction set `0x100`. Recursion depth `805EF270` and profile flag `805EF274` are not image-age counters. See [03 §3](evidence/03_closure.md).

### Frame-driver alternatives

The ordinary edge above is not a one-VI-frame guarantee. With `805F0A54==0`, the live object's virtual +0C can skip traversal, or allow scheduler call `801E2278` before setting the initialized flag. The transition path tests virtual +10/+14; it can self-loop at `801E23E8`, or destroy and make **three extra scheduler calls at `801E241C/2428/2434`**. Pause requests are serviced by `801E41D4` after the ordinary scheduler call. These are actual alternatives within `[801E2210,801E2480)`, not predictions of their frequency.

## 2. Registration and multicamera order

Render-level singleton **R=80574384** is separate from K and M. Its constructor is `801E4B0C`, reached through `800461CC`; getter `800461A0` returns it.

| Phase / R field | Construction BL -> constructor (L) | Installed vtable; Exec word -> target |
|---|---|---|
| Start / +5C | `801E4B4C -> 801E5130` | `8053EAA8`; `[8053EAB4]=801E5100` |
| Div / +60 | `801E4B88 -> 801E506C` | `8053EA70`; `[8053EA7C]=801E4EF0` |
| Static / +64 | `801E4BC4 -> 801E4EA8` | `8053EA54`; `[8053EA60]=801E4E40` |
| End / +68 | `801E4C00 -> 801E50B8` | `8053EA8C`; `[8053EA98]=801E50B4` (`blr`) |

The ordinary level table is at **`804D24EC`**, with record stride `0x14`. Record 11 at **804D25C8** is `{804D23D8,0,0,0,1}`. Selector **804D25D8=1** selects `R+5C+4=R+60`, Div. Its name is **RenderLevel_PostGlare**, not index-9 GlareWorld. Draw constructor loads R+2C at `801D2078` and appends at `801D207C`.

Save's parent is created **before** the level loop: `801E4C30 -> 8004F014`, under R+64, stored at R+58. Record 22 supplies its PostEffect name; that record's -1 selector is not used as an index. Save constructor appends there at `801D20FC`. Draw owns a Save pointer at task+28, but **Save is not Draw's scheduling child**.

| Live Div mode at `8057E7C0` | Div's own work | Outer walk / later Static |
|---|---|---|
| 0 | Selector 0; view setup calls `8027211C` / `80271F9C`; no internal child traversal. | One ordinary visit of Div children. |
| 1 | Select view 1; **`801E4FA0 -> 8004ECAC(this,delta)`**; then select view 2 and advance view index. | A second visit of Div children after Exec returns. |
| 2 | Select view 3; **`801E501C -> 8004ECAC(this,delta)`**; then select view 4 and advance view index. | The same second visit. |
| Other | Falls through without the above setups. | No stronger selected-camera guarantee. |

Static Exec `801E4E40` clears selector/view index, calls `8027211C(0)` at `801E4E70` and `80271F9C` at `801E4E90`, then returns for ordinary child traversal. Thus modes 1/2 permit **two Draw callbacks using the same pre-save H before one Save callback**; each core guard can still suppress its work. There is no per-camera history table.

## 3. Setup and destruction edges

| Site -> target | Blur-relevant effect |
|---|---|
| `801D24DC -> 801D1A1C` | Effect bundle construction; upstream setup window `80178520..80178548` is supplementary, not renamed as a new function. |
| `801D1A60 -> provider virtual +8` | Provider-specific setup precedes scratch/history construction. |
| `801D1A68 -> 80433638` | Allocate shared scratch pair M+34 using construction camera **view** dimensions. |
| `801D1A80 -> 8042F20C` | Allocate B (`803A1380`, 0x13C), call `8042EA50` at `8042F23C`, install allocation at M+40 even after an early constructor return. |
| `8042F00C/F024 -> 8048AEC4`; `8042F034 -> 8048AE14` | History empty view, half-**root**-size root, then attachment. |
| `801D1AE4 -> 803A1380`; `801D1AF0 -> 801D2058` | Allocate 0x30-byte Draw task and construct it. |
| `801D20A8 -> 803A1380`; `801D20B4 -> 801D20D8` | Allocate 0x28-byte Save task; retain at Draw+28. |
| `[8053DC80] -> 801D2788` | Draw task destructor sets Save's deferred-deletion bit 0; then base task destructor, optional signed-low16 delete. |
| `[8053DC64] -> 801D270C` | Save task destructor; does not destroy the history object. |
| `801D19B8 -> 8042F198` (L) | Destroy distinct H root, then view, free B, clear M+40. |
| `801D19D8 -> 804335C8` (L) | Shared scratch teardown occurs after history teardown. |

The neighboring PostGlare task is registered at `801D1AD4`, DrawBlur at `801D1AF0`, and DrawScreenColor at `801D1B0C`. Save is appended before that later Static color task and before later Static levels. Consequently its capture is not necessarily the final presented EFB. Adjacent effects are excluded from this graph except as ordering/ownership boundaries.

## 4. Camera service and installed callback chain

```text
801E28D0..801E28F0 (L): allocate provider 0x10, construct 801E293C
  final vptr 8053E93C; 801E28EC -> 8042B630 -> [805E25C8]
8042B620 reads [805E25C8]
  -> 803A1AFC(object,0,target=8056BCE4,source=8056BCD0,0)
  -> I provider+24: [8053E960]=80028730
       D 8002873C -> 8000DCC0
         lazy camera manager 8056FF1C, cache 805EF068
       returns [camera manager+0], the RwCamera pointer
```

PJS RTTI **805E9D98** points to base list **8053E920**, containing RW3 descriptor **805E9D88** at adjustment 0. Its type-name bytes match core target descriptor **8056BCE4** (name at **8051C318**); `803A1AFC` compares names, not just descriptor addresses. This closes the normal cast despite duplicate descriptors. Scratch uses the parallel descriptors `8056BDB4/BDC8`. Later service mutation and live camera selection remain external contracts, not unread vtable gaps.

Construct, Draw and Save each acquire a camera separately. Draw's service/cast/virtual sites are `8042E280/E29C/E2B4`; Save's virtual site is `8042E8A0`. Camera calls are:

| Adapter / installed route | Actual indirect slot |
|---|---|
| `8048652C` BeginUpdate | Camera+18; `bctrl 80486540` |
| `80486504` EndUpdate | Camera+1C; `bctrl 80486518` |
| `80486AA8` camera construction | Installs `804863A4` begin / `80486340` end. |
| `80461C98` world plugin construction | Saves callbacks at extension+10/+14 and installs `80461B6C/80461BB8`; wrappers forward them. Extension offset is `[805F25B0]`. |
| Default begin `804863A4` | Sets engine current camera, syncs frame, then engine+4C -> `804956F4`. |
| Default end `80486340` | Engine+70 -> `80495CAC`; clear current camera on successful path. |

## 5. Device installation and bottom-up GPU convergence

Engine open `[80487254,804874FC)` (L) gets descriptor **8056F50C** through `804872B0 -> 804960F0`. System callback `80494EDC` indexes **8056F494**. **Operation 4**, called at `80487320`, copies the descriptor to **engine+10** via case `804950A0`. This proves state setter/getter/primitive slots, rather than merely observing similar code nearby.

**Operation 11 (0xB), not 9**, at `804873BC`, supplies `(engine+48,0,count=29)` to case `804954F8`. The installer copies **27 records at 8051D400**, fills default slots, then writes `base+4*index`. Its guard tests the first copied index, not each current index; all actual records fit this count. This installer is a case window, not another manifest function.

| Raw source | Installed engine slot -> target |
|---|---|
| Device `8056F51C` | +20 -> `80498954` SetState |
| Device `8056F520` | +24 -> `804984C4` GetState |
| Device `8056F52C` | +30 -> `80491E80` Im2D primitive |
| Standard `(1,804956F4)` / `(10,80495CAC)` | +4C / +70 camera begin/end |
| Standard `(4,80497CA8)` / `(5,80498058)` | +58 / +5C raster create/destroy |
| Standard `(12,804981B8)` | **+78**, not +94, subraster callback |
| Standard `(8,8049819C)` | +68 raster binding |

```text
8042E254 Draw
  I engine+24: seven Get requests at E2E8/E308/E328/E348/E368/E388/E3A8
  D E3BC -> 8048AB7C; calculate current geometry
  D E60C -> 8048652C (failure exits)
  I engine+20: E634 bind Hroot, E654..E714 state requests
  I E738 engine+30 -> 80491E80(4,B+14,4)
  I E758..E818 engine+20: six implemented restores + ignored ID12
  D E820 -> 80486504

8042E838 Save
  D E8C0 -> 80496208(Sroot,0)
  D E8C8 -> 8048652C; if successful:
    I E8F0 bind Sroot, E910/E930 replacement blend, E954 primitive(4,B+7C,4)
    D E95C -> 80486504
  D E968 -> 80496208(Hview,0)
  D E970 -> 8048652C; if successful:
    I E99C primitive(4,B+DC,4), no rebind
    D E9A4 -> 80486504
  E9AC stores B+78=1 regardless of the optional Begin results
```

Bottom-up: **`803975AC GXCopyTex` has one direct caller, `804963B8` inside `80496208`**. Of the latter's 17 direct clients, only **8042E8C0/E968** belong to this blur capture. Blend **80399D44** is reached by **80498DA0/80498E10** in SetState, with table factors RW5/6 -> GX4/5. Other clients of these shared APIs do not enlarge the blur family.

Im2D chain: **80491EA4 -> 8049148C**, **804915B0 -> 80498854**, **8049887C -> 8049AC88(T,0)**. Realization uses root-plugin pixels in `80397E34` at `8049B2F8`, then `8049B524 -> 8039843C` or `8049B538 -> 803982C0`. **80491EC0 -> 80396934(0x98,0,4)**, followed by four FIFO vertices at **CC008000**; **80492438 -> 80491774** restores conditional viewport/scissor and same-call projection. No extra radial-tap loop or CPU image upload is present.

## 6. Parameter edges and boundaries

There are **12 enumerated direct get/set references** (four getter, eight setter). The complete numerical flow is in [dataflow](motion_blur_dataflow.md) and [temporal state](motion_blur_temporal_state.md).

| Original call site(s) | Target | Producer/consumer |
|---|---|---|
| `800A356C`, `800A375C`, `800A481C` | `8042F114` | Fixed disable, fixed enable, ramp submission |
| `80204720 / 8020475C` | `8042F084 / 8042F0E8` | Disable container A, preserve returned prefix except enable |
| `802055E8 / 80205624` | Same | Disable container B |
| `80206320 / 8020635C` | Same | Disable container C |
| `8043A908` | `8042F084` | Resource kind-1 construction snapshot |
| `80439FD8` | `8042F0E8` | Kind-1 interpolation/target submission |
| `8043A544` | `8042F0E8` | Finalizer writes target, not starting snapshot |

Getter r3 is output storage, r4 manager; setters use r3 manager/r4 source. F114 tolerates absent source via `8042EA1C`; F0E8 does not. Both tolerate absent M+40, not a null manager. Each disable block first calls **8042659C** and performs **three distinct manager lookups**; collapsing them would lose an actual side-effect/alias boundary. `80439AA0 -> 8042B63C` runs before Apply rechecks active/inhibit, so expiry can suppress the setter entirely.

Source availability does not replace this original graph: integrated backend selected paths are **not installed as generic callbacks**, input continuations are **not executable PC-resume machinery**, and SDK/allocator/RTTI bindings remain external. The installed graph is statically closed under its actual tables; runtime mutation, fault ABI and GPU output remain [explicit limits](motion_blur_unknowns.md).
