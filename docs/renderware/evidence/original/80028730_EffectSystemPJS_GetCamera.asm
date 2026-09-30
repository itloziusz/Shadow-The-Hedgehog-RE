; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; EffectSystemPJS_GetCamera: [80028730, 80028754), 0x24 bytes
; classification: shared registered camera-provider vslot0x24 target
; RAM address  DOL offset  raw word  instruction
80028730  00021FF0  9421FFF0  stwu         r1, -0x10(r1)
80028734  00021FF4  7C0802A6  mflr         r0
80028738  00021FF8  90010014  stw          r0, 0x14(r1)
8002873C  00021FFC  4BFE5585  bl           0x8000dcc0
80028740  00022000  80010014  lwz          r0, 0x14(r1)
80028744  00022004  80630000  lwz          r3, 0(r3)
80028748  00022008  7C0803A6  mtlr         r0
8002874C  0002200C  38210010  addi         r1, r1, 0x10
80028750  00022010  4E800020  blr
