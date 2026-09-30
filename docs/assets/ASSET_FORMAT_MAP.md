# Asset format map

Sizes and counts below were checked on the extracted `files/` tree and `sys/fst.bin`. Endian is stated per field. A field that is only "present" is not given a gameplay name.

## ONE archive

| Offset | Type | Meaning | Confidence |
| --- | --- | --- | --- |
| `0x00` | u32le | 0 on all 1,244 archives | CONFIRMED |
| `0x04` | u32le | `file_size - 12` | CONFIRMED |
| `0x08` | u32le | library id, `0x1C020037` on 0.60 files | CONFIRMED as a stored id |
| `0x0C` | 16 bytes | `One Ver 0.60` (1,231 files) or `One Ver 0.50` (13 files) | CONFIRMED |
| `0x1C` | u32le | number of real entries | CONFIRMED |
| `0x20` | 32 bytes | `0xCD` fill | UNKNOWN |
| `0x40 + i*56` | record | index `i`, real entries start at `i = 2` | CONFIRMED |

Record, version 0.60: name in the first 44 bytes, then u32le size, offset-from-file+12, flags.
Record, version 0.50: those three words start at byte 32, so the name field is 32 bytes.

Flag bit 0: 1 = PRS, 0 = raw bytes of `size`. Other bits UNKNOWN.

Inner names seen across archives: `DFF`, `TXD`, `MTN`, `BON`, `DMA`, `UVA`, `SNB`, `CCL`, `MTP`, `CEN`, `BIN`, `PTB`, `ADB`, `BDT`, `PTP`, `GNCP`, `EFD`, `BSP`, `RG1`.

## PRS

Bitstream at `0x80043FE0`. Literal bit 1 copies one source byte. A cleared bit starts a match. The second bit selects the long form (13-bit negative offset, 3-bit length, optional extra length byte, offset word 0 ends the stream) or the short form (2-bit length, 8-bit negative offset). Output length equals the directory size on every entry of `character/shadow.one`.

## FST

Big-endian 12-byte nodes. High byte of word 0 is 1 for a directory. The low 24 bits are the name offset into the string table that follows `count * 12` bytes. Directories store parent and next-index. Files store disc offset and length. 3,480 files, 135 directories.

## RenderWare chunk

Little-endian `u32 id, u32 size, u32 library_id`. `size` is the body and does not include the 12-byte header. Library id `0x1C020037` unpacks, by the usual RenderWare shift, to version 3.7.0.2 build `0x37`. That unpack formula is **STRONGLY INFERRED**. The raw id is **CONFIRMED**. `advertise.txd` uses the older stamp `0x1400FFFF` and still has a `0x16` dictionary.

| Id | Where | Role |
| --- | --- | --- |
| `0x01` | inside containers | struct body |
| `0x02` | materials | name string |
| `0x03` | frames, geometry, materials | extension plugins |
| `0x06` | material | texture reference, not the native raster |
| `0x07` | matlist | material |
| `0x08` | clump, world | material list |
| `0x09` | world, after the material list | sector container, body **PARTIAL** |
| `0x0B` | `.BSP`, and inside `.RG1` | world |
| `0x0E` | clump | frame list, 56 bytes each |
| `0x0F` | clump | geometry |
| `0x10` | `.DFF` | clump |
| `0x14` | clump | atomic, 16-byte struct |
| `0x15` | texture dictionary | native texture |
| `0x16` | `.TXD` | texture dictionary |
| `0x1A` | clump | geometry list |
| `0x1B` | inside `0x2B` | one animation, body **PARTIAL** |
| `0x1E` | `.DMA` | delta morph, body **PARTIAL** |
| `0x29` | `.RG1` prelude | 32 bytes: 4xu32 and a name |
| `0x2A` | end of `.RG1` | empty chunk |
| `0x2B` | `.UVA` | uv-anim dictionary |
| `0x116` | geometry extension | skin |
| `0x11E` | frame extension | HAnim |
| `0x11F` | geometry extension | user strings |
| `0x120` | material extension | MatFX payload |
| `0x50E` | geometry extension | binmesh indices |

### Geometry flags

