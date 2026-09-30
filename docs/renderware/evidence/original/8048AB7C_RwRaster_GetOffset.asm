; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; RwRaster_GetOffset: [8048AB7C, 8048AB90), 0x14 bytes
; classification: shared raster offset dependency
; RAM address  DOL offset  raw word  instruction
8048AB7C  0048443C  A803001C  lha          r0, 0x1c(r3)
8048AB80  00484440  B0040000  sth          r0, 0(r4)
8048AB84  00484444  A803001E  lha          r0, 0x1e(r3)
8048AB88  00484448  B0050000  sth          r0, 0(r5)
8048AB8C  0048444C  4E800020  blr
