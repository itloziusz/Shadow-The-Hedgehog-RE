# 01 — Binary / DOL mapper

## Scope, result, and verification boundary

Target: `<historical-game-root>/sys\main.dol`. Scope is the DOL address model, startup/SDA bases, reference-map correspondence, and the motion-blur code/data anchors needed by the other investigators. This is not a gameplay audit, a GX reconstruction, or a claim that one contiguous code range is the complete subsystem.

**Major conclusions:**

- The live target is **5,773,024 bytes (`0x005816E0`)**. An independently executed `Get-FileHash -Algorithm SHA256 main.dol` returned **`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`**, matching `AGENTS.md:4`, the existing inventory, and the baseline. No target bytes were changed.
- Entry is **`0x80003154` / file `0x00000154`**. Startup establishes **`r2 = 0x805FA780`** and **`r13 = 0x805EC500`**. These are decoded from the actual hexadecimal instruction words in the existing local disassembly, not from reference-map labels.
- The header-derived **BSS envelope `[0x8056FE00, 0x805FC5EC)` overlaps initialized `.data6` and `.data7`**. The startup zero-fill table describes three separate, non-overlapping zero ranges and excludes those initialized sections. A blanket NOBITS/BSS interpretation of that envelope is wrong for extracting initialized SDA values.
- The reference map is **not target-address aligned and has no single valid relocation delta**. Corroborated local-minus-reference deltas are `0` at startup, **`+0x3A0`** for the relevant PJS tasks, **`+0x1050`** for the renderer block, and **`+0x10B0`** for the task-name/RTTI-name string cluster. Names such as `cCharacter::UpdateBlinking` and `homebutton::MotorCallback` must not be inherited by these local wrappers.
- Main anchors: task constructor **`0x801D2058`**, candidate task execution entry **`0x801D27FC`**, its call site **`0x801D280C`**, manager wrapper **`0x8042F16C`**, draw body **`0x8042E254`**, and paired save-screen body **`0x8042E838`**. In particular, **`0x801D280C` is a `bl` instruction inside a function, not another function entry**.

**Important limitation:** background permissions denied both attempted `python -B -c` probes, the supplied bounded `python -B motion_blur_analysis/ppc_tools.py disasm ...` command, and Bash `sha256sum`. The approved PowerShell checksum command did work. Consequently the live hash and filesystem size were verified directly, but a fresh binary-header decode, fresh Capstone run, and raw vtable/RTTI-word dump were not possible in this worker. Section fields below come from the existing header-derived inventory and are independently cross-checked against startup table words and address arithmetic. Every instruction citation below is a bounded **read of the existing** `full_disassembly.txt`, not a claim of a newly generated disassembly. Foreground follow-up commands are provided at the end.

## Evidence sources and method

Paths below are relative to `sys/`:

- `AGENTS.md:1-10`: read first; immutable-target policy, expected identity, tool restrictions.
- `motion_blur_analysis/analyze_dol.py:5-24`: bytes import and big-endian section mapping; `:26-49` is the prohibited writing main routine and was not executed.
- `motion_blur_analysis/inventory.json:1-80`: existing DOL-header inventory. Identity is at `:2-7`; section records at `:9-77`.
- `analysis/motion_blur/evidence/binary_baseline.json:2-7`: existing baseline, including its recorded PowerShell checksum method. This worker also ran that command independently.
- `motion_blur_analysis/ppc_tools.py:6-30`: decoder, branch-xref and pointer-search behavior; the writing `all` branch at `:31-36` was not executed.
- `motion_blur_analysis/review_refs.py:7-12` and `review_vtables.py:2-4`: candidate address lists, not proof of contents. These scripts were read, not executed.
- In the remainder of this report, **FD** means `motion_blur_analysis/full_disassembly.txt`, **MAP** means `motion_blur_analysis/reference_GUPE8P.map`, and **STR** means `motion_blur_analysis/relevant_strings.tsv`. All cited numbers after those names are exact source line numbers.

Method:

