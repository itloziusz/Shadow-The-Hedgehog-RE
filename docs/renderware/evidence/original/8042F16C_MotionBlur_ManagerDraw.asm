; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; MotionBlur_ManagerDraw: [8042F16C, 8042F198), 0x2c bytes
; classification: manager draw bridge
; RAM address  DOL offset  raw word  instruction
8042F16C  00428A2C  9421FFF0  stwu         r1, -0x10(r1)
8042F170  00428A30  7C0802A6  mflr         r0
8042F174  00428A34  90010014  stw          r0, 0x14(r1)
8042F178  00428A38  80630040  lwz          r3, 0x40(r3)
8042F17C  00428A3C  28030000  cmplwi       r3, 0
8042F180  00428A40  41820008  beq          0x8042f188
8042F184  00428A44  4BFFF0D1  bl           0x8042e254
8042F188  00428A48  80010014  lwz          r0, 0x14(r1)
8042F18C  00428A4C  7C0803A6  mtlr         r0
8042F190  00428A50  38210010  addi         r1, r1, 0x10
8042F194  00428A54  4E800020  blr
