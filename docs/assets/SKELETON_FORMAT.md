# Skeleton format

The character skeleton is the RenderWare frame list plus the HAnim plugin (`0x11E`). `SH.BON` is a second copy of the same bone ids and local translations. It is not a substitute for the frame list. Field meanings below come from the chunk layout that fills the file exactly, from the ids shared with the frame plugins and with `SH.BON`, and from the parent indices already stored on the frames.

Endian of every word in this section is little-endian. Confidence is **CONFIRMED** unless a row says otherwise.

## Frame list (`0x0E`)

| Offset | Size | Type | Meaning | Source | Evidence |
| --- | --- | --- | --- | --- | --- |
| 0 | 4 | i32 | frame count | struct size `4 + count * 56` | `SHADOW_BODY.DFF` has 31 frames and the struct length matches |
| 4 + 56*i | 12 | 3× f32 | right | root frame is `(1,0,0)` | identity root on the eye and the body |
| +12 | 12 | 3× f32 | up | root is `(0,1,0)` | same |
| +24 | 12 | 3× f32 | at | root is `(0,0,1)` | same |
| +36 | 12 | 3× f32 | position | root is `(0,0,0)` | frame 2 of the body is `(0, 5.15, 0)`, the same vector as BON node `sh_root` |
| +48 | 4 | i32 | parent frame, `-1` for the root | the index walks a tree | frame 15's parent chain reaches frame 2; composing locals puts the head at y `6.5` |
| +52 | 4 | u32 | matrix flags | stored, not decoded | **UNKNOWN**. No bit test was traced |

Each frame then has its own extension chunk. Plugin `0x11E` inside that extension is HAnim.

## HAnim

Two shapes, both little-endian. The hierarchy chunk on the body is `0x17C` bytes, which is `20 + 30 * 12`.

12-byte plugin, one per bone frame:

| Offset | Size | Type | Meaning | Evidence |
| --- | --- | --- | --- | --- |
| 0 | 4 | i32 | header word, observed `256` on the hierarchy; the 12-byte form stores the same first word | size identity |
| 4 | 4 | i32 | bone id | equals `SH.BON` `bone_id` and is unique per bone frame |
| 8 | 4 | i32 | flags word on the 12-byte form | **UNKNOWN** |

Hierarchy plugin, on the body this sits on frame 1:

| Offset | Size | Type | Meaning | Evidence |
| --- | --- | --- | --- | --- |
| 0 | 4 | i32 | observed `256` | same word on Amy and Bee hierarchies |
| 4 | 4 | i32 | hierarchy id, `0` on the body | the frame that carries this chunk is not itself a bone |
| 8 | 4 | i32 | node count | 30, 21, and 20 on body, Amy, and Bee, and the rest of the chunk is exactly `count * 12` |
| 12 | 4 | i32 | flags, `0` on those files | **UNKNOWN** |
| 16 | 4 | i32 | extra, `0x24` on those three files | **UNKNOWN**. No reader of this word was traced |
| 20 + 12*n | 4 | i32 | bone id | matches a 12-byte frame plugin and a BON node, except id `0` |
| +4 | 4 | i32 | index | `0 .. count-1`, each value once, on the body |
| +8 | 4 | i32 | node flags, observed `0, 1, 2, 3` | **UNKNOWN**. Not used as the parent link. Parents come from the frame record |

Plugin id `286` (`0x11E`) is registered at `0x80448270`. The per-field stream reader inside that plugin was not walked instruction by instruction. The layout above is the one that consumes the chunk and lines up with the frame ids and the BON ids.

## How a bone is found

`SHADOW_BODY.DFF`:

- Frame 0 is the clump root. Parent `-1`. No HAnim plugin.
- Frame 1 holds the 30-node hierarchy. Parent 0. Its own id is the hierarchy id `0`, not a skinned bone.
- Frames 2 through 30 each carry one 12-byte HAnim id. Those 29 ids are hierarchy nodes 1 through 29. Hierarchy node 0 has id `0` and no frame.

`node.index` is the slot inside the hierarchy array. It is not the frame number. The frame for a node is the frame whose 12-byte id equals `node.id`.

## Bind pose

The frame basis is the local bind pose. Globals used by the tests are the row-vector product `local * parent`, using right/up/at as the 3×3 and position as the translation. That product puts frame 2 at `(0, 5.15, 0)` and frame 15 (head, parent chain through an identity translation at frame 3) at y `6.5`.

Skin matrices are a different array. They are not these frame matrices. See `SKINNING_FORMAT.md`.

`bind_globals` is this product on the DFF bases. An animated pose uses the same product after `animate_locals` replaces the basis of each frame whose HAnim id equals a BON bone id. The hierarchy frame is not replaced. Bone id `0` has no frame, so its locator matrix is not in the frame list. BON child links order the channel walk. They are not these parents. See `ANIMATION_PIPELINE.md`.
