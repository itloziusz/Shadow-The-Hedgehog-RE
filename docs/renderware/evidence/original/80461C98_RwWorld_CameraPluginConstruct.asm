; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; RwWorld_CameraPluginConstruct: [80461C98, 80461CF4), 0x5c bytes
; classification: shared world-plugin callback installation
; RAM address  DOL offset  raw word  instruction
80461C98  0045B558  800D60B0  lwz          r0, 0x60b0(r13)
80461C9C  0045B55C  3CC08046  lis          r6, -0x7fba
80461CA0  0045B560  3CA08046  lis          r5, -0x7fba
80461CA4  0045B564  3C808046  lis          r4, -0x7fba
80461CA8  0045B568  7D030214  add          r8, r3, r0
80461CAC  0045B56C  38E00000  li           r7, 0
80461CB0  0045B570  90E80000  stw          r7, 0(r8)
80461CB4  0045B574  38C61BF4  addi         r6, r6, 0x1bf4
80461CB8  0045B578  38A51B6C  addi         r5, r5, 0x1b6c
80461CBC  0045B57C  38041BB8  addi         r0, r4, 0x1bb8
80461CC0  0045B580  90E80004  stw          r7, 4(r8)
80461CC4  0045B584  90E80008  stw          r7, 8(r8)
80461CC8  0045B588  80830018  lwz          r4, 0x18(r3)
80461CCC  0045B58C  90880010  stw          r4, 0x10(r8)
80461CD0  0045B590  8083001C  lwz          r4, 0x1c(r3)
80461CD4  0045B594  90880014  stw          r4, 0x14(r8)
80461CD8  0045B598  80830010  lwz          r4, 0x10(r3)
80461CDC  0045B59C  90880018  stw          r4, 0x18(r8)
80461CE0  0045B5A0  90C30010  stw          r6, 0x10(r3)
80461CE4  0045B5A4  90A30018  stw          r5, 0x18(r3)
80461CE8  0045B5A8  9003001C  stw          r0, 0x1c(r3)
80461CEC  0045B5AC  90E8000C  stw          r7, 0xc(r8)
80461CF0  0045B5B0  4E800020  blr
