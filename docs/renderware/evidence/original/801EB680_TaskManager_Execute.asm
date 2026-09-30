; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; TaskManager_Execute: [801EB680, 801EB6AC), 0x2c bytes
; classification: shared root traversal entry
; RAM address  DOL offset  raw word  instruction
801EB680  001E4F40  9421FFF0  stwu         r1, -0x10(r1)
801EB684  001E4F44  7C0802A6  mflr         r0
801EB688  001E4F48  90010014  stw          r0, 0x14(r1)
801EB68C  001E4F4C  80630000  lwz          r3, 0(r3)
801EB690  001E4F50  28030000  cmplwi       r3, 0
801EB694  001E4F54  41820008  beq          0x801eb69c
801EB698  001E4F58  4BE63615  bl           0x8004ecac
801EB69C  001E4F5C  80010014  lwz          r0, 0x14(r1)
801EB6A0  001E4F60  7C0803A6  mtlr         r0
801EB6A4  001E4F64  38210010  addi         r1, r1, 0x10
801EB6A8  001E4F68  4E800020  blr
