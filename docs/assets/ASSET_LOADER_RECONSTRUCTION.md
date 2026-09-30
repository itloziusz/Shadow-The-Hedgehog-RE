# Asset loader reconstruction

Phase 1 is the behavior of the GameCube `main.dol` expressed as explicit C++ reads. The implementation lives in `asset_system/`. It does not cast file bytes onto a packed struct and it does not emulate the PowerPC.

Confidence words used below:

- **CONFIRMED** — an instruction, size identity, or value checked on the disc files.
- **STRONGLY INFERRED** — the bytes and the call site agree, and the name is the ordinary one for that pattern.
- **PARTIAL** — the container is real, and only some fields are known.
- **UNKNOWN** — preserved, not named.

## Original functions

| DOL address | Role | C++ |
| --- | --- | --- |
| `0x80043FE0` | PRS inflate. Flag bits come from the low bit (`srwi`). A long-form word of 0 returns the output length. Offset is OR-ed with `-8192` or `-256`. | `shadow::gc::prs_decompress` |
| `0x80043D34` | `lwz` + `stwbrx` over 4-byte groups. Turns little-endian file words into native big-endian CPU words. | Explicit little-endian reads of those words. The numeric result is the same. |
| `0x8004CAC0` | Binds a file buffer. `this+0x2C` is the buffer, `this+0x30` is the size, `this+0x34` and `this+0x38` become `buffer+12`. | `shadow::gc::open_one` |
| `0x8004CD38` | `sscanf` of `"One Ver %f"` at `0x804ACCB4`. If the float is less than or equal to the constant at `r2-31224` (`r2 = 0x805FA780`, bits `0x3F170A3D`, about 0.59), copy the three words at entry `+0x20` onto `+0x2C`. Then byte-swap `+0x2C`, `+0x30`, `+0x34`, and add the view base to `+0x30`. | `open_one` version branch. 1,231 archives are 0.60 and use `+0x2C`. 13 archives are 0.50 and use `+0x20`. Both layouts were inflated and the output size matched the size word. |
| `0x8004CA50` | Entry address is `view + index * 56 + 0x34`. Index 0 and 1 return null. Valid indices run from 2 through `count + 1`. | Directory walk in `open_one`. The on-disk count is the little-endian word at file `+0x1C`. |
| `0x8004C938` | Uppercases ASCII `a`–`z` (44 iterations) and compares with `0x803AD96C`. | `OneArchive::find` |
| `0x8004C770` | Loads one entry. Flag bit 0 set calls the PRS path. Flag bit 0 clear copies `size` bytes. The function returns a byte count into a caller buffer. | `OneArchive::load` |
| `0x800145A4` | In-memory `ResourceOneFile` scan. Stride `0x3C`, walks backward, returns the pointer at entry `+0x38`. | Not the file format. The PC cache does not build this 32-bit table. The file directory is the 56-byte records above. |
| `0x800453E0` | Three DVD/read slots at `0x805742A0`. Sizes are rounded up by 31 and masked. A `0x390`-byte worker is constructed. | Not reproduced. See `PC_64BIT_PORT.md`. |
| `0x800466B4` | `LandALoadManager` state machine, 13 states. `this+0xB4` is the state. Fifteen bytes at `this+0xC4` are cleared and scanned. | The path strings are honored. The 15-byte flag array is not a file limit. |
| `0x80046874` | `sprintf` `"%s/%s_TEX%02d.one"`. | Stage discovery lists the files that exist. It does not stop at 15. |
| `0x80046B28` | `sprintf` `"%s/%s_%02d.one"`. | Same. |
| `0x8004874C` | `sprintf` `"stg%04d/stg%04d_light.bin"`. | The file is present for stages that ship it. Its float grouping is still UNKNOWN. |

`r2` was taken from `lis r2, 0x805F` / `ori r2, r2, 0xA780` at `0x8000332C`.

## Typed loaders

RTTI strings in the DOL name the post-I/O objects:

- `TOneFileAsync<RpClump>` — payload root chunk `0x10`
- `TOneFileAsync<RwTexDictionary>` — root chunk `0x16`
- `TOneFileAsync<RpWorld>` — root chunk `0x0B`, and `.RG1` files which are a `0x29` prelude plus a world
- `TOneFileAsync<RpDMorphAnimation>` — root chunk `0x1E` on `.DMA` entries
- `TOneFileAsync<RtDict>` — `.UVA` files are chunk `0x2B` (a dictionary whose children are chunk `0x1B`)

The chunk ids themselves are **CONFIRMED** by walking the decompressed bytes (each child size fits, and the root size equals the directory size). The link from those ids to the RTTI names is **STRONGLY INFERRED**: the typed async loaders are the only constructors for those RenderWare types, and the payloads have the matching container shape (atomics, native textures, world bounds, animation chunks).

The individual `RpClumpStreamRead` / `RwTexture` readers inside the RenderWare library were not isolated instruction by instruction. Field layout below is the layout that consumes each struct exactly and matches counts checked in `asset_tests`.

## ONE record

```text
+0x00  u32le  0
+0x04  u32le  file_size - 12
+0x08  u32le  library id (0x1C020037 on 0.60 files)
+0x0C  "One Ver %f" in a 16-byte field
+0x1C  u32le  entry count
+0x20  32 bytes of 0xCD on every sampled archive
+0x40  records of 56 bytes
```

Index 0 and 1 are empty. A real record starts with the name. Version 0.50 stores size, offset, flags at `+0x20`. Version 0.60 stores them at `+0x2C`. The offset is relative to file+12. Flag bit 0 means PRS. Other flag bits are kept and do not change the path, because `0x8004C7D0` tests only bit 0.

