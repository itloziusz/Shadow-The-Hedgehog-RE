; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXSetZMode: [80399DF0, 80399E24), 0x34 bytes
; classification: shared GX SDK depth dependency
; RAM address  DOL offset  raw word  instruction
80399DF0  003936B0  80C20FD8  lwz          r6, 0xfd8(r2)
80399DF4  003936B4  38000061  li           r0, 0x61
80399DF8  003936B8  80E601D8  lwz          r7, 0x1d8(r6)
80399DFC  003936BC  506707FE  rlwimi       r7, r3, 0, 0x1f, 0x1f
80399E00  003936C0  3C60CC01  lis          r3, -0x33ff
80399E04  003936C4  98038000  stb          r0, -0x8000(r3)
80399E08  003936C8  50870F3C  rlwimi       r7, r4, 1, 0x1c, 0x1e
80399E0C  003936CC  50A726F6  rlwimi       r7, r5, 4, 0x1b, 0x1b
80399E10  003936D0  90E38000  stw          r7, -0x8000(r3)
80399E14  003936D4  38000000  li           r0, 0
80399E18  003936D8  90E601D8  stw          r7, 0x1d8(r6)
80399E1C  003936DC  B0060002  sth          r0, 2(r6)
80399E20  003936E0  4E800020  blr
