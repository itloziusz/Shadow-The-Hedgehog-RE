; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; MotionBlur_SetParametersIfPresent: [8042F114, 8042F140), 0x2c bytes
; classification: manager parameter bridge
; RAM address  DOL offset  raw word  instruction
8042F114  004289D4  9421FFF0  stwu         r1, -0x10(r1)
8042F118  004289D8  7C0802A6  mflr         r0
8042F11C  004289DC  90010014  stw          r0, 0x14(r1)
8042F120  004289E0  80630040  lwz          r3, 0x40(r3)
8042F124  004289E4  28030000  cmplwi       r3, 0
8042F128  004289E8  41820008  beq          0x8042f130
8042F12C  004289EC  4BFFF8F1  bl           0x8042ea1c
8042F130  004289F0  80010014  lwz          r0, 0x14(r1)
8042F134  004289F4  7C0803A6  mtlr         r0
8042F138  004289F8  38210010  addi         r1, r1, 0x10
8042F13C  004289FC  4E800020  blr
