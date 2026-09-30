# Unknown asset fields

Each line is something the loader keeps and does not interpret. "Missing evidence" is the specific trace or byte check that would name it. Nothing here was filled in with a plausible default.

## ONE and I/O

| Item | What is known | Missing evidence |
| --- | --- | --- |
| 32 `0xCD` bytes at ONE `+0x20` | Present on every archive. `0x8004CD38` does not read them. | A store that writes this field, or a read that branches on it. |
| Flag bits above bit 0 | Bit 0 selects PRS versus a raw copy at `0x8004C7D0`. | The readers of the rest of the word. |
| `ResourceOneFile` stride `0x3C`, payload at `+0x38` | In-memory table at `0x800145A4`. The file record is 56 bytes, not 60. | The function that fills the `0x3C` rows from a ONE record. The bytes between the name and `+0x38` are still unnamed. |
| `TOneFileAsync` state names | State byte at `this+0x28`. States 0 and 4 are the prepare and publish ends. | A symbol or string for states 1, 2, and 3. The notes in `RE_Content/re/Original_TOneFileAsync_RE_v14.cpp` mark those names as inferred. |
| 15 bytes at `LandALoadManager+0xC4` | Cleared and scanned. | Whether each byte is a part index, a child id, or a bool. The file set goes to part 98, so the 15 is not the part count. |
| `LandALoadManager` states 0–12 | Jump table at `0x800466B4`. | Per-state names past the ONE open, the texture-part sprintf, and the light-bin sprintf. |

## Geometry and skin

| Item | What is known | Missing evidence |
| --- | --- | --- |
| Triangle `extra` u16 | Fourth little-endian u16. On the eye it is not the material id (the mesh has one material; the values are spread across the vertex range). | The writer, or a use of that halfword in a collision or adjacency walk. |
| Frame `matrix_flags` | The u32 after the parent index. | Which bits the renderer tests. |
| HAnim hierarchy word at `+16` | `0x24` on the body, Amy, and Bee. The node table still fills the chunk. | A read of that word. |
| HAnim node flags | The third i32 of each node. Observed `0, 1, 2, 3`. Parents come from the frame record, not from these flags. | A test of those bits while the hierarchy is built. |
| Skin header byte 3 | The fourth byte of the swapped header word. `0` on the body, Amy, and Bee. | A use other than "the leftover byte". Byte 2 is max weights (`0x8044D7FC`). |
| Skin split tail | `0x8044CC4C` reads three u32s. When the middle one is `> 0` the tail length is `boneCount + 2*middle + 2*third`, split at those boundaries. The tested skins have all three words 0. | A file with a nonzero middle word, and a reader of the bytes after they are split. |
| Skin matrix versus bind pose | 16 little-endian floats per bone, passed with a hierarchy matrix to `0x8047FC08`. Orthogonal on the body. | The paired-single product order inside `0x8047FC08`. They are not the inverse of the bind globals under either multiply order that was tried. |
| 2-weight and 4-weight vertex blend | `0x80451FB8` / `0x804522A4` / `0x80452668` are the loops selected by max weights. Indices and weights are decoded. | A scalar reading of those paired-single loops. |
| Geometry flag names | The numeric flags select the arrays that are present. | A local symbol naming `0x40` or `0x20`. The array layout does not need those names. |
| Native geometry body | Flag `0x01000000` skips CPU arrays. Sampled character meshes do not set it. | One file that sets it, plus the display-list reader. |
| Userdata `0x11F` | Contains ASCII such as `ATTR_OBJPARM`. | The count/type/value records around those strings. |
| MatFX `0x120` | Raw payload. Eye material is 12 bytes starting with u32 `5`. Body materials are larger and contain a nested texture chunk. | The MatFX type enum and the TEV builder that consumes it. No `TEV` string was found in the DOL data strings that were extracted. |

## Textures

| Item | What is known | Missing evidence |
| --- | --- | --- |
| Dictionary u16 after the count | Value 3 on `SHADOW.TXD` and `number.txd`. | A branch on that halfword. |
| Native texture u32s at +8, +12, +16, +20 | Big-endian, stable on the sampled rasters (often 0, 1, 1, 0). | Field names from the GC raster plugin. |
| Raster format word (`0x4204`, `0x4304`) | Distinguishes rasters of the same size. | The GX format enum that this word maps to. CMPR and I4 have the same byte size, so size alone cannot decide. |
| Bytes at +96..+103 | depth / small integers. | Which byte is the mip count versus the GX format. |
| Image bytes | They start at struct offset 104 and run to the end of the struct. | The detile and palette path. Doing that without the format byte would invent pixels. |

## World, collision, stage data

