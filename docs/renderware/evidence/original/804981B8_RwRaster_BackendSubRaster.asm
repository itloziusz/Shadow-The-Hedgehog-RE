; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; RwRaster_BackendSubRaster: [804981B8, 804981E8), 0x30 bytes
; classification: shared raster view attachment callback
; RAM address  DOL offset  raw word  instruction
804981B8  00491A78  80A40018  lwz          r5, 0x18(r4)
804981BC  00491A7C  38000000  li           r0, 0
804981C0  00491A80  90A30018  stw          r5, 0x18(r3)
804981C4  00491A84  80A40014  lwz          r5, 0x14(r4)
804981C8  00491A88  90A30014  stw          r5, 0x14(r3)
804981CC  00491A8C  88A40020  lbz          r5, 0x20(r4)
804981D0  00491A90  98A30020  stb          r5, 0x20(r3)
804981D4  00491A94  88840023  lbz          r4, 0x23(r4)
804981D8  00491A98  98830023  stb          r4, 0x23(r3)
804981DC  00491A9C  90030004  stw          r0, 4(r3)
804981E0  00491AA0  38600001  li           r3, 1
804981E4  00491AA4  4E800020  blr
