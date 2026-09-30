; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; EffectSystemPJS_Construct: [801E293C, 801E2978), 0x3c bytes
; classification: shared provider vtable installation
; RAM address  DOL offset  raw word  instruction
801E293C  001DC1FC  9421FFF0  stwu         r1, -0x10(r1)
801E2940  001DC200  7C0802A6  mflr         r0
801E2944  001DC204  90010014  stw          r0, 0x14(r1)
801E2948  001DC208  93E1000C  stw          r31, 0xc(r1)
801E294C  001DC20C  7C7F1B78  mr           r31, r3
801E2950  001DC210  48000029  bl           0x801e2978
801E2954  001DC214  3C808054  lis          r4, -0x7fac
801E2958  001DC218  7FE3FB78  mr           r3, r31
801E295C  001DC21C  3804E93C  addi         r0, r4, -0x16c4
801E2960  001DC220  901F0000  stw          r0, 0(r31)
801E2964  001DC224  83E1000C  lwz          r31, 0xc(r1)
801E2968  001DC228  80010014  lwz          r0, 0x14(r1)
801E296C  001DC22C  7C0803A6  mtlr         r0
801E2970  001DC230  38210010  addi         r1, r1, 0x10
801E2974  001DC234  4E800020  blr
