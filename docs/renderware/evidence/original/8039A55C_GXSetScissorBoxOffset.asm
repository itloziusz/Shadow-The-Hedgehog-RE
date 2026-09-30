; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXSetScissorBoxOffset: [8039A55C, 8039A59C), 0x40 bytes
; classification: SDK scissor-offset adapter
; RAM address  DOL offset  raw word  instruction
8039A55C  00393E1C  38A30156  addi         r5, r3, 0x156
8039A560  00393E20  80620FD8  lwz          r3, 0xfd8(r2)
8039A564  00393E24  38040156  addi         r0, r4, 0x156
8039A568  00393E28  38800000  li           r4, 0
8039A56C  00393E2C  50A4FDBE  rlwimi       r4, r5, 0x1f, 0x16, 0x1f
8039A570  00393E30  38A40000  addi         r5, r4, 0
8039A574  00393E34  50054B2A  rlwimi       r5, r0, 9, 0xc, 0x15
8039A578  00393E38  38000061  li           r0, 0x61
8039A57C  00393E3C  3C80CC01  lis          r4, -0x33ff
8039A580  00393E40  98048000  stb          r0, -0x8000(r4)
8039A584  00393E44  38000059  li           r0, 0x59
8039A588  00393E48  5005C00E  rlwimi       r5, r0, 0x18, 0, 7
8039A58C  00393E4C  90A48000  stw          r5, -0x8000(r4)
8039A590  00393E50  38000000  li           r0, 0
8039A594  00393E54  B0030002  sth          r0, 2(r3)
8039A598  00393E58  4E800020  blr
