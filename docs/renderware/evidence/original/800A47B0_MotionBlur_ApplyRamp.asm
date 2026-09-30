; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; MotionBlur_ApplyRamp: [800A47B0, 800A4830), 0x80 bytes
; classification: effect input producer
; RAM address  DOL offset  raw word  instruction
800A47B0  0009E070  9421FFE0  stwu         r1, -0x20(r1)
800A47B4  0009E074  7C0802A6  mflr         r0
800A47B8  0009E078  C022944C  lfs          f1, -0x6bb4(r2)
800A47BC  0009E07C  90010024  stw          r0, 0x24(r1)
800A47C0  0009E080  38000000  li           r0, 0
800A47C4  0009E084  C002945C  lfs          f0, -0x6ba4(r2)
800A47C8  0009E088  98010008  stb          r0, 8(r1)
800A47CC  0009E08C  98010009  stb          r0, 9(r1)
800A47D0  0009E090  D021000C  stfs         f1, 0xc(r1)
800A47D4  0009E094  C0230038  lfs          f1, 0x38(r3)
800A47D8  0009E098  FC010040  fcmpo        cr0, f1, f0
800A47DC  0009E09C  4C401382  cror         cr0eq, cr0lt, cr0eq
800A47E0  0009E0A0  40820008  bne          0x800a47e8
800A47E4  0009E0A4  48000008  b            0x800a47ec
800A47E8  0009E0A8  38000001  li           r0, 1
800A47EC  0009E0AC  98010008  stb          r0, 8(r1)
800A47F0  0009E0B0  C0429460  lfs          f2, -0x6ba0(r2)
800A47F4  0009E0B4  C0230038  lfs          f1, 0x38(r3)
800A47F8  0009E0B8  C0029450  lfs          f0, -0x6bb0(r2)
800A47FC  0009E0BC  EC220072  fmuls        f1, f2, f1
800A4800  0009E0C0  D001000C  stfs         f0, 0xc(r1)
800A4804  0009E0C4  FC00081E  fctiwz       f0, f1
800A4808  0009E0C8  D8010018  stfd         f0, 0x18(r1)
800A480C  0009E0CC  8001001C  lwz          r0, 0x1c(r1)
800A4810  0009E0D0  98010009  stb          r0, 9(r1)
800A4814  0009E0D4  4BFFEDC5  bl           0x800a35d8
800A4818  0009E0D8  38810008  addi         r4, r1, 8
800A481C  0009E0DC  4838A8F9  bl           0x8042f114
800A4820  0009E0E0  80010024  lwz          r0, 0x24(r1)
800A4824  0009E0E4  7C0803A6  mtlr         r0
800A4828  0009E0E8  38210020  addi         r1, r1, 0x20
800A482C  0009E0EC  4E800020  blr
