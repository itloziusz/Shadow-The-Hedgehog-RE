; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; MotionBlur_EnableFixedParameters: [800A3714, 800A37A4), 0x90 bytes
; classification: shared function containing immediate blur enable producer
; RAM address  DOL offset  raw word  instruction
800A3714  0009CFD4  9421FFD0  stwu         r1, -0x30(r1)
800A3718  0009CFD8  7C0802A6  mflr         r0
800A371C  0009CFDC  C022944C  lfs          f1, -0x6bb4(r2)
800A3720  0009CFE0  38A00000  li           r5, 0
800A3724  0009CFE4  90010034  stw          r0, 0x34(r1)
800A3728  0009CFE8  38800001  li           r4, 1
800A372C  0009CFEC  C0029450  lfs          f0, -0x6bb0(r2)
800A3730  0009CFF0  38000080  li           r0, 0x80
800A3734  0009CFF4  93E1002C  stw          r31, 0x2c(r1)
800A3738  0009CFF8  7C7F1B78  mr           r31, r3
800A373C  0009CFFC  98A10010  stb          r5, 0x10(r1)
800A3740  0009D000  98A10011  stb          r5, 0x11(r1)
800A3744  0009D004  D0210014  stfs         f1, 0x14(r1)
800A3748  0009D008  98810010  stb          r4, 0x10(r1)
800A374C  0009D00C  98010011  stb          r0, 0x11(r1)
800A3750  0009D010  D0010014  stfs         f0, 0x14(r1)
800A3754  0009D014  4BFFFE85  bl           0x800a35d8
800A3758  0009D018  38810010  addi         r4, r1, 0x10
800A375C  0009D01C  4838B9B9  bl           0x8042f114
800A3760  0009D020  809F0000  lwz          r4, 0(r31)
800A3764  0009D024  38610008  addi         r3, r1, 8
800A3768  0009D028  38A00001  li           r5, 1
800A376C  0009D02C  38C00051  li           r6, 0x51
800A3770  0009D030  8084023C  lwz          r4, 0x23c(r4)
800A3774  0009D034  4BFFF4C9  bl           0x800a2c3c
800A3778  0009D038  387F0028  addi         r3, r31, 0x28
800A377C  0009D03C  38810008  addi         r4, r1, 8
800A3780  0009D040  4BFAF979  bl           0x800530f8
800A3784  0009D044  38610008  addi         r3, r1, 8
800A3788  0009D048  3880FFFF  li           r4, -1
800A378C  0009D04C  4BFAF889  bl           0x80053014
800A3790  0009D050  80010034  lwz          r0, 0x34(r1)
800A3794  0009D054  83E1002C  lwz          r31, 0x2c(r1)
800A3798  0009D058  7C0803A6  mtlr         r0
800A379C  0009D05C  38210030  addi         r1, r1, 0x30
800A37A0  0009D060  4E800020  blr
