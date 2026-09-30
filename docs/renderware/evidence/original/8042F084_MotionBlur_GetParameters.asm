; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; MotionBlur_GetParameters: [8042F084, 8042F0E8), 0x64 bytes
; classification: manager parameter bridge
; RAM address  DOL offset  raw word  instruction
8042F084  00428944  9421FFE0  stwu         r1, -0x20(r1)
8042F088  00428948  7C0802A6  mflr         r0
8042F08C  0042894C  90010024  stw          r0, 0x24(r1)
8042F090  00428950  93E1001C  stw          r31, 0x1c(r1)
8042F094  00428954  7C7F1B78  mr           r31, r3
8042F098  00428958  80840040  lwz          r4, 0x40(r4)
8042F09C  0042895C  28040000  cmplwi       r4, 0
8042F0A0  00428960  4182000C  beq          0x8042f0ac
8042F0A4  00428964  4BFFF921  bl           0x8042e9c4
8042F0A8  00428968  4800002C  b            0x8042f0d4
8042F0AC  0042896C  38000000  li           r0, 0
8042F0B0  00428970  3C608052  lis          r3, -0x7fae
8042F0B4  00428974  981F0000  stb          r0, 0(r31)
8042F0B8  00428978  C003C38C  lfs          f0, -0x3c74(r3)
8042F0BC  0042897C  981F0001  stb          r0, 1(r31)
8042F0C0  00428980  80610010  lwz          r3, 0x10(r1)
8042F0C4  00428984  D01F0004  stfs         f0, 4(r31)
8042F0C8  00428988  80010014  lwz          r0, 0x14(r1)
8042F0CC  0042898C  907F0008  stw          r3, 8(r31)
8042F0D0  00428990  901F000C  stw          r0, 0xc(r31)
8042F0D4  00428994  80010024  lwz          r0, 0x24(r1)
8042F0D8  00428998  83E1001C  lwz          r31, 0x1c(r1)
8042F0DC  0042899C  7C0803A6  mtlr         r0
8042F0E0  004289A0  38210020  addi         r1, r1, 0x20
8042F0E4  004289A4  4E800020  blr
