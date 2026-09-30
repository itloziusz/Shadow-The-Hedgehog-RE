; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; DCInvalidateRange: [80372508, 80372534), 0x2c bytes
; classification: SDK CPU cache invalidation; not pixel initialization
; RAM address  DOL offset  raw word  instruction
80372508  0036BDC8  28040000  cmplwi       r4, 0
8037250C  0036BDCC  4C810020  blelr
80372510  0036BDD0  546506FE  clrlwi       r5, r3, 0x1b
80372514  0036BDD4  7C842A14  add          r4, r4, r5
80372518  0036BDD8  3884001F  addi         r4, r4, 0x1f
8037251C  0036BDDC  5484D97E  srwi         r4, r4, 5
80372520  0036BDE0  7C8903A6  mtctr        r4
80372524  0036BDE4  7C001BAC  dcbi         0, r3
80372528  0036BDE8  38630020  addi         r3, r3, 0x20
8037252C  0036BDEC  4200FFF8  bdnz         0x80372524
80372530  0036BDF0  4E800020  blr