1. Check live file size with read-only directory metadata and compute live SHA-256 with PowerShell.
2. Convert inventory decimals to hexadecimal, independently calculate all section ends/file ends, and check the layout against the startup ROM-copy and zero-fill tables. Bash `printf` with numeric-only arithmetic was used as a calculator; it did not open or write files.
3. Read only bounded FD line ranges. For `.text0`, `line = (VA - 0x80003100)/4 + 1`; for `.text1`, `line = (VA - 0x80006840)/4 + 1` in this existing artifact. The section transition is explicitly verified at FD:2368-2369. Every requested range was checked against the printed VA, rather than trusting line arithmetic alone. The 52 MB file was not searched with grep.
4. Decode `lis`/`ori` and `lis`/signed-`addi` address formation, direct branch words, constructor-installed pointers, startup copy/zero tables, and function boundaries. Compare these facts with MAP labels/lengths only afterward.
5. Keep local VA, local DOL file offset, and reference-map VA as distinct address namespaces. MAP's repeated address columns are not DOL file offsets. `.textN`/`.dataN` below are generated DOL slot names, not recovered original linker names.

All ranges in this report are **half-open `[start,end)`** unless explicitly stated otherwise.

## 1. DOL sections, entry, and file-offset mapping

DOL header layout used by the existing parser: 18 file offsets at `0x000`, 18 load addresses at `0x048`, and 18 sizes at `0x090`; the first 7 slots are text and the next 11 data. BSS address, BSS size, and entry are big-endian words at file offsets `0x0D8`, `0x0DC`, and `0x0E0`. See `analyze_dol.py:8-14,27`.

| DOL slot | File range | RAM VA range | DOL size | Startup copy-table size |
|---|---|---|---|---|
| text 0 / `.text0` | `0x00000100–0x00002600` | `0x80003100–0x80005600` | `0x00002500` | `0x000024E8` |
| text 1 / `.text1` | `0x00002600–0x004A4520` | `0x80008D40–0x804AAC60` | `0x004A1F20` | `0x004A1F08` |
| data 0 / `.data0` | `0x004A4520–0x004A6440` | `0x80005600–0x80007520` | `0x00001F20` | `0x00001F08` |
| data 1 / `.data1` | `0x004A6440–0x004A7C60` | `0x80007520–0x80008D40` | `0x00001820` | `0x00001814` |
| data 2 / `.data2` | `0x004A7C60–0x004A80E0` | `0x804AAC60–0x804AB0E0` | `0x00000480` | `0x0000046C` |
| data 3 / `.data3` | `0x004A80E0–0x004A8100` | `0x804AB0E0–0x804AB100` | `0x00000020` | `0x0000000C` |
| data 4 / `.data4` | `0x004A8100–0x0051A520` | `0x804AB100–0x8051D520` | `0x00072420` | `0x00072418` |
| data 5 / `.data5` | `0x0051A520–0x0056CE00` | `0x8051D520–0x8056FE00` | `0x000528E0` | `0x000528C8` |
| data 6 / `.data6` | `0x0056CE00–0x00577920` | `0x805E4500–0x805EF020` | `0x0000AB20` | `0x0000AB20` |
| data 7 / `.data7` | `0x00577920–0x005816E0` | `0x805F2780–0x805FC540` | `0x00009DC0` | `0x00009DB8` |

Inventory evidence: `inventory.json:9-77`. Cross-check: the ten `(sourceVA,destinationVA,size)` triples at **`0x80005544–0x800055BC` / file `0x00002544–0x000025BC`**, followed by a zero triple, are preserved as raw words at **FD:2322-2354**. Their order is text0, data0, data1, text1, data2 through data7. Both VA fields agree in each triple; they are RAM addresses, not file offsets. The startup loop skips copying when source and destination agree (FD:151-171).

All ten reported DOL sizes are the copy-table size rounded up to `0x20`. The initialized file intervals are adjacent from `0x00000100` to EOF `0x005816E0`, have no mutual overlap, and their RAM intervals have no mutual overlap. Header bytes `[0,0x100)` do not have a mapped VA. Text slots 2-6 and data slots 8-10 are omitted by the nonzero-size parser; direct inspection of their unused offset/address words remains a foreground check.

Address conversion within a selected section is:

