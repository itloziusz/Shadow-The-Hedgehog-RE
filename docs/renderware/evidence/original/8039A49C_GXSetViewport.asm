; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXSetViewport: [8039A49C, 8039A4E4), 0x48 bytes
; classification: SDK viewport adapter
; RAM address  DOL offset  raw word  instruction
8039A49C  00393D5C  7C0802A6  mflr         r0
8039A4A0  00393D60  90010004  stw          r0, 4(r1)
8039A4A4  00393D64  9421FFF8  stwu         r1, -8(r1)
8039A4A8  00393D68  80620FD8  lwz          r3, 0xfd8(r2)
8039A4AC  00393D6C  D02304F4  stfs         f1, 0x4f4(r3)
8039A4B0  00393D70  D04304F8  stfs         f2, 0x4f8(r3)
8039A4B4  00393D74  D06304FC  stfs         f3, 0x4fc(r3)
8039A4B8  00393D78  D0830500  stfs         f4, 0x500(r3)
8039A4BC  00393D7C  D0A30504  stfs         f5, 0x504(r3)
8039A4C0  00393D80  D0C30508  stfs         f6, 0x508(r3)
8039A4C4  00393D84  4BFFFEF1  bl           0x8039a3b4
8039A4C8  00393D88  80620FD8  lwz          r3, 0xfd8(r2)
8039A4CC  00393D8C  38000001  li           r0, 1
8039A4D0  00393D90  B0030002  sth          r0, 2(r3)
8039A4D4  00393D94  8001000C  lwz          r0, 0xc(r1)
8039A4D8  00393D98  38210008  addi         r1, r1, 8
8039A4DC  00393D9C  7C0803A6  mtlr         r0
8039A4E0  00393DA0  4E800020  blr
