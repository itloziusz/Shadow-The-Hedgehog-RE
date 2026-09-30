# Resource dependencies

Dependencies below are edges that the files actually contain, or path templates the DOL uses to open the next file. An edge is not inferred from a filename alone.

## Inside one ONE archive

`TOneFileAsync<T>` loads a single named entry from an archive that is already open (`0x8004CA50` plus the typed constructor). Entries in the same archive are the dependency scope the original lookup uses.

Confirmed on `character/shadow.one`:

- `SHADOW_EYE_L.DFF` material texture name `pl_bw03n`
- `SHADOW.TXD` contains a native texture named `pl_bw03n`
- The PC loader records that as `texture` / archive `character/shadow.one` / entry `SHADOW.TXD` / name `pl_bw03n`, resolved

The same pass records every material texture name on a clump. A name that is not in any texture dictionary in that archive stays unresolved and is still reported. It is not replaced with a default texture.

Other co-resident entries in that archive, not automatically bound:

- `SH.BON` carries the same 30 bone ids as the body HAnim hierarchy, and its translations match the framed bones. `LTE.BON` and `RTE.BON` are separate 7-node hand skeletons. The loader exposes each file on its own. It does not replace DFF frame parents with the BON child links.
- `SHADOW.MTP` is the motion pack (158 clips). Each motion's inner name selects `SH.BON`, `LTE.BON`, or `RTE.BON`, and `0x8041C30C` assigns that BON's chunks to bones. Hand clips stay on the hand skeletons. `.UVA`, `.SNB`, and `.DMA` stay separate as well.

## Stage files the DOL names together

For a stage id, the DOL formats several paths. These are request dependencies, not pointers inside one file:

| Template | Role |
| --- | --- |
| `%s/%s_%02d.one` | stage part archive (geometry, collision BSP, RG1) |
| `%s/%s_TEX%02d.one` | part textures |
| `%s/%s_texCM.one` | common stage textures (`0x804AC2CC`) |
| `stg%04d/stg%04d_dat.one` | `PATH.PTP`, `*.ADB`, `*.BDT` |
| `stg%04d/stg%04d_fx.one` | `EFFD` effect |
| `stg%04d/stg%04d_gdt.one` | gadget BIN |
| `stg%04d/stg%04d.fog` | fog bytes |
| `stg%04d/stg%04d_light.bin` | light floats |
| `stg%04d/stg%04d_cmn.dat`, `_nrm.dat`, `_hrd.dat`, `_ds1.dat` | loose stage data |
| `stg%04d/stg%04d_cam.dat` | camera data |
| `stg%04d/stg%04d_hint.bin` | hint data |

`stg0403` contains `stg0403_98.one`. The part list is whatever the directory holds. `LandALoadManager`'s 15 pending flags are not the number of parts.

Within `stg0100/stg0100_01.one` the entries are one `.BSP` world and four `.RG1` regions. The region prelude stores the name `stg0100_d_01`. That name matches the entry. It does not point at a second file by offset.

## Characters, enemies, weapons

The DOL names these archives directly:

- `common/WeaponResource.one`
- `common/vehicleResource.one` (contains `CV_COL_HIT0.CCL`)
- `common/CommonEffect.one`, `common/SShadowEffect.one`, `common/LensFlare.one`
- `enemy/*.one`, `enemy/EnemyPath.one` (`*.PTB`)
- `enemy/boss/*.one`

A CCL payload is a sized blob (length word equals the buffer). It lives in the vehicle archive next to vehicle meshes. The collision records inside the blob are UNKNOWN, so the loader keeps the blob and does not invent a link from a triangle to a hit volume.

## UI, fonts, events

- `fonts/%s/%s%s.txd` and `fonts/%s.met` are a pair by the format strings. The METRICS1 file for a font is text. The `.fnt` bytes are not yet a glyph table, so the loader does not join them into a font object.
- `csdFiles/` `.gncp` (`CPAF`) and `.txd` files sit in the same folders. No chunk inside CPAF was shown to name a TXD, so they are not linked.
- `event/event%04d.scr` and `event\event%04d_scene%c.one` are requested together. Scene ONEs contain `.CEN` text and model entries. The script grammar was not traced.

## Audio and video

- `sng_stg%04d.adx`, `sng_EV%04d_E.adx`, battle and system ADX names are individual files. No model points at them.
- `PRS_VOICE_E.afs` is the voice bank. Entry offsets are inside that file. Which voice id plays for which event is not in the AFS header that was parsed.
- `sound/AudioData*.bin` and `sound/B7*.mlt` (`gcaxMLT`) are named from the DOL next to stages. Their internal cue tables were not parsed, so they stay raw.
- `E1001.sfd` and the other Sofdec files are cutscene streams. They are not embedded in the ONE archives.

## How the PC object records an edge

`ResourceSystem::dependencies` returns one `Dependency` per material texture name after `load` of a clump entry. `resolved` is true only when a texture dictionary in the same ONE contains that exact name. The test for `SHADOW_EYE_L.DFF` requires `pl_bw03n` resolved to `SHADOW.TXD`.
