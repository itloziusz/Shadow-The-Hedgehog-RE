; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; GXSetNumChans: [80397C80, 80397CBC), 0x3c bytes
; classification: shared SDK color-channel count
; RAM address  DOL offset  raw word  instruction
80397C80  00391540  80C20FD8  lwz          r6, 0xfd8(r2)
80397C84  00391544  5465063E  clrlwi       r5, r3, 0x18
80397C88  00391548  38800010  li           r4, 0x10
80397C8C  0039154C  80060204  lwz          r0, 0x204(r6)
80397C90  00391550  50602676  rlwimi       r0, r3, 4, 0x19, 0x1b
80397C94  00391554  3C60CC01  lis          r3, -0x33ff
80397C98  00391558  90060204  stw          r0, 0x204(r6)
80397C9C  0039155C  38001009  li           r0, 0x1009
80397CA0  00391560  98838000  stb          r4, -0x8000(r3)
80397CA4  00391564  90038000  stw          r0, -0x8000(r3)
80397CA8  00391568  90A38000  stw          r5, -0x8000(r3)
80397CAC  0039156C  800605AC  lwz          r0, 0x5ac(r6)
80397CB0  00391570  60000004  ori          r0, r0, 4
80397CB4  00391574  900605AC  stw          r0, 0x5ac(r6)
80397CB8  00391578  4E800020  blr
