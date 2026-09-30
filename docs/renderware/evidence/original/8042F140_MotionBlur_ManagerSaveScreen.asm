; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; MotionBlur_ManagerSaveScreen: [8042F140, 8042F16C), 0x2c bytes
; classification: manager capture bridge
; RAM address  DOL offset  raw word  instruction
8042F140  00428A00  9421FFF0  stwu         r1, -0x10(r1)
8042F144  00428A04  7C0802A6  mflr         r0
8042F148  00428A08  90010014  stw          r0, 0x14(r1)
8042F14C  00428A0C  80630040  lwz          r3, 0x40(r3)
8042F150  00428A10  28030000  cmplwi       r3, 0
8042F154  00428A14  41820008  beq          0x8042f15c
8042F158  00428A18  4BFFF6E1  bl           0x8042e838
8042F15C  00428A1C  80010014  lwz          r0, 0x14(r1)
8042F160  00428A20  7C0803A6  mtlr         r0
8042F164  00428A24  38210010  addi         r1, r1, 0x10
8042F168  00428A28  4E800020  blr
