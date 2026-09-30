; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXLoadTexObj: [8039843C, 80398490), 0x54 bytes
; classification: SDK texture-region selection and upload adapter
; RAM address  DOL offset  raw word  instruction
8039843C  00391CFC  7C0802A6  mflr         r0
80398440  00391D00  90010004  stw          r0, 4(r1)
80398444  00391D04  9421FFE8  stwu         r1, -0x18(r1)
80398448  00391D08  93E10014  stw          r31, 0x14(r1)
8039844C  00391D0C  3BE40000  addi         r31, r4, 0
80398450  00391D10  93C10010  stw          r30, 0x10(r1)
80398454  00391D14  3BC30000  addi         r30, r3, 0
80398458  00391D18  80A20FD8  lwz          r5, 0xfd8(r2)
8039845C  00391D1C  818504C8  lwz          r12, 0x4c8(r5)
80398460  00391D20  7D8803A6  mtlr         r12
80398464  00391D24  4E800021  blrl
80398468  00391D28  38830000  addi         r4, r3, 0
8039846C  00391D2C  387E0000  addi         r3, r30, 0
80398470  00391D30  38BF0000  addi         r5, r31, 0
80398474  00391D34  4BFFFE4D  bl           0x803982c0
80398478  00391D38  8001001C  lwz          r0, 0x1c(r1)
8039847C  00391D3C  83E10014  lwz          r31, 0x14(r1)
80398480  00391D40  83C10010  lwz          r30, 0x10(r1)
80398484  00391D44  38210018  addi         r1, r1, 0x18
80398488  00391D48  7C0803A6  mtlr         r0
8039848C  00391D4C  4E800020  blr
