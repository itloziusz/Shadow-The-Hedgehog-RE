; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXSetAlphaUpdate: [80399DC4, 80399DF0), 0x2c bytes
; classification: shared GX SDK write-mask dependency
; RAM address  DOL offset  raw word  instruction
80399DC4  00393684  80A20FD8  lwz          r5, 0xfd8(r2)
80399DC8  00393688  38000061  li           r0, 0x61
80399DCC  0039368C  3C80CC01  lis          r4, -0x33ff
80399DD0  00393690  80C501D0  lwz          r6, 0x1d0(r5)
80399DD4  00393694  506626F6  rlwimi       r6, r3, 4, 0x1b, 0x1b
80399DD8  00393698  98048000  stb          r0, -0x8000(r4)
80399DDC  0039369C  38000000  li           r0, 0
80399DE0  003936A0  90C48000  stw          r6, -0x8000(r4)
80399DE4  003936A4  90C501D0  stw          r6, 0x1d0(r5)
80399DE8  003936A8  B0050002  sth          r0, 2(r5)
80399DEC  003936AC  4E800020  blr
