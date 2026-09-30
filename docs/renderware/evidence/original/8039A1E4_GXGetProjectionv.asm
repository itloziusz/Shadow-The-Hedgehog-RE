; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXGetProjectionv: [8039A1E4, 8039A22C), 0x48 bytes
; classification: SDK same-call projection snapshot adapter
; RAM address  DOL offset  raw word  instruction
8039A1E4  00393AA4  80820FD8  lwz          r4, 0xfd8(r2)
8039A1E8  00393AA8  800404D8  lwz          r0, 0x4d8(r4)
8039A1EC  00393AAC  28000000  cmplwi       r0, 0
8039A1F0  00393AB0  4182000C  beq          0x8039a1fc
8039A1F4  00393AB4  C00210BC  lfs          f0, 0x10bc(r2)
8039A1F8  00393AB8  48000008  b            0x8039a200
8039A1FC  00393ABC  C00210B8  lfs          f0, 0x10b8(r2)
8039A200  00393AC0  D0030000  stfs         f0, 0(r3)
8039A204  00393AC4  38830004  addi         r4, r3, 4
8039A208  00393AC8  80620FD8  lwz          r3, 0xfd8(r2)
8039A20C  00393ACC  386304DC  addi         r3, r3, 0x4dc
8039A210  00393AD0  E0430000  psq_l        f2, 0(r3), 0, 0
8039A214  00393AD4  E0230008  psq_l        f1, 8(r3), 0, 0
8039A218  00393AD8  E0030010  psq_l        f0, 16(r3), 0, 0
8039A21C  00393ADC  F0440000  psq_st       f2, 0(r4), 0, 0
8039A220  00393AE0  F0240008  psq_st       f1, 8(r4), 0, 0
8039A224  00393AE4  F0040010  psq_st       f0, 16(r4), 0, 0
8039A228  00393AE8  4E800020  blr
