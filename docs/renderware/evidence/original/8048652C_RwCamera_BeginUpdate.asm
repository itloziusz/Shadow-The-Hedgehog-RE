; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; RwCamera_BeginUpdate: [8048652C, 80486554), 0x28 bytes
; classification: shared callback adapter
; RAM address  DOL offset  raw word  instruction
8048652C  0047FDEC  9421FFF0  stwu         r1, -0x10(r1)
80486530  0047FDF0  7C0802A6  mflr         r0
80486534  0047FDF4  90010014  stw          r0, 0x14(r1)
80486538  0047FDF8  81830018  lwz          r12, 0x18(r3)
8048653C  0047FDFC  7D8903A6  mtctr        r12
80486540  0047FE00  4E800421  bctrl
80486544  0047FE04  80010014  lwz          r0, 0x14(r1)
80486548  0047FE08  7C0803A6  mtlr         r0
8048654C  0047FE0C  38210010  addi         r1, r1, 0x10
80486550  0047FE10  4E800020  blr
