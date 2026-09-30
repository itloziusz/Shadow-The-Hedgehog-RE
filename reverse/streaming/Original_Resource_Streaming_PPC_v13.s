\n# ===== 0x800122a0 .. 0x80012920 =====

/mnt/data/text1_exec.o:	file format elf32-powerpc

Disassembly of section .data:

80008d40 <_binary__mnt_data_text1_bin_start>:
800122a0: 94 21 ff f0  	stwu 1, -16(1)
800122a4: 7c 08 02 a6  	mflr 0
800122a8: 90 01 00 14  	stw 0, 20(1)
800122ac: db e1 00 08  	stfd 31, 8(1)
800122b0: ff e0 08 90  	fmr 31, 1
800122b4: 48 00 00 21  	bl 0x800122d4 <_binary__mnt_data_text1_bin_start+0x9594>
800122b8: fc 20 f8 90  	fmr 1, 31
800122bc: 48 00 07 75  	bl 0x80012a30 <_binary__mnt_data_text1_bin_start+0x9cf0>
800122c0: 80 01 00 14  	lwz 0, 20(1)
800122c4: cb e1 00 08  	lfd 31, 8(1)
800122c8: 7c 08 03 a6  	mtlr 0
800122cc: 38 21 00 10  	addi 1, 1, 16
800122d0: 4e 80 00 20  	blr
800122d4: 94 21 ff f0  	stwu 1, -16(1)
800122d8: 7c 08 02 a6  	mflr 0
800122dc: 90 01 00 14  	stw 0, 20(1)
800122e0: 80 0d 2b 7c  	lwz 0, 11132(13)
800122e4: 28 00 00 00  	cmplwi	0, 0
800122e8: 40 82 00 0c  	bf	2, 0x800122f4 <_binary__mnt_data_text1_bin_start+0x95b4>
800122ec: 48 00 00 1d  	bl 0x80012308 <_binary__mnt_data_text1_bin_start+0x95c8>
800122f0: 90 6d 2b 7c  	stw 3, 11132(13)
800122f4: 80 01 00 14  	lwz 0, 20(1)
800122f8: 80 6d 2b 7c  	lwz 3, 11132(13)
800122fc: 7c 08 03 a6  	mtlr 0
80012300: 38 21 00 10  	addi 1, 1, 16
80012304: 4e 80 00 20  	blr
80012308: 94 21 ff f0  	stwu 1, -16(1)
8001230c: 7c 08 02 a6  	mflr 0
80012310: 90 01 00 14  	stw 0, 20(1)
80012314: 88 0d 2b 78  	lbz 0, 11128(13)
80012318: 7c 00 07 75  	extsb. 0, 0
8001231c: 40 82 00 2c  	bf	2, 0x80012348 <_binary__mnt_data_text1_bin_start+0x9608>
80012320: 3c 60 80 57  	lis 3, -32681
80012324: 38 63 ff 94  	addi 3, 3, -108
80012328: 48 00 09 69  	bl 0x80012c90 <_binary__mnt_data_text1_bin_start+0x9f50>
8001232c: 3c 80 80 01  	lis 4, -32767
80012330: 3c a0 80 57  	lis 5, -32681
80012334: 38 84 2b 58  	addi 4, 4, 11096
80012338: 38 a5 ff 88  	addi 5, 5, -120
8001233c: 48 38 ee d5  	bl 0x803a1210 <_binary__mnt_data_text1_bin_start+0x3984d0>
80012340: 38 00 00 01  	li 0, 1
80012344: 98 0d 2b 78  	stb 0, 11128(13)
80012348: 80 01 00 14  	lwz 0, 20(1)
8001234c: 3c 60 80 57  	lis 3, -32681
80012350: 38 63 ff 94  	addi 3, 3, -108
80012354: 7c 08 03 a6  	mtlr 0
80012358: 38 21 00 10  	addi 1, 1, 16
8001235c: 4e 80 00 20  	blr
80012360: 94 21 ff f0  	stwu 1, -16(1)
80012364: 7c 08 02 a6  	mflr 0
80012368: 90 01 00 14  	stw 0, 20(1)
8001236c: bf c1 00 08  	stmw 30, 8(1)
80012370: 7c 7e 1b 79  	mr.	30, 3
80012374: 7c 9f 23 78  	mr	31, 4
80012378: 41 82 00 28  	bt	2, 0x800123a0 <_binary__mnt_data_text1_bin_start+0x9660>
8001237c: 3c a0 80 52  	lis 5, -32686
80012380: 38 80 00 00  	li 4, 0
80012384: 38 05 d7 7c  	addi 0, 5, -10372
80012388: 90 1e 00 18  	stw 0, 24(30)
8001238c: 48 03 cb 8d  	bl 0x8004ef18 <_binary__mnt_data_text1_bin_start+0x461d8>
80012390: 7f e0 07 35  	extsh. 0, 31
80012394: 40 81 00 0c  	bf	1, 0x800123a0 <_binary__mnt_data_text1_bin_start+0x9660>
80012398: 7f c3 f3 78  	mr	3, 30
8001239c: 48 38 ef 99  	bl 0x803a1334 <_binary__mnt_data_text1_bin_start+0x3985f4>
800123a0: 7f c3 f3 78  	mr	3, 30
800123a4: bb c1 00 08  	lmw 30, 8(1)
800123a8: 80 01 00 14  	lwz 0, 20(1)
800123ac: 7c 08 03 a6  	mtlr 0
800123b0: 38 21 00 10  	addi 1, 1, 16
800123b4: 4e 80 00 20  	blr
800123b8: 94 21 ff f0  	stwu 1, -16(1)
800123bc: 7c 08 02 a6  	mflr 0
800123c0: 90 01 00 14  	stw 0, 20(1)
800123c4: 93 e1 00 0c  	stw 31, 12(1)
800123c8: 7c 7f 1b 78  	mr	31, 3
800123cc: 48 03 cc 49  	bl 0x8004f014 <_binary__mnt_data_text1_bin_start+0x462d4>
800123d0: 3c 80 80 52  	lis 4, -32686
800123d4: 7f e3 fb 78  	mr	3, 31
800123d8: 38 04 d7 7c  	addi 0, 4, -10372
800123dc: 90 1f 00 18  	stw 0, 24(31)
800123e0: 80 0d 80 a8  	lwz 0, -32600(13)
800123e4: 90 1f 00 00  	stw 0, 0(31)
800123e8: a0 1f 00 04  	lhz 0, 4(31)
800123ec: 60 00 01 00  	ori 0, 0, 256
800123f0: b0 1f 00 04  	sth 0, 4(31)
800123f4: 80 01 00 14  	lwz 0, 20(1)
800123f8: 83 e1 00 0c  	lwz 31, 12(1)
800123fc: 7c 08 03 a6  	mtlr 0
80012400: 38 21 00 10  	addi 1, 1, 16
80012404: 4e 80 00 20  	blr
80012408: 94 21 ff e0  	stwu 1, -32(1)
8001240c: 7c 08 02 a6  	mflr 0
80012410: 90 01 00 24  	stw 0, 36(1)
80012414: 38 a1 00 08  	addi 5, 1, 8
80012418: bf c1 00 18  	stmw 30, 24(1)
8001241c: 7c 7e 1b 78  	mr	30, 3
80012420: 38 61 00 10  	addi 3, 1, 16
80012424: 90 81 00 08  	stw 4, 8(1)
80012428: 7f c4 f3 78  	mr	4, 30
8001242c: 48 00 00 b9  	bl 0x800124e4 <_binary__mnt_data_text1_bin_start+0x97a4>
80012430: 83 e1 00 10  	lwz 31, 16(1)
80012434: 7f c4 f3 78  	mr	4, 30
80012438: 38 61 00 0c  	addi 3, 1, 12
8001243c: 48 00 00 41  	bl 0x8001247c <_binary__mnt_data_text1_bin_start+0x973c>
80012440: 80 01 00 0c  	lwz 0, 12(1)
80012444: 7c 1f 00 40  	cmplw	31, 0
80012448: 41 82 00 1c  	bt	2, 0x80012464 <_binary__mnt_data_text1_bin_start+0x9724>
8001244c: 80 7f 00 10  	lwz 3, 16(31)
80012450: 81 83 00 00  	lwz 12, 0(3)
80012454: 81 8c 00 14  	lwz 12, 20(12)
80012458: 7d 89 03 a6  	mtctr 12
8001245c: 4e 80 04 21  	bctrl
80012460: 48 00 00 08  	b 0x80012468 <_binary__mnt_data_text1_bin_start+0x9728>
80012464: 38 60 00 04  	li 3, 4
80012468: bb c1 00 18  	lmw 30, 24(1)
8001246c: 80 01 00 24  	lwz 0, 36(1)
80012470: 7c 08 03 a6  	mtlr 0
80012474: 38 21 00 20  	addi 1, 1, 32
80012478: 4e 80 00 20  	blr
8001247c: 94 21 ff f0  	stwu 1, -16(1)
80012480: 7c 08 02 a6  	mflr 0
80012484: 90 01 00 14  	stw 0, 20(1)
80012488: 93 e1 00 0c  	stw 31, 12(1)
8001248c: 7c 7f 1b 78  	mr	31, 3
80012490: 48 00 00 19  	bl 0x800124a8 <_binary__mnt_data_text1_bin_start+0x9768>
80012494: 80 01 00 14  	lwz 0, 20(1)
80012498: 83 e1 00 0c  	lwz 31, 12(1)
8001249c: 7c 08 03 a6  	mtlr 0
800124a0: 38 21 00 10  	addi 1, 1, 16
800124a4: 4e 80 00 20  	blr
800124a8: 94 21 ff f0  	stwu 1, -16(1)
800124ac: 7c 08 02 a6  	mflr 0
800124b0: 90 01 00 14  	stw 0, 20(1)
800124b4: 93 e1 00 0c  	stw 31, 12(1)
800124b8: 7c 7f 1b 78  	mr	31, 3
800124bc: 7c 83 23 78  	mr	3, 4
800124c0: 48 00 00 1d  	bl 0x800124dc <_binary__mnt_data_text1_bin_start+0x979c>
800124c4: 90 7f 00 00  	stw 3, 0(31)
800124c8: 80 01 00 14  	lwz 0, 20(1)
800124cc: 83 e1 00 0c  	lwz 31, 12(1)
800124d0: 7c 08 03 a6  	mtlr 0
800124d4: 38 21 00 10  	addi 1, 1, 16
800124d8: 4e 80 00 20  	blr
800124dc: 38 63 00 04  	addi 3, 3, 4
800124e0: 4e 80 00 20  	blr
800124e4: 94 21 ff f0  	stwu 1, -16(1)
800124e8: 7c 08 02 a6  	mflr 0
800124ec: 90 01 00 14  	stw 0, 20(1)
800124f0: 93 e1 00 0c  	stw 31, 12(1)
800124f4: 7c 7f 1b 78  	mr	31, 3
800124f8: 48 00 08 91  	bl 0x80012d88 <_binary__mnt_data_text1_bin_start+0xa048>
800124fc: 80 01 00 14  	lwz 0, 20(1)
80012500: 83 e1 00 0c  	lwz 31, 12(1)
80012504: 7c 08 03 a6  	mtlr 0
80012508: 38 21 00 10  	addi 1, 1, 16
8001250c: 4e 80 00 20  	blr
80012510: 94 21 ff d0  	stwu 1, -48(1)
80012514: 7c 08 02 a6  	mflr 0
80012518: 90 01 00 34  	stw 0, 52(1)
8001251c: bf a1 00 24  	stmw 29, 36(1)
80012520: 7c 7d 1b 78  	mr	29, 3
80012524: 7c be 2b 78  	mr	30, 5
80012528: 38 61 00 10  	addi 3, 1, 16
8001252c: 38 a1 00 08  	addi 5, 1, 8
80012530: 90 81 00 08  	stw 4, 8(1)
80012534: 7f a4 eb 78  	mr	4, 29
80012538: 4b ff ff ad  	bl 0x800124e4 <_binary__mnt_data_text1_bin_start+0x97a4>
8001253c: 83 e1 00 10  	lwz 31, 16(1)
80012540: 7f a4 eb 78  	mr	4, 29
80012544: 38 61 00 0c  	addi 3, 1, 12
80012548: 4b ff ff 35  	bl 0x8001247c <_binary__mnt_data_text1_bin_start+0x973c>
8001254c: 80 01 00 0c  	lwz 0, 12(1)
80012550: 7c 1f 00 40  	cmplw	31, 0
80012554: 41 82 00 20  	bt	2, 0x80012574 <_binary__mnt_data_text1_bin_start+0x9834>
80012558: 80 7f 00 10  	lwz 3, 16(31)
8001255c: 7f c4 f3 78  	mr	4, 30
80012560: 81 83 00 00  	lwz 12, 0(3)
80012564: 81 8c 00 30  	lwz 12, 48(12)
80012568: 7d 89 03 a6  	mtctr 12
8001256c: 4e 80 04 21  	bctrl
80012570: 48 00 00 08  	b 0x80012578 <_binary__mnt_data_text1_bin_start+0x9838>
80012574: 38 60 00 00  	li 3, 0
80012578: bb a1 00 24  	lmw 29, 36(1)
8001257c: 80 01 00 34  	lwz 0, 52(1)
80012580: 7c 08 03 a6  	mtlr 0
80012584: 38 21 00 30  	addi 1, 1, 48
80012588: 4e 80 00 20  	blr
8001258c: 94 21 ff d0  	stwu 1, -48(1)
80012590: 7c 08 02 a6  	mflr 0
80012594: 90 01 00 34  	stw 0, 52(1)
80012598: bf a1 00 24  	stmw 29, 36(1)
8001259c: 7c 7d 1b 78  	mr	29, 3
800125a0: 7c be 2b 78  	mr	30, 5
800125a4: 38 61 00 10  	addi 3, 1, 16
800125a8: 38 a1 00 08  	addi 5, 1, 8
800125ac: 90 81 00 08  	stw 4, 8(1)
800125b0: 7f a4 eb 78  	mr	4, 29
800125b4: 4b ff ff 31  	bl 0x800124e4 <_binary__mnt_data_text1_bin_start+0x97a4>
800125b8: 83 e1 00 10  	lwz 31, 16(1)
800125bc: 7f a4 eb 78  	mr	4, 29
800125c0: 38 61 00 0c  	addi 3, 1, 12
800125c4: 4b ff fe b9  	bl 0x8001247c <_binary__mnt_data_text1_bin_start+0x973c>
800125c8: 80 01 00 0c  	lwz 0, 12(1)
800125cc: 7c 1f 00 40  	cmplw	31, 0
800125d0: 41 82 00 20  	bt	2, 0x800125f0 <_binary__mnt_data_text1_bin_start+0x98b0>
800125d4: 80 7f 00 10  	lwz 3, 16(31)
800125d8: 7f c4 f3 78  	mr	4, 30
800125dc: 81 83 00 00  	lwz 12, 0(3)
800125e0: 81 8c 00 2c  	lwz 12, 44(12)
800125e4: 7d 89 03 a6  	mtctr 12
800125e8: 4e 80 04 21  	bctrl
800125ec: 48 00 00 08  	b 0x800125f4 <_binary__mnt_data_text1_bin_start+0x98b4>
800125f0: 38 60 00 00  	li 3, 0
800125f4: bb a1 00 24  	lmw 29, 36(1)
800125f8: 80 01 00 34  	lwz 0, 52(1)
800125fc: 7c 08 03 a6  	mtlr 0
80012600: 38 21 00 30  	addi 1, 1, 48
80012604: 4e 80 00 20  	blr
80012608: 94 21 ff d0  	stwu 1, -48(1)
8001260c: 7c 08 02 a6  	mflr 0
80012610: 90 01 00 34  	stw 0, 52(1)
80012614: bf a1 00 24  	stmw 29, 36(1)
80012618: 7c 7d 1b 78  	mr	29, 3
8001261c: 7c be 2b 78  	mr	30, 5
80012620: 38 61 00 10  	addi 3, 1, 16
80012624: 38 a1 00 08  	addi 5, 1, 8
80012628: 90 81 00 08  	stw 4, 8(1)
8001262c: 7f a4 eb 78  	mr	4, 29
80012630: 4b ff fe b5  	bl 0x800124e4 <_binary__mnt_data_text1_bin_start+0x97a4>
80012634: 83 e1 00 10  	lwz 31, 16(1)
80012638: 7f a4 eb 78  	mr	4, 29
8001263c: 38 61 00 0c  	addi 3, 1, 12
80012640: 4b ff fe 3d  	bl 0x8001247c <_binary__mnt_data_text1_bin_start+0x973c>
80012644: 80 01 00 0c  	lwz 0, 12(1)
80012648: 7c 1f 00 40  	cmplw	31, 0
8001264c: 41 82 00 20  	bt	2, 0x8001266c <_binary__mnt_data_text1_bin_start+0x992c>
80012650: 80 7f 00 10  	lwz 3, 16(31)
80012654: 7f c4 f3 78  	mr	4, 30
80012658: 81 83 00 00  	lwz 12, 0(3)
8001265c: 81 8c 00 28  	lwz 12, 40(12)
80012660: 7d 89 03 a6  	mtctr 12
80012664: 4e 80 04 21  	bctrl
80012668: 48 00 00 08  	b 0x80012670 <_binary__mnt_data_text1_bin_start+0x9930>
8001266c: 38 60 00 00  	li 3, 0
80012670: bb a1 00 24  	lmw 29, 36(1)
80012674: 80 01 00 34  	lwz 0, 52(1)
80012678: 7c 08 03 a6  	mtlr 0
8001267c: 38 21 00 30  	addi 1, 1, 48
80012680: 4e 80 00 20  	blr
80012684: 94 21 ff d0  	stwu 1, -48(1)
80012688: 7c 08 02 a6  	mflr 0
8001268c: 90 01 00 34  	stw 0, 52(1)
80012690: bf a1 00 24  	stmw 29, 36(1)
80012694: 7c 7d 1b 78  	mr	29, 3
80012698: 7c be 2b 78  	mr	30, 5
8001269c: 38 61 00 10  	addi 3, 1, 16
800126a0: 38 a1 00 08  	addi 5, 1, 8
800126a4: 90 81 00 08  	stw 4, 8(1)
800126a8: 7f a4 eb 78  	mr	4, 29
800126ac: 4b ff fe 39  	bl 0x800124e4 <_binary__mnt_data_text1_bin_start+0x97a4>
800126b0: 83 e1 00 10  	lwz 31, 16(1)
800126b4: 7f a4 eb 78  	mr	4, 29
800126b8: 38 61 00 0c  	addi 3, 1, 12
800126bc: 4b ff fd c1  	bl 0x8001247c <_binary__mnt_data_text1_bin_start+0x973c>
800126c0: 80 01 00 0c  	lwz 0, 12(1)
800126c4: 7c 1f 00 40  	cmplw	31, 0
800126c8: 41 82 00 20  	bt	2, 0x800126e8 <_binary__mnt_data_text1_bin_start+0x99a8>
800126cc: 80 7f 00 10  	lwz 3, 16(31)
800126d0: 7f c4 f3 78  	mr	4, 30
800126d4: 81 83 00 00  	lwz 12, 0(3)
800126d8: 81 8c 00 20  	lwz 12, 32(12)
800126dc: 7d 89 03 a6  	mtctr 12
800126e0: 4e 80 04 21  	bctrl
800126e4: 48 00 00 08  	b 0x800126ec <_binary__mnt_data_text1_bin_start+0x99ac>
800126e8: 38 60 00 00  	li 3, 0
800126ec: bb a1 00 24  	lmw 29, 36(1)
800126f0: 80 01 00 34  	lwz 0, 52(1)
800126f4: 7c 08 03 a6  	mtlr 0
800126f8: 38 21 00 30  	addi 1, 1, 48
800126fc: 4e 80 00 20  	blr
80012700: 94 21 ff d0  	stwu 1, -48(1)
80012704: 7c 08 02 a6  	mflr 0
80012708: 90 01 00 34  	stw 0, 52(1)
8001270c: bf a1 00 24  	stmw 29, 36(1)
80012710: 7c 7d 1b 78  	mr	29, 3
80012714: 7c be 2b 78  	mr	30, 5
80012718: 38 61 00 10  	addi 3, 1, 16
8001271c: 38 a1 00 08  	addi 5, 1, 8
80012720: 90 81 00 08  	stw 4, 8(1)
80012724: 7f a4 eb 78  	mr	4, 29
80012728: 4b ff fd bd  	bl 0x800124e4 <_binary__mnt_data_text1_bin_start+0x97a4>
8001272c: 83 e1 00 10  	lwz 31, 16(1)
80012730: 7f a4 eb 78  	mr	4, 29
80012734: 38 61 00 0c  	addi 3, 1, 12
80012738: 4b ff fd 45  	bl 0x8001247c <_binary__mnt_data_text1_bin_start+0x973c>
8001273c: 80 01 00 0c  	lwz 0, 12(1)
80012740: 7c 1f 00 40  	cmplw	31, 0
80012744: 41 82 00 20  	bt	2, 0x80012764 <_binary__mnt_data_text1_bin_start+0x9a24>
80012748: 80 7f 00 10  	lwz 3, 16(31)
8001274c: 7f c4 f3 78  	mr	4, 30
80012750: 81 83 00 00  	lwz 12, 0(3)
80012754: 81 8c 00 1c  	lwz 12, 28(12)
80012758: 7d 89 03 a6  	mtctr 12
8001275c: 4e 80 04 21  	bctrl
80012760: 48 00 00 08  	b 0x80012768 <_binary__mnt_data_text1_bin_start+0x9a28>
80012764: 38 60 00 00  	li 3, 0
80012768: bb a1 00 24  	lmw 29, 36(1)
8001276c: 80 01 00 34  	lwz 0, 52(1)
80012770: 7c 08 03 a6  	mtlr 0
80012774: 38 21 00 30  	addi 1, 1, 48
80012778: 4e 80 00 20  	blr
8001277c: 94 21 ff d0  	stwu 1, -48(1)
80012780: 7c 08 02 a6  	mflr 0
80012784: 90 01 00 34  	stw 0, 52(1)
80012788: bf a1 00 24  	stmw 29, 36(1)
8001278c: 7c 7d 1b 78  	mr	29, 3
80012790: 7c be 2b 78  	mr	30, 5
80012794: 38 61 00 10  	addi 3, 1, 16
80012798: 38 a1 00 08  	addi 5, 1, 8
8001279c: 90 81 00 08  	stw 4, 8(1)
800127a0: 7f a4 eb 78  	mr	4, 29
800127a4: 4b ff fd 41  	bl 0x800124e4 <_binary__mnt_data_text1_bin_start+0x97a4>
800127a8: 83 e1 00 10  	lwz 31, 16(1)
800127ac: 7f a4 eb 78  	mr	4, 29
800127b0: 38 61 00 0c  	addi 3, 1, 12
800127b4: 4b ff fc c9  	bl 0x8001247c <_binary__mnt_data_text1_bin_start+0x973c>
800127b8: 80 01 00 0c  	lwz 0, 12(1)
800127bc: 7c 1f 00 40  	cmplw	31, 0
800127c0: 41 82 00 20  	bt	2, 0x800127e0 <_binary__mnt_data_text1_bin_start+0x9aa0>
800127c4: 80 7f 00 10  	lwz 3, 16(31)
800127c8: 7f c4 f3 78  	mr	4, 30
800127cc: 81 83 00 00  	lwz 12, 0(3)
800127d0: 81 8c 00 18  	lwz 12, 24(12)
800127d4: 7d 89 03 a6  	mtctr 12
800127d8: 4e 80 04 21  	bctrl
800127dc: 48 00 00 08  	b 0x800127e4 <_binary__mnt_data_text1_bin_start+0x9aa4>
800127e0: 38 60 00 00  	li 3, 0
800127e4: bb a1 00 24  	lmw 29, 36(1)
800127e8: 80 01 00 34  	lwz 0, 52(1)
800127ec: 7c 08 03 a6  	mtlr 0
800127f0: 38 21 00 30  	addi 1, 1, 48
800127f4: 4e 80 00 20  	blr
800127f8: 94 21 ff e0  	stwu 1, -32(1)
800127fc: 7c 08 02 a6  	mflr 0
80012800: 90 01 00 24  	stw 0, 36(1)
80012804: 38 a1 00 08  	addi 5, 1, 8
80012808: bf c1 00 18  	stmw 30, 24(1)
8001280c: 7c 7e 1b 78  	mr	30, 3
80012810: 38 61 00 10  	addi 3, 1, 16
80012814: 90 81 00 08  	stw 4, 8(1)
80012818: 7f c4 f3 78  	mr	4, 30
8001281c: 4b ff fc c9  	bl 0x800124e4 <_binary__mnt_data_text1_bin_start+0x97a4>
80012820: 83 e1 00 10  	lwz 31, 16(1)
80012824: 7f c4 f3 78  	mr	4, 30
80012828: 38 61 00 0c  	addi 3, 1, 12
8001282c: 4b ff fc 51  	bl 0x8001247c <_binary__mnt_data_text1_bin_start+0x973c>
80012830: 80 01 00 0c  	lwz 0, 12(1)
80012834: 7c 1f 00 40  	cmplw	31, 0
80012838: 41 82 00 44  	bt	2, 0x8001287c <_binary__mnt_data_text1_bin_start+0x9b3c>
8001283c: 80 7f 00 10  	lwz 3, 16(31)
80012840: 81 83 00 00  	lwz 12, 0(3)
80012844: 81 8c 00 10  	lwz 12, 16(12)
80012848: 7d 89 03 a6  	mtctr 12
8001284c: 4e 80 04 21  	bctrl
80012850: 80 7f 00 10  	lwz 3, 16(31)
80012854: 28 03 00 00  	cmplwi	3, 0
80012858: 41 82 00 18  	bt	2, 0x80012870 <_binary__mnt_data_text1_bin_start+0x9b30>
8001285c: 81 83 00 00  	lwz 12, 0(3)
80012860: 38 80 00 01  	li 4, 1
80012864: 81 8c 00 08  	lwz 12, 8(12)
80012868: 7d 89 03 a6  	mtctr 12
8001286c: 4e 80 04 21  	bctrl
80012870: 7f c3 f3 78  	mr	3, 30
80012874: 38 81 00 08  	addi 4, 1, 8
80012878: 48 00 04 95  	bl 0x80012d0c <_binary__mnt_data_text1_bin_start+0x9fcc>
8001287c: bb c1 00 18  	lmw 30, 24(1)
80012880: 80 01 00 24  	lwz 0, 36(1)
80012884: 7c 08 03 a6  	mtlr 0
80012888: 38 21 00 20  	addi 1, 1, 32
8001288c: 4e 80 00 20  	blr
80012890: 94 21 ff f0  	stwu 1, -16(1)
80012894: 7c 08 02 a6  	mflr 0
80012898: 90 01 00 14  	stw 0, 20(1)
8001289c: 93 e1 00 0c  	stw 31, 12(1)
800128a0: 7c 7f 1b 79  	mr.	31, 3
800128a4: 41 82 00 1c  	bt	2, 0x800128c0 <_binary__mnt_data_text1_bin_start+0x9b80>
800128a8: 3c a0 80 52  	lis 5, -32686
800128ac: 7c 80 07 35  	extsh. 0, 4
800128b0: 38 05 d7 8c  	addi 0, 5, -10356
800128b4: 90 1f 00 00  	stw 0, 0(31)
800128b8: 40 81 00 08  	bf	1, 0x800128c0 <_binary__mnt_data_text1_bin_start+0x9b80>
800128bc: 48 38 ea 79  	bl 0x803a1334 <_binary__mnt_data_text1_bin_start+0x3985f4>
800128c0: 80 01 00 14  	lwz 0, 20(1)
800128c4: 7f e3 fb 78  	mr	3, 31
800128c8: 83 e1 00 0c  	lwz 31, 12(1)
800128cc: 7c 08 03 a6  	mtlr 0
800128d0: 38 21 00 10  	addi 1, 1, 16
800128d4: 4e 80 00 20  	blr
800128d8: 94 21 ff c0  	stwu 1, -64(1)
800128dc: 7c 08 02 a6  	mflr 0
800128e0: 90 01 00 44  	stw 0, 68(1)
800128e4: bf 81 00 30  	stmw 28, 48(1)
800128e8: 7c 7c 1b 78  	mr	28, 3
800128ec: 3b a0 00 00  	li 29, 0
800128f0: 90 81 00 08  	stw 4, 8(1)
800128f4: 83 c3 00 14  	lwz 30, 20(3)
800128f8: 7f 84 e3 78  	mr	4, 28
800128fc: 38 61 00 10  	addi 3, 1, 16
80012900: 4b ff fb 7d  	bl 0x8001247c <_binary__mnt_data_text1_bin_start+0x973c>
80012904: 7f fe ea 14  	add 31, 30, 29
80012908: 7f 84 e3 78  	mr	4, 28
8001290c: 93 e1 00 14  	stw 31, 20(1)
80012910: 38 61 00 18  	addi 3, 1, 24
80012914: 38 a1 00 14  	addi 5, 1, 20
80012918: 4b ff fb cd  	bl 0x800124e4 <_binary__mnt_data_text1_bin_start+0x97a4>
8001291c: 80 61 00 18  	lwz 3, 24(1)
\n# ===== 0x800145a4 .. 0x80014c40 =====

