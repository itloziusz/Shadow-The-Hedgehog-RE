; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXSetProjectionv: [8039A158, 8039A1E4), 0x8c bytes
; classification: SDK projection-vector upload adapter
; RAM address  DOL offset  raw word  instruction
8039A158  00393A18  C02210B8  lfs          f1, 0x10b8(r2)
8039A15C  00393A1C  C0030000  lfs          f0, 0(r3)
8039A160  00393A20  FC010000  fcmpu        cr0, f1, f0
8039A164  00393A24  4082000C  bne          0x8039a170
8039A168  00393A28  38000000  li           r0, 0
8039A16C  00393A2C  48000008  b            0x8039a174
8039A170  00393A30  38000001  li           r0, 1
8039A174  00393A34  80A20FD8  lwz          r5, 0xfd8(r2)
8039A178  00393A38  38630004  addi         r3, r3, 4
8039A17C  00393A3C  900504D8  stw          r0, 0x4d8(r5)
8039A180  00393A40  38C504DC  addi         r6, r5, 0x4dc
8039A184  00393A44  E0430000  psq_l        f2, 0(r3), 0, 0
8039A188  00393A48  E0230008  psq_l        f1, 8(r3), 0, 0
8039A18C  00393A4C  E0030010  psq_l        f0, 16(r3), 0, 0
8039A190  00393A50  F0460000  psq_st       f2, 0(r6), 0, 0
8039A194  00393A54  F0260008  psq_st       f1, 8(r6), 0, 0
8039A198  00393A58  F0060010  psq_st       f0, 16(r6), 0, 0
8039A19C  00393A5C  3C80CC01  lis          r4, -0x33ff
8039A1A0  00393A60  38000010  li           r0, 0x10
8039A1A4  00393A64  3C600006  lis          r3, 6
8039A1A8  00393A68  98048000  stb          r0, -0x8000(r4)
8039A1AC  00393A6C  38031020  addi         r0, r3, 0x1020
8039A1B0  00393A70  90048000  stw          r0, -0x8000(r4)
8039A1B4  00393A74  38648000  addi         r3, r4, -0x8000
8039A1B8  00393A78  E0460000  psq_l        f2, 0(r6), 0, 0
8039A1BC  00393A7C  E0260008  psq_l        f1, 8(r6), 0, 0
8039A1C0  00393A80  E0060010  psq_l        f0, 16(r6), 0, 0
8039A1C4  00393A84  F0430000  psq_st       f2, 0(r3), 0, 0
8039A1C8  00393A88  F0230000  psq_st       f1, 0(r3), 0, 0
8039A1CC  00393A8C  F0030000  psq_st       f0, 0(r3), 0, 0
8039A1D0  00393A90  806504D8  lwz          r3, 0x4d8(r5)
8039A1D4  00393A94  38000001  li           r0, 1
8039A1D8  00393A98  90648000  stw          r3, -0x8000(r4)
8039A1DC  00393A9C  B0050002  sth          r0, 2(r5)
8039A1E0  00393AA0  4E800020  blr
