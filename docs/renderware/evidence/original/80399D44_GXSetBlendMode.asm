; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXSetBlendMode: [80399D44, 80399D98), 0x54 bytes
; classification: shared GX SDK blend dependency
; RAM address  DOL offset  raw word  instruction
80399D44  00393604  81020FD8  lwz          r8, 0xfd8(r2)
80399D48  00393608  20030003  subfic       r0, r3, 3
80399D4C  0039360C  7C070034  cntlzw       r7, r0
80399D50  00393610  20030002  subfic       r0, r3, 2
80399D54  00393614  812801D0  lwz          r9, 0x1d0(r8)
80399D58  00393618  50E93528  rlwimi       r9, r7, 6, 0x14, 0x14
80399D5C  0039361C  38E90000  addi         r7, r9, 0
80399D60  00393620  506707FE  rlwimi       r7, r3, 0, 0x1f, 0x1f
80399D64  00393624  7C000034  cntlzw       r0, r0
80399D68  00393628  5007E7BC  rlwimi       r7, r0, 0x1c, 0x1e, 0x1e
80399D6C  0039362C  50C76426  rlwimi       r7, r6, 0xc, 0x10, 0x13
80399D70  00393630  5087456E  rlwimi       r7, r4, 8, 0x15, 0x17
80399D74  00393634  38000061  li           r0, 0x61
80399D78  00393638  3C60CC01  lis          r3, -0x33ff
80399D7C  0039363C  98038000  stb          r0, -0x8000(r3)
80399D80  00393640  50A72E34  rlwimi       r7, r5, 5, 0x18, 0x1a
80399D84  00393644  38000000  li           r0, 0
80399D88  00393648  90E38000  stw          r7, -0x8000(r3)
80399D8C  0039364C  90E801D0  stw          r7, 0x1d0(r8)
80399D90  00393650  B0080002  sth          r0, 2(r8)
80399D94  00393654  4E800020  blr