/mnt/data/text1_exec.o:	file format elf32-powerpc

Disassembly of section .data:

80008d40 <_binary__mnt_data_text1_bin_start>:
800145a4: 94 21 ff e0  	stwu 1, -32(1)
800145a8: 7c 08 02 a6  	mflr 0
800145ac: 90 01 00 24  	stw 0, 36(1)
800145b0: bf 81 00 10  	stmw 28, 16(1)
800145b4: 7c 7c 1b 78  	mr	28, 3
800145b8: 7c 9d 23 78  	mr	29, 4
800145bc: 80 63 00 18  	lwz 3, 24(3)
800145c0: 3b c3 ff ff  	addi 30, 3, -1
800145c4: 1f fe 00 3c  	mulli 31, 30, 60
800145c8: 48 00 00 34  	b 0x800145fc <_binary__mnt_data_text1_bin_start+0xb8bc>
800145cc: 80 1c 00 14  	lwz 0, 20(28)
800145d0: 7f a4 eb 78  	mr	4, 29
800145d4: 7c 60 fa 14  	add 3, 0, 31
800145d8: 48 39 93 95  	bl 0x803ad96c <_binary__mnt_data_text1_bin_start+0x3a4c2c>
800145dc: 2c 03 00 00  	cmpwi	3, 0
800145e0: 40 82 00 14  	bf	2, 0x800145f4 <_binary__mnt_data_text1_bin_start+0xb8b4>
800145e4: 80 1c 00 14  	lwz 0, 20(28)
800145e8: 7c 60 fa 14  	add 3, 0, 31
800145ec: 80 63 00 38  	lwz 3, 56(3)
800145f0: 48 00 00 18  	b 0x80014608 <_binary__mnt_data_text1_bin_start+0xb8c8>
800145f4: 3b de ff ff  	addi 30, 30, -1
800145f8: 3b ff ff c4  	addi 31, 31, -60
800145fc: 2c 1e 00 00  	cmpwi	30, 0
80014600: 40 80 ff cc  	bf	0, 0x800145cc <_binary__mnt_data_text1_bin_start+0xb88c>
80014604: 38 60 00 00  	li 3, 0
80014608: bb 81 00 10  	lmw 28, 16(1)
8001460c: 80 01 00 24  	lwz 0, 36(1)
80014610: 7c 08 03 a6  	mtlr 0
80014614: 38 21 00 20  	addi 1, 1, 32
80014618: 4e 80 00 20  	blr
8001461c: 80 63 00 14  	lwz 3, 20(3)
80014620: 28 03 00 00  	cmplwi	3, 0
80014624: 41 82 00 14  	bt	2, 0x80014638 <_binary__mnt_data_text1_bin_start+0xb8f8>
80014628: 1c 04 00 3c  	mulli 0, 4, 60
8001462c: 7c 63 02 14  	add 3, 3, 0
80014630: 80 63 ff c0  	lwz 3, -64(3)
80014634: 4e 80 00 20  	blr
80014638: 38 60 00 00  	li 3, 0
8001463c: 4e 80 00 20  	blr
80014640: 94 21 ff e0  	stwu 1, -32(1)
80014644: 7c 08 02 a6  	mflr 0
80014648: 90 01 00 24  	stw 0, 36(1)
8001464c: bf 81 00 10  	stmw 28, 16(1)
80014650: 7c 7c 1b 78  	mr	28, 3
80014654: 7c 9d 23 78  	mr	29, 4
80014658: 80 63 00 18  	lwz 3, 24(3)
8001465c: 3b c3 ff ff  	addi 30, 3, -1
80014660: 1f fe 00 3c  	mulli 31, 30, 60
80014664: 48 00 00 44  	b 0x800146a8 <_binary__mnt_data_text1_bin_start+0xb968>
80014668: 80 1c 00 14  	lwz 0, 20(28)
8001466c: 7f a4 eb 78  	mr	4, 29
80014670: 7c 60 fa 14  	add 3, 0, 31
80014674: 48 39 92 f9  	bl 0x803ad96c <_binary__mnt_data_text1_bin_start+0x3a4c2c>
80014678: 2c 03 00 00  	cmpwi	3, 0
8001467c: 40 82 00 24  	bf	2, 0x800146a0 <_binary__mnt_data_text1_bin_start+0xb960>
80014680: 80 7c 00 14  	lwz 3, 20(28)
80014684: 38 1f 00 20  	addi 0, 31, 32
80014688: 7c 63 00 2e  	lwzx 3, 3, 0
8001468c: 28 03 00 00  	cmplwi	3, 0
80014690: 41 82 00 10  	bt	2, 0x800146a0 <_binary__mnt_data_text1_bin_start+0xb960>
80014694: 38 80 00 02  	li 4, 2
80014698: 4b ff d6 51  	bl 0x80011ce8 <_binary__mnt_data_text1_bin_start+0x8fa8>
8001469c: 48 00 00 18  	b 0x800146b4 <_binary__mnt_data_text1_bin_start+0xb974>
800146a0: 3b de ff ff  	addi 30, 30, -1
800146a4: 3b ff ff c4  	addi 31, 31, -60
800146a8: 2c 1e 00 00  	cmpwi	30, 0
800146ac: 40 80 ff bc  	bf	0, 0x80014668 <_binary__mnt_data_text1_bin_start+0xb928>
800146b0: 38 60 00 00  	li 3, 0
800146b4: bb 81 00 10  	lmw 28, 16(1)
800146b8: 80 01 00 24  	lwz 0, 36(1)
800146bc: 7c 08 03 a6  	mtlr 0
800146c0: 38 21 00 20  	addi 1, 1, 32
800146c4: 4e 80 00 20  	blr
800146c8: 94 21 ff f0  	stwu 1, -16(1)
800146cc: 7c 08 02 a6  	mflr 0
800146d0: 90 01 00 14  	stw 0, 20(1)
800146d4: 38 04 ff fe  	addi 0, 4, -2
800146d8: 80 63 00 14  	lwz 3, 20(3)
800146dc: 28 03 00 00  	cmplwi	3, 0
800146e0: 41 82 00 24  	bt	2, 0x80014704 <_binary__mnt_data_text1_bin_start+0xb9c4>
800146e4: 1c 00 00 3c  	mulli 0, 0, 60
800146e8: 7c 63 02 14  	add 3, 3, 0
800146ec: 80 63 00 20  	lwz 3, 32(3)
800146f0: 28 03 00 00  	cmplwi	3, 0
800146f4: 41 82 00 10  	bt	2, 0x80014704 <_binary__mnt_data_text1_bin_start+0xb9c4>
800146f8: 38 80 00 02  	li 4, 2
800146fc: 4b ff d5 ed  	bl 0x80011ce8 <_binary__mnt_data_text1_bin_start+0x8fa8>
80014700: 48 00 00 08  	b 0x80014708 <_binary__mnt_data_text1_bin_start+0xb9c8>
80014704: 38 60 00 00  	li 3, 0
80014708: 80 01 00 14  	lwz 0, 20(1)
8001470c: 7c 08 03 a6  	mtlr 0
80014710: 38 21 00 10  	addi 1, 1, 16
80014714: 4e 80 00 20  	blr
80014718: 94 21 ff e0  	stwu 1, -32(1)
8001471c: 7c 08 02 a6  	mflr 0
80014720: 90 01 00 24  	stw 0, 36(1)
80014724: bf 81 00 10  	stmw 28, 16(1)
80014728: 7c 7c 1b 78  	mr	28, 3
8001472c: 7c 9d 23 78  	mr	29, 4
80014730: 80 63 00 18  	lwz 3, 24(3)
80014734: 3b c3 ff ff  	addi 30, 3, -1
80014738: 1f fe 00 3c  	mulli 31, 30, 60
8001473c: 48 00 00 34  	b 0x80014770 <_binary__mnt_data_text1_bin_start+0xba30>
80014740: 80 1c 00 14  	lwz 0, 20(28)
80014744: 7f a4 eb 78  	mr	4, 29
80014748: 7c 60 fa 14  	add 3, 0, 31
8001474c: 48 39 92 21  	bl 0x803ad96c <_binary__mnt_data_text1_bin_start+0x3a4c2c>
80014750: 2c 03 00 00  	cmpwi	3, 0
80014754: 40 82 00 14  	bf	2, 0x80014768 <_binary__mnt_data_text1_bin_start+0xba28>
80014758: 80 1c 00 14  	lwz 0, 20(28)
8001475c: 7c 60 fa 14  	add 3, 0, 31
80014760: 80 63 00 34  	lwz 3, 52(3)
80014764: 48 00 00 18  	b 0x8001477c <_binary__mnt_data_text1_bin_start+0xba3c>
80014768: 3b de ff ff  	addi 30, 30, -1
8001476c: 3b ff ff c4  	addi 31, 31, -60
80014770: 2c 1e 00 00  	cmpwi	30, 0
80014774: 40 80 ff cc  	bf	0, 0x80014740 <_binary__mnt_data_text1_bin_start+0xba00>
80014778: 38 60 00 00  	li 3, 0
8001477c: bb 81 00 10  	lmw 28, 16(1)
80014780: 80 01 00 24  	lwz 0, 36(1)
80014784: 7c 08 03 a6  	mtlr 0
80014788: 38 21 00 20  	addi 1, 1, 32
8001478c: 4e 80 00 20  	blr
80014790: 80 03 00 14  	lwz 0, 20(3)
80014794: 38 64 ff fe  	addi 3, 4, -2
80014798: 28 00 00 00  	cmplwi	0, 0
8001479c: 41 82 00 20  	bt	2, 0x800147bc <_binary__mnt_data_text1_bin_start+0xba7c>
800147a0: 1c 63 00 3c  	mulli 3, 3, 60
800147a4: 7c 60 1a 14  	add 3, 0, 3
800147a8: 80 03 00 2c  	lwz 0, 44(3)
800147ac: 28 00 00 02  	cmplwi	0, 2
800147b0: 40 82 00 0c  	bf	2, 0x800147bc <_binary__mnt_data_text1_bin_start+0xba7c>
800147b4: 80 63 00 34  	lwz 3, 52(3)
800147b8: 4e 80 00 20  	blr
800147bc: 38 60 00 00  	li 3, 0
800147c0: 4e 80 00 20  	blr
800147c4: 94 21 ff e0  	stwu 1, -32(1)
800147c8: 7c 08 02 a6  	mflr 0
800147cc: 90 01 00 24  	stw 0, 36(1)
800147d0: bf 81 00 10  	stmw 28, 16(1)
800147d4: 7c 7c 1b 78  	mr	28, 3
800147d8: 7c 9d 23 78  	mr	29, 4
800147dc: 80 63 00 18  	lwz 3, 24(3)
800147e0: 3b c3 ff ff  	addi 30, 3, -1
800147e4: 1f fe 00 3c  	mulli 31, 30, 60
800147e8: 48 00 00 34  	b 0x8001481c <_binary__mnt_data_text1_bin_start+0xbadc>
800147ec: 80 1c 00 14  	lwz 0, 20(28)
800147f0: 7f a4 eb 78  	mr	4, 29
800147f4: 7c 60 fa 14  	add 3, 0, 31
800147f8: 48 39 91 75  	bl 0x803ad96c <_binary__mnt_data_text1_bin_start+0x3a4c2c>
800147fc: 2c 03 00 00  	cmpwi	3, 0
80014800: 40 82 00 14  	bf	2, 0x80014814 <_binary__mnt_data_text1_bin_start+0xbad4>
80014804: 80 1c 00 14  	lwz 0, 20(28)
80014808: 7c 60 fa 14  	add 3, 0, 31
8001480c: 80 63 00 30  	lwz 3, 48(3)
80014810: 48 00 00 18  	b 0x80014828 <_binary__mnt_data_text1_bin_start+0xbae8>
80014814: 3b de ff ff  	addi 30, 30, -1
80014818: 3b ff ff c4  	addi 31, 31, -60
8001481c: 2c 1e 00 00  	cmpwi	30, 0
80014820: 40 80 ff cc  	bf	0, 0x800147ec <_binary__mnt_data_text1_bin_start+0xbaac>
80014824: 38 60 00 00  	li 3, 0
80014828: bb 81 00 10  	lmw 28, 16(1)
8001482c: 80 01 00 24  	lwz 0, 36(1)
80014830: 7c 08 03 a6  	mtlr 0
80014834: 38 21 00 20  	addi 1, 1, 32
80014838: 4e 80 00 20  	blr
8001483c: 80 03 00 14  	lwz 0, 20(3)
80014840: 38 64 ff fe  	addi 3, 4, -2
80014844: 28 00 00 00  	cmplwi	0, 0
80014848: 41 82 00 20  	bt	2, 0x80014868 <_binary__mnt_data_text1_bin_start+0xbb28>
8001484c: 1c 63 00 3c  	mulli 3, 3, 60
80014850: 7c 60 1a 14  	add 3, 0, 3
80014854: 80 03 00 2c  	lwz 0, 44(3)
80014858: 28 00 00 01  	cmplwi	0, 1
8001485c: 40 82 00 0c  	bf	2, 0x80014868 <_binary__mnt_data_text1_bin_start+0xbb28>
80014860: 80 63 00 30  	lwz 3, 48(3)
80014864: 4e 80 00 20  	blr
80014868: 38 60 00 00  	li 3, 0
8001486c: 4e 80 00 20  	blr
80014870: 94 21 ff e0  	stwu 1, -32(1)
80014874: 7c 08 02 a6  	mflr 0
80014878: 90 01 00 24  	stw 0, 36(1)
8001487c: bf 81 00 10  	stmw 28, 16(1)
80014880: 7c 7c 1b 78  	mr	28, 3
80014884: 7c 9d 23 78  	mr	29, 4
80014888: 80 63 00 18  	lwz 3, 24(3)
8001488c: 3b c3 ff ff  	addi 30, 3, -1
80014890: 1f fe 00 3c  	mulli 31, 30, 60
80014894: 48 00 00 2c  	b 0x800148c0 <_binary__mnt_data_text1_bin_start+0xbb80>
80014898: 80 1c 00 14  	lwz 0, 20(28)
8001489c: 7f a4 eb 78  	mr	4, 29
800148a0: 7c 60 fa 14  	add 3, 0, 31
800148a4: 48 39 90 c9  	bl 0x803ad96c <_binary__mnt_data_text1_bin_start+0x3a4c2c>
800148a8: 2c 03 00 00  	cmpwi	3, 0
800148ac: 40 82 00 0c  	bf	2, 0x800148b8 <_binary__mnt_data_text1_bin_start+0xbb78>
800148b0: 38 7e 00 02  	addi 3, 30, 2
800148b4: 48 00 00 18  	b 0x800148cc <_binary__mnt_data_text1_bin_start+0xbb8c>
800148b8: 3b de ff ff  	addi 30, 30, -1
800148bc: 3b ff ff c4  	addi 31, 31, -60
800148c0: 2c 1e 00 00  	cmpwi	30, 0
800148c4: 40 80 ff d4  	bf	0, 0x80014898 <_binary__mnt_data_text1_bin_start+0xbb58>
800148c8: 38 60 ff ff  	li 3, -1
800148cc: bb 81 00 10  	lmw 28, 16(1)
800148d0: 80 01 00 24  	lwz 0, 36(1)
800148d4: 7c 08 03 a6  	mtlr 0
800148d8: 38 21 00 20  	addi 1, 1, 32
800148dc: 4e 80 00 20  	blr
800148e0: 94 21 ff e0  	stwu 1, -32(1)
800148e4: 7c 08 02 a6  	mflr 0
800148e8: 90 01 00 24  	stw 0, 36(1)
800148ec: bf 81 00 10  	stmw 28, 16(1)
800148f0: 7c 7c 1b 78  	mr	28, 3
800148f4: 7c 9d 23 78  	mr	29, 4
800148f8: 80 63 00 18  	lwz 3, 24(3)
800148fc: 3b c3 ff ff  	addi 30, 3, -1
80014900: 1f fe 00 3c  	mulli 31, 30, 60
80014904: 48 00 00 5c  	b 0x80014960 <_binary__mnt_data_text1_bin_start+0xbc20>
80014908: 80 1c 00 14  	lwz 0, 20(28)
8001490c: 7f a4 eb 78  	mr	4, 29
80014910: 7c 60 fa 14  	add 3, 0, 31
80014914: 48 39 90 59  	bl 0x803ad96c <_binary__mnt_data_text1_bin_start+0x3a4c2c>
80014918: 2c 03 00 00  	cmpwi	3, 0
8001491c: 40 82 00 3c  	bf	2, 0x80014958 <_binary__mnt_data_text1_bin_start+0xbc18>
80014920: 80 1c 00 14  	lwz 0, 20(28)
80014924: 7c 60 fa 14  	add 3, 0, 31
80014928: 80 03 00 2c  	lwz 0, 44(3)
8001492c: 28 00 00 06  	cmplwi	0, 6
80014930: 40 82 00 10  	bf	2, 0x80014940 <_binary__mnt_data_text1_bin_start+0xbc00>
80014934: 80 63 00 28  	lwz 3, 40(3)
80014938: 80 63 00 14  	lwz 3, 20(3)
8001493c: 48 00 00 30  	b 0x8001496c <_binary__mnt_data_text1_bin_start+0xbc2c>
80014940: 28 00 00 08  	cmplwi	0, 8
80014944: 40 82 00 0c  	bf	2, 0x80014950 <_binary__mnt_data_text1_bin_start+0xbc10>
80014948: 80 63 00 24  	lwz 3, 36(3)
8001494c: 48 00 00 20  	b 0x8001496c <_binary__mnt_data_text1_bin_start+0xbc2c>
80014950: 80 63 00 20  	lwz 3, 32(3)
80014954: 48 00 00 18  	b 0x8001496c <_binary__mnt_data_text1_bin_start+0xbc2c>
80014958: 3b de ff ff  	addi 30, 30, -1
8001495c: 3b ff ff c4  	addi 31, 31, -60
80014960: 2c 1e 00 00  	cmpwi	30, 0
80014964: 40 80 ff a4  	bf	0, 0x80014908 <_binary__mnt_data_text1_bin_start+0xbbc8>
80014968: 38 60 00 00  	li 3, 0
8001496c: bb 81 00 10  	lmw 28, 16(1)
80014970: 80 01 00 24  	lwz 0, 36(1)
80014974: 7c 08 03 a6  	mtlr 0
80014978: 38 21 00 20  	addi 1, 1, 32
8001497c: 4e 80 00 20  	blr
80014980: 80 03 00 14  	lwz 0, 20(3)
80014984: 28 00 00 00  	cmplwi	0, 0
80014988: 41 82 00 3c  	bt	2, 0x800149c4 <_binary__mnt_data_text1_bin_start+0xbc84>
8001498c: 1c 64 00 3c  	mulli 3, 4, 60
80014990: 7c 60 1a 14  	add 3, 0, 3
80014994: 80 03 ff b4  	lwz 0, -76(3)
80014998: 28 00 00 06  	cmplwi	0, 6
8001499c: 40 82 00 10  	bf	2, 0x800149ac <_binary__mnt_data_text1_bin_start+0xbc6c>
800149a0: 80 63 ff b0  	lwz 3, -80(3)
800149a4: 80 63 00 14  	lwz 3, 20(3)
800149a8: 4e 80 00 20  	blr
800149ac: 28 00 00 08  	cmplwi	0, 8
800149b0: 40 82 00 0c  	bf	2, 0x800149bc <_binary__mnt_data_text1_bin_start+0xbc7c>
800149b4: 80 63 ff ac  	lwz 3, -84(3)
800149b8: 4e 80 00 20  	blr
800149bc: 80 63 ff a8  	lwz 3, -88(3)
800149c0: 4e 80 00 20  	blr
800149c4: 38 60 00 00  	li 3, 0
800149c8: 4e 80 00 20  	blr
800149cc: 80 63 00 04  	lwz 3, 4(3)
800149d0: 4e 80 00 20  	blr
800149d4: 94 21 ff f0  	stwu 1, -16(1)
800149d8: 7c 08 02 a6  	mflr 0
800149dc: 90 01 00 14  	stw 0, 20(1)
800149e0: 4b ff ef e1  	bl 0x800139c0 <_binary__mnt_data_text1_bin_start+0xac80>
800149e4: 80 01 00 14  	lwz 0, 20(1)
800149e8: 7c 08 03 a6  	mtlr 0
800149ec: 38 21 00 10  	addi 1, 1, 16
800149f0: 4e 80 00 20  	blr
800149f4: 94 21 ff f0  	stwu 1, -16(1)
800149f8: 7c 08 02 a6  	mflr 0
800149fc: 90 01 00 14  	stw 0, 20(1)
80014a00: bf c1 00 08  	stmw 30, 8(1)
80014a04: 7c 7f 1b 78  	mr	31, 3
80014a08: 80 03 00 04  	lwz 0, 4(3)
80014a0c: 2c 00 00 02  	cmpwi	0, 2
80014a10: 41 82 01 30  	bt	2, 0x80014b40 <_binary__mnt_data_text1_bin_start+0xbe00>
80014a14: 40 80 01 68  	bf	0, 0x80014b7c <_binary__mnt_data_text1_bin_start+0xbe3c>
80014a18: 2c 00 00 00  	cmpwi	0, 0
80014a1c: 41 82 00 0c  	bt	2, 0x80014a28 <_binary__mnt_data_text1_bin_start+0xbce8>
80014a20: 48 00 01 5c  	b 0x80014b7c <_binary__mnt_data_text1_bin_start+0xbe3c>
80014a24: 48 00 01 58  	b 0x80014b7c <_binary__mnt_data_text1_bin_start+0xbe3c>
80014a28: 38 7f 00 1c  	addi 3, 31, 28
80014a2c: 38 80 00 00  	li 4, 0
80014a30: 48 00 04 05  	bl 0x80014e34 <_binary__mnt_data_text1_bin_start+0xc0f4>
80014a34: 54 60 06 3f  	clrlwi.	0, 3, 24
80014a38: 40 82 00 5c  	bf	2, 0x80014a94 <_binary__mnt_data_text1_bin_start+0xbd54>
80014a3c: 38 60 00 58  	li 3, 88
80014a40: 48 38 c9 41  	bl 0x803a1380 <_binary__mnt_data_text1_bin_start+0x398640>
80014a44: 7c 7e 1b 79  	mr.	30, 3
80014a48: 41 82 00 20  	bt	2, 0x80014a68 <_binary__mnt_data_text1_bin_start+0xbd28>
80014a4c: 38 7f 00 08  	addi 3, 31, 8
80014a50: 48 00 04 01  	bl 0x80014e50 <_binary__mnt_data_text1_bin_start+0xc110>
80014a54: 7c 64 1b 78  	mr	4, 3
80014a58: 7f c3 f3 78  	mr	3, 30
80014a5c: 38 a0 00 00  	li 5, 0
80014a60: 48 03 85 79  	bl 0x8004cfd8 <_binary__mnt_data_text1_bin_start+0x44298>
80014a64: 7c 7e 1b 78  	mr	30, 3
80014a68: 28 1e 00 00  	cmplwi	30, 0
80014a6c: 41 82 00 1c  	bt	2, 0x80014a88 <_binary__mnt_data_text1_bin_start+0xbd48>
80014a70: 7f e3 fb 78  	mr	3, 31
80014a74: 7f c4 f3 78  	mr	4, 30
80014a78: 4b ff f1 b9  	bl 0x80013c30 <_binary__mnt_data_text1_bin_start+0xaef0>
80014a7c: 7f c3 f3 78  	mr	3, 30
80014a80: 38 80 00 01  	li 4, 1
80014a84: 48 03 85 0d  	bl 0x8004cf90 <_binary__mnt_data_text1_bin_start+0x44250>
80014a88: 38 00 00 03  	li 0, 3
80014a8c: 90 1f 00 04  	stw 0, 4(31)
80014a90: 48 00 00 ec  	b 0x80014b7c <_binary__mnt_data_text1_bin_start+0xbe3c>
80014a94: 38 60 00 58  	li 3, 88
80014a98: 48 38 c8 e9  	bl 0x803a1380 <_binary__mnt_data_text1_bin_start+0x398640>
80014a9c: 7c 7e 1b 79  	mr.	30, 3
80014aa0: 41 82 00 14  	bt	2, 0x80014ab4 <_binary__mnt_data_text1_bin_start+0xbd74>
80014aa4: 38 80 00 00  	li 4, 0
80014aa8: 38 a0 00 00  	li 5, 0
80014aac: 48 03 85 2d  	bl 0x8004cfd8 <_binary__mnt_data_text1_bin_start+0x44298>
80014ab0: 7c 7e 1b 78  	mr	30, 3
80014ab4: 28 1e 00 00  	cmplwi	30, 0
80014ab8: 41 82 00 7c  	bt	2, 0x80014b34 <_binary__mnt_data_text1_bin_start+0xbdf4>
80014abc: 93 df 00 20  	stw 30, 32(31)
80014ac0: 38 1f 00 04  	addi 0, 31, 4
80014ac4: 38 7f 00 08  	addi 3, 31, 8
80014ac8: 90 1f 00 24  	stw 0, 36(31)
80014acc: 48 00 03 85  	bl 0x80014e50 <_binary__mnt_data_text1_bin_start+0xc110>
80014ad0: 3c a0 80 01  	lis 5, -32767
80014ad4: 7c 64 1b 78  	mr	4, 3
80014ad8: 7f c3 f3 78  	mr	3, 30
80014adc: 38 df 00 20  	addi 6, 31, 32
80014ae0: 38 a5 4d ac  	addi 5, 5, 19884
80014ae4: 38 e0 00 00  	li 7, 0
80014ae8: 39 00 00 00  	li 8, 0
80014aec: 48 03 81 a9  	bl 0x8004cc94 <_binary__mnt_data_text1_bin_start+0x43f54>
80014af0: 54 60 06 3f  	clrlwi.	0, 3, 24
80014af4: 41 82 00 28  	bt	2, 0x80014b1c <_binary__mnt_data_text1_bin_start+0xbddc>
80014af8: 38 00 00 01  	li 0, 1
80014afc: 90 1f 00 04  	stw 0, 4(31)
80014b00: 48 00 00 91  	bl 0x80014b90 <_binary__mnt_data_text1_bin_start+0xbe50>
80014b04: 3c a0 80 4b  	lis 5, -32693
80014b08: 7f e4 fb 78  	mr	4, 31
80014b0c: 38 c5 b6 90  	addi 6, 5, -18800
80014b10: 38 a0 00 00  	li 5, 0
80014b14: 48 24 8a 6d  	bl 0x8025d580 <_binary__mnt_data_text1_bin_start+0x254840>
80014b18: 48 00 00 0c  	b 0x80014b24 <_binary__mnt_data_text1_bin_start+0xbde4>
80014b1c: 38 00 00 04  	li 0, 4
80014b20: 90 1f 00 04  	stw 0, 4(31)
80014b24: 7f c3 f3 78  	mr	3, 30
80014b28: 38 80 00 01  	li 4, 1
80014b2c: 48 03 84 65  	bl 0x8004cf90 <_binary__mnt_data_text1_bin_start+0x44250>
80014b30: 48 00 00 4c  	b 0x80014b7c <_binary__mnt_data_text1_bin_start+0xbe3c>
80014b34: 38 00 00 04  	li 0, 4
80014b38: 90 1f 00 04  	stw 0, 4(31)
80014b3c: 48 00 00 40  	b 0x80014b7c <_binary__mnt_data_text1_bin_start+0xbe3c>
80014b40: 80 9f 00 20  	lwz 4, 32(31)
80014b44: 28 04 00 00  	cmplwi	4, 0
80014b48: 41 82 00 34  	bt	2, 0x80014b7c <_binary__mnt_data_text1_bin_start+0xbe3c>
80014b4c: 4b ff f0 e5  	bl 0x80013c30 <_binary__mnt_data_text1_bin_start+0xaef0>
80014b50: 80 7f 00 20  	lwz 3, 32(31)
80014b54: 38 80 00 01  	li 4, 1
80014b58: 48 03 84 39  	bl 0x8004cf90 <_binary__mnt_data_text1_bin_start+0x44250>
80014b5c: 38 60 00 00  	li 3, 0
80014b60: 38 00 00 03  	li 0, 3
80014b64: 90 7f 00 20  	stw 3, 32(31)
80014b68: 90 7f 00 24  	stw 3, 36(31)
80014b6c: 90 1f 00 04  	stw 0, 4(31)
80014b70: 48 00 00 21  	bl 0x80014b90 <_binary__mnt_data_text1_bin_start+0xbe50>
80014b74: 7f e4 fb 78  	mr	4, 31
80014b78: 48 24 89 51  	bl 0x8025d4c8 <_binary__mnt_data_text1_bin_start+0x254788>
80014b7c: bb c1 00 08  	lmw 30, 8(1)
80014b80: 80 01 00 14  	lwz 0, 20(1)
80014b84: 7c 08 03 a6  	mtlr 0
80014b88: 38 21 00 10  	addi 1, 1, 16
80014b8c: 4e 80 00 20  	blr
80014b90: 94 21 ff f0  	stwu 1, -16(1)
80014b94: 7c 08 02 a6  	mflr 0
80014b98: 90 01 00 14  	stw 0, 20(1)
80014b9c: 80 0d 2b 90  	lwz 0, 11152(13)
80014ba0: 28 00 00 00  	cmplwi	0, 0
80014ba4: 40 82 00 30  	bf	2, 0x80014bd4 <_binary__mnt_data_text1_bin_start+0xbe94>
80014ba8: 88 0d 2b 80  	lbz 0, 11136(13)
80014bac: 7c 00 07 75  	extsb. 0, 0
80014bb0: 40 82 00 18  	bf	2, 0x80014bc8 <_binary__mnt_data_text1_bin_start+0xbe88>
80014bb4: 3c 60 80 57  	lis 3, -32681
80014bb8: 38 63 ff c8  	addi 3, 3, -56
80014bbc: 48 24 8a 79  	bl 0x8025d634 <_binary__mnt_data_text1_bin_start+0x2548f4>
80014bc0: 38 00 00 01  	li 0, 1
80014bc4: 98 0d 2b 80  	stb 0, 11136(13)
80014bc8: 3c 60 80 57  	lis 3, -32681
80014bcc: 38 03 ff c8  	addi 0, 3, -56
80014bd0: 90 0d 2b 90  	stw 0, 11152(13)
80014bd4: 80 01 00 14  	lwz 0, 20(1)
80014bd8: 80 6d 2b 90  	lwz 3, 11152(13)
80014bdc: 7c 08 03 a6  	mtlr 0
80014be0: 38 21 00 10  	addi 1, 1, 16
80014be4: 4e 80 00 20  	blr
80014be8: 94 21 ff f0  	stwu 1, -16(1)
80014bec: 7c 08 02 a6  	mflr 0
80014bf0: 90 01 00 14  	stw 0, 20(1)
80014bf4: bf c1 00 08  	stmw 30, 8(1)
80014bf8: 7c 7e 1b 79  	mr.	30, 3
80014bfc: 7c 9f 23 78  	mr	31, 4
80014c00: 41 82 00 40  	bt	2, 0x80014c40 <_binary__mnt_data_text1_bin_start+0xbf00>
80014c04: 3c 80 80 52  	lis 4, -32686
80014c08: 38 7e 00 08  	addi 3, 30, 8
80014c0c: 38 04 d7 dc  	addi 0, 4, -10276
80014c10: 38 80 ff ff  	li 4, -1
80014c14: 90 1e 00 00  	stw 0, 0(30)
80014c18: 48 00 02 a9  	bl 0x80014ec0 <_binary__mnt_data_text1_bin_start+0xc180>
80014c1c: 28 1e 00 00  	cmplwi	30, 0
80014c20: 41 82 00 10  	bt	2, 0x80014c30 <_binary__mnt_data_text1_bin_start+0xbef0>
80014c24: 3c 60 80 52  	lis 3, -32686
80014c28: 38 03 d7 8c  	addi 0, 3, -10356
80014c2c: 90 1e 00 00  	stw 0, 0(30)
80014c30: 7f e0 07 35  	extsh. 0, 31
80014c34: 40 81 00 0c  	bf	1, 0x80014c40 <_binary__mnt_data_text1_bin_start+0xbf00>
80014c38: 7f c3 f3 78  	mr	3, 30
80014c3c: 48 38 c6 f9  	bl 0x803a1334 <_binary__mnt_data_text1_bin_start+0x3985f4>
\n# ===== 0x800453e0 .. 0x80046020 =====

