; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; RwRaster_PluginRegister: [80496440, 80496478), 0x38 bytes
; classification: shared raster extension registration
; RAM address  DOL offset  raw word  instruction
80496440  0048FD00  9421FFF0  stwu         r1, -0x10(r1)
80496444  0048FD04  7C0802A6  mflr         r0
80496448  0048FD08  38600034  li           r3, 0x34
8049644C  0048FD0C  3880040C  li           r4, 0x40c
80496450  0048FD10  90010014  stw          r0, 0x14(r1)
80496454  0048FD14  38A00000  li           r5, 0
80496458  0048FD18  38C00000  li           r6, 0
8049645C  0048FD1C  38E00000  li           r7, 0
80496460  0048FD20  4BFF4845  bl           0x8048aca4
80496464  0048FD24  906D6200  stw          r3, 0x6200(r13)
80496468  0048FD28  80010014  lwz          r0, 0x14(r1)
8049646C  0048FD2C  7C0803A6  mtlr         r0
80496470  0048FD30  38210010  addi         r1, r1, 0x10
80496474  0048FD34  4E800020  blr
