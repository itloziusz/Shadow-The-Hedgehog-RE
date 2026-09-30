; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; RwTexture_SetRaster: [8049819C, 804981B8), 0x1c bytes
; classification: shared texture binding dependency
; RAM address  DOL offset  raw word  instruction
8049819C  00491A5C  90830000  stw          r4, 0(r3)
804981A0  00491A60  3CA00100  lis          r5, 0x100
804981A4  00491A64  808D6218  lwz          r4, 0x6218(r13)
804981A8  00491A68  38040020  addi         r0, r4, 0x20
804981AC  00491A6C  7CA3012E  stwx         r5, r3, r0
804981B0  00491A70  38600001  li           r3, 1
804981B4  00491A74  4E800020  blr
