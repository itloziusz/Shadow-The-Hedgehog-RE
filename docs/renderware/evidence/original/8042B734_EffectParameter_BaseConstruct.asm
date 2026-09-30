; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; EffectParameter_BaseConstruct: [8042B734, 8042B7C0), 0x8c bytes
; classification: shared descriptor flags/resource/duration transfer
; RAM address  DOL offset  raw word  instruction
8042B734  00424FF4  9421FFF0  stwu         r1, -0x10(r1)
8042B738  00424FF8  7C0802A6  mflr         r0
8042B73C  00424FFC  90010014  stw          r0, 0x14(r1)
8042B740  00425000  BFC10008  stmw         r30, 8(r1)
8042B744  00425004  7C7E1B78  mr           r30, r3
8042B748  00425008  7C9F2378  mr           r31, r4
8042B74C  0042500C  4BFFB841  bl           0x80426f8c
8042B750  00425010  3C608057  lis          r3, -0x7fa9
8042B754  00425014  38000000  li           r0, 0
8042B758  00425018  3863B984  addi         r3, r3, -0x467c
8042B75C  0042501C  907E0000  stw          r3, 0(r30)
8042B760  00425020  901E00FC  stw          r0, 0xfc(r30)
8042B764  00425024  807E0008  lwz          r3, 8(r30)
8042B768  00425028  3803FF04  addi         r0, r3, -0xfc
8042B76C  0042502C  901E0008  stw          r0, 8(r30)
8042B770  00425030  807E0008  lwz          r3, 8(r30)
8042B774  00425034  3803010C  addi         r0, r3, 0x10c
8042B778  00425038  901E0008  stw          r0, 8(r30)
8042B77C  0042503C  801E0018  lwz          r0, 0x18(r30)
8042B780  00425040  540007FF  clrlwi.      r0, r0, 0x1f
8042B784  00425044  4082000C  bne          0x8042b790
8042B788  00425048  7FC3F378  mr           r3, r30
8042B78C  0042504C  48000020  b            0x8042b7ac
8042B790  00425050  801F0034  lwz          r0, 0x34(r31)
8042B794  00425054  7FC3F378  mr           r3, r30
8042B798  00425058  901E0108  stw          r0, 0x108(r30)
8042B79C  0042505C  C01F0030  lfs          f0, 0x30(r31)
8042B7A0  00425060  D01E0104  stfs         f0, 0x104(r30)
8042B7A4  00425064  801F002C  lwz          r0, 0x2c(r31)
8042B7A8  00425068  901E00FC  stw          r0, 0xfc(r30)
8042B7AC  0042506C  BBC10008  lmw          r30, 8(r1)
8042B7B0  00425070  80010014  lwz          r0, 0x14(r1)
8042B7B4  00425074  7C0803A6  mtlr         r0
8042B7B8  00425078  38210010  addi         r1, r1, 0x10
8042B7BC  0042507C  4E800020  blr
