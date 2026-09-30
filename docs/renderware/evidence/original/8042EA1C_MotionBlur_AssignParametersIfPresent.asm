; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; MotionBlur_AssignParametersIfPresent: [8042EA1C, 8042EA50), 0x34 bytes
; classification: effect parameter transfer
; RAM address  DOL offset  raw word  instruction
8042EA1C  004282DC  28040000  cmplwi       r4, 0
8042EA20  004282E0  4D820020  beqlr
8042EA24  004282E4  88040000  lbz          r0, 0(r4)
8042EA28  004282E8  98030000  stb          r0, 0(r3)
8042EA2C  004282EC  88040001  lbz          r0, 1(r4)
8042EA30  004282F0  98030001  stb          r0, 1(r3)
8042EA34  004282F4  C0040004  lfs          f0, 4(r4)
8042EA38  004282F8  D0030004  stfs         f0, 4(r3)
8042EA3C  004282FC  80A40008  lwz          r5, 8(r4)
8042EA40  00428300  8004000C  lwz          r0, 0xc(r4)
8042EA44  00428304  90A30008  stw          r5, 8(r3)
8042EA48  00428308  9003000C  stw          r0, 0xc(r3)
8042EA4C  0042830C  4E800020  blr
