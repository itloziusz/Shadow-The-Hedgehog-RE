; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; CameraManager_Get: [8000DCC0, 8000DD2C), 0x6c bytes
; classification: shared camera source singleton; provider returns its field0
; RAM address  DOL offset  raw word  instruction
8000DCC0  00007580  9421FFF0  stwu         r1, -0x10(r1)
8000DCC4  00007584  7C0802A6  mflr         r0
8000DCC8  00007588  90010014  stw          r0, 0x14(r1)
8000DCCC  0000758C  800D2B68  lwz          r0, 0x2b68(r13)
8000DCD0  00007590  28000000  cmplwi       r0, 0
8000DCD4  00007594  40820044  bne          0x8000dd18
8000DCD8  00007598  880D2B50  lbz          r0, 0x2b50(r13)
8000DCDC  0000759C  7C000775  extsb.       r0, r0
8000DCE0  000075A0  4082002C  bne          0x8000dd0c
8000DCE4  000075A4  3C608057  lis          r3, -0x7fa9
8000DCE8  000075A8  3863FF1C  addi         r3, r3, -0xe4
8000DCEC  000075AC  482648AD  bl           0x80272598
8000DCF0  000075B0  3C808027  lis          r4, -0x7fd9
8000DCF4  000075B4  3CA08057  lis          r5, -0x7fa9
8000DCF8  000075B8  3884245C  addi         r4, r4, 0x245c
8000DCFC  000075BC  38A5FF10  addi         r5, r5, -0xf0
8000DD00  000075C0  48393511  bl           0x803a1210
8000DD04  000075C4  38000001  li           r0, 1
8000DD08  000075C8  980D2B50  stb          r0, 0x2b50(r13)
8000DD0C  000075CC  3C608057  lis          r3, -0x7fa9
8000DD10  000075D0  3803FF1C  addi         r0, r3, -0xe4
8000DD14  000075D4  900D2B68  stw          r0, 0x2b68(r13)
8000DD18  000075D8  80010014  lwz          r0, 0x14(r1)
8000DD1C  000075DC  806D2B68  lwz          r3, 0x2b68(r13)
8000DD20  000075E0  7C0803A6  mtlr         r0
8000DD24  000075E4  38210010  addi         r1, r1, 0x10
8000DD28  000075E8  4E800020  blr