The `0xCD` bytes are **UNKNOWN**. The loader never reads them. They are the MSVC debug fill pattern, serialized into the retail files.

## Clump (`0x10`)

CONFIRMED on `SHADOW_EYE_L.DFF`, `SHADOW_BODY.DFF`, and `CUBE_MODEL.DFF` from `DebugModels.one` (a 0.50 archive):

- Clump struct is 12 bytes: atomic count, light count, camera count.
- Frame list struct is `4 + count * 56`. Each frame is 12 little-endian floats, a parent index, and a flags word. The root parent is `-1` and the root basis is identity, so the float order is right, up, at, position.
- One extension chunk follows each frame. Plugin `0x11E` is the hierarchy. A 12-byte plugin is one node id. A larger plugin is a 20-byte header plus `numNodes * 12` node records. Body has 30 nodes and the chunk is `0x17C` bytes.
- Geometry struct (non-native) is flags, triangle count, vertex count, morph count, then UVs, 8-byte triangles, then morphs. Each morph is a 4-float sphere, two int32 presence flags, then positions and normals. Eye: 45 vertices, 67 triangles, one morph. Body: 1,382 vertices.
- The fourth triangle u16 is **UNKNOWN**. It is not a material index.
- Material struct is 28 bytes: flags, 4 color bytes, one unused word, a textured flag, then ambient, specular, and diffuse. Eye's color is opaque white and the three floats are 1.
- A texture chunk inside a material is a little-endian filter word, a name string, and a mask string. Eye references `pl_bw03n`.
- Binmesh plugin `0x50E` is flags, mesh count, total index count, then per mesh a count, a material index, and that many little-endian u32 indices. Eye is 91 indices and the byte length matches `12 + 8 + 91 * 4`. Body is 4 meshes and 2,703 indices.
- Skin plugin `0x116` on a non-native mesh is the `0x8044D7A4` layout: bone count, used count, max weights, used-bone ids, four index bytes and four weights per vertex, `boneCount` matrices of 16 floats, and three trailer u32s. See `SKINNING_FORMAT.md`.
- Userdata plugin `0x11F` is kept raw, plus the printable strings inside it (`ATTR_OBJPARM` on the body).

If the geometry flags include `0x01000000`, the vertex arrays are not in the struct. The remainder is kept as `native_body`. None of the sampled character meshes set that flag.

## Textures (`0x16` / `0x15`)

Dictionary struct is 4 bytes. The first u16 is the texture count (7 in `SHADOW.TXD`, 1 in `number.txd`). The second u16 is 3 on those files and is **UNKNOWN**.

Each native texture struct is big-endian from the start of its struct body:

- platform id 6
- filter/address word. `0x00001101` is nearest, wrap, wrap if the usual RenderWare packing is used (**STRONGLY INFERRED** for the bit split; the raw word is **CONFIRMED**)
- four unknown u32s
- 32-byte name and 32-byte mask
- at offset 88: raster format, width, height, depth, level count, two more bytes, and one tail u32
- the rest of the struct is the image in file order

`SHADOW.TXD` edges are 32, 128, 128, 32, 64, 64, 32. There is no 1024-wide cap in the reader. The image is not rearranged into linear pixels. The GX tile format was not isolated in the DOL, so a detile would be a guess.

## World and region

`.BSP` files are one chunk `0x0B`. The struct is 64 bytes: 10 little-endian words, then six floats. On `STG0100_COLI_01.BSP` those floats are a pair of large stage coordinates (about 453, 520, 519 and -200, -5939, -1902). They are stored as `bound_a` then `bound_b` in file order. Which end is the minimum is **STRONGLY INFERRED** only as "two vec3 bounds"; the order is not swapped.

The material list on that world has 4 materials. The following chunk id `0x09` is kept raw. Its vertex layout was not traced.

`.RG1` files are chunk `0x29` (32-byte prelude: four u32s and a name such as `stg0100_d_01`), then a world, then an empty chunk `0x2A`. The four prelude words other than the name, and the empty `0x2A`, are **UNKNOWN**.

## Other containers

| Kind | Evidence | Parser |
| --- | --- | --- |
| FST | `sys/fst.bin`, big-endian entries, root is a directory, 3,615 nodes | `gc::open_fst` |
| AFS | magic `AFS\0`, little-endian count, offset/size pairs inside the file | `gc::parse_afs` |
| ADX | `0x8000`, copyright offset lands on a CRI marker, sampled rate 48000, 2 channels | `gc::parse_adx`. Sample decode is not implemented. |
| SFD | `00 00 01 BA` | magic only |
| setid.bin | `8 + count * 12 == file size`, count 302 | three u32s per record, names UNKNOWN |
| EFFD | magic plus a version and a name | rest raw |
| METRICS1 | text signature | whole text kept |
| CPAF | `.gncp` magic | whole file kept |
| CCL, PTB, BIN, PTP | first big-endian u32 equals the decompressed length | header words kept, record stride UNKNOWN |
| CEN | uncompressed ONE entry, ASCII | raw text bytes |
| DMA, UVA, EFD body, fog, light, fnt | see the format map | partial or raw |
| BON, MTP / MTN | `BON_FORMAT.md`, `MTN_FORMAT.md`, `ANIMATION_PIPELINE.md` | decoded; BON node `+6` maps chunks onto bones |

## What was not traced

The RenderWare GX TEV setup, the world-sector vertex reader, the paired-single skin blend (`0x8047FC08` and the 2-weight loop), ADX ADPCM, and the 13 `LandALoadManager` state bodies beyond the path strings and the 15 flag bytes. Those gaps are listed with the missing evidence in `UNKNOWN_ASSET_FIELDS.md`.