```text
fileoff = section_file_start + (VA - section_VA_start)
VA      = section_VA_start + (fileoff - section_file_start)
```

Useful piecewise `VA - fileoff` constants:

| Local section(s) | `VA - fileoff` |
|---|---|
| `.text0` | `0x80003000` |
| `.text1` (all task/renderer functions below) | `0x80006740` |
| `.data0`, `.data1` | `0x7FB610E0` |
| `.data2` through `.data5` (task names, vtable addresses, renderer pool) | `0x80003000` |
| `.data6` | `0x80077700` |
| `.data7` | `0x8007AE60` |

Do not apply `VA - 0x80003000` indiscriminately to code, initialized SDA, or BSS addresses.

Header-derived entry is `0x80003154` (file `0x00000154`), consistent with the first startup calls:

```text
VA          fileoff     raw word   decoded instruction
80003154    00000154    4800015D   bl 0x800032B0
80003158    00000158    480002A9   bl 0x80003400
8000316C    0000016C    480001D5   bl 0x80003340
```

Evidence: `inventory.json:5`; FD:22-28. MAP:21-24 happens to agree at these startup addresses.

## 2. BSS envelope versus actual zero-filled storage

Header-derived fields: BSS base **`0x8056FE00`**, size **`0x0008C7EC`**, end **`0x805FC5EC`** (`inventory.json:6-7`). This is an enclosing span, not a declaration that every address inside it lacks file bytes.

The zero-fill table at **`0x800055C8` / file `0x000025C8`** gives:

| Table entry VA / fileoff | Zero start | Size | Zero end | Conventional interpretation |
|---|---|---|---|---|
| `0x800055C8 / 0x000025C8` | `0x8056FE00` | `0x00074700` | `0x805E4500` | main BSS |
| `0x800055D0 / 0x000025D0` | `0x805EF020` | `0x0000375C` | `0x805F277C` | SDA zero storage / `.sbss` |
| `0x800055D8 / 0x000025D8` | `0x805FC540` | `0x000000AC` | `0x805FC5EC` | SDA2 zero storage / `.sbss2` |

Evidence: **FD:2355-2362**. The zero-pair terminator is at `0x800055E0`; the loader loop reads `(address,size)` and calls a zeroing routine at `0x800033D8` (FD:172-185). The table words must be read as data; FD also prints incidental instruction mnemonics for them because they reside in the first DOL text slot.

Consequences:

- `.data6` `[0x805E4500,0x805EF020)` and `.data7` `[0x805F2780,0x805FC540)` are **file-backed even though both lie inside the header BSS envelope**.
- There is a four-byte alignment gap `[0x805F277C,0x805F2780)` between the second zero range and `.data7`; it is not in a loaded section or an explicit startup zero entry.
- Total explicit zero-fill is `0x00077F08`. The envelope additionally contains `0x000148E0` initialized bytes plus that four-byte gap, totaling `0x0008C7EC`.
- True zero-range addresses have no DOL file offset. The helper correctly checks loaded sections first (`analyze_dol.py:20-24`). Runtime pointers stored there must not be reconstructed as original file words.
- **Existing analysis-ELF caveat:** `analyze_dol.py:44` constructs one `.bss` NOBITS section from the entire header envelope. Thus that generated ELF's `.bss` overlaps initialized sections. Do not let an ELF import erase/hide `.data6` or `.data7`, or infer zero initial values from the overlapping section label. No ELF was regenerated or modified here.

## 3. SDA bases from startup instructions

The register initializer beginning at `0x800032B0` reaches the following four words:

| Instruction VA | DOL fileoff | Word | Decode |
|---|---|---|---|
| `0x8000332C` | `0x0000032C` | `3C40805F` | `lis r2,0x805F` (Capstone prints signed immediate `-0x7FA1`) |
| `0x80003330` | `0x00000330` | `6042A780` | `ori r2,r2,0xA780` |
| `0x80003334` | `0x00000334` | `3DA0805E` | `lis r13,0x805E` (signed `-0x7FA2`) |
| `0x80003338` | `0x00000338` | `61ADC500` | `ori r13,r13,0xC500` |

