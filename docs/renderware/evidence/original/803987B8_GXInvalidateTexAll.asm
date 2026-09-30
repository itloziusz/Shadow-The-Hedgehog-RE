; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXInvalidateTexAll: [803987B8, 80398800), 0x48 bytes
; classification: SDK global texture-cache invalidation adapter
; RAM address  DOL offset  raw word  instruction
803987B8  00392078  7C0802A6  mflr         r0
803987BC  0039207C  90010004  stw          r0, 4(r1)
803987C0  00392080  9421FFF8  stwu         r1, -8(r1)
803987C4  00392084  48000B05  bl           0x803992c8
803987C8  00392088  38C00061  li           r6, 0x61
803987CC  0039208C  3C606600  lis          r3, 0x6600
803987D0  00392090  3CA0CC01  lis          r5, -0x33ff
803987D4  00392094  98C58000  stb          r6, -0x8000(r5)
803987D8  00392098  38831000  addi         r4, r3, 0x1000
803987DC  0039209C  38031100  addi         r0, r3, 0x1100
803987E0  003920A0  90858000  stw          r4, -0x8000(r5)
803987E4  003920A4  98C58000  stb          r6, -0x8000(r5)
803987E8  003920A8  90058000  stw          r0, -0x8000(r5)
803987EC  003920AC  48000ADD  bl           0x803992c8
803987F0  003920B0  8001000C  lwz          r0, 0xc(r1)
803987F4  003920B4  38210008  addi         r1, r1, 8
803987F8  003920B8  7C0803A6  mtlr         r0
803987FC  003920BC  4E800020  blr
