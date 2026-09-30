; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; MotionBlur_SetParameters: [8042F0E8, 8042F114), 0x2c bytes
; classification: manager parameter bridge
; RAM address  DOL offset  raw word  instruction
8042F0E8  004289A8  9421FFF0  stwu         r1, -0x10(r1)
8042F0EC  004289AC  7C0802A6  mflr         r0
8042F0F0  004289B0  90010014  stw          r0, 0x14(r1)
8042F0F4  004289B4  80630040  lwz          r3, 0x40(r3)
8042F0F8  004289B8  28030000  cmplwi       r3, 0
8042F0FC  004289BC  41820008  beq          0x8042f104
8042F100  004289C0  4BFFF8F1  bl           0x8042e9f0
8042F104  004289C4  80010014  lwz          r0, 0x14(r1)
8042F108  004289C8  7C0803A6  mtlr         r0
8042F10C  004289CC  38210010  addi         r1, r1, 0x10
8042F110  004289D0  4E800020  blr
