# Apploader reverse map

## Header — CONFIRMED

- Revision/date string: `2004/11/10`
- Entry: `0x81200258`
- Code size: `0x00001A98` (6808)
- Trailer size: `0x0001C3A0` (115616)
- Header + code + trailer = `0x0001DE58` (122456), exactly the supplied file size.

## Callback handoff — CONFIRMED from PowerPC instructions

The entry routine writes three function pointers supplied by the caller:

- Init: `0x81200278`
- Main: `0x81200298`
- Close: `0x812002B8`

`Main` returns nonzero while it has another DVD read request and zero when loading has finished. `Close` yields the loaded DOL's entry point.

## Main state machine — CONFIRMED/STRUCTURAL

| State | Handler | Meaning |
|---:|---:|---|
| 0 | `0x81200820` | Request 0x20 bytes at disc offset 0x420 into the internal boot-header buffer; transition to state 2. |
| 1 | `0x81200820` | Alias of state 0 / same initial header request path. |
| 2 | `0x81200850` | Validate/inspect initial header values, then request 0x20 bytes at disc offset 0x440 (start of BI2); transition to state 3. |
| 3 | `0x812008C8` | Publish low-memory boot metadata, derive BI2 destination from low memory, then request the full 0x2000-byte BI2 block; continue boot-layout setup. |
| 4 | `0x81200ADC` | Resolve/validate DOL/FST placement and loader flags; choose DOL-header load path. |
| 5 | `0x81200C00` | Request 0x100-byte DOL header from the DOL disc offset; transition to state 6. |
| 6 | `0x81200C30` | Parse DOL header, total section footprint, BSS/range constraints, and initialize section index; transition to state 7. |
| 7 | `0x81200DC8` | Iterate the 7 DOL text sections. For every non-empty section return destination address, 32-byte-aligned length, and disc offset. |
| 8 | `0x81200EE0` | Iterate the 11 DOL data sections using the same destination/length/disc-offset request contract. |
| 9 | `0x81200FEC` | Request/copy the FST or alternate relocated FST range depending on the placement decision; transition to finalization. |
| 10 | `0x8120108C` | Finalize low-memory boot info / arena and FST metadata; transition to state 11. |
| 11 | `0x8120109C` | Final optional low-memory boot flag update, then return 0 from AplMain (loading finished). |
| 12 | `0x812010C8` | Chunk continuation path for requests larger than 0x20000 bytes; emit the next <=128 KiB slice and return to the saved state. |


## Important implementation details

- The loader uses a **13-state switch** (`0..12`).
- The DOL header read is exactly `0x100` bytes.
- Text loading iterates all 7 DOL text slots; data loading iterates all 11 DOL data slots.
- Load sizes are rounded/aligned as required by the loader before DVD reads.
- A helper computes the sum/footprint of DOL sections.
- A range check protects the apploader-reserved area around `0x81100000..0x81130000`.
- Requests larger than `0x20000` bytes are split by state 12 into <=128 KiB chunks.
- After section/FST loading, low-memory boot information is finalized and `Main` returns zero.

## Recompiler consequence

None of this state machine should be emitted as translated runtime code. The build pipeline already has random access to `main.dol` and `fst.bin`; therefore it can materialize the final loaded image directly. Keep this reverse map as a **validation oracle**: the build-time materialized image should match what the apploader would have produced.

Recommended test: generate a byte-level guest-memory snapshot immediately before the DOL entry point in both (a) the old/apploader path and (b) the new build-time path, then diff all initialized ranges and low-memory fields. A zero semantic diff is the gate for deleting the loader runtime.

## Final low-memory writes — instruction-confirmed

The helper at `0x81200774` writes the standard boot handoff block through the pointer it receives:

- `+0x20 = 0x0D15EA5E`
- `+0x24 = 1`
- `+0x30 = 0`
- `+0x34` and `+0x38` receive the same loader-computed placement value (the FST/ArenaHi handoff)
- `+0x3C` receives the loader's FST maximum-size value

For the native rewrite, treat `LOW_MEMORY_BOOT_FIELDS.json` as a generated compatibility contract. Instrument game reads and prune fields only after proving they are unobserved.
