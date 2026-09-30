; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXSetNumTevStages: [803999E4, 80399A0C), 0x28 bytes
; classification: shared SDK TEV stage count
; RAM address  DOL offset  raw word  instruction
803999E4  003932A4  80820FD8  lwz          r4, 0xfd8(r2)
803999E8  003932A8  5463063E  clrlwi       r3, r3, 0x18
803999EC  003932AC  3803FFFF  addi         r0, r3, -1
803999F0  003932B0  80640204  lwz          r3, 0x204(r4)
803999F4  003932B4  500354AA  rlwimi       r3, r0, 0xa, 0x12, 0x15
803999F8  003932B8  90640204  stw          r3, 0x204(r4)
803999FC  003932BC  800405AC  lwz          r0, 0x5ac(r4)
80399A00  003932C0  60000004  ori          r0, r0, 4
80399A04  003932C4  900405AC  stw          r0, 0x5ac(r4)
80399A08  003932C8  4E800020  blr