`flags` little-endian. Texcoord set count is bits 16–23. Bit `0x01000000` means the CPU arrays are absent. Sampled character meshes use `0x00010037` (eye: tristrip, positions, textured, normals, light, one UV set) and `0x00010077` (body, also modulate-material-color). Those bit names are the standard RenderWare mask and are **STRONGLY INFERRED**; the numbers and the arrays that follow are **CONFIRMED** because the struct is consumed exactly.

### Native texture header

Big-endian, at the start of the `0x15` struct:

| Offset | Field | Confidence |
| --- | --- | --- |
| 0 | platform id 6 | CONFIRMED |
| 4 | filter/address word | CONFIRMED raw |
| 8, 12, 16, 20 | four u32s | UNKNOWN |
| 24 | name, 32 bytes | CONFIRMED |
| 56 | mask, 32 bytes | CONFIRMED |
| 88 | raster format u32 | CONFIRMED raw (`0x4204` / `0x4304` in `SHADOW.TXD`) |
| 92 | width, height as big-endian u16 | CONFIRMED |
| 96 | depth, levels, kind, format_param | levels is a small count; the other three UNKNOWN |
| 100 | tail u32 | UNKNOWN |
| 104 | image bytes through the end of the struct | CONFIRMED as the payload, not as a linear RGBA buffer |

The 32-byte name field is the format. The PC texture stores a `std::string` and a `u32` width so a later tool is not stuck with a 1024 GX ceiling. A u16 in the file still cannot exceed 65535.

## AFS

`AFS\0`, u32le count, then `count` pairs of u32le offset and size. Each pair must land inside the file. `PRS_VOICE_E.afs` passes that check. The filename table that sometimes follows the entries was not required to locate entry 0 and is **PARTIAL**.

## ADX

| Offset | Type | Sampled value | Confidence |
| --- | --- | --- | --- |
| 0 | u16be | `0x8000` | CONFIRMED |
| 2 | u16be | copyright offset; bytes there contain `CRI` | CONFIRMED |
| 4 | u8 | 3 | STRONGLY INFERRED as encoding, stable on the sampled songs |
| 5 | u8 | 18 | STRONGLY INFERRED as block size |
| 6 | u8 | 4 | STRONGLY INFERRED as sample bits |
| 7 | u8 | 2 | STRONGLY INFERRED as channels |
| 8 | u32be | 48000 | STRONGLY INFERRED as sample rate |
| 12 | u32be | total samples | STRONGLY INFERRED |
| 16 | u16be | highpass | STRONGLY INFERRED |

ADX ADPCM coefficients are not decoded.

## Sofdec / SFD

First four bytes `00 00 01 BA` (MPEG program-stream pack). Demux is not implemented.

## setid.bin

u32le 0, u32le count, then `count` records of three u32le. `8 + 302 * 12 = 3632`, which is the file size. The three words are UNKNOWN. The first word of each record increments (`0x2580`, `0x2581`, …) in the header of the file. The other two are `0xFFFFFFFF` there.

## Sized blobs

`PATH.PTP`, and the CCL / PTB / BIN samples, start with a big-endian u32 equal to the decompressed length. `PATH.PTP` is `0x2374` bytes and the word is `0x00002374`. Further words are kept and not named.

## EFFD

`EFFD`, u32le version, then a C string name (`stg0100_fx` on the stage fx archive). The rest of the effect body is UNKNOWN.

## METRICS1

ASCII, first line `METRICS1`. The rest is text (`Advertise_EN00` and glyph rows). It is not turned into a glyph-metric struct.

## CPAF

`.gncp` files start with `CPAF`. Interior tags such as `NGIF` are visible and were not given a grammar.

## Left as raw bytes

`.SNB` (`NSIF` magic is visible), `.fnt`, `.fog`, `.light.bin`, `.scr`, `.mlt` (`gcaxMLT`), `.tp`, `.dat` outside an ONE, and `.inf`. Their magics or first words are listed in `UNKNOWN_ASSET_FIELDS.md` so a later pass can start from the right offset. `.BON`, `.MTN`, and `.MTP` are decoded in `BON_FORMAT.md` and `MTN_FORMAT.md`.
