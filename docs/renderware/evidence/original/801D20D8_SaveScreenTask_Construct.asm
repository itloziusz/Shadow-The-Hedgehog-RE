; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; SaveScreenTask_Construct: [801D20D8, 801D213C), 0x64 bytes
; classification: motion-blur task lifecycle
; RAM address  DOL offset  raw word  instruction
801D20D8  001CB998  9421FFF0  stwu         r1, -0x10(r1)
801D20DC  001CB99C  7C0802A6  mflr         r0
801D20E0  001CB9A0  90010014  stw          r0, 0x14(r1)
801D20E4  001CB9A4  93E1000C  stw          r31, 0xc(r1)
801D20E8  001CB9A8  7C7F1B78  mr           r31, r3
801D20EC  001CB9AC  4BE740B5  bl           0x800461a0
801D20F0  001CB9B0  7C641B78  mr           r4, r3
801D20F4  001CB9B4  7FE3FB78  mr           r3, r31
801D20F8  001CB9B8  80840058  lwz          r4, 0x58(r4)
801D20FC  001CB9BC  4BE7CF19  bl           0x8004f014
801D2100  001CB9C0  3C608054  lis          r3, -0x7fac
801D2104  001CB9C4  3C80804D  lis          r4, -0x7fb3
801D2108  001CB9C8  3803DC5C  addi         r0, r3, -0x23a4
801D210C  001CB9CC  901F0018  stw          r0, 0x18(r31)
801D2110  001CB9D0  38040F24  addi         r0, r4, 0xf24
801D2114  001CB9D4  7FE3FB78  mr           r3, r31
801D2118  001CB9D8  901F0000  stw          r0, 0(r31)
801D211C  001CB9DC  A01F0004  lhz          r0, 4(r31)
801D2120  001CB9E0  60000100  ori          r0, r0, 0x100
801D2124  001CB9E4  B01F0004  sth          r0, 4(r31)
801D2128  001CB9E8  80010014  lwz          r0, 0x14(r1)
801D212C  001CB9EC  83E1000C  lwz          r31, 0xc(r1)
801D2130  001CB9F0  7C0803A6  mtlr         r0
801D2134  001CB9F4  38210010  addi         r1, r1, 0x10
801D2138  001CB9F8  4E800020  blr
