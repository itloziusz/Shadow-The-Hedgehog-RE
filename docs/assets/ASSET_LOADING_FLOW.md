# Asset loading flow

This is the path from a disc-relative request to a native object. Addresses are in `sys/main.dol`.

## 1. Disc path

The game's file table is `sys/fst.bin`. `shadow::gc::open_fst` reads the big-endian nodes and builds full paths with ordinary strings. `character/shadow.one` is in that table and its length field equals the extracted file (665,612 bytes).

Path strings compiled into the DOL select what gets requested. Examples:

- `%s/%s_%02d.one` at `0x804ACAD0`, used from `0x80046B28`
- `%s/%s_TEX%02d.one` at `0x804ACABC`, used from `0x80046874`
- `stg%04d/stg%04d_light.bin` at `0x804ACB18`
- `stg%04d/stg%04d_dat.one`, `stg%04d/stg%04d_fx.one`, `stg%04d/stg%04d.fog`
- `fonts/%s/%s%s.txd`, `fonts/%s.met`
- `event/event%04d.scr`, `event\event%04d_scene%c.one`
- `sound/AudioData*.bin`, `sound/B7*.mlt`, `PRS_VOICE_E.afs`, `sng_*.adx`, `*.sfd`

`LandALoadManager::` update at `0x800466B4` is the stage coordinator. It formats those ONE paths, polls `TOneFileAsync` completion through `0x8004CB20`, and walks archive entries with `0x8004CA50`. The PC `ResourceSystem::stage_files` lists the directory instead of stepping a 15-byte flag array. The format strings are unchanged.

## 2. Read

Originally `TFileControlAsycManager` (`0x800453E0`) keeps three slots at `0x805742A0`, rounds the read up to 32 bytes, and builds a `0x390`-byte worker. That is the DVD path. The bytes that come back are the file.

The PC loader reads the extracted file with a normal stream. The buffer length is the file length. It is not rounded to 32. A request queue holds every pending path; it does not refuse a fourth request.

## 3. Archive

`open_one` matches `0x8004CAC0` and `0x8004CD38`:

1. Require word0 `== 0` and the size word `== file_size - 12`.
2. Parse `One Ver %f` at offset `0x0C`.
3. Read the count at `0x1C`.
4. For index `2 .. count+1`, read the 56-byte record.
5. If the version is `<=` the 0.59 constant, the size/offset/flags sit at `+0x20`. Otherwise they sit at `+0x2C`.
6. The name is the C string in front of those words. The 32- or 44-byte field is only the on-disk width. The result is a `std::string`.

`find` folds ASCII `a`–`z` the way `0x8004C938` does, then compares the whole string.

## 4. One entry

`OneArchive::load` matches `0x8004C770`:

- Absolute offset is `12 + offset_from_view`, computed in 64-bit arithmetic.
- If flag bit 0 is set, PRS-inflate and require the output length to equal the directory size.
- If flag bit 0 is clear, copy that many bytes. `EVENT8006.CEN` is one of those raw entries.
- The output vector is exactly that size. It is not padded to a DVD sector or a 32-byte DMA line.

## 5. Type dispatch

The original game then runs a `TOneFileAsync<T>` specialization (`0x8004AE60` through `0x8004C408`). The specialization is chosen by the caller, not by a stage-specific offset. The PC decoder looks at the root chunk id of the inflated bytes:

| Root id | Native result |
| --- | --- |
| `0x10` | `NativeModel` |
| `0x16` | `NativeTextureDictionary` |
| `0x0B` or `0x29` | `NativeWorld` |
| `0x2B` | uv-anim blob (chunk retained) |
| `0x1E` | delta-morph blob (chunk retained) |
| `EFFD` | effect blob |
| `AFS\0`, ADX `0x8000`, Sofdec `00 00 01 BA`, `METRICS1`, `CPAF` | the matching container |
| big-endian length word equal to the buffer | sized blob (collision, paths, some stage bins) |
| anything else | raw blob, still addressable |

A failure throws `ParseError`. It does not return an empty asset.

## 6. Native object

`to_native_model` copies positions, normals, the first UV set, the triangle list, the binmesh index buffers, materials, the frame basis, the HAnim hierarchy, and the skin indices, weights, and matrices. `SH.BON` becomes `NativeBon`. `SHADOW.MTP` becomes `NativeMotionPack`. Counts are the file counts. There is no compiled-in maximum for vertices, bones, materials, atomics, tracks, or keyframes.

Texture dependencies are the material texture names. After the archive's texture dictionaries are inflated, a name that exists in one of them is marked resolved. `SHADOW_EYE_L.DFF` resolves `pl_bw03n` to `SHADOW.TXD` in the same archive.

## 7. Handle

`ResourceHandle` is a 64-bit value: generation in the high half, slot in the low half. The slot indexes a growable table. Releasing an asset bumps the generation, so the old handle no longer resolves. The slot is not a GameCube pointer and it is not reused under the old generation.

## 8. What the original still does that this loader does not

The 13-state `LandALoadManager` update, the async completion callback at `this+0x2C` of `TOneFileAsync`, and the in-memory `0x3C` resource row are described in `RE_Content/re/` from earlier disassembly. They schedule and publish objects. They do not change the bytes of a clump or a texture. The PC cache publishes the native object as soon as `load` returns, with no 3-slot gate and no 15-child gate.
