# Animation pipeline

The path that is implemented, and the step that is not.

```
DFF / BON / MTP
    → shadow::gc   (endian, counts, and the fields a traced function reads)
    → native structs (dynamic arrays, host floats)
    → shadow::pc   (channel map, local pose, hierarchy matrices)
```

No character name selects a parser. `SHADOW_BODY.DFF`, `BEE.DFF`, `SH.BON`, and `SHADOW.MTP` go through the same functions. Counts come from the files: 30 body bones, 20 Bee bones, 7 hand bones, 158 motions, 46,470 chunks.

## What the original does

1. **Load the DFF.** The clump reader walks chunk `0x10`. Frames, geometry, and the extensions are nested chunks. `SHADOW_BODY.DFF` is one geometry, 1,382 vertices, 31 frames.

2. **Find the hierarchy.** Frame 1's extension contains HAnim `0x11E` with 30 nodes. Frames 2–30 each carry the matching bone id. Parents are the frame parent indices. See `SKELETON_FORMAT.md`.

3. **Associate skin with vertices.** Geometry extension `0x116` is read by `0x8044D7A4` using `geometry+20` as the vertex count. Four indices and four weights are stored per vertex.

4. **Resolve bone indices.** Influence `i` is file byte `i` of that vertex's four index bytes. `0x8044E684` and `0x8044DB00` both take the bytes in that order out of the swapped word. Used-bone ids are a separate byte list and are multiplied by 64 to reach a matrix.

5. **Load BON and MTP.** `.BON` → `0x80411A9C`. `.MTP` → `0x80413DCC`, then one motion object per directory entry. `.MTN` would be a single motion object via `0x80413C4C`. The disc character archive ships the pack, not loose `.MTN` files.

6. **Map chunks onto bones.** `0x8041C30C` walks the BON from the first node. Child A (`+72`) then child B (`+76`) are file offsets of the next nodes. The halfword at node `+6` decides how many chunks the cursor `0x8041352C` consumes, in file order. The same walk on `SH.BON` consumes 357 chunks, on `LTE.BON` and `RTE.BON` 81, on `AMY.BON` 249, and on `BEE.BON` 237. `map_skeleton_channels` is that walk. It does not read the motion; every motion for that skeleton uses the same order.

   The mask, the consumer, and the order inside one node:

   | Node `+6` | Chunks | Consumer | What the float is |
   | --- | --- | --- | --- |
   | bit `0x0400` | 3, always | `0x80420B28` → `0x8040C7E0` | scale x, y, z, applied first |
   | bits `0x0001`, `0x0002`, `0x0004` | one each, if set | `0x80420D10` → `0x8040C5EC` | local translation x, y, z |
   | bits `0x0008`, `0x0010`, `0x0020`, and only when `0x0040` is clear | one each, if set | `0x80420B9C` | rotation x, y, z. Applied as Z, then Y, then X |
   | bits `0x0080`, `0x0100`, `0x0200` | one each, if set | `0x80420B28` → `0x8040C7E0` | scale x, y, z, applied after the rotation |

   `0x03BF` is the locator: no leading scale, nine chunks. `0x07BF` is every other sampled node: all twelve. `9 + 29 * 12 = 357`.

   `0x8041B88C` is not this map. `0x8041C9D0` allocates the anim instance, writes `-1` through the s32 list at `+36`, then stores `0` in the first entry. `0x8041B88C` walks that list and does one `fmadds` into the float table at `0x805E25B0`. The list length follows the slot argument clamped to 1..4. It does not name bones.

7. **Decode keyframes.** Where `payload size = prefix byte + group count * 6` (46,421 of 46,470 chunks), each group is three u16s decoded by `0x80414B80`. The other 49 stay raw. A forward walk that treats header bit `0x8000` or `0x4000` as an 8-byte key does not cover all 49, and it disagrees with the 46,421 chunks that are exactly six bytes per group while `0x8000` is set. See `MTN_FORMAT.md`.

8. **Interpolate.** `sample_channel` applies `0x80415768`: the key is the last one with `clock > time - 0.0001`, or the first key if the clock has not passed any. The result is `value_c` when `value_b` is 0, otherwise `value_b * (clock - time) + value_c`. One channel, one float. An unsplit chunk does not replace the BON default for that component.

9. **Build the local matrix, then the hierarchy.** `animate_locals` starts from identity. It column-scales (`0x8040C7E0` skips the call when x, y, and z are all `1`), adds the translation in the current axes (`0x8040C5EC`), then right-multiplies Z, Y, and X rotations. Each angle is `trunc(sample * 65536 / 2π)` and, when that integer is not 0, the low 16 bits as a signed step times `2π/65536`. `0x80031568` writes the sine and cosine used by `0x8040C110`, `0x8040C188`, and `0x8040C200`. The constants are `r2+5916` (`65536`) and `r2+5920` (`2π`). `pose_globals` writes that local over the DFF frame with the same HAnim id and composes `local * parent`. The hierarchy frame is left on its bind basis. Bone id `0` has no frame.

   On `sh_idle` at clock 0, `sh_root` (bone id 1) is an identity 3×3 and its Y equals the sampled translation channel, `4.48828125`, which is not the bind Y `5.15`. `sh_run` at clock `0.5` is a different pose. Both are finite, and composing either pose twice matches.

10. **Skin the vertices.** Still open. `0x8044E394` passes a 16-float skin matrix and a hierarchy matrix to `0x8047FC08`. The paired-single product order was not recovered, and the 2-weight loop `0x80451FB8` did not disassemble to scalar arithmetic. The pose path stops at the hierarchy matrices. It does not transform vertices and does not rename the skin matrices.

## Loop, root motion, compression

- **Compression.** The key packing is the `u14` time and the `f16` component above. Both constants were read from `r2` (`0x805FA780 + 5840/5848/5856`).
- **Loop.** `0x80415768`, `0x80414B80`, and `0x8041C01C` do not wrap the clock. `0x804167AC` copies four floats into the clock global `0x805E2380`; the sampler reads the float at `+12`. `word_16` and `word_18` are converted when a motion is looked up. Nothing traced here treats `word_18` as a duration or a loop length.
- **Root motion.** Translation channels are absolute local translation, not a delta on the bind pose. Bone id `0` (`shadow_locator`) is animated and has no DFF frame. Bone id `1` (`sh_root`) is the framed root. No separate extraction pass was found. Frame 0 and the hierarchy frame keep their bind basis.
- **Hands.** The motion's inner name selects the BON (`sh`, `lte`, `rte`). Hand chunks are not remapped onto the body. All 7 `LTE.BON` ids occur in `SHADOW_HAND_L.DFF` (8 frames). All 7 `RTE.BON` ids occur in `SHADOW_HAND_R.DFF`. None of the left-hand ids occur on the body frames.

## Native types

`shadow::pc` keeps host-endian copies: one bone per frame, one hierarchy node per file node, four influences per vertex, one track per chunk, one key per decoded group, one motion per MTP entry. Nothing in those vectors is capped at 30 bones, 4 influences, or a fixed key count. The four influence slots and the one-byte bone count are how this plugin encodes the data, not a PC ceiling.

`map_skeleton_channels`, `animate_locals`, and `pose_globals` are the runtime side. They take the native BON, motion, and model. Counts and channel order come from the BON mask and the chunk list. `ResourceSystem::load` of `SH.BON` returns `NativeBon`. `SHADOW.MTP` returns `NativeMotionPack`. A `.DFF` still returns `NativeModel`, with the skin indices, weights, and matrices filled and not yet multiplied by the pose.