/mnt/data/text1_exec.o:	file format elf32-powerpc

Disassembly of section .data:

80008d40 <_binary__mnt_data_text1_bin_start>:
800453e0: 94 21 ff e0  	stwu 1, -32(1)
800453e4: 7c 08 02 a6  	mflr 0
800453e8: 90 01 00 24  	stw 0, 36(1)
800453ec: bf a1 00 14  	stmw 29, 20(1)
800453f0: 80 0d 2c d8  	lwz 0, 11480(13)
800453f4: 2c 00 00 00  	cmpwi	0, 0
800453f8: 40 82 01 14  	bf	2, 0x8004550c <_binary__mnt_data_text1_bin_start+0x3c7cc>
800453fc: 88 0d 2c dc  	lbz 0, 11484(13)
80045400: 28 00 00 00  	cmplwi	0, 0
80045404: 41 82 00 08  	bt	2, 0x8004540c <_binary__mnt_data_text1_bin_start+0x3c6cc>
80045408: 48 00 01 04  	b 0x8004550c <_binary__mnt_data_text1_bin_start+0x3c7cc>
8004540c: 3c 80 80 57  	lis 4, -32681
80045410: 38 00 00 03  	li 0, 3
80045414: 38 a4 42 a0  	addi 5, 4, 17056
80045418: 3b e0 00 00  	li 31, 0
8004541c: 38 80 00 00  	li 4, 0
80045420: 7c 09 03 a6  	mtctr 0
80045424: 7c 05 20 2e  	lwzx 0, 5, 4
80045428: 28 00 00 00  	cmplwi	0, 0
8004542c: 40 82 00 10  	bf	2, 0x8004543c <_binary__mnt_data_text1_bin_start+0x3c6fc>
80045430: 3b ff 00 01  	addi 31, 31, 1
80045434: 38 84 00 04  	addi 4, 4, 4
80045438: 42 00 ff ec  	bdnz 0x80045424 <_binary__mnt_data_text1_bin_start+0x3c6e4>
8004543c: 2c 1f 00 03  	cmpwi	31, 3
80045440: 41 80 00 10  	bt	0, 0x80045450 <_binary__mnt_data_text1_bin_start+0x3c710>
80045444: 38 80 00 00  	li 4, 0
80045448: 4b ff ff 29  	bl 0x80045370 <_binary__mnt_data_text1_bin_start+0x3c630>
8004544c: 48 00 00 c0  	b 0x8004550c <_binary__mnt_data_text1_bin_start+0x3c7cc>
80045450: 38 80 00 01  	li 4, 1
80045454: 4b ff ff 1d  	bl 0x80045370 <_binary__mnt_data_text1_bin_start+0x3c630>
80045458: 3c 60 80 57  	lis 3, -32681
8004545c: 57 e0 10 3a  	slwi 0, 31, 2
80045460: 38 63 42 a0  	addi 3, 3, 17056
80045464: 38 80 00 01  	li 4, 1
80045468: 7c 63 00 2e  	lwzx 3, 3, 0
8004546c: 38 a0 00 00  	li 5, 0
80045470: 3b a3 00 04  	addi 29, 3, 4
80045474: 80 63 00 04  	lwz 3, 4(3)
80045478: 7f a6 eb 78  	mr	6, 29
8004547c: 48 43 5b 25  	bl 0x8047afa0 <_binary__mnt_data_text1_bin_start+0x472260>
80045480: 7c 7e 1b 79  	mr.	30, 3
80045484: 41 82 00 64  	bt	2, 0x800454e8 <_binary__mnt_data_text1_bin_start+0x3c7a8>
80045488: 80 1d 00 04  	lwz 0, 4(29)
8004548c: 28 00 00 00  	cmplwi	0, 0
80045490: 40 82 00 2c  	bf	2, 0x800454bc <_binary__mnt_data_text1_bin_start+0x3c77c>
80045494: 80 1e 00 04  	lwz 0, 4(30)
80045498: 38 80 00 02  	li 4, 2
8004549c: 90 1d 00 08  	stw 0, 8(29)
800454a0: 80 7d 00 08  	lwz 3, 8(29)
800454a4: 38 03 00 1f  	addi 0, 3, 31
800454a8: 54 00 00 34  	rlwinm 0, 0, 0, 0, 26
800454ac: 90 1d 00 08  	stw 0, 8(29)
800454b0: 80 7d 00 08  	lwz 3, 8(29)
800454b4: 48 00 47 05  	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
800454b8: 90 7d 00 04  	stw 3, 4(29)
800454bc: 48 31 4b 5d  	bl 0x8035a018 <_binary__mnt_data_text1_bin_start+0x3512d8>
800454c0: 38 60 03 90  	li 3, 912
800454c4: 48 35 be bd  	bl 0x803a1380 <_binary__mnt_data_text1_bin_start+0x398640>
800454c8: 7c 60 1b 79  	mr.	0, 3
800454cc: 41 82 00 14  	bt	2, 0x800454e0 <_binary__mnt_data_text1_bin_start+0x3c7a0>
800454d0: 7f a4 eb 78  	mr	4, 29
800454d4: 7f c5 f3 78  	mr	5, 30
800454d8: 48 00 02 a1  	bl 0x80045778 <_binary__mnt_data_text1_bin_start+0x3ca38>
800454dc: 7c 60 1b 78  	mr	0, 3
800454e0: 7c 03 03 78  	mr	3, 0
800454e4: 48 31 4b 31  	bl 0x8035a014 <_binary__mnt_data_text1_bin_start+0x3512d4>
800454e8: 3c 60 80 57  	lis 3, -32681
800454ec: 57 fe 10 3a  	slwi 30, 31, 2
800454f0: 3b e3 42 a0  	addi 31, 3, 17056
800454f4: 7c 7f f0 2e  	lwzx 3, 31, 30
800454f8: 28 03 00 00  	cmplwi	3, 0
800454fc: 41 82 00 10  	bt	2, 0x8004550c <_binary__mnt_data_text1_bin_start+0x3c7cc>
80045500: 83 a3 00 00  	lwz 29, 0(3)
80045504: 48 00 46 59  	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
80045508: 7f bf f1 2e  	stwx 29, 31, 30
8004550c: bb a1 00 14  	lmw 29, 20(1)
80045510: 80 01 00 24  	lwz 0, 36(1)
80045514: 7c 08 03 a6  	mtlr 0
80045518: 38 21 00 20  	addi 1, 1, 32
8004551c: 4e 80 00 20  	blr
80045520: 94 21 ff e0  	stwu 1, -32(1)
80045524: 7c 08 02 a6  	mflr 0
80045528: 90 01 00 24  	stw 0, 36(1)
8004552c: bf a1 00 14  	stmw 29, 20(1)
80045530: 7c 7d 1b 78  	mr	29, 3
80045534: 7c 9e 23 78  	mr	30, 4
80045538: 38 60 00 60  	li 3, 96
8004553c: 38 80 00 02  	li 4, 2
80045540: 48 00 46 79  	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
80045544: 7c 7f 1b 78  	mr	31, 3
80045548: 3c 60 80 57  	lis 3, -32681
8004554c: 93 df 00 5c  	stw 30, 92(31)
80045550: 38 80 00 00  	li 4, 0
80045554: 57 c0 10 3a  	slwi 0, 30, 2
80045558: 38 63 42 a0  	addi 3, 3, 17056
8004555c: 90 9f 00 00  	stw 4, 0(31)
80045560: 7c 83 00 2e  	lwzx 4, 3, 0
80045564: 28 04 00 00  	cmplwi	4, 0
80045568: 41 82 00 20  	bt	2, 0x80045588 <_binary__mnt_data_text1_bin_start+0x3c848>
8004556c: 48 00 00 08  	b 0x80045574 <_binary__mnt_data_text1_bin_start+0x3c834>
80045570: 7c 04 03 78  	mr	4, 0
80045574: 80 04 00 00  	lwz 0, 0(4)
80045578: 28 00 00 00  	cmplwi	0, 0
8004557c: 40 82 ff f4  	bf	2, 0x80045570 <_binary__mnt_data_text1_bin_start+0x3c830>
80045580: 93 e4 00 00  	stw 31, 0(4)
80045584: 48 00 00 08  	b 0x8004558c <_binary__mnt_data_text1_bin_start+0x3c84c>
80045588: 7f e3 01 2e  	stwx 31, 3, 0
8004558c: 28 1f 00 00  	cmplwi	31, 0
80045590: 40 82 00 0c  	bf	2, 0x8004559c <_binary__mnt_data_text1_bin_start+0x3c85c>
80045594: 38 60 00 00  	li 3, 0
80045598: 48 00 00 50  	b 0x800455e8 <_binary__mnt_data_text1_bin_start+0x3c8a8>
8004559c: 80 1d 00 00  	lwz 0, 0(29)
800455a0: 38 7f 00 1c  	addi 3, 31, 28
800455a4: 90 1f 00 04  	stw 0, 4(31)
800455a8: 80 1d 00 04  	lwz 0, 4(29)
800455ac: 90 1f 00 08  	stw 0, 8(31)
800455b0: 80 1d 00 08  	lwz 0, 8(29)
800455b4: 90 1f 00 0c  	stw 0, 12(31)
800455b8: 80 1d 00 0c  	lwz 0, 12(29)
800455bc: 90 1f 00 10  	stw 0, 16(31)
800455c0: 80 1d 00 10  	lwz 0, 16(29)
800455c4: 90 1f 00 14  	stw 0, 20(31)
800455c8: 80 1d 00 14  	lwz 0, 20(29)
800455cc: 90 1f 00 18  	stw 0, 24(31)
800455d0: 80 9d 00 00  	lwz 4, 0(29)
800455d4: 48 36 85 7d  	bl 0x803adb50 <_binary__mnt_data_text1_bin_start+0x3a4e10>
800455d8: 38 1f 00 1c  	addi 0, 31, 28
800455dc: 38 60 00 01  	li 3, 1
800455e0: 90 1f 00 04  	stw 0, 4(31)
800455e4: 93 df 00 5c  	stw 30, 92(31)
800455e8: bb a1 00 14  	lmw 29, 20(1)
800455ec: 80 01 00 24  	lwz 0, 36(1)
800455f0: 7c 08 03 a6  	mtlr 0
800455f4: 38 21 00 20  	addi 1, 1, 32
800455f8: 4e 80 00 20  	blr
800455fc: 94 21 ff f0  	stwu 1, -16(1)
80045600: 7c 08 02 a6  	mflr 0
80045604: 90 01 00 14  	stw 0, 20(1)
80045608: 93 e1 00 0c  	stw 31, 12(1)
8004560c: 7c 7f 1b 78  	mr	31, 3
80045610: 4b ff 1b f1  	bl 0x80037200 <_binary__mnt_data_text1_bin_start+0x2e4c0>
80045614: 7c 64 1b 78  	mr	4, 3
80045618: 7f e3 fb 78  	mr	3, 31
8004561c: 80 84 00 00  	lwz 4, 0(4)
80045620: 48 00 99 f5  	bl 0x8004f014 <_binary__mnt_data_text1_bin_start+0x462d4>
80045624: 3c 80 80 52  	lis 4, -32686
80045628: 3c 60 80 57  	lis 3, -32681
8004562c: 38 84 e2 b4  	addi 4, 4, -7500
80045630: 38 00 00 00  	li 0, 0
80045634: 90 9f 00 18  	stw 4, 24(31)
80045638: 38 63 42 a0  	addi 3, 3, 17056
8004563c: 38 80 00 00  	li 4, 0
80045640: 38 a0 00 0c  	li 5, 12
80045644: 98 1f 00 28  	stb 0, 40(31)
80045648: 80 0d 84 1c  	lwz 0, -31716(13)
8004564c: 90 1f 00 00  	stw 0, 0(31)
80045650: a0 1f 00 04  	lhz 0, 4(31)
80045654: 60 00 01 00  	ori 0, 0, 256
80045658: b0 1f 00 04  	sth 0, 4(31)
8004565c: 4b fb fd b1  	bl 0x8000540c <_binary__mnt_data_text1_bin_size+0x7fb634ec>
80045660: 38 00 00 00  	li 0, 0
80045664: 7f e3 fb 78  	mr	3, 31
80045668: 98 0d 2c dc  	stb 0, 11484(13)
8004566c: 83 e1 00 0c  	lwz 31, 12(1)
80045670: 80 01 00 14  	lwz 0, 20(1)
80045674: 7c 08 03 a6  	mtlr 0
80045678: 38 21 00 10  	addi 1, 1, 16
8004567c: 4e 80 00 20  	blr
80045680: 94 21 ff f0  	stwu 1, -16(1)
80045684: 7c 08 02 a6  	mflr 0
80045688: 90 01 00 14  	stw 0, 20(1)
8004568c: bf c1 00 08  	stmw 30, 8(1)
80045690: 7c 7e 1b 78  	mr	30, 3
80045694: 48 2e 7a a5  	bl 0x8032d138 <_binary__mnt_data_text1_bin_start+0x3243f8>
80045698: 28 03 00 00  	cmplwi	3, 0
8004569c: 41 82 00 08  	bt	2, 0x800456a4 <_binary__mnt_data_text1_bin_start+0x3c964>
800456a0: 48 33 3c 25  	bl 0x803792c4 <_binary__mnt_data_text1_bin_start+0x370584>
800456a4: 83 fe 03 48  	lwz 31, 840(30)
800456a8: 28 1f 00 00  	cmplwi	31, 0
800456ac: 41 82 00 20  	bt	2, 0x800456cc <_binary__mnt_data_text1_bin_start+0x3c98c>
800456b0: 4b ff 1b 51  	bl 0x80037200 <_binary__mnt_data_text1_bin_start+0x2e4c0>
800456b4: 80 de 03 38  	lwz 6, 824(30)
800456b8: 7f e4 fb 78  	mr	4, 31
800456bc: 38 be 03 34  	addi 5, 30, 820
800456c0: 38 e0 20 00  	li 7, 8192
800456c4: 4b ff fa 8d  	bl 0x80045150 <_binary__mnt_data_text1_bin_start+0x3c410>
800456c8: 90 7e 03 38  	stw 3, 824(30)
800456cc: 7f c3 f3 78  	mr	3, 30
800456d0: 48 00 00 1d  	bl 0x800456ec <_binary__mnt_data_text1_bin_start+0x3c9ac>
800456d4: bb c1 00 08  	lmw 30, 8(1)
800456d8: 38 60 00 00  	li 3, 0
800456dc: 80 01 00 14  	lwz 0, 20(1)
800456e0: 7c 08 03 a6  	mtlr 0
800456e4: 38 21 00 10  	addi 1, 1, 16
800456e8: 4e 80 00 20  	blr
800456ec: 94 21 ff f0  	stwu 1, -16(1)
800456f0: 7c 08 02 a6  	mflr 0
800456f4: 38 83 03 30  	addi 4, 3, 816
800456f8: 90 01 00 14  	stw 0, 20(1)
800456fc: 80 a3 03 48  	lwz 5, 840(3)
80045700: 48 00 01 91  	bl 0x80045890 <_binary__mnt_data_text1_bin_start+0x3cb50>
80045704: 80 01 00 14  	lwz 0, 20(1)
80045708: 7c 08 03 a6  	mtlr 0
8004570c: 38 21 00 10  	addi 1, 1, 16
80045710: 4e 80 00 20  	blr
80045714: 94 21 ff f0  	stwu 1, -16(1)
80045718: 7c 08 02 a6  	mflr 0
8004571c: 90 01 00 14  	stw 0, 20(1)
80045720: bf c1 00 08  	stmw 30, 8(1)
80045724: 7c 7e 1b 79  	mr.	30, 3
80045728: 7c 9f 23 78  	mr	31, 4
8004572c: 41 82 00 34  	bt	2, 0x80045760 <_binary__mnt_data_text1_bin_start+0x3ca20>
80045730: 3c a0 80 52  	lis 5, -32686
80045734: 38 80 00 00  	li 4, 0
80045738: 38 05 e2 d0  	addi 0, 5, -7472
8004573c: 90 1e 03 28  	stw 0, 808(30)
80045740: 80 ad 2c d8  	lwz 5, 11480(13)
80045744: 38 05 ff ff  	addi 0, 5, -1
80045748: 90 0d 2c d8  	stw 0, 11480(13)
8004574c: 48 31 49 7d  	bl 0x8035a0c8 <_binary__mnt_data_text1_bin_start+0x351388>
80045750: 7f e0 07 35  	extsh. 0, 31
80045754: 40 81 00 0c  	bf	1, 0x80045760 <_binary__mnt_data_text1_bin_start+0x3ca20>
80045758: 7f c3 f3 78  	mr	3, 30
8004575c: 48 35 bb d9  	bl 0x803a1334 <_binary__mnt_data_text1_bin_start+0x3985f4>
80045760: 7f c3 f3 78  	mr	3, 30
80045764: bb c1 00 08  	lmw 30, 8(1)
80045768: 80 01 00 14  	lwz 0, 20(1)
8004576c: 7c 08 03 a6  	mtlr 0
80045770: 38 21 00 10  	addi 1, 1, 16
80045774: 4e 80 00 20  	blr
80045778: 94 21 ff e0  	stwu 1, -32(1)
8004577c: 7c 08 02 a6  	mflr 0
80045780: 90 01 00 24  	stw 0, 36(1)
80045784: bf a1 00 14  	stmw 29, 20(1)
80045788: 7c 9e 23 78  	mr	30, 4
8004578c: 7c 7d 1b 78  	mr	29, 3
80045790: 7c bf 2b 78  	mr	31, 5
80045794: 38 80 00 00  	li 4, 0
80045798: 48 31 49 c5  	bl 0x8035a15c <_binary__mnt_data_text1_bin_start+0x35141c>
8004579c: 3c 80 80 52  	lis 4, -32686
800457a0: 38 7d 03 4c  	addi 3, 29, 844
800457a4: 38 04 e2 d0  	addi 0, 4, -7472
800457a8: 90 1d 03 28  	stw 0, 808(29)
800457ac: 80 9e 00 00  	lwz 4, 0(30)
800457b0: 48 36 83 a1  	bl 0x803adb50 <_binary__mnt_data_text1_bin_start+0x3a4e10>
800457b4: 80 1e 00 00  	lwz 0, 0(30)
800457b8: 7f a3 eb 78  	mr	3, 29
800457bc: 90 1d 03 30  	stw 0, 816(29)
800457c0: 80 1e 00 04  	lwz 0, 4(30)
800457c4: 90 1d 03 34  	stw 0, 820(29)
800457c8: 80 1e 00 08  	lwz 0, 8(30)
800457cc: 90 1d 03 38  	stw 0, 824(29)
800457d0: 80 1e 00 0c  	lwz 0, 12(30)
800457d4: 90 1d 03 3c  	stw 0, 828(29)
800457d8: 80 1e 00 10  	lwz 0, 16(30)
800457dc: 90 1d 03 40  	stw 0, 832(29)
800457e0: 80 1e 00 14  	lwz 0, 20(30)
800457e4: 90 1d 03 44  	stw 0, 836(29)
800457e8: 93 fd 03 48  	stw 31, 840(29)
800457ec: 80 8d 2c d8  	lwz 4, 11480(13)
800457f0: 38 04 00 01  	addi 0, 4, 1
800457f4: 90 0d 2c d8  	stw 0, 11480(13)
800457f8: bb a1 00 14  	lmw 29, 20(1)
800457fc: 80 01 00 24  	lwz 0, 36(1)
80045800: 7c 08 03 a6  	mtlr 0
80045804: 38 21 00 20  	addi 1, 1, 32
80045808: 4e 80 00 20  	blr
8004580c: 94 21 ff f0  	stwu 1, -16(1)
80045810: 7c 08 02 a6  	mflr 0
80045814: 90 01 00 14  	stw 0, 20(1)
80045818: 93 e1 00 0c  	stw 31, 12(1)
8004581c: 7c 7f 1b 78  	mr	31, 3
80045820: a0 03 00 04  	lhz 0, 4(3)
80045824: 54 00 04 21  	rlwinm. 0, 0, 0, 16, 16
80045828: 41 82 00 54  	bt	2, 0x8004587c <_binary__mnt_data_text1_bin_start+0x3cb3c>
8004582c: 80 1f 00 44  	lwz 0, 68(31)
80045830: 28 00 00 00  	cmplwi	0, 0
80045834: 41 82 00 0c  	bt	2, 0x80045840 <_binary__mnt_data_text1_bin_start+0x3cb00>
80045838: 38 00 00 00  	li 0, 0
8004583c: 90 1f 00 44  	stw 0, 68(31)
80045840: 80 7f 00 40  	lwz 3, 64(31)
80045844: 28 03 00 00  	cmplwi	3, 0
80045848: 41 82 00 28  	bt	2, 0x80045870 <_binary__mnt_data_text1_bin_start+0x3cb30>
8004584c: 48 43 5b 8d  	bl 0x8047b3d8 <_binary__mnt_data_text1_bin_start+0x472698>
80045850: 81 9f 00 38  	lwz 12, 56(31)
80045854: 28 0c 00 00  	cmplwi	12, 0
80045858: 41 82 00 10  	bt	2, 0x80045868 <_binary__mnt_data_text1_bin_start+0x3cb28>
8004585c: 38 7f 00 28  	addi 3, 31, 40
80045860: 7d 89 03 a6  	mtctr 12
80045864: 4e 80 04 21  	bctrl
80045868: 38 00 00 00  	li 0, 0
8004586c: 90 1f 00 40  	stw 0, 64(31)
80045870: a0 1f 00 04  	lhz 0, 4(31)
80045874: 60 00 00 01  	ori 0, 0, 1
80045878: b0 1f 00 04  	sth 0, 4(31)
8004587c: 80 01 00 14  	lwz 0, 20(1)
80045880: 83 e1 00 0c  	lwz 31, 12(1)
80045884: 7c 08 03 a6  	mtlr 0
80045888: 38 21 00 10  	addi 1, 1, 16
8004588c: 4e 80 00 20  	blr
80045890: 94 21 ff e0  	stwu 1, -32(1)
80045894: 7c 08 02 a6  	mflr 0
80045898: 90 01 00 24  	stw 0, 36(1)
8004589c: bf 81 00 10  	stmw 28, 16(1)
800458a0: 7c 7c 1b 78  	mr	28, 3
800458a4: 7c 9d 23 78  	mr	29, 4
800458a8: 7c be 2b 78  	mr	30, 5
800458ac: 38 60 00 48  	li 3, 72
800458b0: 48 35 ba d1  	bl 0x803a1380 <_binary__mnt_data_text1_bin_start+0x398640>
800458b4: 7c 7f 1b 79  	mr.	31, 3
800458b8: 41 82 00 24  	bt	2, 0x800458dc <_binary__mnt_data_text1_bin_start+0x3cb9c>
800458bc: 4b ff 19 45  	bl 0x80037200 <_binary__mnt_data_text1_bin_start+0x2e4c0>
800458c0: 7c 64 1b 78  	mr	4, 3
800458c4: 7f e3 fb 78  	mr	3, 31
800458c8: 80 a4 00 00  	lwz 5, 0(4)
800458cc: 7f 84 e3 78  	mr	4, 28
800458d0: 7f a6 eb 78  	mr	6, 29
800458d4: 7f c7 f3 78  	mr	7, 30
800458d8: 48 00 00 19  	bl 0x800458f0 <_binary__mnt_data_text1_bin_start+0x3cbb0>
800458dc: bb 81 00 10  	lmw 28, 16(1)
800458e0: 80 01 00 24  	lwz 0, 36(1)
800458e4: 7c 08 03 a6  	mtlr 0
800458e8: 38 21 00 20  	addi 1, 1, 32
800458ec: 4e 80 00 20  	blr
800458f0: 94 21 ff e0  	stwu 1, -32(1)
800458f4: 7c 08 02 a6  	mflr 0
800458f8: 90 01 00 24  	stw 0, 36(1)
800458fc: bf 81 00 10  	stmw 28, 16(1)
80045900: 7c 9c 23 78  	mr	28, 4
80045904: 7c 7f 1b 78  	mr	31, 3
80045908: 7c dd 33 78  	mr	29, 6
8004590c: 7c fe 3b 78  	mr	30, 7
80045910: 7c a4 2b 78  	mr	4, 5
80045914: 48 00 97 01  	bl 0x8004f014 <_binary__mnt_data_text1_bin_start+0x462d4>
80045918: 3c 60 80 52  	lis 3, -32686
8004591c: 38 00 00 00  	li 0, 0
80045920: 38 83 e2 f0  	addi 4, 3, -7440
80045924: 90 9f 00 18  	stw 4, 24(31)
80045928: 7f e3 fb 78  	mr	3, 31
8004592c: 90 1f 00 44  	stw 0, 68(31)
80045930: 80 0d 84 18  	lwz 0, -31720(13)
80045934: 90 1f 00 00  	stw 0, 0(31)
80045938: a0 1f 00 04  	lhz 0, 4(31)
8004593c: 60 00 01 00  	ori 0, 0, 256
80045940: b0 1f 00 04  	sth 0, 4(31)
80045944: 80 1d 00 00  	lwz 0, 0(29)
80045948: 90 1f 00 28  	stw 0, 40(31)
8004594c: 80 1d 00 04  	lwz 0, 4(29)
80045950: 90 1f 00 2c  	stw 0, 44(31)
80045954: 80 1d 00 08  	lwz 0, 8(29)
80045958: 90 1f 00 30  	stw 0, 48(31)
8004595c: 80 1d 00 0c  	lwz 0, 12(29)
80045960: 90 1f 00 34  	stw 0, 52(31)
80045964: 80 1d 00 10  	lwz 0, 16(29)
80045968: 90 1f 00 38  	stw 0, 56(31)
8004596c: 80 1d 00 14  	lwz 0, 20(29)
80045970: 90 1f 00 3c  	stw 0, 60(31)
80045974: 93 df 00 40  	stw 30, 64(31)
80045978: 93 9f 00 44  	stw 28, 68(31)
8004597c: a0 1f 00 04  	lhz 0, 4(31)
80045980: 60 00 80 00  	ori 0, 0, 32768
80045984: b0 1f 00 04  	sth 0, 4(31)
80045988: bb 81 00 10  	lmw 28, 16(1)
8004598c: 80 01 00 24  	lwz 0, 36(1)
80045990: 7c 08 03 a6  	mtlr 0
80045994: 38 21 00 20  	addi 1, 1, 32
80045998: 4e 80 00 20  	blr
8004599c: 94 21 ff f0  	stwu 1, -16(1)
800459a0: 7c 08 02 a6  	mflr 0
800459a4: 90 01 00 14  	stw 0, 20(1)
800459a8: bf c1 00 08  	stmw 30, 8(1)
800459ac: 7c 7e 1b 79  	mr.	30, 3
800459b0: 7c 9f 23 78  	mr	31, 4
800459b4: 41 82 00 28  	bt	2, 0x800459dc <_binary__mnt_data_text1_bin_start+0x3cc9c>
800459b8: 3c a0 80 52  	lis 5, -32686
800459bc: 38 80 00 00  	li 4, 0
800459c0: 38 05 e2 f0  	addi 0, 5, -7440
800459c4: 90 1e 00 18  	stw 0, 24(30)
800459c8: 48 00 95 51  	bl 0x8004ef18 <_binary__mnt_data_text1_bin_start+0x461d8>
800459cc: 7f e0 07 35  	extsh. 0, 31
800459d0: 40 81 00 0c  	bf	1, 0x800459dc <_binary__mnt_data_text1_bin_start+0x3cc9c>
800459d4: 7f c3 f3 78  	mr	3, 30
800459d8: 48 35 b9 5d  	bl 0x803a1334 <_binary__mnt_data_text1_bin_start+0x3985f4>
800459dc: 7f c3 f3 78  	mr	3, 30
800459e0: bb c1 00 08  	lmw 30, 8(1)
800459e4: 80 01 00 14  	lwz 0, 20(1)
800459e8: 7c 08 03 a6  	mtlr 0
800459ec: 38 21 00 10  	addi 1, 1, 16
800459f0: 4e 80 00 20  	blr
800459f4: 94 21 ff f0  	stwu 1, -16(1)
800459f8: 7c 08 02 a6  	mflr 0
800459fc: 90 01 00 14  	stw 0, 20(1)
80045a00: 93 e1 00 0c  	stw 31, 12(1)
80045a04: 7c 7f 1b 78  	mr	31, 3
80045a08: 48 00 00 21  	bl 0x80045a28 <_binary__mnt_data_text1_bin_start+0x3cce8>
80045a0c: 38 9f 00 04  	addi 4, 31, 4
80045a10: 48 00 03 25  	bl 0x80045d34 <_binary__mnt_data_text1_bin_start+0x3cff4>
80045a14: 80 01 00 14  	lwz 0, 20(1)
80045a18: 83 e1 00 0c  	lwz 31, 12(1)
80045a1c: 7c 08 03 a6  	mtlr 0
80045a20: 38 21 00 10  	addi 1, 1, 16
80045a24: 4e 80 00 20  	blr
80045a28: 94 21 ff f0  	stwu 1, -16(1)
80045a2c: 7c 08 02 a6  	mflr 0
80045a30: 90 01 00 14  	stw 0, 20(1)
80045a34: 80 0d 2c ec  	lwz 0, 11500(13)
80045a38: 28 00 00 00  	cmplwi	0, 0
80045a3c: 40 82 00 3c  	bf	2, 0x80045a78 <_binary__mnt_data_text1_bin_start+0x3cd38>
80045a40: 88 0d 2c e0  	lbz 0, 11488(13)
80045a44: 7c 00 07 75  	extsb. 0, 0
80045a48: 40 82 00 28  	bf	2, 0x80045a70 <_binary__mnt_data_text1_bin_start+0x3cd30>
80045a4c: 38 6d 2c e4  	addi 3, 13, 11492
80045a50: 48 00 04 31  	bl 0x80045e80 <_binary__mnt_data_text1_bin_start+0x3d140>
80045a54: 3c 80 80 04  	lis 4, -32764
80045a58: 3c a0 80 57  	lis 5, -32681
80045a5c: 38 84 5e 24  	addi 4, 4, 24100
80045a60: 38 a5 42 b0  	addi 5, 5, 17072
80045a64: 48 35 b7 ad  	bl 0x803a1210 <_binary__mnt_data_text1_bin_start+0x3984d0>
80045a68: 38 00 00 01  	li 0, 1
80045a6c: 98 0d 2c e0  	stb 0, 11488(13)
80045a70: 38 0d 2c e4  	addi 0, 13, 11492
80045a74: 90 0d 2c ec  	stw 0, 11500(13)
80045a78: 80 01 00 14  	lwz 0, 20(1)
80045a7c: 80 6d 2c ec  	lwz 3, 11500(13)
80045a80: 7c 08 03 a6  	mtlr 0
80045a84: 38 21 00 10  	addi 1, 1, 16
80045a88: 4e 80 00 20  	blr
80045a8c: 94 21 ff f0  	stwu 1, -16(1)
80045a90: 7c 08 02 a6  	mflr 0
80045a94: 90 01 00 14  	stw 0, 20(1)
80045a98: 93 e1 00 0c  	stw 31, 12(1)
80045a9c: 7c 7f 1b 78  	mr	31, 3
80045aa0: 4b ff ff 89  	bl 0x80045a28 <_binary__mnt_data_text1_bin_start+0x3cce8>
80045aa4: 7f e4 fb 78  	mr	4, 31
80045aa8: 48 00 03 05  	bl 0x80045dac <_binary__mnt_data_text1_bin_start+0x3d06c>
80045aac: 80 01 00 14  	lwz 0, 20(1)
80045ab0: 83 e1 00 0c  	lwz 31, 12(1)
80045ab4: 7c 08 03 a6  	mtlr 0
80045ab8: 38 21 00 10  	addi 1, 1, 16
80045abc: 4e 80 00 20  	blr
80045ac0: 94 21 ff f0  	stwu 1, -16(1)
80045ac4: 7c 08 02 a6  	mflr 0
80045ac8: a0 83 00 00  	lhz 4, 0(3)
80045acc: 90 01 00 14  	stw 0, 20(1)
80045ad0: a8 03 00 02  	lha 0, 2(3)
80045ad4: b0 81 00 08  	sth 4, 8(1)
80045ad8: b0 01 00 0a  	sth 0, 10(1)
80045adc: 4b ff ff 4d  	bl 0x80045a28 <_binary__mnt_data_text1_bin_start+0x3cce8>
80045ae0: 38 81 00 08  	addi 4, 1, 8
80045ae4: 48 00 01 c1  	bl 0x80045ca4 <_binary__mnt_data_text1_bin_start+0x3cf64>
80045ae8: 80 01 00 14  	lwz 0, 20(1)
80045aec: 7c 08 03 a6  	mtlr 0
80045af0: 38 21 00 10  	addi 1, 1, 16
80045af4: 4e 80 00 20  	blr
80045af8: 94 21 ff f0  	stwu 1, -16(1)
80045afc: 7c 08 02 a6  	mflr 0
80045b00: 90 01 00 14  	stw 0, 20(1)
80045b04: 80 0d 2c f4  	lwz 0, 11508(13)
80045b08: 28 00 00 00  	cmplwi	0, 0
80045b0c: 40 82 00 0c  	bf	2, 0x80045b18 <_binary__mnt_data_text1_bin_start+0x3cdd8>
80045b10: 48 00 00 1d  	bl 0x80045b2c <_binary__mnt_data_text1_bin_start+0x3cdec>
80045b14: 90 6d 2c f4  	stw 3, 11508(13)
80045b18: 80 01 00 14  	lwz 0, 20(1)
80045b1c: 80 6d 2c f4  	lwz 3, 11508(13)
80045b20: 7c 08 03 a6  	mtlr 0
80045b24: 38 21 00 10  	addi 1, 1, 16
80045b28: 4e 80 00 20  	blr
80045b2c: 94 21 ff f0  	stwu 1, -16(1)
80045b30: 7c 08 02 a6  	mflr 0
80045b34: 90 01 00 14  	stw 0, 20(1)
80045b38: 88 0d 2c f0  	lbz 0, 11504(13)
80045b3c: 7c 00 07 75  	extsb. 0, 0
80045b40: 40 82 00 30  	bf	2, 0x80045b70 <_binary__mnt_data_text1_bin_start+0x3ce30>
80045b44: 3c 60 80 57  	lis 3, -32681
80045b48: 38 80 00 80  	li 4, 128
80045b4c: 38 63 42 cc  	addi 3, 3, 17100
80045b50: 48 00 00 95  	bl 0x80045be4 <_binary__mnt_data_text1_bin_start+0x3cea4>
80045b54: 3c 80 80 04  	lis 4, -32764
80045b58: 3c a0 80 57  	lis 5, -32681
80045b5c: 38 84 5b 88  	addi 4, 4, 23432
80045b60: 38 a5 42 c0  	addi 5, 5, 17088
80045b64: 48 35 b6 ad  	bl 0x803a1210 <_binary__mnt_data_text1_bin_start+0x3984d0>
80045b68: 38 00 00 01  	li 0, 1
80045b6c: 98 0d 2c f0  	stb 0, 11504(13)
80045b70: 80 01 00 14  	lwz 0, 20(1)
80045b74: 3c 60 80 57  	lis 3, -32681
80045b78: 38 63 42 cc  	addi 3, 3, 17100
80045b7c: 7c 08 03 a6  	mtlr 0
80045b80: 38 21 00 10  	addi 1, 1, 16
80045b84: 4e 80 00 20  	blr
80045b88: 94 21 ff f0  	stwu 1, -16(1)
80045b8c: 7c 08 02 a6  	mflr 0
80045b90: 90 01 00 14  	stw 0, 20(1)
80045b94: bf c1 00 08  	stmw 30, 8(1)
80045b98: 7c 7e 1b 79  	mr.	30, 3
80045b9c: 7c 9f 23 78  	mr	31, 4
80045ba0: 41 82 00 2c  	bt	2, 0x80045bcc <_binary__mnt_data_text1_bin_start+0x3ce8c>
80045ba4: 80 7e 00 00  	lwz 3, 0(30)
80045ba8: 28 03 00 00  	cmplwi	3, 0
80045bac: 41 82 00 10  	bt	2, 0x80045bbc <_binary__mnt_data_text1_bin_start+0x3ce7c>
80045bb0: 48 00 3f ad  	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
80045bb4: 38 00 00 00  	li 0, 0
80045bb8: 90 1e 00 00  	stw 0, 0(30)
80045bbc: 7f e0 07 35  	extsh. 0, 31
80045bc0: 40 81 00 0c  	bf	1, 0x80045bcc <_binary__mnt_data_text1_bin_start+0x3ce8c>
80045bc4: 7f c3 f3 78  	mr	3, 30
80045bc8: 48 35 b7 6d  	bl 0x803a1334 <_binary__mnt_data_text1_bin_start+0x3985f4>
80045bcc: 7f c3 f3 78  	mr	3, 30
80045bd0: bb c1 00 08  	lmw 30, 8(1)
80045bd4: 80 01 00 14  	lwz 0, 20(1)
80045bd8: 7c 08 03 a6  	mtlr 0
80045bdc: 38 21 00 10  	addi 1, 1, 16
80045be0: 4e 80 00 20  	blr
80045be4: 94 21 ff f0  	stwu 1, -16(1)
80045be8: 7c 08 02 a6  	mflr 0
80045bec: 38 a0 00 02  	li 5, 2
80045bf0: 90 01 00 14  	stw 0, 20(1)
80045bf4: bf c1 00 08  	stmw 30, 8(1)
80045bf8: 7c 7e 1b 78  	mr	30, 3
80045bfc: 7c 9f 23 78  	mr	31, 4
80045c00: 38 60 00 10  	li 3, 16
80045c04: 48 00 3f 85  	bl 0x80049b88 <_binary__mnt_data_text1_bin_start+0x40e48>
80045c08: 28 03 00 00  	cmplwi	3, 0
80045c0c: 90 7e 00 00  	stw 3, 0(30)
80045c10: 40 82 00 08  	bf	2, 0x80045c18 <_binary__mnt_data_text1_bin_start+0x3ced8>
80045c14: 48 00 00 00  	b 0x80045c14 <_binary__mnt_data_text1_bin_start+0x3ced4>
80045c18: 93 fe 00 08  	stw 31, 8(30)
80045c1c: 38 a0 00 00  	li 5, 0
80045c20: 38 7f ff ff  	addi 3, 31, -1
80045c24: 38 e0 00 00  	li 7, 0
80045c28: 90 be 00 0c  	stw 5, 12(30)
80045c2c: 38 80 ff ff  	li 4, -1
80045c30: 80 1e 00 00  	lwz 0, 0(30)
80045c34: 90 1e 00 04  	stw 0, 4(30)
80045c38: 80 de 00 04  	lwz 6, 4(30)
80045c3c: 7f e9 03 a6  	mtctr 31
80045c40: 2c 1f 00 00  	cmpwi	31, 0
80045c44: 40 81 00 48  	bf	1, 0x80045c8c <_binary__mnt_data_text1_bin_start+0x3cf4c>
80045c48: b0 a6 00 00  	sth 5, 0(6)
80045c4c: 2c 07 00 00  	cmpwi	7, 0
80045c50: b0 86 00 02  	sth 4, 2(6)
80045c54: 40 82 00 0c  	bf	2, 0x80045c60 <_binary__mnt_data_text1_bin_start+0x3cf20>
80045c58: 90 a6 00 04  	stw 5, 4(6)
80045c5c: 48 00 00 0c  	b 0x80045c68 <_binary__mnt_data_text1_bin_start+0x3cf28>
80045c60: 38 06 ff f0  	addi 0, 6, -16
80045c64: 90 06 00 04  	stw 0, 4(6)
80045c68: 7c 07 18 00  	cmpw	7, 3
80045c6c: 40 82 00 0c  	bf	2, 0x80045c78 <_binary__mnt_data_text1_bin_start+0x3cf38>
80045c70: 90 a6 00 08  	stw 5, 8(6)
80045c74: 48 00 00 0c  	b 0x80045c80 <_binary__mnt_data_text1_bin_start+0x3cf40>
80045c78: 38 06 00 10  	addi 0, 6, 16
80045c7c: 90 06 00 08  	stw 0, 8(6)
80045c80: 38 e7 00 01  	addi 7, 7, 1
80045c84: 38 c6 00 10  	addi 6, 6, 16
80045c88: 42 00 ff c0  	bdnz 0x80045c48 <_binary__mnt_data_text1_bin_start+0x3cf08>
80045c8c: 7f c3 f3 78  	mr	3, 30
80045c90: bb c1 00 08  	lmw 30, 8(1)
80045c94: 80 01 00 14  	lwz 0, 20(1)
80045c98: 7c 08 03 a6  	mtlr 0
80045c9c: 38 21 00 10  	addi 1, 1, 16
80045ca0: 4e 80 00 20  	blr
80045ca4: 94 21 ff f0  	stwu 1, -16(1)
80045ca8: 7c 08 02 a6  	mflr 0
80045cac: a8 c4 00 02  	lha 6, 2(4)
80045cb0: 90 01 00 14  	stw 0, 20(1)
80045cb4: 2c 06 ff ff  	cmpwi	6, -1
80045cb8: 41 82 00 28  	bt	2, 0x80045ce0 <_binary__mnt_data_text1_bin_start+0x3cfa0>
80045cbc: a0 a4 00 00  	lhz 5, 0(4)
80045cc0: 54 c0 18 38  	slwi 0, 6, 3
80045cc4: b0 c1 00 0a  	sth 6, 10(1)
80045cc8: 38 81 00 08  	addi 4, 1, 8
80045ccc: b0 a1 00 08  	sth 5, 8(1)
80045cd0: 80 63 00 00  	lwz 3, 0(3)
80045cd4: 7c 63 02 14  	add 3, 3, 0
80045cd8: 48 00 00 1d  	bl 0x80045cf4 <_binary__mnt_data_text1_bin_start+0x3cfb4>
80045cdc: 48 00 00 08  	b 0x80045ce4 <_binary__mnt_data_text1_bin_start+0x3cfa4>
80045ce0: 38 60 00 00  	li 3, 0
80045ce4: 80 01 00 14  	lwz 0, 20(1)
80045ce8: 7c 08 03 a6  	mtlr 0
80045cec: 38 21 00 10  	addi 1, 1, 16
80045cf0: 4e 80 00 20  	blr
80045cf4: a8 a4 00 02  	lha 5, 2(4)
80045cf8: 38 c0 00 00  	li 6, 0
80045cfc: a8 03 00 02  	lha 0, 2(3)
80045d00: 7c 05 00 00  	cmpw	5, 0
80045d04: 40 82 00 18  	bf	2, 0x80045d1c <_binary__mnt_data_text1_bin_start+0x3cfdc>
80045d08: a0 84 00 00  	lhz 4, 0(4)
80045d0c: a0 03 00 00  	lhz 0, 0(3)
80045d10: 7c 04 00 40  	cmplw	4, 0
80045d14: 40 82 00 08  	bf	2, 0x80045d1c <_binary__mnt_data_text1_bin_start+0x3cfdc>
80045d18: 38 c0 00 01  	li 6, 1
80045d1c: 54 c0 06 3f  	clrlwi.	0, 6, 24
80045d20: 41 82 00 0c  	bt	2, 0x80045d2c <_binary__mnt_data_text1_bin_start+0x3cfec>
80045d24: 80 63 00 04  	lwz 3, 4(3)
80045d28: 4e 80 00 20  	blr
80045d2c: 38 60 00 00  	li 3, 0
80045d30: 4e 80 00 20  	blr
80045d34: 94 21 ff f0  	stwu 1, -16(1)
80045d38: 7c 08 02 a6  	mflr 0
80045d3c: 90 01 00 14  	stw 0, 20(1)
80045d40: bf c1 00 08  	stmw 30, 8(1)
80045d44: 7c 9f 23 78  	mr	31, 4
80045d48: 7c 7e 1b 78  	mr	30, 3
80045d4c: a8 04 00 02  	lha 0, 2(4)
80045d50: 2c 00 ff ff  	cmpwi	0, -1
80045d54: 41 82 00 38  	bt	2, 0x80045d8c <_binary__mnt_data_text1_bin_start+0x3d04c>
80045d58: 7f e3 fb 78  	mr	3, 31
80045d5c: 48 00 00 49  	bl 0x80045da4 <_binary__mnt_data_text1_bin_start+0x3d064>
80045d60: 80 be 00 00  	lwz 5, 0(30)
80045d64: 7c 63 07 34  	extsh 3, 3
80045d68: 38 80 00 00  	li 4, 0
80045d6c: 38 00 ff ff  	li 0, -1
80045d70: b0 9f 00 00  	sth 4, 0(31)
80045d74: 54 63 18 38  	slwi 3, 3, 3
80045d78: 7c a5 1a 14  	add 5, 5, 3
80045d7c: 38 60 00 01  	li 3, 1
80045d80: b0 1f 00 02  	sth 0, 2(31)
80045d84: 90 85 00 04  	stw 4, 4(5)
80045d88: 48 00 00 08  	b 0x80045d90 <_binary__mnt_data_text1_bin_start+0x3d050>
80045d8c: 38 60 00 00  	li 3, 0
80045d90: bb c1 00 08  	lmw 30, 8(1)
80045d94: 80 01 00 14  	lwz 0, 20(1)
80045d98: 7c 08 03 a6  	mtlr 0
80045d9c: 38 21 00 10  	addi 1, 1, 16
80045da0: 4e 80 00 20  	blr
80045da4: a8 63 00 02  	lha 3, 2(3)
80045da8: 4e 80 00 20  	blr
80045dac: 94 21 ff f0  	stwu 1, -16(1)
80045db0: 7c 08 02 a6  	mflr 0
80045db4: 38 e0 00 00  	li 7, 0
80045db8: 90 01 00 14  	stw 0, 20(1)
80045dbc: 80 a3 00 04  	lwz 5, 4(3)
80045dc0: 38 e7 00 01  	addi 7, 7, 1
80045dc4: 2c 05 10 00  	cmpwi	5, 4096
80045dc8: 41 80 00 0c  	bt	0, 0x80045dd4 <_binary__mnt_data_text1_bin_start+0x3d094>
80045dcc: 38 05 f0 00  	addi 0, 5, -4096
80045dd0: 90 03 00 04  	stw 0, 4(3)
80045dd4: 80 a3 00 04  	lwz 5, 4(3)
80045dd8: 80 c3 00 00  	lwz 6, 0(3)
80045ddc: 54 a0 18 38  	slwi 0, 5, 3
80045de0: 7c c6 02 14  	add 6, 6, 0
80045de4: 80 06 00 04  	lwz 0, 4(6)
80045de8: 28 00 00 00  	cmplwi	0, 0
80045dec: 40 82 00 14  	bf	2, 0x80045e00 <_binary__mnt_data_text1_bin_start+0x3d0c0>
80045df0: 7c c3 33 78  	mr	3, 6
80045df4: 48 00 01 0d  	bl 0x80045f00 <_binary__mnt_data_text1_bin_start+0x3d1c0>
80045df8: 38 60 00 01  	li 3, 1
80045dfc: 48 00 00 18  	b 0x80045e14 <_binary__mnt_data_text1_bin_start+0x3d0d4>
80045e00: 38 05 00 01  	addi 0, 5, 1
80045e04: 2c 07 10 00  	cmpwi	7, 4096
80045e08: 90 03 00 04  	stw 0, 4(3)
80045e0c: 40 81 ff b0  	bf	1, 0x80045dbc <_binary__mnt_data_text1_bin_start+0x3d07c>
80045e10: 38 60 00 00  	li 3, 0
80045e14: 80 01 00 14  	lwz 0, 20(1)
80045e18: 7c 08 03 a6  	mtlr 0
80045e1c: 38 21 00 10  	addi 1, 1, 16
80045e20: 4e 80 00 20  	blr
80045e24: 94 21 ff f0  	stwu 1, -16(1)
80045e28: 7c 08 02 a6  	mflr 0
80045e2c: 90 01 00 14  	stw 0, 20(1)
80045e30: bf c1 00 08  	stmw 30, 8(1)
80045e34: 7c 7e 1b 79  	mr.	30, 3
80045e38: 7c 9f 23 78  	mr	31, 4
80045e3c: 41 82 00 2c  	bt	2, 0x80045e68 <_binary__mnt_data_text1_bin_start+0x3d128>
80045e40: 80 7e 00 00  	lwz 3, 0(30)
80045e44: 28 03 00 00  	cmplwi	3, 0
80045e48: 41 82 00 10  	bt	2, 0x80045e58 <_binary__mnt_data_text1_bin_start+0x3d118>
80045e4c: 48 00 3d 11  	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
80045e50: 38 00 00 00  	li 0, 0
80045e54: 90 1e 00 00  	stw 0, 0(30)
80045e58: 7f e0 07 35  	extsh. 0, 31
80045e5c: 40 81 00 0c  	bf	1, 0x80045e68 <_binary__mnt_data_text1_bin_start+0x3d128>
80045e60: 7f c3 f3 78  	mr	3, 30
80045e64: 48 35 b4 d1  	bl 0x803a1334 <_binary__mnt_data_text1_bin_start+0x3985f4>
80045e68: 7f c3 f3 78  	mr	3, 30
80045e6c: bb c1 00 08  	lmw 30, 8(1)
80045e70: 80 01 00 14  	lwz 0, 20(1)
80045e74: 7c 08 03 a6  	mtlr 0
80045e78: 38 21 00 10  	addi 1, 1, 16
80045e7c: 4e 80 00 20  	blr
80045e80: 94 21 ff f0  	stwu 1, -16(1)
80045e84: 7c 08 02 a6  	mflr 0
80045e88: 3c 80 00 01  	lis 4, 1
80045e8c: 90 01 00 14  	stw 0, 20(1)
80045e90: 93 e1 00 0c  	stw 31, 12(1)
80045e94: 7c 7f 1b 78  	mr	31, 3
80045e98: 38 64 80 00  	addi 3, 4, -32768
80045e9c: 38 80 00 02  	li 4, 2
80045ea0: 48 00 3d 19  	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
80045ea4: 90 7f 00 00  	stw 3, 0(31)
80045ea8: 38 a0 00 00  	li 5, 0
80045eac: 38 60 00 00  	li 3, 0
80045eb0: 48 00 00 24  	b 0x80045ed4 <_binary__mnt_data_text1_bin_start+0x3d194>
80045eb4: 7c a0 07 34  	extsh 0, 5
80045eb8: 80 9f 00 00  	lwz 4, 0(31)
80045ebc: 54 00 18 38  	slwi 0, 0, 3
80045ec0: 7c 84 02 14  	add 4, 4, 0
80045ec4: b0 64 00 00  	sth 3, 0(4)
80045ec8: b0 a4 00 02  	sth 5, 2(4)
80045ecc: 38 a5 00 01  	addi 5, 5, 1
80045ed0: 90 64 00 04  	stw 3, 4(4)
80045ed4: 7c a0 07 34  	extsh 0, 5
80045ed8: 2c 00 10 00  	cmpwi	0, 4096
80045edc: 41 80 ff d8  	bt	0, 0x80045eb4 <_binary__mnt_data_text1_bin_start+0x3d174>
80045ee0: 38 00 00 00  	li 0, 0
80045ee4: 7f e3 fb 78  	mr	3, 31
80045ee8: 90 1f 00 04  	stw 0, 4(31)
80045eec: 83 e1 00 0c  	lwz 31, 12(1)
80045ef0: 80 01 00 14  	lwz 0, 20(1)
80045ef4: 7c 08 03 a6  	mtlr 0
80045ef8: 38 21 00 10  	addi 1, 1, 16
80045efc: 4e 80 00 20  	blr
80045f00: a0 a3 00 00  	lhz 5, 0(3)
80045f04: 38 05 00 01  	addi 0, 5, 1
80045f08: b0 03 00 00  	sth 0, 0(3)
80045f0c: a0 03 00 00  	lhz 0, 0(3)
80045f10: b0 04 00 04  	sth 0, 4(4)
80045f14: a8 03 00 02  	lha 0, 2(3)
80045f18: b0 04 00 06  	sth 0, 6(4)
80045f1c: 90 83 00 04  	stw 4, 4(3)
80045f20: 4e 80 00 20  	blr
80045f24: 80 c4 00 00  	lwz 6, 0(4)
80045f28: 80 04 00 04  	lwz 0, 4(4)
80045f2c: 90 c3 00 00  	stw 6, 0(3)
80045f30: 80 85 00 00  	lwz 4, 0(5)
80045f34: 90 03 00 04  	stw 0, 4(3)
80045f38: 80 05 00 04  	lwz 0, 4(5)
80045f3c: 90 83 00 08  	stw 4, 8(3)
80045f40: 90 03 00 0c  	stw 0, 12(3)
80045f44: 4e 80 00 20  	blr
80045f48: 94 21 ff e0  	stwu 1, -32(1)
80045f4c: 7c 08 02 a6  	mflr 0
80045f50: 38 80 00 00  	li 4, 0
80045f54: 38 a0 01 54  	li 5, 340
80045f58: 90 01 00 24  	stw 0, 36(1)
80045f5c: 38 61 00 10  	addi 3, 1, 16
80045f60: 48 00 00 59  	bl 0x80045fb8 <_binary__mnt_data_text1_bin_start+0x3d278>
80045f64: 80 81 00 10  	lwz 4, 16(1)
80045f68: 38 cd 2c f8  	addi 6, 13, 11512
80045f6c: 80 01 00 14  	lwz 0, 20(1)
80045f70: 38 61 00 08  	addi 3, 1, 8
80045f74: 90 8d 2c f8  	stw 4, 11512(13)
80045f78: 38 80 02 80  	li 4, 640
80045f7c: 38 a0 00 78  	li 5, 120
80045f80: 90 06 00 04  	stw 0, 4(6)
80045f84: 48 00 00 29  	bl 0x80045fac <_binary__mnt_data_text1_bin_start+0x3d26c>
80045f88: 80 81 00 08  	lwz 4, 8(1)
80045f8c: 38 6d 2d 00  	addi 3, 13, 11520
80045f90: 80 01 00 0c  	lwz 0, 12(1)
80045f94: 90 8d 2d 00  	stw 4, 11520(13)
80045f98: 90 03 00 04  	stw 0, 4(3)
80045f9c: 80 01 00 24  	lwz 0, 36(1)
80045fa0: 7c 08 03 a6  	mtlr 0
80045fa4: 38 21 00 20  	addi 1, 1, 32
80045fa8: 4e 80 00 20  	blr
80045fac: 90 83 00 00  	stw 4, 0(3)
80045fb0: 90 a3 00 04  	stw 5, 4(3)
80045fb4: 4e 80 00 20  	blr
80045fb8: 90 83 00 00  	stw 4, 0(3)
80045fbc: 90 a3 00 04  	stw 5, 4(3)
80045fc0: 4e 80 00 20  	blr
80045fc4: 4e 80 00 20  	blr
80045fc8: 7c 64 1a 14  	add 3, 4, 3
80045fcc: 38 04 ff ff  	addi 0, 4, -1
80045fd0: 38 63 ff ff  	addi 3, 3, -1
80045fd4: 7c 63 00 78  	andc 3, 3, 0
80045fd8: 4e 80 00 20  	blr
80045fdc: 94 21 ff f0  	stwu 1, -16(1)
80045fe0: 7c 08 02 a6  	mflr 0
80045fe4: 90 01 00 14  	stw 0, 20(1)
80045fe8: bf c1 00 08  	stmw 30, 8(1)
80045fec: 7c 7e 1b 79  	mr.	30, 3
80045ff0: 7c 9f 23 78  	mr	31, 4
80045ff4: 41 82 00 40  	bt	2, 0x80046034 <_binary__mnt_data_text1_bin_start+0x3d2f4>
80045ff8: 3c 80 80 52  	lis 4, -32686
80045ffc: 38 7e 00 28  	addi 3, 30, 40
80046000: 38 a4 e4 a4  	addi 5, 4, -7004
80046004: 38 80 00 00  	li 4, 0
80046008: 90 be 00 18  	stw 5, 24(30)
8004600c: 38 05 00 10  	addi 0, 5, 16
80046010: 90 1e 00 28  	stw 0, 40(30)
80046014: 4b ff 4d 59  	bl 0x8003ad6c <_binary__mnt_data_text1_bin_start+0x3202c>
80046018: 7f c3 f3 78  	mr	3, 30
8004601c: 38 80 00 00  	li 4, 0
\n# ===== 0x8040bc90 .. 0x8040c020 =====

