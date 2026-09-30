; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; RwCamera_EndUpdate: [80486504, 8048652C), 0x28 bytes
; classification: shared callback adapter
; RAM address  DOL offset  raw word  instruction
80486504  0047FDC4  9421FFF0  stwu         r1, -0x10(r1)
80486508  0047FDC8  7C0802A6  mflr         r0
8048650C  0047FDCC  90010014  stw          r0, 0x14(r1)
80486510  0047FDD0  8183001C  lwz          r12, 0x1c(r3)
80486514  0047FDD4  7D8903A6  mtctr        r12
80486518  0047FDD8  4E800421  bctrl
8048651C  0047FDDC  80010014  lwz          r0, 0x14(r1)
80486520  0047FDE0  7C0803A6  mtlr         r0
80486524  0047FDE4  38210010  addi         r1, r1, 0x10
80486528  0047FDE8  4E800020  blr
