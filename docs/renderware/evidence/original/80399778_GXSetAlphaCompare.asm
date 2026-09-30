; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXSetAlphaCompare: [80399778, 803997BC), 0x44 bytes
; classification: shared SDK alpha-test state
; RAM address  DOL offset  raw word  instruction
80399778  00393038  5480063E  clrlwi       r0, r4, 0x18
8039977C  0039303C  80820FD8  lwz          r4, 0xfd8(r2)
80399780  00393040  3D00F300  lis          r8, -0xd00
80399784  00393044  5008063E  rlwimi       r8, r0, 0, 0x18, 0x1f
80399788  00393048  54E0063E  clrlwi       r0, r7, 0x18
8039978C  0039304C  38E80000  addi         r7, r8, 0
80399790  00393050  5007442E  rlwimi       r7, r0, 8, 0x10, 0x17
80399794  00393054  5067835E  rlwimi       r7, r3, 0x10, 0xd, 0xf
80399798  00393058  50C79A98  rlwimi       r7, r6, 0x13, 0xa, 0xc
8039979C  0039305C  38000061  li           r0, 0x61
803997A0  00393060  3C60CC01  lis          r3, -0x33ff
803997A4  00393064  98038000  stb          r0, -0x8000(r3)
803997A8  00393068  50A7B212  rlwimi       r7, r5, 0x16, 8, 9
803997AC  0039306C  38000000  li           r0, 0
803997B0  00393070  90E38000  stw          r7, -0x8000(r3)
803997B4  00393074  B0040002  sth          r0, 2(r4)
803997B8  00393078  4E800020  blr
