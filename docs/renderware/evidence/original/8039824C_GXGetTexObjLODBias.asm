; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXGetTexObjLODBias: [8039824C, 8039828C), 0x40 bytes
; classification: SDK sampler-preservation adapter
; RAM address  DOL offset  raw word  instruction
8039824C  00391B0C  9421FFE0  stwu         r1, -0x20(r1)
80398250  00391B10  3C004330  lis          r0, 0x4330
80398254  00391B14  80630000  lwz          r3, 0(r3)
80398258  00391B18  C8421070  lfd          f2, 0x1070(r2)
8039825C  00391B1C  5463BE3E  rlwinm       r3, r3, 0x17, 0x18, 0x1f
80398260  00391B20  C0021068  lfs          f0, 0x1068(r2)
80398264  00391B24  7C630734  extsh        r3, r3
80398268  00391B28  7C630774  extsb        r3, r3
8039826C  00391B2C  6C638000  xoris        r3, r3, 0x8000
80398270  00391B30  9061001C  stw          r3, 0x1c(r1)
80398274  00391B34  90010018  stw          r0, 0x18(r1)
80398278  00391B38  C8210018  lfd          f1, 0x18(r1)
8039827C  00391B3C  EC211028  fsubs        f1, f1, f2
80398280  00391B40  EC210032  fmuls        f1, f1, f0
80398284  00391B44  38210020  addi         r1, r1, 0x20
80398288  00391B48  4E800020  blr