Evidence: **FD:109-144**, especially `:140-143`. Hence:

```text
r2  = 0x805FA780  = .data7 VA start + 0x8000   (SDA2 base)
r13 = 0x805EC500  = .data6 VA start + 0x8000   (SDA base)
```

The low halves above are ORed, not sign-extended additions. By contrast, a later `lis r3,0x8054; addi r0,r3,-0x2388` forms `0x8053DC78`, not `0x8054DC78`.

For an ordinary D-form access, effective address is `(base + sign_extend_16(displacement)) mod 2^32`. The base locations themselves are file-backed: `r13` corresponds to file `0x00574E00`; `r2` to file `0x0057F920`. The conventional section names SDA/SDA2 describe these relationships, not recovered linker symbols.

Directly relevant example: **`0x800A35D8–0x800A360C` / file `0x0009CE98–0x0009CECC`** is the lazy manager getter called by both task wrappers. At `0x800A35E4`, raw word `800D2DD8` loads `r13 + 0x2DD8 = 0x805EF2D8`; `0x800A35F4` stores to it and `0x800A35FC` returns it. This location is in the explicit SDA zero range, **not in `.data6` and not in the DOL file**. Evidence: FD:160615-160627. The follow-on initializer uses guard `r13+0x3129 = 0x805EF629` and static storage at `0x8057798C`, also zero-backed (FD:160628-160639).

The renderer's indirect API-root load forms **`0x805F265C`** with `lis 0x805F; addi 0x265C`, then dereferences it (e.g. `0x8042E2CC–0x8042E2E4`, FD:1089188-1089194). This is another SDA zero-range location (`r13+0x615C`), not an initialized pointer recoverable from a file offset. This report does not assign its runtime pointed-to table or GX call targets.

## 4. Reference-map alignment and region/build limits

The following are **local minus reference** deltas. They apply only to the demonstrated neighborhoods, not to all intervening addresses or an entire region build.

| Anchor | Reference VA | Local VA | Delta | Evidence |
|---|---|---|---|---|
| startup entry | `0x80003154` | `0x80003154` | `0` | MAP:21; FD:22 |
| register initializer | `0x800032B0` | `0x800032B0` | `0` | MAP:22; FD:109-144 |
| ROM-copy / zero-table starts | `0x80005544 / 0x800055C8` | same | `0` | MAP:34363-34364; FD:2322-2362 |
| DrawBlur constructor | `0x801D1CB8` | `0x801D2058` | `+0x3A0` | MAP:15950; FD:470535-470566 |
| SaveScreen constructor | `0x801D1D38` | `0x801D20D8` | `+0x3A0` | MAP:15951; FD:470567-470591 |
| DrawBlur destructor | `0x801D23E8` | `0x801D2788` | `+0x3A0` | MAP:15972; FD:470995-471023 |
| candidate DrawBlur execution wrapper | `0x801D245C` | `0x801D27FC` | `+0x3A0` | MAP:15973; FD:471024-471032 |
| draw body (reference anonymous symbol) | `0x8042D204` | `0x8042E254` | `+0x1050` | MAP:32847; FD:1089158,1089530-1089534 |
| save body (reference anonymous symbol) | `0x8042D7E8` | `0x8042E838` | `+0x1050` | MAP:32848; FD:1089535-1089633 |
| manager draw wrapper (reference misleading name) | `0x8042E11C` | `0x8042F16C` | `+0x1050` | MAP:32857; FD:1090124-1090134 |
| `DrawBlur` string | `0x804CFE68` | `0x804D0F18` | `+0x10B0` | MAP:38113; STR:135; FD:470546,470549,470551 |
| `SaveScreen` string | `0x804CFE74` | `0x804D0F24` | `+0x10B0` | MAP:38114; STR:136; FD:470578,470581,470583 |
| DrawBlur RTTI-name string candidate | `0x804CFFA8` | `0x804D1058` | `+0x10B0` | MAP:38125 (length `0x20`); STR:142 |
| SaveScreen RTTI-name string candidate | `0x804CFF84` | `0x804D1034` | `+0x10B0` | MAP:38124 (length `0x22`); STR:141 |

