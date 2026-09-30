; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; RwWorld_CameraBegin: [80461B6C, 80461BB8), 0x4c bytes
; classification: shared world-plugin camera callback wrapper
; RAM address  DOL offset  raw word  instruction
80461B6C  0045B42C  9421FFF0  stwu         r1, -0x10(r1)
80461B70  0045B430  7C0802A6  mflr         r0
80461B74  0045B434  90010014  stw          r0, 0x14(r1)
80461B78  0045B438  800D60B0  lwz          r0, 0x60b0(r13)
80461B7C  0045B43C  808D615C  lwz          r4, 0x615c(r13)
80461B80  0045B440  7CC30214  add          r6, r3, r0
80461B84  0045B444  8006000C  lwz          r0, 0xc(r6)
80461B88  0045B448  90040004  stw          r0, 4(r4)
80461B8C  0045B44C  80AD615C  lwz          r5, 0x615c(r13)
80461B90  0045B450  A0850008  lhz          r4, 8(r5)
80461B94  0045B454  38040001  addi         r0, r4, 1
80461B98  0045B458  B0050008  sth          r0, 8(r5)
80461B9C  0045B45C  81860010  lwz          r12, 0x10(r6)
80461BA0  0045B460  7D8903A6  mtctr        r12
80461BA4  0045B464  4E800421  bctrl
80461BA8  0045B468  80010014  lwz          r0, 0x14(r1)
80461BAC  0045B46C  7C0803A6  mtlr         r0
80461BB0  0045B470  38210010  addi         r1, r1, 0x10
80461BB4  0045B474  4E800020  blr
