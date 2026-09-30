; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXSetScissor: [8039A4E4, 8039A55C), 0x78 bytes
; classification: SDK scissor adapter
; RAM address  DOL offset  raw word  instruction
8039A4E4  00393DA4  80E20FD8  lwz          r7, 0xfd8(r2)
8039A4E8  00393DA8  38840156  addi         r4, r4, 0x156
8039A4EC  00393DAC  38C6FFFF  addi         r6, r6, -1
8039A4F0  00393DB0  800700F8  lwz          r0, 0xf8(r7)
8039A4F4  00393DB4  5080057E  rlwimi       r0, r4, 0, 0x15, 0x1f
8039A4F8  00393DB8  38630156  addi         r3, r3, 0x156
8039A4FC  00393DBC  900700F8  stw          r0, 0xf8(r7)
8039A500  00393DC0  3905FFFF  addi         r8, r5, -1
8039A504  00393DC4  7CC43214  add          r6, r4, r6
8039A508  00393DC8  800700F8  lwz          r0, 0xf8(r7)
8039A50C  00393DCC  50606266  rlwimi       r0, r3, 0xc, 9, 0x13
8039A510  00393DD0  7D034214  add          r8, r3, r8
8039A514  00393DD4  900700F8  stw          r0, 0xf8(r7)
8039A518  00393DD8  38A00061  li           r5, 0x61
8039A51C  00393DDC  3C80CC01  lis          r4, -0x33ff
8039A520  00393DE0  806700FC  lwz          r3, 0xfc(r7)
8039A524  00393DE4  50C3057E  rlwimi       r3, r6, 0, 0x15, 0x1f
8039A528  00393DE8  38000000  li           r0, 0
8039A52C  00393DEC  906700FC  stw          r3, 0xfc(r7)
8039A530  00393DF0  806700FC  lwz          r3, 0xfc(r7)
8039A534  00393DF4  51036266  rlwimi       r3, r8, 0xc, 9, 0x13
8039A538  00393DF8  906700FC  stw          r3, 0xfc(r7)
8039A53C  00393DFC  98A48000  stb          r5, -0x8000(r4)
8039A540  00393E00  806700F8  lwz          r3, 0xf8(r7)
8039A544  00393E04  90648000  stw          r3, -0x8000(r4)
8039A548  00393E08  98A48000  stb          r5, -0x8000(r4)
8039A54C  00393E0C  806700FC  lwz          r3, 0xfc(r7)
8039A550  00393E10  90648000  stw          r3, -0x8000(r4)
8039A554  00393E14  B0070002  sth          r0, 2(r7)
8039A558  00393E18  4E800020  blr
