; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; MotionBlur_AssignParameters: [8042E9F0, 8042EA1C), 0x2c bytes
; classification: effect parameter transfer
; RAM address  DOL offset  raw word  instruction
8042E9F0  004282B0  88A40000  lbz          r5, 0(r4)
8042E9F4  004282B4  88040001  lbz          r0, 1(r4)
8042E9F8  004282B8  98A30000  stb          r5, 0(r3)
8042E9FC  004282BC  C0040004  lfs          f0, 4(r4)
8042EA00  004282C0  98030001  stb          r0, 1(r3)
8042EA04  004282C4  80A40008  lwz          r5, 8(r4)
8042EA08  004282C8  D0030004  stfs         f0, 4(r3)
8042EA0C  004282CC  8004000C  lwz          r0, 0xc(r4)
8042EA10  004282D0  90A30008  stw          r5, 8(r3)
8042EA14  004282D4  9003000C  stw          r0, 0xc(r3)
8042EA18  004282D8  4E800020  blr
