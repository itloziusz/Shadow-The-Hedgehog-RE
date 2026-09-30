; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXSetViewportJitter: [8039A444, 8039A49C), 0x58 bytes
; classification: SDK viewport field-jitter adapter
; RAM address  DOL offset  raw word  instruction
8039A444  00393D04  7C0802A6  mflr         r0
8039A448  00393D08  28030000  cmplwi       r3, 0
8039A44C  00393D0C  90010004  stw          r0, 4(r1)
8039A450  00393D10  9421FFF8  stwu         r1, -8(r1)
8039A454  00393D14  4082000C  bne          0x8039a460
8039A458  00393D18  C00210C0  lfs          f0, 0x10c0(r2)
8039A45C  00393D1C  EC420028  fsubs        f2, f2, f0
8039A460  00393D20  80620FD8  lwz          r3, 0xfd8(r2)
8039A464  00393D24  D02304F4  stfs         f1, 0x4f4(r3)
8039A468  00393D28  D04304F8  stfs         f2, 0x4f8(r3)
8039A46C  00393D2C  D06304FC  stfs         f3, 0x4fc(r3)
8039A470  00393D30  D0830500  stfs         f4, 0x500(r3)
8039A474  00393D34  D0A30504  stfs         f5, 0x504(r3)
8039A478  00393D38  D0C30508  stfs         f6, 0x508(r3)
8039A47C  00393D3C  4BFFFF39  bl           0x8039a3b4
8039A480  00393D40  80620FD8  lwz          r3, 0xfd8(r2)
8039A484  00393D44  38000001  li           r0, 1
8039A488  00393D48  B0030002  sth          r0, 2(r3)
8039A48C  00393D4C  8001000C  lwz          r0, 0xc(r1)
8039A490  00393D50  38210008  addi         r1, r1, 8
8039A494  00393D54  7C0803A6  mtlr         r0
8039A498  00393D58  4E800020  blr
