; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; MotionBlur_CopyParameters: [8042E9C4, 8042E9F0), 0x2c bytes
; classification: effect parameter transfer
; RAM address  DOL offset  raw word  instruction
8042E9C4  00428284  88040000  lbz          r0, 0(r4)
8042E9C8  00428288  98030000  stb          r0, 0(r3)
8042E9CC  0042828C  88040001  lbz          r0, 1(r4)
8042E9D0  00428290  98030001  stb          r0, 1(r3)
8042E9D4  00428294  C0040004  lfs          f0, 4(r4)
8042E9D8  00428298  D0030004  stfs         f0, 4(r3)
8042E9DC  0042829C  80A40008  lwz          r5, 8(r4)
8042E9E0  004282A0  8004000C  lwz          r0, 0xc(r4)
8042E9E4  004282A4  90A30008  stw          r5, 8(r3)
8042E9E8  004282A8  9003000C  stw          r0, 0xc(r3)
8042E9EC  004282AC  4E800020  blr
