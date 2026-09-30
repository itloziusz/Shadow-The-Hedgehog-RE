; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; MotionBlur_RampTaskUpdate: [800A4830, 800A492C), 0xfc bytes
; classification: shared input update; unrelated secondary effect calls excluded from reconstruction
; RAM address  DOL offset  raw word  instruction
800A4830  0009E0F0  9421FFE0  stwu         r1, -0x20(r1)
800A4834  0009E0F4  7C0802A6  mflr         r0
800A4838  0009E0F8  90010024  stw          r0, 0x24(r1)
800A483C  0009E0FC  93E1001C  stw          r31, 0x1c(r1)
800A4840  0009E100  7C7F1B78  mr           r31, r3
800A4844  0009E104  88030034  lbz          r0, 0x34(r3)
800A4848  0009E108  28000000  cmplwi       r0, 0
800A484C  0009E10C  41820030  beq          0x800a487c
800A4850  0009E110  C0029454  lfs          f0, -0x6bac(r2)
800A4854  0009E114  C05F0038  lfs          f2, 0x38(r31)
800A4858  0009E118  EC210024  fdivs        f1, f1, f0
800A485C  0009E11C  C002945C  lfs          f0, -0x6ba4(r2)
800A4860  0009E120  EC220828  fsubs        f1, f2, f1
800A4864  0009E124  D03F0038  stfs         f1, 0x38(r31)
800A4868  0009E128  C03F0038  lfs          f1, 0x38(r31)
800A486C  0009E12C  FC010040  fcmpo        cr0, f1, f0
800A4870  0009E130  40800034  bge          0x800a48a4
800A4874  0009E134  D01F0038  stfs         f0, 0x38(r31)
800A4878  0009E138  4800002C  b            0x800a48a4
800A487C  0009E13C  C0029454  lfs          f0, -0x6bac(r2)
800A4880  0009E140  C05F0038  lfs          f2, 0x38(r31)
800A4884  0009E144  EC210024  fdivs        f1, f1, f0
800A4888  0009E148  C002944C  lfs          f0, -0x6bb4(r2)
800A488C  0009E14C  EC22082A  fadds        f1, f2, f1
800A4890  0009E150  D03F0038  stfs         f1, 0x38(r31)
800A4894  0009E154  C03F0038  lfs          f1, 0x38(r31)
800A4898  0009E158  FC010040  fcmpo        cr0, f1, f0
800A489C  0009E15C  40810008  ble          0x800a48a4
800A48A0  0009E160  D01F0038  stfs         f0, 0x38(r31)
800A48A4  0009E164  C0229464  lfs          f1, -0x6b9c(r2)
800A48A8  0009E168  3C608058  lis          r3, -0x7fa8
800A48AC  0009E16C  C01F0038  lfs          f0, 0x38(r31)
800A48B0  0009E170  3883E760  addi         r4, r3, -0x18a0
800A48B4  0009E174  7FE3FB78  mr           r3, r31
800A48B8  0009E178  EC010032  fmuls        f0, f1, f0
800A48BC  0009E17C  D0040030  stfs         f0, 0x30(r4)
800A48C0  0009E180  D0040024  stfs         f0, 0x24(r4)
800A48C4  0009E184  4BFFFEED  bl           0x800a47b0
800A48C8  0009E188  4BFD5075  bl           0x8007993c
800A48CC  0009E18C  80BF0028  lwz          r5, 0x28(r31)
800A48D0  0009E190  38800000  li           r4, 0
800A48D4  0009E194  C0229468  lfs          f1, -0x6b98(r2)
800A48D8  0009E198  3863070C  addi         r3, r3, 0x70c
800A48DC  0009E19C  80A50060  lwz          r5, 0x60(r5)
800A48E0  0009E1A0  C0050000  lfs          f0, 0(r5)
800A48E4  0009E1A4  EC010032  fmuls        f0, f1, f0
800A48E8  0009E1A8  FC00001E  fctiwz       f0, f0
800A48EC  0009E1AC  D8010008  stfd         f0, 8(r1)
800A48F0  0009E1B0  80A1000C  lwz          r5, 0xc(r1)
800A48F4  0009E1B4  480CABBD  bl           0x8016f4b0
800A48F8  0009E1B8  C03F0038  lfs          f1, 0x38(r31)
800A48FC  0009E1BC  C002945C  lfs          f0, -0x6ba4(r2)
800A4900  0009E1C0  FC010040  fcmpo        cr0, f1, f0
800A4904  0009E1C4  4C401382  cror         cr0eq, cr0lt, cr0eq
800A4908  0009E1C8  40820010  bne          0x800a4918
800A490C  0009E1CC  A01F0004  lhz          r0, 4(r31)
800A4910  0009E1D0  60000001  ori          r0, r0, 1
800A4914  0009E1D4  B01F0004  sth          r0, 4(r31)
800A4918  0009E1D8  80010024  lwz          r0, 0x24(r1)
800A491C  0009E1DC  83E1001C  lwz          r31, 0x1c(r1)
800A4920  0009E1E0  7C0803A6  mtlr         r0
800A4924  0009E1E4  38210020  addi         r1, r1, 0x20
800A4928  0009E1E8  4E800020  blr
