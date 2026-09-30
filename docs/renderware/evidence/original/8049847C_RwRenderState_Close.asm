; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; RwRenderState_Close: [8049847C, 804984C4), 0x48 bytes
; classification: shared texture descriptor teardown
; RAM address  DOL offset  raw word  instruction
8049847C  00491D3C  9421FFF0  stwu         r1, -0x10(r1)
80498480  00491D40  7C0802A6  mflr         r0
80498484  00491D44  90010014  stw          r0, 0x14(r1)
80498488  00491D48  806D6208  lwz          r3, 0x6208(r13)
8049848C  00491D4C  4BFF27A1  bl           0x8048ac2c
80498490  00491D50  38000000  li           r0, 0
80498494  00491D54  806D620C  lwz          r3, 0x620c(r13)
80498498  00491D58  900D6208  stw          r0, 0x6208(r13)
8049849C  00491D5C  38800000  li           r4, 0
804984A0  00491D60  4BFF55B1  bl           0x8048da50
804984A4  00491D64  806D620C  lwz          r3, 0x620c(r13)
804984A8  00491D68  4BFF5935  bl           0x8048dddc
804984AC  00491D6C  38000000  li           r0, 0
804984B0  00491D70  900D620C  stw          r0, 0x620c(r13)
804984B4  00491D74  80010014  lwz          r0, 0x14(r1)
804984B8  00491D78  7C0803A6  mtlr         r0
804984BC  00491D7C  38210010  addi         r1, r1, 0x10
804984C0  00491D80  4E800020  blr
