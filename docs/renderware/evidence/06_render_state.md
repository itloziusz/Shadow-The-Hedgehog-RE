# Role 06: exact RenderWare -> GX blur render-state evidence

Work in progress; only this report and `../motion_blur_gx_state.md` are owned by Role 06. Original/prior/GPT_SOL files are not modified.

## Immediate corrections for integration

- **State 9 is texture filtering**, not flat shading. Verified setter dispatch word at `0x8056FAF8` is `0x80498BFC` (`data_manifest.json:136-176`); that arm replaces only the low byte of `currentTexture+0x50` (`original/80498954_RwRenderState_Set.asm:175-181`).
- **State 12 is unsupported in BOTH directions, not the `80498E48` return-input arm.** Raw setter entry `0x8056FB04 = 0x80498F58` goes straight to the return block, with `r6=0` from `80498960`; no GX call or state write. Newly exported getter entry `0x8056FA88 = 0x804986B0` (`data_manifest.json:709-749`) returns 0 without writing `*out`. Draw passes uninitialized `sp+1C` at `8042E328`, then loads it at `8042E780` for the ineffective restore. The always-one getter/return-input setter arms belong to ID 5. Do not initialize a fictitious saved vertex-alpha state to 1.
- Im2D does not force cull-none, color/alpha write enable, zero indirect stages, TEV-direct, TEV swap identity, or automatic texture-coordinate scaling. CameraBegin `804956F4..80495CAC` does not supply these resets either. The exact GX TEV program is table-driven (`GXSetTevOp` at `803992EC`) and preserves alpha-word low four swap-selector bits.
- The compound requested depth disable is cache-mediated `GXSetZMode(1,7,0)`, not `GXSetZMode(0,...)`; source/destination factors map 5/6 to GX 4/5.

## Parent raw dumps needed

Highest priority missing data (original bytes, not runtime values):

1. `0x80568B00..0x80568B70`: GXSetTevOp stage-zero color/alpha programs and GXSetTevOrder channel lookup. The exact selected words are `80568B00` (stage0/op0 color), `80568B28` (stage0/op0 alpha), and `80568B60` (order channel 4); optional untextured words `80568B10`/`80568B38`.
2. Getter dispatch `0x8056FA58..0x8056FAD4` is now resolved in `data_manifest.json:709-749`; state 12 (`8056FA88`) points to `804986B0`.
3. `0x8056FC90..0x8056FCDC`: RW min/mag pairs and wrap lookup; filter 1 uses `8056FC98/FC9C`, wraps use `8056FCC8 + 4*inheritedNibble`.
4. `0x805FC448..0x805FC458`: initial-color/fog-disable constants; `0x8056FA18..0x8056FA28`: fog-type translation needed by restoring enabled fog; `0x805FC498..0x805FC4A8`: RW texture LOD zero and unsigned conversion bias.
5. `0x805FB7C0..0x805FB7F8`, `0x805EED30..0x805EED36`: GX texture LOD constants and min-filter bit encoding.
6. `0x805FB840..0x805FB848`: remaining GX viewport jitter-half and hardware-offset constants. Projection markers `805FB838..B840`, CameraBegin constants `805FC428..C448`, and camera projection template `8056F4F0..F50C` are now resolved by the expanded manifest.
7. Selected GX descriptor/VAT/texgen dispatch words: `80568784`, `8056878C`, `80568794` (GXSetVtxDesc IDs 9/11/13); `805687C8`, `805687D0`, `805687D8` (GXSetVtxAttrFmt attributes 9/11/13); `80568850` and `8056887C` (texgen slot 0 and source 4). These close numeric-request -> XF/CP-field linkage without assuming SDK lookup contents. Whole bounded tables, if easier: `80568760..8056880C` and `80568850..805688C0`.

Already read by verified-address bounded `motion_blur_analysis/full_disassembly.txt` reads, but not yet exported/original-byte-checked by this role:

- `803992EC..80399378` TEV table realization; `80399778..803997BC`, `80399848..80399A0C` compare/order/count; `80397C80..80397D6C` channels.
- `80399228..80399298` indirect count/direct/dirty helper (the helper at `80399294` is exactly `blr`).
- `8049AC88..8049ACE8`, `8049B1EC..8049B568` non-paletted texture realization; `80397E34..80398490` texture object/LOD/load.
- `803968B4..80396A8C`, `80396B54..80396BD4`, `803988A4..80398AC0` dirty-state/GXBegin/cull/automatic texture-coordinate scale realization.
- `8039A158..8039A278`, `8039A2CC..8039A300`, `8039A3B4..8039A59C` projection/matrix/viewport/scissor; paired-single loads/stores must be decoded as Gekko, not Capstone's accidental VSX mnemonics.
- `804956F4..80495CAC` CameraBegin, only required inherited viewport/scissor/pixel-format/projection branches. `803952B0..803952FC` is thread bookkeeping, not a GX state reset.
- Selected raster-format branches: `80497458..804974F8`, `8049765C..804976A4`, `80497850..804978A0`, `80497A54..80497A70`, `80497CA8..80497DD0`. They establish successful nonempty created `0x0505` roots have GX format 6, alpha-plugin bit 1, no mipmap flag, max LOD 0.

No executable commands or runtime validation have been performed by Role 06.