The constructor establishes the local string and installed pointer independently of the reference name:

```text
801D2080  3C608054  lis   r3,0x8054
801D2088  3803DC78  addi  r0,r3,-0x2388   -> 0x8053DC78
801D208C  901F0018  stw   r0,0x18(r31)
801D2084  3C80804D  lis   r4,0x804D
801D2090  38040F18  addi  r0,r4,0x0F18    -> 0x804D0F18 (DrawBlur)
801D2098  901F0000  stw   r0,0(r31)
```

The lines above are grouped by the two address-building chains; their actual execution order is by ascending VA. Full evidence: FD:470535-470566. The destructor independently reinstalls the same `0x8053DC78` pointer (FD:471002-471004). SaveScreen analogously installs `0x8053DC5C` and uses `0x804D0F24` (FD:470577-470583,470971-470974).

**Concrete falsification of direct MAP-address reuse:** local address `0x801D1CB8` contains word **`4BE7D35D`**, `bl 0x8004F014`, in a function already begun at `0x801D1C94`; it is not the DrawBlur constructor entry (FD:470294-470310). Applying MAP:15950 verbatim therefore labels an interior instruction of a different local task.

**Name contamination:** MAP:15971,15973 include `homebutton::MotorCallback` aliases, and MAP:32854-32857 call the neighboring manager wrappers `cCharacter::UpdateBlinking`. Local instructions instead traverse the common `+0x40` subobject and reach the paired draw/save bodies. Even at zero-delta startup, MAP:20's `WPADGetDpdSensitivity` label is not supported by the local implementation: FD:17-21 shows a byte setter/getter sharing `r13+0x5AF0`. A matching address or length does not validate an imported semantic name.

**Region/build conclusion:** the map filename advertises `GUPE8P`, but this worker did not obtain a reference DOL, its checksum, a trusted region/version manifest, or the local disc-ID header. These observations establish a different address layout and piecewise correspondence; they do **not** establish whether the cause is PAL/NTSC, revision, localization, or another link/build difference. Exact local region and exact reference provenance remain unresolved. No region-specific global rebasing rule is defensible from this map alone.

## 5. Candidate motion-blur code correspondence

### PJS task and call-site crosswalk

These are local `.text1` addresses; all offsets use `VA - 0x80006740`.

| Role | Local VA range | DOL file range | Reference start / size |
|---|---|---|---|
| DrawBlur constructor | `0x801D2058–0x801D20D8` | `0x001CB918–0x001CB998` | `0x801D1CB8 / 0x80` |
| owned SaveScreen constructor | `0x801D20D8–0x801D213C` | `0x001CB998–0x001CB9FC` | `0x801D1D38 / 0x64` |
| SaveScreen destructor | `0x801D270C–0x801D2764` | `0x001CBFCC–0x001CC024` | `0x801D236C / 0x58` |
| candidate SaveScreen execution entry | `0x801D2764–0x801D2788` | `0x001CC024–0x001CC048` | `0x801D23C4 / 0x24` |
| DrawBlur destructor | `0x801D2788–0x801D27FC` | `0x001CC048–0x001CC0BC` | `0x801D23E8 / 0x74` |
| candidate DrawBlur execution entry | `0x801D27FC–0x801D2820` | `0x001CC0BC–0x001CC0E0` | `0x801D245C / 0x24` |

Evidence: FD:470535-470591,470964-471032; MAP:15950-15951,15970-15973. The DrawBlur constructor allocates `0x28` bytes and calls the SaveScreen constructor at `0x801D20B4`, then stores that child at `this+0x28`. Its own caller at `0x801D1AE0–0x801D1AF8` requests **`0x30` bytes** before invoking DrawBlur (FD:470185-470191). Thus the child allocation size must not be mistaken for the DrawBlur task allocation size.

Branch words independently corroborate the functional connection:

