# BP/XF register map for the semantic lifter

This is a compact implementation checklist. The production decoder should keep the full raw register word and layer named bitfields over it; never discard unknown/reserved bits.

## BP (`0x61` packet)

A BP packet is one byte `0x61` followed by one BE32 word. The high byte selects the BP register and the low 24 bits are its value.

Major register/range families used by current open reference implementations:

- `0x00` general mode / rasterizer pipeline counts
- `0x01..0x04` display-copy filter coefficients
- `0x06..0x0E` indirect texture matrices
- `0x0F` indirect mask
- `0x10..0x1F` per-TEV-stage indirect commands
- `0x20..0x21` scissor top-left / bottom-right
- `0x22` line and point width
- `0x25..0x26` raster scale/size state
- `0x27` indirect texture reference
- `0x28..0x2F` TEV texture/raster order pairs
- `0x30..0x3F` texture-coordinate S/T sizes
- `0x40` Z mode
- `0x41` blend / logic / color-alpha update mode
- `0x42` constant destination alpha
- `0x43` pixel/Z compare control / EFB format family
- `0x44` field mask
- `0x45` draw-done trigger
- `0x47..0x48` PE token / token interrupt
- `0x49..0x52` EFB copy source/destination/scale/clear/trigger state
- `0x53..0x54` copy filter
- `0x55..0x56` bounding-box clear
- `0x59` scissor offset
- `0x60..0x63` texture preload/TMEM state
- `0x64..0x65` TLUT loading
- `0x66` texture cache invalidate
- `0x68` field mode
- `0x80..0x9B` texture units 0..3 mode/image/TLUT groups
- `0xA0..0xBB` texture units 4..7 mode/image/TLUT groups
- `0xC0..0xDF` TEV color/alpha environments for 16 stages
- `0xE0..0xE7` TEV color/konst register data pairs
- `0xE8..0xF2` fog range/parameters/color
- `0xF3` alpha compare
- `0xF4..0xF5` Z texture bias/op state
- `0xF6..0xFD` TEV konst selectors / swap-table state
- `0xFE` BP write mask

Do not map a single BP write directly to an SDK function unless the SDK function has exactly that state transition. Some GX APIs update multiple BP words and an internal SDK shadow structure.

## XF memory and registers (`0x10` packet)

Important XF address ranges:

- `0x000..0x0FF`: position matrices
- `0x400..0x45F`: normal matrices
- `0x500..0x5FF`: post-transform matrices
- `0x600..0x67F`: light objects
- `0x1000+`: XF control registers

Named control addresses:

- `0x1005`: clipping-disable controls
- `0x1008`: input vertex specification (number of colors/normals/texcoords)
- `0x1009`: number of color channels
- `0x100A..0x100D`: ambient/material colors
- `0x100E..0x1011`: channel color/alpha lighting controls
- `0x1012`: dual-texture enable
- `0x1018..0x1019`: matrix-index A/B
- `0x101A..0x101F`: viewport (six words/floats)
- `0x1020..0x1026`: projection parameters/type
- `0x103F`: number of texture generators
- `0x1040..0x1047`: texture generator matrix/source info
- `0x1050..0x1057`: post-matrix info

The XF packet can write 1..16 consecutive 32-bit words. Preserve the original base+count so the lifter can recognize batched matrix/projection/viewport uploads without pretending each word was a separate API call.
