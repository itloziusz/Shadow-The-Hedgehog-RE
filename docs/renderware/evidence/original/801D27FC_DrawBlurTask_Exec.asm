; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; DrawBlurTask_Exec: [801D27FC, 801D2820), 0x24 bytes
; classification: motion-blur draw entrypoint
; RAM address  DOL offset  raw word  instruction
801D27FC  001CC0BC  9421FFF0  stwu         r1, -0x10(r1)
801D2800  001CC0C0  7C0802A6  mflr         r0
801D2804  001CC0C4  90010014  stw          r0, 0x14(r1)
801D2808  001CC0C8  4BED0DD1  bl           0x800a35d8
801D280C  001CC0CC  4825C961  bl           0x8042f16c
801D2810  001CC0D0  80010014  lwz          r0, 0x14(r1)
801D2814  001CC0D4  7C0803A6  mtlr         r0
801D2818  001CC0D8  38210010  addi         r1, r1, 0x10
801D281C  001CC0DC  4E800020  blr
