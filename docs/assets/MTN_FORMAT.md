# MTN and MTP format

`character/shadow.one` does not contain a loose `.MTN`. The character motions are `SHADOW.MTP`. Both extensions are dispatched from the same table:

- `.MTN` at `0x80013FBC` calls `0x80413C4C`. That function calls `0x80414A04` on the file. `0x80414A04` returns failure when byte 0 is below 3.
- `.MTP` at `0x80014068` calls `0x80413BC4`, which calls `0x80413DCC` on the file.

`0x80413DCC` is the container walker. `0x80414A04` is the same header walk used on each motion object inside the container (version byte `≥ 3`, endian flag, then the chunk chain at offset 52). `SHADOW.MTP` byte 5 is 1, and the directory is coherent only as big-endian: 158 entries, table at offset `0x14`, `20 + 158 * 12 = 1916`.

There is no second MTP on the disc in this workspace. The container rule is the one `0x80413DCC` implements; `SHADOW.MTP` is the file it was checked against. A standalone buffer whose byte 0 is `≥ 3` and whose size word equals the buffer is parsed as one motion (`parse_motion`), which is the `.MTN` shape.

Endian flag 1 means big-endian multi-byte fields. Flag 0 means little-endian. The flag is the byte `0x80413DCC` tests at object `+5` for a pack and at object `+1` for a motion.

## MTP directory

| Offset | Size | Type | Meaning | Function | Confidence | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 2 | u16 | swapped | `0x80413E24` | UNKNOWN | value `1` on `SHADOW.MTP` |
| 2 | 2 | u16 | entry count | `0x80414088` compares the loop index with this half | CONFIRMED | 158, and 158 names and motions follow |
| 4 | 4 | u32 | not in the swap list of `0x80413DCC` | — | UNKNOWN | `0x00010000` |
| 5 | 1 | u8 | endian flag (this overlaps the previous word) | `0x80413DE4` | CONFIRMED | `1`, and the offsets below only fit big-endian |
| 8 | 4 | u32 | not swapped by that function | — | UNKNOWN | `0` |
| 12 | 4 | u32 | not swapped by that function | — | UNKNOWN | `0` |
| 16 | 4 | u32 | offset of the entry array, then relocated by adding the base | `0x80413E44`, `0x80413E58` | CONFIRMED | `0x14` |
| 20 + 12*i | 4 | u32 | name offset | relocated, then used as a C string | CONFIRMED | `lte_CB`, `sh_run`, `sh_wall_run_r` |
| +4 | 4 | u32 | motion offset | relocated, byte 0 of the target is checked against 3 | CONFIRMED | each target parses as a motion whose size word reaches the next region |
| +8 | 4 | u32 | optional extra, `0` if absent | `0x80413FE8`. If nonzero, records advance by the u16 at `+2` until that signed half is `-1` | CONFIRMED as a pointer | 76 motions have a nonzero extra. Flag bits `0x8000` and `0x4000` select how many inner halfwords `0x80414020` swaps. The payload past the 4-byte header is otherwise **UNKNOWN** |

Entry names in this file are unique (158 distinct strings).

## Motion object

Relative to the motion offset.

| Offset | Size | Type | Meaning | Function | Confidence | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 1 | u8 | version, must be `≥ 3` | `0x80413EDC` / `0x80414A20` | CONFIRMED | `4` on the sampled motions |
| 1 | 1 | u8 | endian flag | the same XOR as BON | CONFIRMED | `1` |
| 4 | 4 | u32 | swapped | `0x80413F20` | UNKNOWN | `0` on `lte_CB` and `sh_CB` |
| 8 | 4 | u32 | size of this object | swapped at `0x80413F30`. The chunk walk consumes exactly this many bytes | CONFIRMED | `lte_CB` is 2808; the next motion begins 2808 bytes later |
| 13 | 1 | u8 | flag bits | bit 0 selects u16 versus u32 swaps of chunk elements (`0x80413F84`). Bits 0, 1, and 2 make `0x80414A04` return 0, which rejects a buffer as a direct `.MTN` | PARTIAL | `SHADOW.MTP` motions use `0x07`, so they pass the MTP walker and would fail `0x80414A04`. The bit names beyond that branch are **UNKNOWN** |
| 16 | 2 | u16 | swapped | `0x80413F40` | UNKNOWN | `0` on the first motions |
| 18 | 2 | u16 | swapped | `0x80413F50` | UNKNOWN | `240` on `sh_CB`, `35` on `sh_run`, `100` on `sh_idle`. Not shown to be the duration: key times are not bounded by this half in the reader that was traced |
| 20 | 32 | bytes | name | fixed gap before the chunk loop at offset 52 | CONFIRMED | `sh`, `lte`, `rte`. 122 motions named `sh`, 18 `lte`, 18 `rte` |
| 52 | — | chunks | see below | `addi r30, r29, 52` in `0x80413F54` | CONFIRMED | every one of the 158 motions is filled by the chunk chain and nothing is left over |

