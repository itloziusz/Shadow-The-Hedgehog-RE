; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; RwCamera_BackendEnd: [80495CAC, 80495CBC), 0x10 bytes
; classification: shared current-camera release
; RAM address  DOL offset  raw word  instruction
80495CAC  0048F56C  38000000  li           r0, 0
80495CB0  0048F570  38600001  li           r3, 1
80495CB4  0048F574  900D61F8  stw          r0, 0x61f8(r13)
80495CB8  0048F578  4E800020  blr
