; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXSetCurrentMtx: [8039A2CC, 8039A300), 0x34 bytes
; classification: SDK current-matrix selection adapter
; RAM address  DOL offset  raw word  instruction
8039A2CC  00393B8C  7C0802A6  mflr         r0
8039A2D0  00393B90  90010004  stw          r0, 4(r1)
8039A2D4  00393B94  9421FFF8  stwu         r1, -8(r1)
8039A2D8  00393B98  80820FD8  lwz          r4, 0xfd8(r2)
8039A2DC  00393B9C  80040080  lwz          r0, 0x80(r4)
8039A2E0  00393BA0  506006BE  rlwimi       r0, r3, 0, 0x1a, 0x1f
8039A2E4  00393BA4  38600000  li           r3, 0
8039A2E8  00393BA8  90040080  stw          r0, 0x80(r4)
8039A2EC  00393BAC  480002D9  bl           0x8039a5c4
8039A2F0  00393BB0  8001000C  lwz          r0, 0xc(r1)
8039A2F4  00393BB4  38210008  addi         r1, r1, 8
8039A2F8  00393BB8  7C0803A6  mtlr         r0
8039A2FC  00393BBC  4E800020  blr
