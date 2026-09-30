# PC 64-bit port

The native layer is `shadow::pc` in `asset_system/`. It consumes the same GameCube files. It does not link Dolphin, it does not interpret PowerPC, and it does not keep a 32-bit guest heap.

Rule used for every limit: if the file or a traced branch needs it, it stays. If it only exists so the console can DMA, fit in RAM, or reuse a fixed array, it is gone.

## Removed

| Original limit | Why it existed | PC replacement |
| --- | --- | --- |
| Three read slots at `0x805742A0` (`0x800453E0`) | DVD/worker ceiling | `ResourceSystem` queue. `queue_limit` is the maximum `size_t`. The validation loads 6 requests in one pump, including a 4th, 5th, and 6th. |
| 32-byte read alignment (`+31` then clear the low 5 bits at `0x800454A4`) | Disc DMA | The output vector length is the directory size. No padding. |
| `0x390`-byte worker object | Fixed worker allocation | No worker object. `load` decodes on the caller. The queue is still unbounded, so a later job system can run the same requests without a slot cap. |
| 15 pending bytes at `LandALoadManager+0xC4` | Fixed child table in the stage object | `stage_files` lists the directory. `stg0403` has more than 15 files and includes `stg0403_98.one`. |
| In-memory ONE row of 60 bytes and a guest pointer at `+0x38` | 32-bit object after the file is parsed | `std::vector<OneIndexEntry>`. The offset is a `u32` from the file, added to the buffer in `u64`. |
| 32-bit pointers in `TOneFileAsync` (`+0x38` buffer, `+0x48` result) | PowerPC ABI | `ResourceHandle` is 64 bits: generation in the high half, slot in the low half. Release bumps the generation. |
| `Sonicteam::System::Heap` arenas | Console pools | `std::vector` / `shared_ptr` to the file buffer. The file buffer is the compressed archive. Entries inflate one at a time into their own vector. |
| GX texture edge ceiling (hardware max 1024, and power-of-two uploads) | GPU constraint | Width and height are the u16s in the file, stored in a `u32`. The reader does not reject a non-power-of-two edge and does not clamp to 1024. The image bytes stay in file order because the tile format is still unknown, not because a GX upload path is required. |
| Fixed 32-byte texture name as the runtime string | The native struct field is 32 bytes on disk | Parsed out, then stored as `std::string`. The 32-byte field is still the maximum stored in that record. |
| Fixed vertex, index, bone, and material arrays | RAM | `std::vector`. Eye is 45 vertices because the file says 45. Body is 1,382. The hierarchy is 30 nodes. None of those numbers are constants in the PC types. |
| Binmesh indices narrowed to u16 | Some GX index formats | The file stores u32 indices (`0x50E`). The PC mesh keeps `vector<uint32_t>`. The separate triangle list stays u16 because that is the 8-byte triangle record. |
| Synchronous single-slot DVD open as the only API | The game could not overlap more than 3 reads | `enqueue` accepts every request, `pump` runs the whole queue. There is no code path that returns "busy" at 3. |
| 44-byte stack buffer as the only name storage (`0x8004C938` copies at most 44 bytes) | The 0.60 record's name field is 44 bytes (32 on 0.50) | The field width is still how many bytes are read from the record. The value is a `std::string`. A longer query simply does not match a shorter stored name. |

## Kept, because the files or the branch need them

| Rule | Why it stays |
| --- | --- |
| Two reserved ONE slots, real entries at index `>= 2` | `0x8004CA50` returns null for index 0 and 1. The bytes there are empty. |
| Version split at the float `0x3F170A3D` | `0x8004CD38`. 0.50 files do not decode if the 0.60 field offset is used. Checked on `DebugModels.one`. |
| Name fold of ASCII `a`–`z` only | `0x8004C938`. Other bytes are not case-folded. |
| Flag bit 0 selects PRS | `0x8004C770`. |
| PRS end-of-stream and the exact directory size | The inflate either matches that size or throws. |
| Little-endian RenderWare chunks, big-endian native texture header | Mixing them up breaks the width or the vertex count. |
| Frame basis order and parent `-1` | Identity root and the parent index are the skeleton. Not converted to another up-axis. |
| Triangle winding, binmesh index order, material color, surface floats | Rendered meaning. Not replaced with a PBR shade. |
| MatFX and userdata kept as raw bytes | The TEV meaning is unknown. Dropping the bytes would lose the only copy of that state. |
| World bounds in file order | Two vec3s. Not reordered into a min/max pair until a trace shows which is which. |
| `%02d` part names and the other DOL path templates | Those are the request strings. Discovery does not stop at 15, and it does not rewrite the names. |
| FST big-endian directory | The path table is the disc index. |

## Layout of the PC side

```text
GameCube file
  -> BinaryReader (explicit endian, bounds, overflow checks)
  -> gc::open_one / open_fst / parse_clump / parse_tex_dictionary / parse_world_or_region
  -> pc::to_native_*
  -> ResourceSystem handle
```

`gc` is the decoder. `pc` does not expose `RpClump`, GX registers, or a guest pointer. A material on the PC side is color, three surface floats, the filter word, the texture name, and the MatFX bytes. That is the renderer-independent state the file actually has.

## Malformed input

The reader throws `ParseError` on a short header, a directory that does not fit, a chunk size past its parent, a PRS size mismatch, a frame parent outside the list, an atomic aimed at a missing mesh, a binmesh whose index sum does not match its header, and a negative count. A 32-byte buffer is rejected by `open_one`. There is no per-stage override and no skipped chunk.

The declared size of a PRS entry is trusted as the output length and the inflate stops if it would pass that length. That bound comes from the directory word, not from a console RAM cap.