## Chunk

| Offset | Size | Type | Meaning | Function | Confidence | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 4 | u32 | chunk size, including this header | advance `r27 + size` while byte `+7` is nonzero; a zero byte ends the chain | CONFIRMED | 46,470 chunks, each motion's cursor equals its size word |
| 4 | 2 | u16 | group count | swapped | STRONGLY INFERRED | for 46,421 chunks, `payloadBytes == byte6 + groupCount * 6` |
| 6 | 1 | u8 | prefix length | the bytes before the 6-byte groups | STRONGLY INFERRED | only `0` or `2` occur. `2` is a 2-byte prefix, often `0xC000` |
| 7 | 1 | s8 | link | `extsb` then compare with 0. Nonzero continues | CONFIRMED | the chain stops on the size word either way; a zero ends it early |
| 8 | size − 8 | bytes | payload | element swap is u32s when flag bit 0 is set | PARTIAL | the 6-byte split covers 46,421 chunks. The other 49 do not match `prefix + count * 6` and stay raw |

A 6-byte group is three u16s. `0x80414B80` reads three halfwords when bit `0x8000` of the first half is clear, and a fourth half when that bit is set. In this file the group is 6 bytes even when that bit is set, so the fourth half is not in the group. The three values are still decoded with the two converters that function uses:

- Low 14 bits of the first half become a float through the `0x43300000` / `0x80000000` double subtract at `r2+5848` (`decode_motion_u14`). The top bits `0x8000` and `0x4000` are the flag bits that function tests.
- The other two halfwords become floats through `decode_motion_f16`: a zero half is `0` (the constant at `r2+5840`). Otherwise the low 15 bits are shifted left 13, `0x38000000` is added, and bit 15 is the sign.

`0x80415768` then evaluates the current key as `value_c` when `value_b` is 0, and as `value_b * (clock - time) + value_c` otherwise. The compare epsilon subtracted from the key time is the float at `r2+5856` (`≈ 0.0001`).

Chunk order is not stored in the MTP entry. `0x8041C30C` consumes chunks with the cursor `0x8041352C` while walking BON nodes. The halfword at BON node `+6` is the mask. Child links at `+72` and `+76` are the recursion, child A then child B. The resulting order is the same for every motion that names that skeleton:

- `SH.BON` → 357 channels. Every `sh` motion has 357 chunks.
- `LTE.BON` and `RTE.BON` → 81. Every `lte` and `rte` motion has 81 chunks.
- `AMY.BON` → 249. `BEE.BON` → 237. `BEEWING.BON` → 45. Each of those packs' motions matches the BON named by its inner name.

The bit tests and the functions that consume the sampled float are in `ANIMATION_PIPELINE.md`. Nothing multiplies the node count by 12. The `12 * nodes - 3` count is what the mask produces: the first node is `0x03BF` (9 channels) and the rest are `0x07BF` (12).

The 49 payloads that are not `prefix + count * 6` are still chunks in that sequence. `0x80414E88` places the cursor at the end of the chunk and steps by 6 or 8 using `rlwinm` rotate 18, mask bit 31, which is header bit `0x4000`. Trying that bit, and trying `0x8000`, as the 8-byte switch does not land on the end of every one of the 49. The same switch would also split chunks that are exactly six bytes per group, including keys whose header has `0x8000` set (135,101 of 158,725 keys in the 6-byte chunks). Those 49 payloads stay raw. A channel whose chunk did not split keeps the BON default for that component. `SHADOW.MTP` has 49 such channels in total; `sh_idle` has none.

Not claimed: that key times are sorted on every track, that `word_16` or `word_18` is a duration, or that the clock wraps. The sampler reads the float at global `0x805E2380+12` and does not wrap it.
