; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; RenderAttach_StaticExec: [801E4E40, 801E4EA8), 0x68 bytes
; classification: shared camera selector reset before SaveScreen
; RAM address  DOL offset  raw word  instruction
801E4E40  001DE700  9421FFF0  stwu         r1, -0x10(r1)
801E4E44  001DE704  7C0802A6  mflr         r0
801E4E48  001DE708  90010014  stw          r0, 0x14(r1)
801E4E4C  001DE70C  BFC10008  stmw         r30, 8(r1)
801E4E50  001DE710  4BE28E71  bl           0x8000dcc0
801E4E54  001DE714  3C808058  lis          r4, -0x7fa8
801E4E58  001DE718  38000000  li           r0, 0
801E4E5C  001DE71C  3BE4E760  addi         r31, r4, -0x18a0
801E4E60  001DE720  7C7E1B78  mr           r30, r3
801E4E64  001DE724  901F0070  stw          r0, 0x70(r31)
801E4E68  001DE728  38800000  li           r4, 0
801E4E6C  001DE72C  901F006C  stw          r0, 0x6c(r31)
801E4E70  001DE730  4808D2AD  bl           0x8027211c
801E4E74  001DE734  801F0070  lwz          r0, 0x70(r31)
801E4E78  001DE738  3C608058  lis          r3, -0x7fa8
801E4E7C  001DE73C  3883E760  addi         r4, r3, -0x18a0
801E4E80  001DE740  7FC3F378  mr           r3, r30
801E4E84  001DE744  5400103A  slwi         r0, r0, 2
801E4E88  001DE748  7C840214  add          r4, r4, r0
801E4E8C  001DE74C  80840064  lwz          r4, 0x64(r4)
801E4E90  001DE750  4808D10D  bl           0x80271f9c
801E4E94  001DE754  BBC10008  lmw          r30, 8(r1)
801E4E98  001DE758  80010014  lwz          r0, 0x14(r1)
801E4E9C  001DE75C  7C0803A6  mtlr         r0
801E4EA0  001DE760  38210010  addi         r1, r1, 0x10
801E4EA4  001DE764  4E800020  blr
