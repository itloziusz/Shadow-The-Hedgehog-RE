; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; RwWorld_CameraEnd: [80461BB8, 80461BF4), 0x3c bytes
; classification: shared world-plugin camera callback wrapper
; RAM address  DOL offset  raw word  instruction
80461BB8  0045B478  9421FFF0  stwu         r1, -0x10(r1)
80461BBC  0045B47C  7C0802A6  mflr         r0
80461BC0  0045B480  90010014  stw          r0, 0x14(r1)
80461BC4  0045B484  38000000  li           r0, 0
80461BC8  0045B488  80AD60B0  lwz          r5, 0x60b0(r13)
80461BCC  0045B48C  808D615C  lwz          r4, 0x615c(r13)
80461BD0  0045B490  7CA32A14  add          r5, r3, r5
80461BD4  0045B494  90040004  stw          r0, 4(r4)
80461BD8  0045B498  81850014  lwz          r12, 0x14(r5)
80461BDC  0045B49C  7D8903A6  mtctr        r12
80461BE0  0045B4A0  4E800421  bctrl
80461BE4  0045B4A4  80010014  lwz          r0, 0x14(r1)
80461BE8  0045B4A8  7C0803A6  mtlr         r0
80461BEC  0045B4AC  38210010  addi         r1, r1, 0x10
80461BF0  0045B4B0  4E800020  blr
