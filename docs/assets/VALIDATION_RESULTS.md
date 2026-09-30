# Validation results

Command:

```text
cmake -S asset_system -B asset_system/build -G "Visual Studio 18 2026" -A x64
cmake --build asset_system/build --config Release --target asset_tests
asset_system/build/Release/asset_tests.exe
```

Result: `asset_tests passed` (exit 0).

Printed markers:

```text
EYE_FNV 3d9da5730067cf3d
RAW_ENTRY event8006_sceneE.one EVENT8006.CEN
```

`EYE_FNV` is FNV-1a 64 of the 45 eye positions, little-endian float bytes. It is a regression lock for that mesh, not a hash taken from GameCube RAM. The original heap image was not captured in this workspace, so runtime allocation addresses were not compared.

## Archives

| Check | Result |
| --- | --- |
| Every `*.one` under `files/` opens | 1,244 / 1,244 |
| `One Ver 0.60` | 1,231 |
| `One Ver 0.50` | 13, including `DebugModels.one` |
| `character/shadow.one` entry count | 28, matching the word at `+0x1C` |
| Every shadow entry, PRS output length | equal to the directory size |
| Case-insensitive find `shadow_eye_l.dff` | hits `SHADOW_EYE_L.DFF` |
| Uncompressed entry | `EVENT8006.CEN` in `event8006_sceneE.one`, copied length equals the directory size |
| 32-byte buffer passed to `open_one` | throws |
| `DebugModels.one` (0.50) `CUBE_MODEL.DFF` | clump parses, positions non-empty |

## Clumps

| Asset | Check | Result |
| --- | --- | --- |
| `SHADOW_EYE_L.DFF` | frames | 2, root parent `-1` |
| | vertices | 45 |
| | triangles | 67 |
| | binmesh indices | 91, one mesh |
| | material texture | `pl_bw03n` |
| | atomics | 1 |
| `SHADOW_BODY.DFF` | frames | 31 |
| | vertices | 1,382 |
| | binmesh | 4 meshes, 2,703 indices (`0xA8F`) |
| | HAnim hierarchy | 1 hierarchy, 30 nodes, indices `0..29` each once |
| | skin | 30 bones, 26 used, max weights 2, 1,382×4 indices and weights, 30 matrices, trailer words 0 |
| | vertex 0 | indices `[4, 5, 0, 0]`, weights `0.889807` and `0.110193` |
| | weight sums | every vertex in `0..1` and within `1e-3` of 1 |
| | bind pose | frame 2 global Y `5.15`, frame 15 global Y `6.5` |
| `AMY.DFF` | frames / bones / vertices | 22 frames, 21 bones, max weights 2, 930 vertices, hierarchy check empty |
| `BEE.DFF` | skin | 20 bones, max weights 4, four slots stored per vertex, weights above `0.01` only in slots 0 and 1 |

## Skeleton resources in `character/shadow.one`

| Asset | Check | Result |
| --- | --- | --- |
| `SH.BON` | nodes | 30, name `sh`, endian flag 1, `header_12` `5.15` |
| | ids and translation | 29 framed bones match the DFF local translation; id `0` has no frame |
| | child offsets | every nonzero child is `48 + n * 112` |
| `LTE.BON` / `RTE.BON` | nodes | 7 and 7, names `lte` and `rte` |
| `SHADOW.MTP` | motions | 158, unique names, 122 inner name `sh` (357 chunks), 36 hand motions (81 chunks) |
| | channel map | `SH.BON` walk is 357 bindings, `LTE.BON` and `RTE.BON` are 81, and every motion's track count equals the BON named by its inner name |
| | first `sh` bindings | bone 0 translation x/y/z, then rotation x/y/z, then post-scale x/y/z; bone 1 starts with pre-scale x |
| | `sh_idle` at clock 0 | `sh_root` 3×3 is identity and local Y equals that bone's translation-Y sample, `4.48828125` |
| | `sh_run` at clock 0.5 | local matrices differ from `sh_idle`; composed globals are finite and match a second compose |
| | unsplit channels | 49 across the pack; those components keep the BON default |
| | hands | 7/7 `LTE.BON` ids are in `SHADOW_HAND_L.DFF`; 7/7 `RTE.BON` ids are in `SHADOW_HAND_R.DFF`; left-hand ids are absent from the body |
| | `sh_CB` | inner name `sh`, `word_18` `240`, a constant zero key samples to 0 |
| `AMY.MTP` / `AMY.BON` | channel map | 249 bindings, every motion's inner name is that BON, samples at clock 0 are finite |
| `BEE.MTP` | channel map | body BON 237 and wing BON 45; every motion matches one of those two names |
| `ResourceSystem` | `SH.BON`, `SHADOW.MTP` | `NativeBon` with 30 nodes, `NativeMotionPack` with 158 motions |

Negative checks that throw or return a problem: bone index 40 with weight 1, weights that sum to 0.25, a BON child offset outside the node table, a motion whose size word does not match its chunks, a non-finite key time, a hierarchy node id with no frame, and a duplicated hierarchy index.

No original CPU or GPU capture of a bone matrix is in the workspace. The bind numbers above are the composed frame matrices, checked against the translations stored in the DFF and in `SH.BON`. The animated numbers are the reconstructed local matrices at a stated clock. They are not a dump of GameCube RAM. The skin-matrix multiply in `0x8047FC08` is not part of this comparison.

## Textures

`SHADOW.TXD` count field 7, seven native textures, platform id 6, edges 32, 128, 128, 32, 64, 64, 32. Image bytes start at struct offset 104 (image length + 104 = struct length).

`number.txd` is a loose dictionary with one texture and a non-zero edge.

## World

| Asset | Check | Result |
| --- | --- | --- |
| `stg0100/stg0100_01.one` / `STG0100_COLI_01.BSP` | materials | 4 |
| | chunk `0x09` | present, non-empty |
| | bounds | not the zero vector |
| `STG0100_D_01.RG1` | prelude name | `stg0100_d_01` |

## Disc table and loose files

| File | Check | Result |
| --- | --- | --- |
| `sys/fst.bin` | `character/shadow.one` | found, length 665,612, equal to the extracted file |
| `PRS_VOICE_E.afs` | entry table | count matches the vector, count > 3, entries inside the file |
| `sng_stg0100.adx` | header | rate 48000, 2 channels, copyright text contains `CRI` |
| `setid.bin` | records | 302 |
| `E1001.sfd` | pack header | accepted, size > 1000 |
| `stg0100_dat.one` / `PATH.PTP` | inflated size | `0x2374`, first big-endian word matches |

## PC handles and limits

| Check | Result |
| --- | --- |
| Queue of 6 requests, then `pump` | queue returns to 0. The original reader stops at 3 slots. |
| `queue_limit` | greater than 3 |
| Eye model through `ResourceSystem` | 45 positions, 67 triangles, 2 bones |
| Dependency `pl_bw03n` | resolved to entry `SHADOW.TXD` |
| Handle after `release` | `model()` is null |
| `stg0403` file list | more than 15 files, includes `stg0403_98.one` |

## Not compared

No Dolphin memory dump or allocation trace is in the workspace, so these were not checked against a live game:

- guest heap addresses
- the 15 `LandALoadManager` flag bytes at runtime
- TEV register values after a material upload
- decoded ADX PCM
- world-sector triangle coordinates

Those stay open until that evidence exists. The structural checks above are the ones the files and the traced branches can support.
