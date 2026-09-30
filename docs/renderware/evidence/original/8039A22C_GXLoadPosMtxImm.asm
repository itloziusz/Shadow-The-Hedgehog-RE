; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXLoadPosMtxImm: [8039A22C, 8039A27C), 0x50 bytes
; classification: SDK position-matrix upload adapter
; RAM address  DOL offset  raw word  instruction
8039A22C  00393AEC  3CA0CC01  lis          r5, -0x33ff
8039A230  00393AF0  38000010  li           r0, 0x10
8039A234  00393AF4  5484103A  slwi         r4, r4, 2
8039A238  00393AF8  98058000  stb          r0, -0x8000(r5)
8039A23C  00393AFC  6480000B  oris         r0, r4, 0xb
8039A240  00393B00  90058000  stw          r0, -0x8000(r5)
8039A244  00393B04  38858000  addi         r4, r5, -0x8000
8039A248  00393B08  E0A30000  psq_l        f5, 0(r3), 0, 0
8039A24C  00393B0C  E0830008  psq_l        f4, 8(r3), 0, 0
8039A250  00393B10  E0630010  psq_l        f3, 16(r3), 0, 0
8039A254  00393B14  E0430018  psq_l        f2, 24(r3), 0, 0
8039A258  00393B18  E0230020  psq_l        f1, 32(r3), 0, 0
8039A25C  00393B1C  E0030028  psq_l        f0, 40(r3), 0, 0
8039A260  00393B20  F0A40000  psq_st       f5, 0(r4), 0, 0
8039A264  00393B24  F0840000  psq_st       f4, 0(r4), 0, 0
8039A268  00393B28  F0640000  psq_st       f3, 0(r4), 0, 0
8039A26C  00393B2C  F0440000  psq_st       f2, 0(r4), 0, 0
8039A270  00393B30  F0240000  psq_st       f1, 0(r4), 0, 0
8039A274  00393B34  F0040000  psq_st       f0, 0(r4), 0, 0
8039A278  00393B38  4E800020  blr
