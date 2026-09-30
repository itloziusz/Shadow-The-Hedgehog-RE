; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; EffectParameter_UpdateProgress: [8042B63C, 8042B6CC), 0x90 bytes
; classification: shared progress calculation feeding blur interpolation
; RAM address  DOL offset  raw word  instruction
8042B63C  00424EFC  80830018  lwz          r4, 0x18(r3)
8042B640  00424F00  548007FF  clrlwi.      r0, r4, 0x1f
8042B644  00424F04  4D820020  beqlr
8042B648  00424F08  5480077B  rlwinm.      r0, r4, 0, 0x1d, 0x1d
8042B64C  00424F0C  4C820020  bnelr
8042B650  00424F10  548007BD  rlwinm.      r0, r4, 0, 0x1e, 0x1e
8042B654  00424F14  41820010  beq          0x8042b664
8042B658  00424F18  C003000C  lfs          f0, 0xc(r3)
8042B65C  00424F1C  EC00082A  fadds        f0, f0, f1
8042B660  00424F20  D003000C  stfs         f0, 0xc(r3)
8042B664  00424F24  3C808052  lis          r4, -0x7fae
8042B668  00424F28  C0430104  lfs          f2, 0x104(r3)
8042B66C  00424F2C  C024BFA4  lfs          f1, -0x405c(r4)
8042B670  00424F30  C063000C  lfs          f3, 0xc(r3)
8042B674  00424F34  EC01102A  fadds        f0, f1, f2
8042B678  00424F38  FC030040  fcmpo        cr0, f3, f0
8042B67C  00424F3C  40810014  ble          0x8042b690
8042B680  00424F40  80030018  lwz          r0, 0x18(r3)
8042B684  00424F44  5400003C  rlwinm       r0, r0, 0, 0, 0x1e
8042B688  00424F48  90030018  stw          r0, 0x18(r3)
8042B68C  00424F4C  4E800020  blr
8042B690  00424F50  FC020840  fcmpo        cr0, f2, f1
8042B694  00424F54  40810010  ble          0x8042b6a4
8042B698  00424F58  EC031024  fdivs        f0, f3, f2
8042B69C  00424F5C  D0030100  stfs         f0, 0x100(r3)
8042B6A0  00424F60  48000010  b            0x8042b6b0
8042B6A4  00424F64  3C808052  lis          r4, -0x7fae
8042B6A8  00424F68  C004BFA8  lfs          f0, -0x4058(r4)
8042B6AC  00424F6C  D0030100  stfs         f0, 0x100(r3)
8042B6B0  00424F70  3C808052  lis          r4, -0x7fae
8042B6B4  00424F74  C0230100  lfs          f1, 0x100(r3)
8042B6B8  00424F78  C004BFA8  lfs          f0, -0x4058(r4)
8042B6BC  00424F7C  FC010040  fcmpo        cr0, f1, f0
8042B6C0  00424F80  4C810020  blelr
8042B6C4  00424F84  D0030100  stfs         f0, 0x100(r3)
8042B6C8  00424F88  4E800020  blr
