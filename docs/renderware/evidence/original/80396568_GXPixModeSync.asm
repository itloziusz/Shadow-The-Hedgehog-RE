; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXPixModeSync: [80396568, 8039658C), 0x24 bytes
; classification: shared GX SDK synchronization dependency
; RAM address  DOL offset  raw word  instruction
80396568  0038FE28  38000061  li           r0, 0x61
8039656C  0038FE2C  80820FD8  lwz          r4, 0xfd8(r2)
80396570  0038FE30  3CA0CC01  lis          r5, -0x33ff
80396574  0038FE34  98058000  stb          r0, -0x8000(r5)
80396578  0038FE38  38000000  li           r0, 0
8039657C  0038FE3C  806401DC  lwz          r3, 0x1dc(r4)
80396580  0038FE40  90658000  stw          r3, -0x8000(r5)
80396584  0038FE44  B0040002  sth          r0, 2(r4)
80396588  0038FE48  4E800020  blr
