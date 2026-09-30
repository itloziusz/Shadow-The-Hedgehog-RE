; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; EffectService_Get: [8042B620, 8042B630), 0x10 bytes
; classification: shared camera-provider service accessor
; RAM address  DOL offset  raw word  instruction
8042B620  00424EE0  3C60805E  lis          r3, -0x7fa2
8042B624  00424EE4  386325C8  addi         r3, r3, 0x25c8
8042B628  00424EE8  80630000  lwz          r3, 0(r3)
8042B62C  00424EEC  4E800020  blr