| Call-site VA / fileoff | Word | Target |
|---|---|---|
| `0x801D1AF0 / 0x001CB3B0` | `48000569` | DrawBlur constructor `0x801D2058` |
| `0x801D20B4 / 0x001CB974` | `48000025` | SaveScreen constructor `0x801D20D8` |
| `0x801D2770 / 0x001CC030` | `4BED0E69` | manager getter `0x800A35D8` |
| `0x801D2774 / 0x001CC034` | `4825C9CD` | save wrapper `0x8042F140` |
| `0x801D2808 / 0x001CC0C8` | `4BED0DD1` | manager getter `0x800A35D8` |
| **`0x801D280C / 0x001CC0CC`** | **`4825C961`** | **draw wrapper `0x8042F16C`** |
| `0x8042F158 / 0x00428A18` | `4BFFF6E1` | save body `0x8042E838` |
| `0x8042F184 / 0x00428A44` | `4BFFF0D1` | draw body `0x8042E254` |

Sources: FD:470189,470558,470989-470990,471027-471028,1090119,1090130. The existing branch index independently lists the corresponding selected edges at `motion_blur_analysis/branches.tsv:62565,62614,62659,62663,138315-138316`.

The renderer wrappers load `r3 = [r3+0x40]`, check for null, and call the paired bodies. The save body clears object byte `+0x78` at `0x8042E850`, then sets it at `0x8042E9AC`; the draw body checks that byte at `0x8042E2C0–0x8042E2C8`. Both check object bytes `+0` and `+1`. This is additional local evidence connecting the pair, without importing the misleading MAP function names (FD:1089163-1089168,1089185-1089187,1089538-1089547,1089627-1089628).

The exact vtable slots that dispatch into the two candidate execution entries still require the raw data check in section 7. Function boundaries, direct branches, strings constructed by the constructors, and the piecewise crosswalk are corroborated; virtual-table membership is not asserted as freshly read.

### Contiguous renderer/state block

The observed function partition is **`[0x8042E254,0x8042F258)`**, size **`0x1004`**, file **`[0x00427B14,0x00428B18)`**. Every listed reference start is local minus `0x1050`; reference lengths exactly match the local boundary sequence. This is a useful extraction anchor, **not the whole subsystem or its dependency closure**.

| Local VA range | DOL file range | Ref. start / size | Local role supported by instructions |
|---|---|---|---|
| `0x8042E254–0x8042E838` | `0x00427B14–0x004280F8` | `0x8042D204 / 0x5E4` | draw body reached by DrawBlur-side wrapper |
| `0x8042E838–0x8042E9C4` | `0x004280F8–0x00428284` | `0x8042D7E8 / 0x18C` | paired save body / validity update |
| `0x8042E9C4–0x8042E9F0` | `0x00428284–0x004282B0` | `0x8042D974 / 0x2C` | parameter-record copy used by getter wrapper |
| `0x8042E9F0–0x8042EA1C` | `0x004282B0–0x004282DC` | `0x8042D9A0 / 0x2C` | parameter-record copy used by setter wrapper |
| `0x8042EA1C–0x8042EA50` | `0x004282DC–0x00428310` | `0x8042D9CC / 0x34` | null-source-checked parameter-record copy |
| `0x8042EA50–0x8042F084` | `0x00428310–0x00428944` | `0x8042DA00 / 0x634` | subobject construction/initialization |
| `0x8042F084–0x8042F0E8` | `0x00428944–0x004289A8` | `0x8042E034 / 0x64` | parameter getter with null-subobject fallback |
| `0x8042F0E8–0x8042F114` | `0x004289A8–0x004289D4` | `0x8042E098 / 0x2C` | forwards to `0x8042E9F0` |
| `0x8042F114–0x8042F140` | `0x004289D4–0x00428A00` | `0x8042E0C4 / 0x2C` | forwards to `0x8042EA1C` |
| `0x8042F140–0x8042F16C` | `0x00428A00–0x00428A2C` | `0x8042E0F0 / 0x2C` | save wrapper |
| `0x8042F16C–0x8042F198` | `0x00428A2C–0x00428A58` | `0x8042E11C / 0x2C` | draw wrapper |
| `0x8042F198–0x8042F20C` | `0x00428A58–0x00428ACC` | `0x8042E148 / 0x74` | release subobject/resources and clear owner `+0x40` |
| `0x8042F20C–0x8042F258` | `0x00428ACC–0x00428B18` | `0x8042E1BC / 0x4C` | allocate `0x13C`, call `0x8042EA50`, install at owner `+0x40` |

