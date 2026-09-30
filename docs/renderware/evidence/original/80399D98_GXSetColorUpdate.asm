; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXSetColorUpdate: [80399D98, 80399DC4), 0x2c bytes
; classification: shared GX SDK write-mask dependency
; RAM address  DOL offset  raw word  instruction
80399D98  00393658  80A20FD8  lwz          r5, 0xfd8(r2)
80399D9C  0039365C  38000061  li           r0, 0x61
80399DA0  00393660  3C80CC01  lis          r4, -0x33ff
80399DA4  00393664  80C501D0  lwz          r6, 0x1d0(r5)
80399DA8  00393668  50661F38  rlwimi       r6, r3, 3, 0x1c, 0x1c
80399DAC  0039366C  98048000  stb          r0, -0x8000(r4)
80399DB0  00393670  38000000  li           r0, 0
80399DB4  00393674  90C48000  stw          r6, -0x8000(r4)
80399DB8  00393678  90C501D0  stw          r6, 0x1d0(r5)
80399DBC  0039367C  B0050002  sth          r0, 2(r5)
80399DC0  00393680  4E800020  blr
