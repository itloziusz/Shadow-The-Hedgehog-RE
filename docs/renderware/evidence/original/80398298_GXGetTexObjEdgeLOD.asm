; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXGetTexObjEdgeLOD: [80398298, 803982AC), 0x14 bytes
; classification: SDK sampler-preservation adapter
; RAM address  DOL offset  raw word  instruction
80398298  00391B58  80030000  lwz          r0, 0(r3)
8039829C  00391B5C  5400C7FE  rlwinm       r0, r0, 0x18, 0x1f, 0x1f
803982A0  00391B60  7C000034  cntlzw       r0, r0
803982A4  00391B64  5403DE3E  rlwinm       r3, r0, 0x1b, 0x18, 0x1f
803982A8  00391B68  4E800020  blr
