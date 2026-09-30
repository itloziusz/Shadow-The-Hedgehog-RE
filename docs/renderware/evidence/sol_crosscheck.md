# Cross-check of Sol's contribution against the target DOL

## Provenance and scope

The user supplied `GPT_SOL_analysis/` on 2026-09-14. All five files were read. They are preserved unchanged. Sol's report identifies its input as 31 assembly files from `original.zip`, not an independently hashed DOL or a runtime trace. Its 31 CSV entry addresses agree with the corresponding entries in this investigation's original-byte-verified manifest. The archive itself was not present in the supplied directory, so no archive checksum/provenance claim is made.

Target remains `main.dol`, SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`. Authoritative local bytes and exact per-function instruction listings are in `address_manifest.json`, `data_manifest.json`, and `original/`. This report distinguishes independent interpretation of the same instructions from independent acquisition of another binary.

## Findings that agree with the original instructions

- Draw core `0x8042E254–0x8042E838`; save core `0x8042E838–0x8042E9C4`; manager bridges `0x8042F16C` and `0x8042F140`.
- State allocation is `0x13C`; parameters occupy a 0x10-byte prefix; enable and alpha are separate bytes; geometry scale is at +4.
- Four 0x18-byte vertices at each of +0x14, +0x7C, +0xDC; a retained raster view at +0x74 and a separate validity byte at +0x78.
- Save contains two copies and two conditional draw submissions: preserve full EFB in the manager scratch raster, render the reduced image, copy it to history, restore the full image.
- Composite binds the parent of the history view, uses RW blend values 5/6 and vertex alpha, and saves/restores exactly state IDs 10,11,12,6,8,14,9. Texture state 1 is not saved/restored.
- The view/backing raster pair is ownership structure, not two alternating history images.
- Original task and GX addresses in Sol's CSV agree with the independently decoded local targets, including the corrected local GXCopyTex address `0x803975AC`.

## Semantic differences that must not be carried into the reconstruction

| Sol artifact and lines | Difference | Original evidence and required semantics |
|---|---|---|
| `motion_blur_reconstructed.cpp:193–196` | UVs omit a constant offset; convert offset and size separately | `0x8042E3D8/E3E8` perform 32-bit integer additions before conversion; `0x8042E440/E45C/E480/E48C` add the constant whose bytes at `0x8051C388` are `3F000000` = 0.5. Divide each rounded numerator by the unsigned-converted parent dimension. |
| `motion_blur_reconstructed.cpp:198–201,219–222` | Vertices 1 and 2 use top-right/bottom-left order | `0x8042E514/E518` write vertex 1 `(u0,v1)`; `0x8042E51C/E520` write vertex 2 `(u1,v0)`. Absolute positions are TL, BL, TR, BR. The verified primitive-table entry `0x8056F130` is `0x98`, GX triangle strip. |
| `motion_blur_reconstructed.cpp:219–222` | `+=`/`-=` cumulatively enlarge prior vertex positions | `0x8042E5E0–0x8042E600` overwrite all eight xy fields with `(-dx,-dy),(-dx,H+dy),(W+dx,-dy),(W+dx,H+dy)`. Repeated equal inputs produce the same rectangle; prior positions are not inputs. |
| `motion_blur_reconstructed.cpp:84,113,125`; `motion_blur_analysis.md:154` | The second raster-copy argument is named/interpreted as clear-after-copy | At `0x8049624C–0x804962B8` it selects doubled source dimensions and the GX destination downsample flag. At `0x804963AC/B4` the actual GX clear argument is independently loaded from `[r13+0x6188] = [0x805F2688]` and narrowed to a byte. Blur's literal zero does not prove clear=false. |
| `motion_blur_reconstructed.cpp:109–110` | Adds a scratch-raster null early return | `0x8042E8AC–0x8042E8C0` directly dereferences the owner/scratch chain and calls the copy routine. No such null guard is present. Preserve the original preconditions/failure behavior; do not silently harden it. |
| `motion_blur_reconstructed.cpp:169–174` | Moves history-valid test ahead of camera acquisition | The original enable/alpha tests precede context acquisition, but validity is tested only at `0x8042E2C0–0x8042E2C8`, after the context cast and camera callback. Preserve call/guard order. |
| `motion_blur_reconstructed.cpp:185–188` | Parent dimensions are represented as signed | The original parent-dimension conversion uses `0x4330000000000000` without `xoris`; current raster offsets and dimensions use the signed bias `0x4330000080000000`. The distinction is preserved even though normal dimensions are positive. |
| `motion_blur_analysis.md:17,45` | A 0x10-byte parameter block can be mistaken for a whole-struct memcpy | `0x8042E9C4–0x8042EA50` copies bytes +0/+1, float +4 and words +8/+0xC only; bytes +2/+3 are untouched. |

Sol's draft labels itself structural pseudocode and leaves engine helpers abstract. Its constructor, input producers, raster/GX backend, callback-installation chain and complete lifecycle are not implemented as source in that file. Its high-level use is valuable; it is not the requested complete, semantics-preserving reconstruction by itself.

## Additional raw facts now available

- Neutral scale `0x8051C38C = 0x3F800000 = 1.0f`; coordinate zero `0x8051C3A0 = 0x00000000`.
- Blend translation table `0x8056FA28` maps RW 5/6 to GX 4/5, and RW 2/1 to GX 1/0.
- Render-level record 11 at `0x804D25C8` names `RenderLevel_PostGlare` (`0x804D23D8`) and has parent selector **1** at `0x804D25D8`. It is not established by the contaminated reference map's GlareWorld annotation.
- Raw task vtables resolve `0x8053DC84 -> 0x801D27FC` and `0x8053DC68 -> 0x801D2764`.
- Device descriptor `0x8056F50C` contains state setter `0x80498954`, getter `0x804984C4`, and primitive submitter `0x80491E80` at offsets +0x10, +0x14, +0x20. The remaining installation/traversal dependencies must still be closed separately.

No DOL patch, rendering redesign, extra state restoration, or GPU API replacement is adopted from the supplied material. No runtime validation is inferred from either investigation's static artifacts.