Evidence for boundaries/calls: MAP:32847-32859; FD:1089158-1089187,1089530-1089682,1090053-1090183. The whole constructor interior and all draw instructions were not audited by this mapper; the quoted complete range is a boundary correspondence, not full semantic validation. The next function begins at `0x8042F258` and is outside this contiguous anchor.

## 6. Candidate data correspondence

| Local address/range | Local DOL offset/range | Storage and evidence |
|---|---|---|
| `0x804D0F18` | `0x004CDF18` | `.data4`: `DrawBlur`; STR:135, constructor uses this exact VA |
| `0x804D0F24` | `0x004CDF24` | `.data4`: `SaveScreen`; STR:136, constructor uses this exact VA |
| `0x804D1034` | `0x004CE034` | `.data4`: `Effect::PJS::EffectSaveScreenTask`; STR:141; RTTI-name candidate, descriptor link not dumped |
| `0x804D1058` | `0x004CE058` | `.data4`: `Effect::PJS::EffectDrawBlurTask`; STR:142; RTTI-name candidate, descriptor link not dumped |
| `0x8053DC5C` | `0x0053AC5C` | `.data5`: pointer installed at SaveScreen `this+0x18`; FD:470577-470580,470971-470974 |
| `0x8053DC78` | `0x0053AC78` | `.data5`: pointer installed at DrawBlur `this+0x18`; FD:470545-470548,471002-471004 |
| `0x8053DC50–0x8053DC94` | `0x0053AC50–0x0053AC94` | candidate raw-word inspection window from `review_vtables.py:2-3`; **contents/slot layout not read live here** |
| `0x8051C378–0x8051C3A0` | `0x00519378–0x005193A0` | `.data4`: renderer parameter/constant pool addressed directly by `lis 0x8052` plus negative displacements |
| `0x8056BCD0`, `0x8056BCE4` | `0x00568CD0`, `0x00568CE4` | `.data5`: descriptor-like arguments constructed before the common call to `0x803A1AFC`; FD:1089170-1089176,1089549-1089555 |
| `0x8057798C` | **none** | main zero range: static manager storage address formed by follow-on getter initializer; FD:160634-160636 |
| `0x805EF2D8` | **none** | SDA zero range: lazy manager pointer at `r13+0x2DD8`; FD:160618,160622,160624 |
| `0x805EF629` | **none** | SDA zero range: initializer guard at `r13+0x3129`; FD:160631 |
| `0x805F265C` | **none** | SDA zero range: indirect API-root pointer location; FD:1089188-1089194 |

Constant-pool address formation is directly visible at FD:1089252-1089285,1089329-1089332: `0x8051C378`, `0x8051C380` are word-pair sources, `0x8051C388` and `0x8051C38C` are `lfs` sources, and `0x8051C390`/`0x8051C398` are `lfd` sources. **Their numerical contents are not established by this report**; no float values should be inferred from their addresses alone. These are not r2-relative accesses.

Installed pointers prove the local table-address anchors but not table extent, headers, RTTI parent chains, or individual virtual slots. In particular, the fact that the two installed values differ by `0x1C` must not replace a raw table dump. The reference map's unrelated names at the same numeric data addresses are not evidence of local identity.

## 7. Confidence, unresolved checks, and reproducible follow-up

### Confidence assessment

| Finding | Confidence / qualification |
|---|---|
| Live file size and SHA-256 match | **High, directly executed against the live target** |
| DOL section layout and entry | **High consistency**, inventory fields agree with file size and startup raw table/address evidence; fresh direct header parse was blocked |
| r2/r13 values | **High static confidence**, four explicit startup words and exact section-base `+0x8000` cross-check; source is existing local disassembly |
| Separate zero ranges and initialized-SDA overlap caveat | **High static confidence**, explicit table words and the zero-loop behavior |
| Piecewise map deltas at listed anchors | **High local confidence**, strings, installed pointers, matching function-length partitions, and direct call targets; not an all-map guarantee |
| DrawBlur/SaveScreen constructors and installed addresses | **High static confidence**, local name construction and repeated installed values |
| Exact task execution vtable slots / RTTI links | **Pending direct raw data check**; candidate execution entries are strongly corroborated, not freshly pointer-validated |
| Constant values, runtime API table, region/version identity | **Unresolved** by this worker |
| Runtime behavior / complete subsystem extraction | **Not claimed**; no emulator trace, game run, build, or runtime test was performed |

