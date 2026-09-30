; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; SaveScreenTask_Destroy: [801D270C, 801D2764), 0x58 bytes
; classification: motion-blur task lifecycle
; RAM address  DOL offset  raw word  instruction
801D270C  001CBFCC  9421FFF0  stwu         r1, -0x10(r1)
801D2710  001CBFD0  7C0802A6  mflr         r0
801D2714  001CBFD4  90010014  stw          r0, 0x14(r1)
801D2718  001CBFD8  BFC10008  stmw         r30, 8(r1)
801D271C  001CBFDC  7C7E1B79  or.          r30, r3, r3
801D2720  001CBFE0  7C9F2378  mr           r31, r4
801D2724  001CBFE4  41820028  beq          0x801d274c
801D2728  001CBFE8  3CA08054  lis          r5, -0x7fac
801D272C  001CBFEC  38800000  li           r4, 0
801D2730  001CBFF0  3805DC5C  addi         r0, r5, -0x23a4
801D2734  001CBFF4  901E0018  stw          r0, 0x18(r30)
801D2738  001CBFF8  4BE7C7E1  bl           0x8004ef18
801D273C  001CBFFC  7FE00735  extsh.       r0, r31
801D2740  001CC000  4081000C  ble          0x801d274c
801D2744  001CC004  7FC3F378  mr           r3, r30
801D2748  001CC008  481CEBED  bl           0x803a1334
801D274C  001CC00C  7FC3F378  mr           r3, r30
801D2750  001CC010  BBC10008  lmw          r30, 8(r1)
801D2754  001CC014  80010014  lwz          r0, 0x14(r1)
801D2758  001CC018  7C0803A6  mtlr         r0
801D275C  001CC01C  38210010  addi         r1, r1, 0x10
801D2760  001CC020  4E800020  blr