/mnt/data/text1_exec.o:	file format elf32-powerpc

Disassembly of section .data:

80008d40 <_binary__mnt_data_text1_bin_start>:
8040bc90: 80 83 00 04  	lwz 4, 4(3)
8040bc94: 38 60 00 00  	li 3, 0
8040bc98: 80 84 00 00  	lwz 4, 0(4)
8040bc9c: 48 00 00 10  	b 0x8040bcac <_binary__mnt_data_text1_bin_start+0x402f6c>
8040bca0: 80 04 00 04  	lwz 0, 4(4)
8040bca4: 80 84 00 00  	lwz 4, 0(4)
8040bca8: 7c 63 02 14  	add 3, 3, 0
8040bcac: 28 04 00 00  	cmplwi	4, 0
8040bcb0: 40 82 ff f0  	bf	2, 0x8040bca0 <_binary__mnt_data_text1_bin_start+0x402f60>
8040bcb4: 4e 80 00 20  	blr
8040bcb8: 94 21 ff e0  	stwu 1, -32(1)
8040bcbc: 7c 08 02 a6  	mflr 0
8040bcc0: 90 01 00 24  	stw 0, 36(1)
8040bcc4: bf a1 00 14  	stmw 29, 20(1)
8040bcc8: 7c 9e 23 79  	mr.	30, 4
8040bccc: 7c 7d 1b 78  	mr	29, 3
8040bcd0: 90 a1 00 08  	stw 5, 8(1)
8040bcd4: 40 82 00 1c  	bf	2, 0x8040bcf0 <_binary__mnt_data_text1_bin_start+0x402fb0>
8040bcd8: 81 83 00 00  	lwz 12, 0(3)
8040bcdc: 7c a4 2b 78  	mr	4, 5
8040bce0: 81 8c 00 08  	lwz 12, 8(12)
8040bce4: 7d 89 03 a6  	mtctr 12
8040bce8: 4e 80 04 21  	bctrl
8040bcec: 48 00 00 88  	b 0x8040bd74 <_binary__mnt_data_text1_bin_start+0x403034>
8040bcf0: 80 1e ff fc  	lwz 0, -4(30)
8040bcf4: 7c 00 28 40  	cmplw	0, 5
8040bcf8: 40 82 00 0c  	bf	2, 0x8040bd04 <_binary__mnt_data_text1_bin_start+0x402fc4>
8040bcfc: 7f c3 f3 78  	mr	3, 30
8040bd00: 48 00 00 74  	b 0x8040bd74 <_binary__mnt_data_text1_bin_start+0x403034>
8040bd04: 81 83 00 00  	lwz 12, 0(3)
8040bd08: 7c a4 2b 78  	mr	4, 5
8040bd0c: 81 8c 00 08  	lwz 12, 8(12)
8040bd10: 7d 89 03 a6  	mtctr 12
8040bd14: 4e 80 04 21  	bctrl
8040bd18: 7c 7f 1b 79  	mr.	31, 3
8040bd1c: 40 82 00 0c  	bf	2, 0x8040bd28 <_binary__mnt_data_text1_bin_start+0x402fe8>
8040bd20: 38 60 00 00  	li 3, 0
8040bd24: 48 00 00 50  	b 0x8040bd74 <_binary__mnt_data_text1_bin_start+0x403034>
8040bd28: 80 9e ff fc  	lwz 4, -4(30)
8040bd2c: 38 61 00 0c  	addi 3, 1, 12
8040bd30: 80 01 00 08  	lwz 0, 8(1)
8040bd34: 38 84 ff f8  	addi 4, 4, -8
8040bd38: 7c 00 20 40  	cmplw	0, 4
8040bd3c: 90 81 00 0c  	stw 4, 12(1)
8040bd40: 40 80 00 08  	bf	0, 0x8040bd48 <_binary__mnt_data_text1_bin_start+0x403008>
8040bd44: 38 61 00 08  	addi 3, 1, 8
8040bd48: 80 a3 00 00  	lwz 5, 0(3)
8040bd4c: 7f e3 fb 78  	mr	3, 31
8040bd50: 7f c4 f3 78  	mr	4, 30
8040bd54: 4b bf 97 a1  	bl 0x800054f4 <_binary__mnt_data_text1_bin_size+0x7fb635d4>
8040bd58: 7f a3 eb 78  	mr	3, 29
8040bd5c: 7f c4 f3 78  	mr	4, 30
8040bd60: 81 9d 00 00  	lwz 12, 0(29)
8040bd64: 81 8c 00 18  	lwz 12, 24(12)
8040bd68: 7d 89 03 a6  	mtctr 12
8040bd6c: 4e 80 04 21  	bctrl
8040bd70: 7f e3 fb 78  	mr	3, 31
8040bd74: bb a1 00 14  	lmw 29, 20(1)
8040bd78: 80 01 00 24  	lwz 0, 36(1)
8040bd7c: 7c 08 03 a6  	mtlr 0
8040bd80: 38 21 00 20  	addi 1, 1, 32
8040bd84: 4e 80 00 20  	blr
8040bd88: 80 83 00 04  	lwz 4, 4(3)
8040bd8c: 38 60 00 00  	li 3, 0
8040bd90: 80 84 00 00  	lwz 4, 0(4)
8040bd94: 48 00 00 18  	b 0x8040bdac <_binary__mnt_data_text1_bin_start+0x40306c>
8040bd98: 80 04 00 04  	lwz 0, 4(4)
8040bd9c: 7c 03 00 40  	cmplw	3, 0
8040bda0: 40 80 00 08  	bf	0, 0x8040bda8 <_binary__mnt_data_text1_bin_start+0x403068>
8040bda4: 7c 03 03 78  	mr	3, 0
8040bda8: 80 84 00 00  	lwz 4, 0(4)
8040bdac: 28 04 00 00  	cmplwi	4, 0
8040bdb0: 40 82 ff e8  	bf	2, 0x8040bd98 <_binary__mnt_data_text1_bin_start+0x403058>
8040bdb4: 4e 80 00 20  	blr
8040bdb8: 28 04 00 00  	cmplwi	4, 0
8040bdbc: 4d 82 00 20  	bclr	12, 2
8040bdc0: 80 a3 00 04  	lwz 5, 4(3)
8040bdc4: 38 84 ff f8  	addi 4, 4, -8
8040bdc8: 48 00 00 08  	b 0x8040bdd0 <_binary__mnt_data_text1_bin_start+0x403090>
8040bdcc: 7c c5 33 78  	mr	5, 6
8040bdd0: 80 c5 00 00  	lwz 6, 0(5)
8040bdd4: 7c 06 20 40  	cmplw	6, 4
8040bdd8: 40 80 00 0c  	bf	0, 0x8040bde4 <_binary__mnt_data_text1_bin_start+0x4030a4>
8040bddc: 28 06 00 00  	cmplwi	6, 0
8040bde0: 40 82 ff ec  	bf	2, 0x8040bdcc <_binary__mnt_data_text1_bin_start+0x40308c>
8040bde4: 28 06 00 00  	cmplwi	6, 0
8040bde8: 41 82 00 38  	bt	2, 0x8040be20 <_binary__mnt_data_text1_bin_start+0x4030e0>
8040bdec: 80 04 00 04  	lwz 0, 4(4)
8040bdf0: 7c 04 02 14  	add 0, 4, 0
8040bdf4: 7c 00 30 40  	cmplw	0, 6
8040bdf8: 40 82 00 20  	bf	2, 0x8040be18 <_binary__mnt_data_text1_bin_start+0x4030d8>
8040bdfc: 80 06 00 00  	lwz 0, 0(6)
8040be00: 90 04 00 00  	stw 0, 0(4)
8040be04: 80 64 00 04  	lwz 3, 4(4)
8040be08: 80 06 00 04  	lwz 0, 4(6)
8040be0c: 7c 03 02 14  	add 0, 3, 0
8040be10: 90 04 00 04  	stw 0, 4(4)
8040be14: 48 00 00 14  	b 0x8040be28 <_binary__mnt_data_text1_bin_start+0x4030e8>
8040be18: 90 c4 00 00  	stw 6, 0(4)
8040be1c: 48 00 00 0c  	b 0x8040be28 <_binary__mnt_data_text1_bin_start+0x4030e8>
8040be20: 38 00 00 00  	li 0, 0
8040be24: 90 04 00 00  	stw 0, 0(4)
8040be28: 80 05 00 04  	lwz 0, 4(5)
8040be2c: 7c 05 02 14  	add 0, 5, 0
8040be30: 7c 00 20 40  	cmplw	0, 4
8040be34: 40 82 00 20  	bf	2, 0x8040be54 <_binary__mnt_data_text1_bin_start+0x403114>
8040be38: 80 04 00 00  	lwz 0, 0(4)
8040be3c: 90 05 00 00  	stw 0, 0(5)
8040be40: 80 65 00 04  	lwz 3, 4(5)
8040be44: 80 04 00 04  	lwz 0, 4(4)
8040be48: 7c 03 02 14  	add 0, 3, 0
8040be4c: 90 05 00 04  	stw 0, 4(5)
8040be50: 4e 80 00 20  	blr
8040be54: 90 85 00 00  	stw 4, 0(5)
8040be58: 4e 80 00 20  	blr
8040be5c: 4e 80 00 20  	blr
8040be60: 94 21 ff f0  	stwu 1, -16(1)
8040be64: 7c 08 02 a6  	mflr 0
8040be68: 90 01 00 14  	stw 0, 20(1)
8040be6c: bf c1 00 08  	stmw 30, 8(1)
8040be70: 7c 9e 23 78  	mr	30, 4
8040be74: 81 83 00 00  	lwz 12, 0(3)
8040be78: 81 8c 00 08  	lwz 12, 8(12)
8040be7c: 7d 89 03 a6  	mtctr 12
8040be80: 4e 80 04 21  	bctrl
8040be84: 7c 7f 1b 79  	mr.	31, 3
8040be88: 41 82 00 10  	bt	2, 0x8040be98 <_binary__mnt_data_text1_bin_start+0x403158>
8040be8c: 7f c5 f3 78  	mr	5, 30
8040be90: 38 80 00 00  	li 4, 0
8040be94: 4b bf 95 79  	bl 0x8000540c <_binary__mnt_data_text1_bin_size+0x7fb634ec>
8040be98: 7f e3 fb 78  	mr	3, 31
8040be9c: bb c1 00 08  	lmw 30, 8(1)
8040bea0: 80 01 00 14  	lwz 0, 20(1)
8040bea4: 7c 08 03 a6  	mtlr 0
8040bea8: 38 21 00 10  	addi 1, 1, 16
8040beac: 4e 80 00 20  	blr
8040beb0: 94 21 ff e0  	stwu 1, -32(1)
8040beb4: 7c 08 02 a6  	mflr 0
8040beb8: 90 01 00 24  	stw 0, 36(1)
8040bebc: bf 81 00 10  	stmw 28, 16(1)
8040bec0: 7c 7d 1b 78  	mr	29, 3
8040bec4: 7c 9c 23 78  	mr	28, 4
8040bec8: 38 65 00 08  	addi 3, 5, 8
8040becc: 38 80 00 08  	li 4, 8
8040bed0: 4b c3 a0 f9  	bl 0x80045fc8 <_binary__mnt_data_text1_bin_start+0x3d288>
8040bed4: 83 fd 00 04  	lwz 31, 4(29)
8040bed8: 7f bc 1a 14  	add 29, 28, 3
8040bedc: 48 00 00 a8  	b 0x8040bf84 <_binary__mnt_data_text1_bin_start+0x403244>
8040bee0: 80 1e 00 04  	lwz 0, 4(30)
8040bee4: 7c 00 e8 40  	cmplw	0, 29
8040bee8: 41 80 00 98  	bt	0, 0x8040bf80 <_binary__mnt_data_text1_bin_start+0x403240>
8040beec: 7f 84 e3 78  	mr	4, 28
8040bef0: 38 7e 00 08  	addi 3, 30, 8
8040bef4: 4b c3 a0 d5  	bl 0x80045fc8 <_binary__mnt_data_text1_bin_start+0x3d288>
8040bef8: 80 1e 00 04  	lwz 0, 4(30)
8040befc: 38 83 ff f8  	addi 4, 3, -8
8040bf00: 7c 1d 00 40  	cmplw	29, 0
8040bf04: 40 82 00 30  	bf	2, 0x8040bf34 <_binary__mnt_data_text1_bin_start+0x4031f4>
8040bf08: 7c 04 f0 40  	cmplw	4, 30
8040bf0c: 40 82 00 10  	bf	2, 0x8040bf1c <_binary__mnt_data_text1_bin_start+0x4031dc>
8040bf10: 80 1e 00 00  	lwz 0, 0(30)
8040bf14: 90 1f 00 00  	stw 0, 0(31)
8040bf18: 48 00 00 60  	b 0x8040bf78 <_binary__mnt_data_text1_bin_start+0x403238>
8040bf1c: 7c 1e 20 50  	sub	0, 4, 30
8040bf20: 90 1e 00 04  	stw 0, 4(30)
8040bf24: 80 1e 00 04  	lwz 0, 4(30)
8040bf28: 7c 00 e8 50  	sub	0, 29, 0
8040bf2c: 90 04 00 04  	stw 0, 4(4)
8040bf30: 48 00 00 48  	b 0x8040bf78 <_binary__mnt_data_text1_bin_start+0x403238>
8040bf34: 80 1e 00 00  	lwz 0, 0(30)
8040bf38: 7c 7e ea 14  	add 3, 30, 29
8040bf3c: 7c 04 f0 40  	cmplw	4, 30
8040bf40: 90 03 00 00  	stw 0, 0(3)
8040bf44: 80 1e 00 04  	lwz 0, 4(30)
8040bf48: 7c 1d 00 50  	sub	0, 0, 29
8040bf4c: 90 03 00 04  	stw 0, 4(3)
8040bf50: 40 82 00 10  	bf	2, 0x8040bf60 <_binary__mnt_data_text1_bin_start+0x403220>
8040bf54: 93 be 00 04  	stw 29, 4(30)
8040bf58: 90 7f 00 00  	stw 3, 0(31)
8040bf5c: 48 00 00 1c  	b 0x8040bf78 <_binary__mnt_data_text1_bin_start+0x403238>
8040bf60: 7c 1e 20 50  	sub	0, 4, 30
8040bf64: 90 1e 00 04  	stw 0, 4(30)
8040bf68: 90 7e 00 00  	stw 3, 0(30)
8040bf6c: 80 1e 00 04  	lwz 0, 4(30)
8040bf70: 7c 00 e8 50  	sub	0, 29, 0
8040bf74: 90 04 00 04  	stw 0, 4(4)
8040bf78: 38 64 00 08  	addi 3, 4, 8
8040bf7c: 48 00 00 18  	b 0x8040bf94 <_binary__mnt_data_text1_bin_start+0x403254>
8040bf80: 7f df f3 78  	mr	31, 30
8040bf84: 83 df 00 00  	lwz 30, 0(31)
8040bf88: 28 1e 00 00  	cmplwi	30, 0
8040bf8c: 40 82 ff 54  	bf	2, 0x8040bee0 <_binary__mnt_data_text1_bin_start+0x4031a0>
8040bf90: 38 60 00 00  	li 3, 0
8040bf94: bb 81 00 10  	lmw 28, 16(1)
8040bf98: 80 01 00 24  	lwz 0, 36(1)
8040bf9c: 7c 08 03 a6  	mtlr 0
8040bfa0: 38 21 00 20  	addi 1, 1, 32
8040bfa4: 4e 80 00 20  	blr
8040bfa8: 94 21 ff f0  	stwu 1, -16(1)
8040bfac: 7c 08 02 a6  	mflr 0
8040bfb0: 90 01 00 14  	stw 0, 20(1)
8040bfb4: 93 e1 00 0c  	stw 31, 12(1)
8040bfb8: 7c 7f 1b 78  	mr	31, 3
8040bfbc: 38 64 00 08  	addi 3, 4, 8
8040bfc0: 38 80 00 08  	li 4, 8
8040bfc4: 4b c3 a0 05  	bl 0x80045fc8 <_binary__mnt_data_text1_bin_start+0x3d288>
8040bfc8: 80 9f 00 04  	lwz 4, 4(31)
8040bfcc: 48 00 00 50  	b 0x8040c01c <_binary__mnt_data_text1_bin_start+0x4032dc>
8040bfd0: 80 05 00 04  	lwz 0, 4(5)
8040bfd4: 7c 00 18 40  	cmplw	0, 3
8040bfd8: 41 80 00 40  	bt	0, 0x8040c018 <_binary__mnt_data_text1_bin_start+0x4032d8>
8040bfdc: 7c 03 00 40  	cmplw	3, 0
8040bfe0: 40 82 00 10  	bf	2, 0x8040bff0 <_binary__mnt_data_text1_bin_start+0x4032b0>
8040bfe4: 80 05 00 00  	lwz 0, 0(5)
8040bfe8: 90 04 00 00  	stw 0, 0(4)
8040bfec: 48 00 00 24  	b 0x8040c010 <_binary__mnt_data_text1_bin_start+0x4032d0>
8040bff0: 80 05 00 00  	lwz 0, 0(5)
8040bff4: 7c c5 1a 14  	add 6, 5, 3
8040bff8: 90 06 00 00  	stw 0, 0(6)
8040bffc: 80 05 00 04  	lwz 0, 4(5)
8040c000: 7c 03 00 50  	sub	0, 0, 3
8040c004: 90 06 00 04  	stw 0, 4(6)
8040c008: 90 65 00 04  	stw 3, 4(5)
8040c00c: 90 c4 00 00  	stw 6, 0(4)
8040c010: 38 65 00 08  	addi 3, 5, 8
8040c014: 48 00 00 18  	b 0x8040c02c <_binary__mnt_data_text1_bin_start+0x4032ec>
8040c018: 7c a4 2b 78  	mr	4, 5
8040c01c: 80 a4 00 00  	lwz 5, 0(4)