### Required foreground checks

Run from `sys/`, using `python -B` and stdout only. These commands do not call either writing main routine:

```text
Get-FileHash -Algorithm SHA256 main.dol

python -B -c "import sys; sys.path.insert(0,'motion_blur_analysis'); import analyze_dol as d; print(len(d.data)); print([(i,hex(d.u32(i*4)),hex(d.u32(0x48+i*4)),hex(d.u32(0x90+i*4))) for i in range(18)]); print('entry',hex(d.u32(0xe0)),'bss',hex(d.u32(0xd8)),hex(d.u32(0xdc)))"

python -B motion_blur_analysis/ppc_tools.py disasm 0x80003154 0x80003400
python -B motion_blur_analysis/ppc_tools.py disasm 0x801D2058 0x801D213C
python -B motion_blur_analysis/ppc_tools.py disasm 0x801D270C 0x801D2820
python -B motion_blur_analysis/ppc_tools.py disasm 0x8042E254 0x8042F258
python -B motion_blur_analysis/review_vtables.py
python -B motion_blur_analysis/review_refs.py
python -B motion_blur_analysis/ppc_tools.py pointers 0x801D27FC
python -B motion_blur_analysis/ppc_tools.py pointers 0x801D2764
```

Priorities: (1) directly confirm all 18 header tuples/unused slots; (2) dump and connect the installed tables to the execution entries and RTTI strings; (3) read actual renderer constant-pool bytes; (4) obtain trusted reference-DOL/region provenance before attempting any broader map translation. The constructor/descriptor/constant-window data in section 6 are sufficient exact local VA/file-offset anchors for those read-only checks.

Tool limitations relevant to complete extraction:

- `ppc_tools.py:8-11` maps only the start of a disassembly request then slices contiguous file bytes. Keep each requested interval inside one mapped section and align addresses to four bytes; do not disassemble across the unusual text0/data0/text1 layout using one VA interval.
- `ppc_tools.py:12-25` indexes opcode-18 direct `b`/`bl` targets, not conditional-branch families, indirect `bctr`/`bctrl`, or data/vtable edges. No direct xref does **not** mean a virtual task method is unused.
- Its pointer search matches aligned literal words, not addresses synthesized by `lis`/`addi`, SDA-relative access, or dynamic initialization. `:30` prints an unmapped result as zero; do not treat that formatting as proof of a meaningful VA-zero reference.
- FD has executable-slot data and padding decoded as instructions. Startup tables above were interpreted from raw words, not from those incidental mnemonics.

### Change and command ledger

- **Only new file written by this worker:** `analysis/motion_blur/evidence/01_binary_mapper.md`. No mapping JSON was needed. No pre-existing file, DOL, script, config, patch, ELF, disassembly, or baseline was edited; no `__pycache__` output was requested.
- Successful shell operations: `pwd`; read-only `ls -ld` of the target, reference map, and evidence directory; numeric-only `printf` calculations for identity/section ends/code offsets; **PowerShell `Get-FileHash -Algorithm SHA256 main.dol`**. Read/grep/glob tools inspected only existing sources/artifacts and checked that this owned report did not already exist.
- Denied before execution: two `python -B -c` inventory/hash probes; `python -B motion_blur_analysis/ppc_tools.py disasm 0x80003100 0x80003470`; Bash `sha256sum "main.dol"`. A text-tool read of `main.dol` also failed because it is not UTF-8. No denied command is counted as a completed verification.
- The imported-writing main routine and `ppc_tools.py all` were never run. No output was redirected into pre-existing files. No gameplay logic was audited beyond the directly relevant task/renderer references, and no runtime validation is claimed.
