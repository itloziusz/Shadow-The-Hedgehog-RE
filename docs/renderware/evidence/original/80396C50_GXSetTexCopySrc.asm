; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXSetTexCopySrc: [80396C50, 80396CCC), 0x7c bytes
; classification: shared GX SDK copy dependency
; RAM address  DOL offset  raw word  instruction
80396C50  00390510  81020FD8  lwz          r8, 0xfd8(r2)
80396C54  00390514  39200000  li           r9, 0
80396C58  00390518  54A7043E  clrlwi       r7, r5, 0x10
80396C5C  0039051C  912801F0  stw          r9, 0x1f0(r8)
80396C60  00390520  54C5043E  clrlwi       r5, r6, 0x10
80396C64  00390524  5460043E  clrlwi       r0, r3, 0x10
80396C68  00390528  806801F0  lwz          r3, 0x1f0(r8)
80396C6C  0039052C  500305BE  rlwimi       r3, r0, 0, 0x16, 0x1f
80396C70  00390530  5480043E  clrlwi       r0, r4, 0x10
80396C74  00390534  906801F0  stw          r3, 0x1f0(r8)
80396C78  00390538  38C00049  li           r6, 0x49
80396C7C  0039053C  3887FFFF  addi         r4, r7, -1
80396C80  00390540  80E801F0  lwz          r7, 0x1f0(r8)
80396C84  00390544  5007532A  rlwimi       r7, r0, 0xa, 0xc, 0x15
80396C88  00390548  3865FFFF  addi         r3, r5, -1
80396C8C  0039054C  90E801F0  stw          r7, 0x1f0(r8)
80396C90  00390550  3800004A  li           r0, 0x4a
80396C94  00390554  80A801F0  lwz          r5, 0x1f0(r8)
80396C98  00390558  50C5C00E  rlwimi       r5, r6, 0x18, 0, 7
80396C9C  0039055C  90A801F0  stw          r5, 0x1f0(r8)
80396CA0  00390560  912801F4  stw          r9, 0x1f4(r8)
80396CA4  00390564  80A801F4  lwz          r5, 0x1f4(r8)
80396CA8  00390568  508505BE  rlwimi       r5, r4, 0, 0x16, 0x1f
80396CAC  0039056C  90A801F4  stw          r5, 0x1f4(r8)
80396CB0  00390570  808801F4  lwz          r4, 0x1f4(r8)
80396CB4  00390574  5064532A  rlwimi       r4, r3, 0xa, 0xc, 0x15
80396CB8  00390578  908801F4  stw          r4, 0x1f4(r8)
80396CBC  0039057C  806801F4  lwz          r3, 0x1f4(r8)
80396CC0  00390580  5003C00E  rlwimi       r3, r0, 0x18, 0, 7
80396CC4  00390584  906801F4  stw          r3, 0x1f4(r8)
80396CC8  00390588  4E800020  blr
