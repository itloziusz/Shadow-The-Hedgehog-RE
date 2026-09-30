; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXGetTexObjBiasClamp: [8039828C, 80398298), 0xc bytes
; classification: SDK sampler-preservation adapter
; RAM address  DOL offset  raw word  instruction
8039828C  00391B4C  80030000  lwz          r0, 0(r3)
80398290  00391B50  54035FFE  rlwinm       r3, r0, 0xb, 0x1f, 0x1f
80398294  00391B54  4E800020  blr
