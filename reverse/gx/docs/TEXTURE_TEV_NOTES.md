# Texture / TEV notes for GX decompilation

## Native texture-format IDs

A useful canonical set for register decoding is:

| ID | Format |
|---:|---|
| 0x0 | I4 |
| 0x1 | I8 |
| 0x2 | IA4 |
| 0x3 | IA8 |
| 0x4 | RGB565 |
| 0x5 | RGB5A3 |
| 0x6 | RGBA8 |
| 0x8 | C4 / CI4 |
| 0x9 | C8 / CI8 |
| 0xA | C14X2 |
| 0xE | CMPR |

Do not decode texture bytes as linear rows. GX textures are tiled/swizzled, and RGBA8 uses split component planes inside a tile. Palette/TLUT state is part of the interpretation for indexed formats.

## TEV recovery

TEV is state, not a conventional shader program stored as code. Recover it as a versioned graph/state description first.

For each of up to 16 stages preserve:

- order: texcoord / texmap / raster color source,
- color A/B/C/D inputs,
- alpha A/B/C/D inputs,
- op, bias, scale, clamp and destination register,
- konst selections,
- swap tables,
- indirect-texture stage and matrix state,
- texture coordinate generation and post-matrix state from XF.

Only after the state graph is known should it be emitted as a higher-level shader or GX setter sequence.

## BP state families worth separating

Keep distinct domains for:

- general mode / number of TEV stages,
- scissor and line/point state,
- depth/Z mode,
- blend/logic op / destination alpha,
- EFB copy source/destination/filter/gamma,
- texture image/mode/TLUT state,
- TEV color/alpha environments,
- TEV registers and konst selectors,
- fog,
- alpha compare,
- Z-texture,
- indirect texturing.

This makes equivalence testing and source reconstruction much easier than treating BP as a flat 256-register array.

## Conservative lifting rule

`BP register write -> GXSet* call` is not always a bijection. One SDK call may write several registers, may preserve selected bits from shadow state, or may defer the hardware write. Therefore emit a named GX setter only when the complete before/after transition matches the setter semantics.