| Item | What is known | Missing evidence |
| --- | --- | --- |
| World header words 0–9 | 10 little-endian words before the six bound floats. On the stage BSP, word 0 is 1 and later words include `0x345` and `0x2C9`. | Which words are vertex counts, sector counts, or flags. The world-sector reader was not located. |
| Bound order | Six floats, two vec3s, file order kept. | A compare that shows the first vec3 is the minimum. |
| Chunk `0x09` | Follows the world material list. Large struct inside. | Sector vertex format. |
| Region prelude u32s | Four little-endian words before the name. The name matches the entry (`stg0100_d_01`). | The meaning of `1`, `2`, `0x10`, and the repeated library id. |
| Empty chunk `0x2A` | Ends every `.RG1` in `stg0100_01.one`, size 0. | A branch on id `0x2A`. |
| CCL / PTP / PTB / BIN / ADB / BDT | PTP, CCL, PTB, and BIN start with a big-endian length equal to the buffer. ADB and BDT do not follow that rule (BDT starts with the stage name). | Record stride and the collision query that reads them. |
| `.fog` | 960 bytes on `stg0100`. First u32le is 2. Floats `1.0` and a large value repeat. | Record size. |
| `*_light.bin` | Little-endian floats. `stg0000` is 208 floats and starts with three `0.352` values and ones. | A count or a stride. |
| `.dat` camera / nrm / hrd / ds1 / cmn | DOL path strings only. | A header walk. |
| `.scr` | Event script path in the DOL. | The opcode table. |

## Animation, effects, UI, audio

| Item | What is known | Missing evidence |
| --- | --- | --- |
| `.UVA` / chunk `0x1B` | Dictionary count matches the `0x1B` children. | Keyframe layout inside `0x1B`. |
| `.DMA` / chunk `0x1E` | Root chunk size equals the file. | Morph target packing. `TOneFileAsync<RpDMorphAnimation>` is the consumer; its stream reader was not opened. |
| BON `+2`, `+8`, `+12` | Swapped or stored. `+10` is the node count. `+12` is `5.15` on `SH.BON`. | A read of `+8` or `+12` after `0x80411B3C`. |
| BON node halves at `+4`, `+8`, `+10`, `+12`, `+14` | `+6` is the channel mask (`0x8041C30C`). The others are swapped or stored. `+10` carries values such as `49` and `32817`. | A branch on `+4`, `+8`, `+10`, `+12`, or `+14` that changes the pose. `0x8041CE80` compares `+4` with 4 and `+12` with 6 while building the runtime object; that comparison's effect on the matrix was not separated from the mask at `+6`. |
| BON float at `+32` and `+48`, fourth component | The first three floats are the default rotation and the default post-scale. The copy is 16 and 12 bytes. | A read of the fourth float. |
| BON bytes at `+64` | 8 bytes, not swapped, zero on the sampled nodes. | Any load from node `+64`. |
| Motion `word_4`, `word_16`, `word_18`, flag byte `+13` past bit 0 | Swapped or tested. `word_18` is `240` on `sh_CB` and `35` on `sh_run`. Bit 0 of `+13` selects the element swap width. | A compare that treats `word_18` as a duration, a loop, or a frame rate. |
| MTP words at `+0`, `+4`, `+8`, `+12` | `+2` is the entry count and `+16` is the table. `+4` on `SHADOW.MTP` is `0x00010000`. | Readers of those words. `+4` is not in the swap list of `0x80413DCC`. |
| 49 MTP chunks whose payload is not `prefix + count * 6` | They remain channels in the BON walk. Header bit `0x4000` selects a 6-step versus an 8-step in `0x80414E88`, and bit `0x8000` selects an extra half inside `0x80414B80`. Neither bit, used as a record size, consumes all 49, and `0x8000` is set on keys that are still 6 bytes. The pose leaves that component at the BON default. | A size rule that lands on the end of all 49 payloads and still accepts the 46,421 six-byte chunks. |
| Anim-instance s32 list at `+36` | `0x8041C9D0` allocates it, fills `-1`, then writes `0` at the first entry. `0x8041B88C` uses it as a slot index for one `fmadds` into `0x805E25B0`. The slot count is clamped to 1..4. | A later store that fills the list with bone ids. It is not the chunk-to-bone map. |
| Motion loop, speed, and root extraction | The sampler does not wrap. `0x804167AC` copies four floats to `0x805E2380`; playback reads `+12`. Translation channels are absolute local values. Locator bone `0` has no frame. | The instruction that advances `+12`, and any pass that subtracts the root from the pose. |
| MTP extra records | Pointer at entry `+8`, stride in the u16 at `+2`, stop when that signed half is `-1`. | The meaning of flag bits other than the swap tests `0x8000` and `0x4000` in `0x80414020`. |
| `.SNB` | Magic `NSIF`. | Chunk grammar past the first header. |
| EFFD body | Magic, version, name. | Effect tracks. The DOL has many `Effect::` RTTI names and no field map tied to this file. |
| `.fnt` | Not a RenderWare chunk. Tags `OL`, `SL`, `WL` are visible in `Advertise_EN.fnt`. | The tag directory. |
| CPAF / `.gncp` | Magic. `NGIF` appears inside. | Chunk sizes. |
| `.mlt` | Magic `gcaxMLT`. | Cue records. |
| AFS names | Offset/size table is validated. | The filename block after the table, if the voice id is stored there. |
| ADX samples | Header fields listed in the format map. | The ADPCM decoder at the CRI call site. It was not separated from the rest of the audio library. |
| SFD | Pack header `00 00 01 BA`. | Demux and the Sofdec private streams. |
| `.tp`, `.inf`, `.bnr`, `.srd` | Present on disc. | Any reader. |

## RenderWare TEV

Models do not carry a full GX TEV stage list in the material struct that was decoded. What they do carry is the material color, the three surface floats, the texture filter word, the raster format word, and the raw MatFX plugin. A DX12 renderer can consume those. Inventing TEV stages from the material color would hide the missing plugin reader.
