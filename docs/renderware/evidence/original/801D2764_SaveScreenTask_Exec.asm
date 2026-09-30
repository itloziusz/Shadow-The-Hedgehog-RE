; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; SaveScreenTask_Exec: [801D2764, 801D2788), 0x24 bytes
; classification: motion-blur capture entrypoint
; RAM address  DOL offset  raw word  instruction
801D2764  001CC024  9421FFF0  stwu         r1, -0x10(r1)
801D2768  001CC028  7C0802A6  mflr         r0
801D276C  001CC02C  90010014  stw          r0, 0x14(r1)
801D2770  001CC030  4BED0E69  bl           0x800a35d8
801D2774  001CC034  4825C9CD  bl           0x8042f140
801D2778  001CC038  80010014  lwz          r0, 0x14(r1)
801D277C  001CC03C  7C0803A6  mtlr         r0
801D2780  001CC040  38210010  addi         r1, r1, 0x10
801D2784  001CC044  4E800020  blr
