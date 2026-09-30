# Skinning format

Plugin `0x116` on a non-native geometry. The reader is `0x8044D7A4`. It runs when geometry flags bit `0x01000000` is clear (`bf 2` after the bit test falls through into this function). `SHADOW_BODY.DFF` flags are `0x00010077`, so this is the path those meshes take. The other reader, `0x8044D910`, skips 4 bytes and then 12 bytes per bone. Its size formula does not match these files. Native geometries are not parsed as this layout.

`0x8044CE40` allocates the runtime object: bone count at `+0`, used count at `+4`, matrices at `+12` (`numBones * 64`), max weights at `+16`, indices at `+20` (`numVertices * 4`), weights at `+24` (`numVertices * 16`). `numVertices` is geometry `+20`.

Stream words are little-endian on disk. `0x8047E8B8` / `0x8047E7D4` swap each 32-bit group into the big-endian CPU. Floats and the three trailer words are therefore read as little-endian. Index bytes are kept in file order: after the swap, `0x8044E684` takes the low byte of the native word (`rlwinm ..., 6, 18, 25`, times 64) as influence 0, and `0x8044DB00` shifts the same word by `influence * 8`. That low byte is file byte 0. Vertex 0 of the body is file bytes `[4, 5, 0, 0]` with weights `(0.889807, 0.110193, 0, 0)`.

## Body

| Offset | Size | Type | Meaning | Function | Confidence | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 1 | u8 | bone count | `0x8044D804` `clrlwi , 24` | CONFIRMED | 30, 21, 20 on body, Amy, Bee. Equals the HAnim node count |
| 1 | 1 | u8 | used-bone count | `0x8044D7F8` `rlwinm , 24` | CONFIRMED | 26 on the body. The next bytes are that many ids, each `< bone count` |
| 2 | 1 | u8 | max weights | `0x8044D7FC` `rlwinm , 16`, stored at skin `+16` | CONFIRMED | 2 on the body (weights only in slots 0 and 1). 4 on `BEE.DFF`. `0x8044E644` branches on this byte to the 1, 2, 3, or 4 weight skin loops |
| 3 | 1 | u8 | pad, observed `0` | the leftover byte of the swapped header word | CONFIRMED as a stored byte | no separate use was traced. **UNKNOWN** role |
| 4 | used count | u8 | used-bone ids | `0x8044D834` raw read into skin `+8`, no swap | CONFIRMED | body ids max 28, 26 unique. `0x8044E428` loads one byte and shifts it by 6 to index a 64-byte matrix |
| 4 + used | vertex count × 4 | u8 | four bone indices, file order | `0x8044D854` swapped read into skin `+20` | CONFIRMED | see the low-byte extract above. Every body index is `< 30` |
| next | vertex count × 16 | f32 | four weights | `0x8044D874` swapped float read into skin `+24` | CONFIRMED | every body and Bee weight is in `0..1` and the four sum to 1 within `1e-3` |
| next | bone count × 64 | 16× f32 | bone matrix, right / up / at / position | `0x8044D89C` `numBones * 64` into skin `+12` | CONFIRMED as the matrix block | 30 orthogonal matrices on the body. Bone 0 translation `(0, 5.15, 0)`. They are **not** named inverse-bind: multiplying them by the bind globals, either order, does not produce identity |
| next | 12 | 3× u32 | trailer counts | `0x8044CC4C` | CONFIRMED | all three are 0 on the body, Amy, and Bee. The middle word is compared with `cmpwi , 0; bf 1`. `bf 1` leaves the function when the count is not greater than 0, which is the path these files take |
| next | boneCount + 2×trailer[1] + 2×trailer[2] | bytes | split tail | `0x8044CC4C` when trailer[1] `> 0` | CONFIRMED as a length | the three counts are stored at skin `+48/+52/+56`. The allocated bytes are split at `numBones` and at `2 * trailer[1]`. No file in the test set has trailer[1] `> 0`, so the bytes inside the tail are **UNKNOWN** |

`0x8044DD10` stores the skin pointer on the geometry. It does not read further fields.

`0x8044E394` walks the used-bone bytes, scales each by 64, and calls `0x8047FC08` with that matrix and a hierarchy matrix. `0x8047FC08` is a paired-single matrix multiply; the listing does not decode the product order, and it ANDs the flag words at matrix `+12`. The 16 floats are kept in file order.

The 2-weight vertex loop is `0x80451FB8`, reached from `0x8044E6CC` when max weights is 2. The paired-single blend in that function did not disassemble to scalar operations, so the PC runtime does not invent a two-weight vertex shader. Indices and weights are stored per vertex, four slots, with no compiled cap. Bee's fourth slot is present and its significant weights (`> 0.01`) sit in the first two slots; the file's max-weight byte is still 4.

A skin whose bytes do not match this consumption throws. A native geometry's `0x116` chunk is kept as an unknown plugin instead of being forced through this reader.
