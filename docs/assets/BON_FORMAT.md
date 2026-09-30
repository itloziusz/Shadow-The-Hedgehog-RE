# BON format

`.BON` is loaded from the extension table at `0x80013E7C` (string `.BON` at `r2 - 32416`). The file is passed to `0x80411A14`, which calls `0x80411A9C`. That function rejects a first byte below 4. Byte 1 is the endian flag used with `0x8042221C`: the multi-byte fields are byte-swapped only when the flag disagrees with the host. `SH.BON`, `LTE.BON`, and `RTE.BON` have byte 1 = 1, and `48 + nodeCount * 112` equals the file size only when the multi-byte fields are read big-endian (30 × 112 + 48 = 3408, 7 × 112 + 48 = 832). The PC reader therefore treats flag 1 as big-endian and flag 0 as little-endian.

`0x80411D90` is called on file offset 48 and then on the two child offsets. The node stride used to step a following top-level record is `u16 at +10`, times 112, plus 48. These files contain one record.

## Record

| Offset | Size | Type | Endian | Meaning | Function | Confidence | Evidence |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 1 | u8 | — | version, must be `≥ 4` | `0x80411AC4` | CONFIRMED | all three files start with `4` |
| 1 | 1 | u8 | — | endian flag | `0x80411AD0` and `0x8042221C` | CONFIRMED | flag 1 matches the size identity above |
| 2 | 2 | u16 | file | not in the swap list | — | UNKNOWN | zero on these files |
| 4 | 4 | u32 | file | swapped, then replaced by the name-lookup pointer | `0x80411B0C`, `0x80421F30` | PARTIAL | zero on disk. The runtime store overwrites it |
| 8 | 2 | u16 | file | swapped | `0x80411B1C` | UNKNOWN | `255` (`0x00FF`) on all three files. No later read was traced |
| 10 | 2 | u16 | file | node count, times 112 | `0x80411B8C` `mulli , 112` | CONFIRMED | 30 and 7, and the product fills the file |
| 12 | 4 | f32 | file | swapped as a u32 | `0x80411B3C` | UNKNOWN | `5.15` on `SH.BON` (the root translation's Y) and `≈ 0.852` on the hand files. No use after the swap was traced |
| 16 | 32 | bytes | — | name | `0x80411B60` passes `file+16` to `0x80421F30` | CONFIRMED | `sh`, `lte`, `rte` |

## Node (112 bytes)

Base of node `i` is `48 + i * 112`. Child offsets are relative to the start of the file and land on those bases.

| Offset | Size | Type | Meaning | Function | Confidence | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 4 | u32 | bone id | swapped at `0x80411DE4`, passed with the name to `0x80421DF4` | CONFIRMED | the 30 `SH.BON` ids are the HAnim ids, including `0`, `1`, `0x28`, `0x2D` |
| 4 | 2 | u16 | swapped half | `0x80411DF4` | UNKNOWN | `1` on the node at offset 48, `0` on the rest |
| 6 | 2 | u16 | channel mask | `0x8041C30C` tests bits `0x0400`, `0x0007`, `0x0040`, `0x0038`, `0x0380` | CONFIRMED | `0x03BF` on the first node (9 chunks), `0x07BF` on the rest (12). The walk sums to the motion's chunk count |
| 8 | 2 | u16 | swapped half | `0x80411E14` | UNKNOWN | `0` on `SH.BON` |
| 10 | 2 | u16 | swapped half | `0x80411E24` | UNKNOWN | values such as `49`, `32817`, `16433` |
| 12 | 2 | u16 | swapped half | `0x80411E34` | UNKNOWN | `0` on `SH.BON` |
| 14 | 2 | u16 | not in the swap list | — | UNKNOWN | `0` on these files. Stored in file endian |
| 16 | 16 | 4× f32 | translation, xyz plus a zero | four swapped u32s at `0x80411E44` | CONFIRMED for xyz | matches the DFF frame translation of the same bone id on all 29 framed body bones |
| 32 | 16 | 4× f32 | default rotation x, y, z, and an unread fourth float | `0x8041CE80` copies 16 bytes from node `+32`; `0x80420B9C` reads the first three | CONFIRMED | channels overwrite a component when its mask bit is set. The apply order is Z, then Y, then X |
| 48 | 16 | 4× f32 | default post-rotation scale x, y, z, and an unread fourth float | `0x8041CE80` copies 12 bytes from node `+48` into the group that calls `0x8040C7E0` | CONFIRMED | `1, 1, 1, 0` on the sampled nodes. The leading scale group (bit `0x0400`) is initialized to `1, 1, 1` instead |
| 64 | 8 | bytes | not swapped | — | UNKNOWN | zero |
| 72 | 4 | u32 | child offset A | swapped, added to the record base, recurses | CONFIRMED | `0x80411EA4`. Offsets are `0` or `48 + n * 112` |
| 76 | 4 | u32 | child offset B | same | CONFIRMED | `0x80411FE0` |
| 80 | 32 | bytes | name | `0x80411E90` passes `node+80` to the name registry | CONFIRMED | `sh_root`, `sh_j_kosi`, `shadow_locator`, `sh_j_l1te`, … |

The two child links are the links `0x80411D90` walks, and they are the recursion order of `0x8041C30C` (child A, then child B). On `SH.BON` that walk visits each node once. The skeleton parent of a bone is still the DFF frame parent, not these links. The loader does not replace frame parents with BON children.

`LTE.BON` and `RTE.BON` use the same record. Their ids (`0x46` and up, `0x32` and up) are not in the body hierarchy. They are separate skeletons whose names are the hand joints (`sh_j_l1yubi1`, `sh_j_r1yubi1`, …).
