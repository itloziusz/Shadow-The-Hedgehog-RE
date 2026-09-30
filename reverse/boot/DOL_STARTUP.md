# DOL and CRT startup reverse map

## DOL header — CONFIRMED

- Entry point: **`0x80003154`**
- BSS envelope in DOL header: `0x8056FE00` + `0x0008C7EC`
- Non-empty sections: 10

| Section | File offset | Guest load address | Size | Guest end |
|---|---:|---:|---:|---:|
| text0 | `0x00000100` | `0x80003100` | `0x00002500` | `0x80005600` |
| text1 | `0x00002600` | `0x80008D40` | `0x004A1F20` | `0x804AAC60` |
| data0 | `0x004A4520` | `0x80005600` | `0x00001F20` | `0x80007520` |
| data1 | `0x004A6440` | `0x80007520` | `0x00001820` | `0x80008D40` |
| data2 | `0x004A7C60` | `0x804AAC60` | `0x00000480` | `0x804AB0E0` |
| data3 | `0x004A80E0` | `0x804AB0E0` | `0x00000020` | `0x804AB100` |
| data4 | `0x004A8100` | `0x804AB100` | `0x00072420` | `0x8051D520` |
| data5 | `0x0051A520` | `0x8051D520` | `0x000528E0` | `0x8056FE00` |
| data6 | `0x0056CE00` | `0x805E4500` | `0x0000AB20` | `0x805EF020` |
| data7 | `0x00577920` | `0x805F2780` | `0x00009DC0` | `0x805FC540` |


## Entry sequence — CONFIRMED/STRUCTURAL

At `0x80003154` the startup sequence immediately calls the standard-style CRT helpers:

1. `0x800032B0` — register initialization.
   - `r1` stack = `0x8060C5F0`
   - `r2` SDA2 = `0x805FA780`
   - `r13` SDA = `0x805EC500`
2. `0x80003400` — low-level hardware/runtime initialization path.
3. `0x80003340` — process copy and zero tables.
4. debug/TRK/OS/runtime initialization branches.
5. user/runtime constructors.
6. call `0x800510C0`, the **PROVEN static application event-loop wrapper**: its recurring `0x800511E4..0x800511FC` path dispatches event `0x12`, calls `0x8032D444`, and checks exit word `0x80576DBC`. See `research/APPLICATION_LOOP.md`; live reachability and display output remain UNKNOWN.
7. exit path candidate: `0x803A3B04` (**INFERRED symbol name, address confirmed as branch target**).

The strings in the startup area include `Metrowerks Target Resident Kernel for PowerPC`, consistent with the CodeWarrior-era CRT/debug path.

## CRT copy table at `0x80005544` — CONFIRMED

| Source | Destination | Size |
|---:|---:|---:|
| `0x80003100` | `0x80003100` | `0x000024E8` |
| `0x80005600` | `0x80005600` | `0x00001F08` |
| `0x80007520` | `0x80007520` | `0x00001814` |
| `0x80008D40` | `0x80008D40` | `0x004A1F08` |
| `0x804AAC60` | `0x804AAC60` | `0x0000046C` |
| `0x804AB0E0` | `0x804AB0E0` | `0x0000000C` |
| `0x804AB100` | `0x804AB100` | `0x00072418` |
| `0x8051D520` | `0x8051D520` | `0x000528C8` |
| `0x805E4500` | `0x805E4500` | `0x0000AB20` |
| `0x805F2780` | `0x805F2780` | `0x00009DB8` |


The source and destination addresses are identical here because the apploader has already placed the DOL sections at their final RAM addresses. The startup loop still performs the generic table-driven initialization contract.

## CRT zero table at `0x800055C8` — CONFIRMED

| Destination | Size |
|---:|---:|
| `0x8056FE00` | `0x00074700` |
| `0x805EF020` | `0x0000375C` |
| `0x805FC540` | `0x000000AC` |


The zero-table total is `0x00077F08`. Do not blindly replace the entire DOL BSS envelope with one host `memset`: preserve the actual zero-table semantics unless a later proof shows the remaining BSS gap is unobservable/reserved.

## Modernization implication

The recompiler should lower these generic CRT operations to a generated host-side initialization descriptor. That lets the linker remove the original PPC CRT loops while preserving the exact initialized image. SDA/SDA2 values become translated-address constants or runtime bases rather than something the native CPU has to reproduce in PowerPC registers.
