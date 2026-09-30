# GX packet / vertex quick reference

All FIFO multi-byte values are big-endian from the GameCube CPU's point of view.

## Main FIFO opcodes

| Opcode | Meaning | Encoded payload |
|---:|---|---|
| `0x00` | NOP | none |
| `0x08` | Load CP register | `u8 reg, be32 value` |
| `0x10` | Load XF register(s) | `be32 ((count-1)<<16 | addr), count * be32` |
| `0x20/0x28/0x30/0x38` | Indexed XF load A/B/C/D | one BE32 packed descriptor |
| `0x40` | Call display list | `be32 address, be32 size` |
| `0x44` | metrics/unknown command | implementation-specific |
| `0x48` | Invalidate vertex cache | none |
| `0x61` | Load BP register | `be32 (reg<<24 | value24)` |
| `0x80..0xBF` | Primitive begin | `be16 vertex_count`, then vertex stream |

Primitive opcode is `primitive | vat_index`; VAT index is low 3 bits.

Primitive bases:
`0x80 QUADS`, `0x88 QUADS2`, `0x90 TRIANGLES`, `0x98 TRIANGLESTRIP`, `0xA0 TRIANGLEFAN`, `0xA8 LINES`, `0xB0 LINESTRIP`, `0xB8 POINTS`.

## CP register ranges important to vertex decoding

- `0x30`: matrix index A
- `0x40`: matrix index B
- `0x50`: VCD low
- `0x60`: VCD high
- `0x70..0x77`: VAT A for formats 0..7
- `0x80..0x87`: VAT B
- `0x90..0x97`: VAT C
- `0xA0..0xAF`: vertex array bases
- `0xB0..0xBF`: vertex array strides

## VCD low

- bits 0..8: PNMTXIDX + TEX0..7MTXIDX presence, one bit each; if present they are direct 8-bit values.
- bits 9..10: position: 0 none / 1 direct / 2 index8 / 3 index16
- bits 11..12: normal
- bits 13..14: color0
- bits 15..16: color1

VCD high contains 8 two-bit texture-coordinate descriptors for TEX0..TEX7 at bits 0,2,...14.

## VAT A

- bit 0: position XY/XYZ
- bits 1..3: position component format
- bits 4..8: position fraction
- bit 9: normal N/NTB
- bits 10..12: normal format
- bit 13 + bits 14..16: color0 count/format
- bit 17 + bits 18..20: color1 count/format
- bit 21 + bits 22..24 + bits 25..29: tex0 count/format/fraction
- bit 30: byte dequant
- bit 31: normal index3

Scalar component format: U8=0, S8=1, U16=2, S16=3, F32=4. Values 5..7 behave as 4-byte float-like formats in current Dolphin handling but should remain tagged as uncertain in a decompiler.

Color packed sizes: RGB565=2, RGB888=3, RGB888x=4, RGBA4444=2, RGBA6666=3, RGBA8888=4 bytes.

## Vertex attribute order

The hardware stream order is fixed:
PNMTXIDX, TEX0MTXIDX..TEX7MTXIDX, POS, NRM/NBT, CLR0, CLR1, TEX0..TEX7.

That fixed order is what lets a decompiler regroup a flat series of WGPIPE stores into semantic `GX_Position*`, `GX_Normal*`, `GX_Color*`, and `GX_TexCoord*` operations once VCD/VAT are known.
