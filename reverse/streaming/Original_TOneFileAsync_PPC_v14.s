# ===== TOneFileAsync<RpDMorphAnimation>::StateMachine [8004AE60..8004B11C) =====
8004ae60:      	stwu 1, -16(1)
8004ae64:      	mflr 0
8004ae68:      	stw 0, 20(1)
8004ae6c:      	stmw 30, 8(1)
8004ae70:      	mr	31, 3
8004ae74:      	mr	30, 4
8004ae78:      	lwz 0, 40(3)
8004ae7c:      	cmpwi	0, 2
8004ae80:      	bt	2, 0x8004b010 <_binary__mnt_data_text1_bin_start+0x422d0>
8004ae84:      	bf	0, 0x8004ae98 <_binary__mnt_data_text1_bin_start+0x42158>
8004ae88:      	cmpwi	0, 0
8004ae8c:      	bt	2, 0x8004aea8 <_binary__mnt_data_text1_bin_start+0x42168>
8004ae90:      	bf	0, 0x8004afc4 <_binary__mnt_data_text1_bin_start+0x42284>
8004ae94:      	b 0x8004b114 <_binary__mnt_data_text1_bin_start+0x423d4>
8004ae98:      	cmpwi	0, 4
8004ae9c:      	bt	2, 0x8004b078 <_binary__mnt_data_text1_bin_start+0x42338>
8004aea0:      	bf	0, 0x8004b114 <_binary__mnt_data_text1_bin_start+0x423d4>
8004aea4:      	b 0x8004b060 <_binary__mnt_data_text1_bin_start+0x42320>
8004aea8:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004aeac:      	bl 0x8004b13c <_binary__mnt_data_text1_bin_start+0x423fc>
8004aeb0:      	cmpwi	3, 16
8004aeb4:      	bt	0, 0x8004aec0 <_binary__mnt_data_text1_bin_start+0x42180>
8004aeb8:      	li 3, 0
8004aebc:      	b 0x8004b118 <_binary__mnt_data_text1_bin_start+0x423d8>
8004aec0:      	lwz 3, 64(31)
8004aec4:      	lwz 4, 52(31)
8004aec8:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004aecc:      	lwz 3, 44(3)
8004aed0:      	li 4, 2
8004aed4:      	addi 0, 3, 16415
8004aed8:      	rlwinm 3, 0, 0, 0, 26
8004aedc:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004aee0:      	stw 3, 56(31)
8004aee4:      	lwz 3, 56(31)
8004aee8:      	cmplwi	3, 0
8004aeec:      	bt	2, 0x8004af18 <_binary__mnt_data_text1_bin_start+0x421d8>
8004aef0:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004aef4:      	lwz 3, 64(31)
8004aef8:      	lwz 4, 52(31)
8004aefc:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004af00:      	lwz 3, 44(3)
8004af04:      	li 4, 2
8004af08:      	addi 0, 3, 31
8004af0c:      	rlwinm 3, 0, 0, 0, 26
8004af10:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004af14:      	stw 3, 56(31)
8004af18:      	lwz 0, 56(31)
8004af1c:      	cmplwi	0, 0
8004af20:      	bf	2, 0x8004af48 <_binary__mnt_data_text1_bin_start+0x42208>
8004af24:      	lwz 3, 64(31)
8004af28:      	lwz 4, 52(31)
8004af2c:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004af30:      	lwz 3, 44(3)
8004af34:      	li 4, 1
8004af38:      	addi 0, 3, 31
8004af3c:      	rlwinm 3, 0, 0, 0, 26
8004af40:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004af44:      	stw 3, 56(31)
8004af48:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004af4c:      	lwz 5, 64(31)
8004af50:      	mr	4, 31
8004af54:      	lwz 6, 52(31)
8004af58:      	bl 0x8004d564 <_binary__mnt_data_text1_bin_start+0x44824>
8004af5c:      	lwz 0, 44(31)
8004af60:      	cmplwi	0, 0
8004af64:      	bt	2, 0x8004af90 <_binary__mnt_data_text1_bin_start+0x42250>
8004af68:      	lis 4, -32763
8004af6c:      	lwz 3, 64(31)
8004af70:      	addi 6, 4, -20180
8004af74:      	lwz 4, 52(31)
8004af78:      	lwz 5, 56(31)
8004af7c:      	mr	7, 31
8004af80:      	bl 0x8004c770 <_binary__mnt_data_text1_bin_start+0x43a30>
8004af84:      	li 0, 1
8004af88:      	stb 0, 80(31)
8004af8c:      	b 0x8004afb4 <_binary__mnt_data_text1_bin_start+0x42274>
8004af90:      	lwz 3, 64(31)
8004af94:      	li 6, 0
8004af98:      	lwz 4, 52(31)
8004af9c:      	li 7, 0
8004afa0:      	lwz 5, 56(31)
8004afa4:      	bl 0x8004c770 <_binary__mnt_data_text1_bin_start+0x43a30>
8004afa8:      	stw 3, 72(31)
8004afac:      	li 0, 0
8004afb0:      	stb 0, 80(31)
8004afb4:      	li 0, 1
8004afb8:      	li 3, 0
8004afbc:      	stw 0, 40(31)
8004afc0:      	b 0x8004b118 <_binary__mnt_data_text1_bin_start+0x423d8>
8004afc4:      	lbz 0, 80(31)
8004afc8:      	cmplwi	0, 0
8004afcc:      	bf	2, 0x8004b008 <_binary__mnt_data_text1_bin_start+0x422c8>
8004afd0:      	lwz 0, 72(31)
8004afd4:      	cmplwi	0, 0
8004afd8:      	bf	2, 0x8004aff4 <_binary__mnt_data_text1_bin_start+0x422b4>
8004afdc:      	lwz 3, 56(31)
8004afe0:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004afe4:      	li 0, 4
8004afe8:      	li 3, 0
8004afec:      	stw 0, 40(31)
8004aff0:      	b 0x8004b118 <_binary__mnt_data_text1_bin_start+0x423d8>
8004aff4:      	li 0, 2
8004aff8:      	stw 0, 40(31)
8004affc:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004b000:      	mr	4, 31
8004b004:      	bl 0x8004d424 <_binary__mnt_data_text1_bin_start+0x446e4>
8004b008:      	li 3, 0
8004b00c:      	b 0x8004b118 <_binary__mnt_data_text1_bin_start+0x423d8>
8004b010:      	lwz 0, 56(31)
8004b014:      	addi 5, 31, 68
8004b018:      	li 3, 3
8004b01c:      	li 4, 1
8004b020:      	stw 0, 68(31)
8004b024:      	bl 0x804823b8 <_binary__mnt_data_text1_bin_start+0x479678>
8004b028:      	stw 3, 76(31)
8004b02c:      	li 4, 30
8004b030:      	li 5, 0
8004b034:      	li 6, 0
8004b038:      	lwz 3, 76(31)
8004b03c:      	bl 0x8047dfa0 <_binary__mnt_data_text1_bin_start+0x475260>
8004b040:      	cmpwi	3, 0
8004b044:      	bt	2, 0x8004b054 <_binary__mnt_data_text1_bin_start+0x42314>
8004b048:      	lwz 3, 76(31)
8004b04c:      	bl 0x80443c74 <_binary__mnt_data_text1_bin_start+0x43af34>
8004b050:      	stw 3, 60(31)
8004b054:      	li 0, 4
8004b058:      	stw 0, 40(31)
8004b05c:      	b 0x8004b114 <_binary__mnt_data_text1_bin_start+0x423d4>
8004b060:      	lbz 0, 88(31)
8004b064:      	cmplwi	0, 0
8004b068:      	bt	2, 0x8004b114 <_binary__mnt_data_text1_bin_start+0x423d4>
8004b06c:      	li 0, 4
8004b070:      	stw 0, 40(31)
8004b074:      	b 0x8004b114 <_binary__mnt_data_text1_bin_start+0x423d4>
8004b078:      	lwz 3, -31560(13)
8004b07c:      	lwz 4, -31600(13)
8004b080:      	bl 0x803ad96c <_binary__mnt_data_text1_bin_start+0x3a4c2c>
8004b084:      	cmpwi	3, 0
8004b088:      	bt	2, 0x8004b0a4 <_binary__mnt_data_text1_bin_start+0x42364>
8004b08c:      	lwz 3, 56(31)
8004b090:      	cmplwi	3, 0
8004b094:      	bt	2, 0x8004b0a4 <_binary__mnt_data_text1_bin_start+0x42364>
8004b098:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004b09c:      	li 0, 0
8004b0a0:      	stw 0, 56(31)
8004b0a4:      	lwz 3, 76(31)
8004b0a8:      	cmplwi	3, 0
8004b0ac:      	bt	2, 0x8004b0b8 <_binary__mnt_data_text1_bin_start+0x42378>
8004b0b0:      	li 4, 0
8004b0b4:      	bl 0x8048229c <_binary__mnt_data_text1_bin_start+0x47955c>
8004b0b8:      	cmplwi	30, 0
8004b0bc:      	bt	2, 0x8004b0c8 <_binary__mnt_data_text1_bin_start+0x42388>
8004b0c0:      	lwz 0, 60(31)
8004b0c4:      	stw 0, 0(30)
8004b0c8:      	lwz 12, 44(31)
8004b0cc:      	cmplwi	12, 0
8004b0d0:      	bt	2, 0x8004b100 <_binary__mnt_data_text1_bin_start+0x423c0>
8004b0d4:      	lwz 3, 60(31)
8004b0d8:      	lwz 4, 48(31)
8004b0dc:      	mtctr 12
8004b0e0:      	bctrl
8004b0e4:      	lwz 4, 64(31)
8004b0e8:      	lwz 3, 84(4)
8004b0ec:      	addi 0, 3, -1
8004b0f0:      	stw 0, 84(4)
8004b0f4:      	lwz 3, 11572(13)
8004b0f8:      	addi 0, 3, -1
8004b0fc:      	stw 0, 11572(13)
8004b100:      	lhz 0, 4(31)
8004b104:      	li 3, 1
8004b108:      	ori 0, 0, 1
8004b10c:      	sth 0, 4(31)
8004b110:      	b 0x8004b118 <_binary__mnt_data_text1_bin_start+0x423d8>
8004b114:      	li 3, 0
8004b118:      	lmw 30, 8(1)

# ===== TOneFileAsync<RpWorld>::StateMachine [8004B3A0..8004B668) =====
8004b3a0:      	stwu 1, -16(1)
8004b3a4:      	mflr 0
8004b3a8:      	stw 0, 20(1)
8004b3ac:      	stmw 30, 8(1)
8004b3b0:      	mr	31, 3
8004b3b4:      	mr	30, 4
8004b3b8:      	lwz 0, 40(3)
8004b3bc:      	cmpwi	0, 2
8004b3c0:      	bt	2, 0x8004b550 <_binary__mnt_data_text1_bin_start+0x42810>
8004b3c4:      	bf	0, 0x8004b3d8 <_binary__mnt_data_text1_bin_start+0x42698>
8004b3c8:      	cmpwi	0, 0
8004b3cc:      	bt	2, 0x8004b3e8 <_binary__mnt_data_text1_bin_start+0x426a8>
8004b3d0:      	bf	0, 0x8004b504 <_binary__mnt_data_text1_bin_start+0x427c4>
8004b3d4:      	b 0x8004b660 <_binary__mnt_data_text1_bin_start+0x42920>
8004b3d8:      	cmpwi	0, 4
8004b3dc:      	bt	2, 0x8004b5c4 <_binary__mnt_data_text1_bin_start+0x42884>
8004b3e0:      	bf	0, 0x8004b660 <_binary__mnt_data_text1_bin_start+0x42920>
8004b3e4:      	b 0x8004b5ac <_binary__mnt_data_text1_bin_start+0x4286c>
8004b3e8:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004b3ec:      	bl 0x8004b13c <_binary__mnt_data_text1_bin_start+0x423fc>
8004b3f0:      	cmpwi	3, 16
8004b3f4:      	bt	0, 0x8004b400 <_binary__mnt_data_text1_bin_start+0x426c0>
8004b3f8:      	li 3, 0
8004b3fc:      	b 0x8004b664 <_binary__mnt_data_text1_bin_start+0x42924>
8004b400:      	lwz 3, 64(31)
8004b404:      	lwz 4, 52(31)
8004b408:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004b40c:      	lwz 3, 44(3)
8004b410:      	li 4, 2
8004b414:      	addi 0, 3, 16415
8004b418:      	rlwinm 3, 0, 0, 0, 26
8004b41c:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004b420:      	stw 3, 56(31)
8004b424:      	lwz 3, 56(31)
8004b428:      	cmplwi	3, 0
8004b42c:      	bt	2, 0x8004b458 <_binary__mnt_data_text1_bin_start+0x42718>
8004b430:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004b434:      	lwz 3, 64(31)
8004b438:      	lwz 4, 52(31)
8004b43c:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004b440:      	lwz 3, 44(3)
8004b444:      	li 4, 2
8004b448:      	addi 0, 3, 31
8004b44c:      	rlwinm 3, 0, 0, 0, 26
8004b450:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004b454:      	stw 3, 56(31)
8004b458:      	lwz 0, 56(31)
8004b45c:      	cmplwi	0, 0
8004b460:      	bf	2, 0x8004b488 <_binary__mnt_data_text1_bin_start+0x42748>
8004b464:      	lwz 3, 64(31)
8004b468:      	lwz 4, 52(31)
8004b46c:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004b470:      	lwz 3, 44(3)
8004b474:      	li 4, 1
8004b478:      	addi 0, 3, 31
8004b47c:      	rlwinm 3, 0, 0, 0, 26
8004b480:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004b484:      	stw 3, 56(31)
8004b488:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004b48c:      	lwz 5, 64(31)
8004b490:      	mr	4, 31
8004b494:      	lwz 6, 52(31)
8004b498:      	bl 0x8004d564 <_binary__mnt_data_text1_bin_start+0x44824>
8004b49c:      	lwz 0, 44(31)
8004b4a0:      	cmplwi	0, 0
8004b4a4:      	bt	2, 0x8004b4d0 <_binary__mnt_data_text1_bin_start+0x42790>
8004b4a8:      	lis 4, -32763
8004b4ac:      	lwz 3, 64(31)
8004b4b0:      	addi 6, 4, -18824
8004b4b4:      	lwz 4, 52(31)
8004b4b8:      	lwz 5, 56(31)
8004b4bc:      	mr	7, 31
8004b4c0:      	bl 0x8004c770 <_binary__mnt_data_text1_bin_start+0x43a30>
8004b4c4:      	li 0, 1
8004b4c8:      	stb 0, 80(31)
8004b4cc:      	b 0x8004b4f4 <_binary__mnt_data_text1_bin_start+0x427b4>
8004b4d0:      	lwz 3, 64(31)
8004b4d4:      	li 6, 0
8004b4d8:      	lwz 4, 52(31)
8004b4dc:      	li 7, 0
8004b4e0:      	lwz 5, 56(31)
8004b4e4:      	bl 0x8004c770 <_binary__mnt_data_text1_bin_start+0x43a30>
8004b4e8:      	stw 3, 72(31)
8004b4ec:      	li 0, 0
8004b4f0:      	stb 0, 80(31)
8004b4f4:      	li 0, 1
8004b4f8:      	li 3, 0
8004b4fc:      	stw 0, 40(31)
8004b500:      	b 0x8004b664 <_binary__mnt_data_text1_bin_start+0x42924>
8004b504:      	lbz 0, 80(31)
8004b508:      	cmplwi	0, 0
8004b50c:      	bf	2, 0x8004b548 <_binary__mnt_data_text1_bin_start+0x42808>
8004b510:      	lwz 0, 72(31)
8004b514:      	cmplwi	0, 0
8004b518:      	bf	2, 0x8004b534 <_binary__mnt_data_text1_bin_start+0x427f4>
8004b51c:      	lwz 3, 56(31)
8004b520:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004b524:      	li 0, 4
8004b528:      	li 3, 0
8004b52c:      	stw 0, 40(31)
8004b530:      	b 0x8004b664 <_binary__mnt_data_text1_bin_start+0x42924>
8004b534:      	li 0, 2
8004b538:      	stw 0, 40(31)
8004b53c:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004b540:      	mr	4, 31
8004b544:      	bl 0x8004d424 <_binary__mnt_data_text1_bin_start+0x446e4>
8004b548:      	li 3, 0
8004b54c:      	b 0x8004b664 <_binary__mnt_data_text1_bin_start+0x42924>
8004b550:      	lwz 0, 56(31)
8004b554:      	addi 5, 31, 68
8004b558:      	li 3, 3
8004b55c:      	li 4, 1
8004b560:      	stw 0, 68(31)
8004b564:      	bl 0x804823b8 <_binary__mnt_data_text1_bin_start+0x479678>
8004b568:      	stw 3, 76(31)
8004b56c:      	li 4, 11
8004b570:      	li 5, 0
8004b574:      	li 6, 0
8004b578:      	lwz 30, 84(31)
8004b57c:      	lwz 3, 76(31)
8004b580:      	bl 0x8047dfa0 <_binary__mnt_data_text1_bin_start+0x475260>
8004b584:      	cmpwi	3, 0
8004b588:      	bt	2, 0x8004b5a0 <_binary__mnt_data_text1_bin_start+0x42860>
8004b58c:      	mr	3, 30
8004b590:      	bl 0x8048e114 <_binary__mnt_data_text1_bin_start+0x4853d4>
8004b594:      	lwz 3, 76(31)
8004b598:      	bl 0x80456744 <_binary__mnt_data_text1_bin_start+0x44da04>
8004b59c:      	stw 3, 60(31)
8004b5a0:      	li 0, 4
8004b5a4:      	stw 0, 40(31)
8004b5a8:      	b 0x8004b660 <_binary__mnt_data_text1_bin_start+0x42920>
8004b5ac:      	lbz 0, 88(31)
8004b5b0:      	cmplwi	0, 0
8004b5b4:      	bt	2, 0x8004b660 <_binary__mnt_data_text1_bin_start+0x42920>
8004b5b8:      	li 0, 4
8004b5bc:      	stw 0, 40(31)
8004b5c0:      	b 0x8004b660 <_binary__mnt_data_text1_bin_start+0x42920>
8004b5c4:      	lwz 3, -31568(13)
8004b5c8:      	lwz 4, -31600(13)
8004b5cc:      	bl 0x803ad96c <_binary__mnt_data_text1_bin_start+0x3a4c2c>
8004b5d0:      	cmpwi	3, 0
8004b5d4:      	bt	2, 0x8004b5f0 <_binary__mnt_data_text1_bin_start+0x428b0>
8004b5d8:      	lwz 3, 56(31)
8004b5dc:      	cmplwi	3, 0
8004b5e0:      	bt	2, 0x8004b5f0 <_binary__mnt_data_text1_bin_start+0x428b0>
8004b5e4:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004b5e8:      	li 0, 0
8004b5ec:      	stw 0, 56(31)
8004b5f0:      	lwz 3, 76(31)
8004b5f4:      	cmplwi	3, 0
8004b5f8:      	bt	2, 0x8004b604 <_binary__mnt_data_text1_bin_start+0x428c4>
8004b5fc:      	li 4, 0
8004b600:      	bl 0x8048229c <_binary__mnt_data_text1_bin_start+0x47955c>
8004b604:      	cmplwi	30, 0
8004b608:      	bt	2, 0x8004b614 <_binary__mnt_data_text1_bin_start+0x428d4>
8004b60c:      	lwz 0, 60(31)
8004b610:      	stw 0, 0(30)
8004b614:      	lwz 12, 44(31)
8004b618:      	cmplwi	12, 0
8004b61c:      	bt	2, 0x8004b64c <_binary__mnt_data_text1_bin_start+0x4290c>
8004b620:      	lwz 3, 60(31)
8004b624:      	lwz 4, 48(31)
8004b628:      	mtctr 12
8004b62c:      	bctrl
8004b630:      	lwz 4, 64(31)
8004b634:      	lwz 3, 84(4)
8004b638:      	addi 0, 3, -1
8004b63c:      	stw 0, 84(4)
8004b640:      	lwz 3, 11572(13)
8004b644:      	addi 0, 3, -1
8004b648:      	stw 0, 11572(13)
8004b64c:      	lhz 0, 4(31)
8004b650:      	li 3, 1
8004b654:      	ori 0, 0, 1
8004b658:      	sth 0, 4(31)
8004b65c:      	b 0x8004b664 <_binary__mnt_data_text1_bin_start+0x42924>
8004b660:      	li 3, 0
8004b664:      	lmw 30, 8(1)

# ===== TOneFileAsync<RtDict>::StateMachine [8004B7BC..8004BA80) =====
8004b7bc:      	stwu 1, -16(1)
8004b7c0:      	mflr 0
8004b7c4:      	stw 0, 20(1)
8004b7c8:      	stmw 30, 8(1)
8004b7cc:      	mr	31, 3
8004b7d0:      	mr	30, 4
8004b7d4:      	lwz 0, 40(3)
8004b7d8:      	cmpwi	0, 2
8004b7dc:      	bt	2, 0x8004b96c <_binary__mnt_data_text1_bin_start+0x42c2c>
8004b7e0:      	bf	0, 0x8004b7f4 <_binary__mnt_data_text1_bin_start+0x42ab4>
8004b7e4:      	cmpwi	0, 0
8004b7e8:      	bt	2, 0x8004b804 <_binary__mnt_data_text1_bin_start+0x42ac4>
8004b7ec:      	bf	0, 0x8004b920 <_binary__mnt_data_text1_bin_start+0x42be0>
8004b7f0:      	b 0x8004ba78 <_binary__mnt_data_text1_bin_start+0x42d38>
8004b7f4:      	cmpwi	0, 4
8004b7f8:      	bt	2, 0x8004b9dc <_binary__mnt_data_text1_bin_start+0x42c9c>
8004b7fc:      	bf	0, 0x8004ba78 <_binary__mnt_data_text1_bin_start+0x42d38>
8004b800:      	b 0x8004b9c4 <_binary__mnt_data_text1_bin_start+0x42c84>
8004b804:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004b808:      	bl 0x8004b13c <_binary__mnt_data_text1_bin_start+0x423fc>
8004b80c:      	cmpwi	3, 16
8004b810:      	bt	0, 0x8004b81c <_binary__mnt_data_text1_bin_start+0x42adc>
8004b814:      	li 3, 0
8004b818:      	b 0x8004ba7c <_binary__mnt_data_text1_bin_start+0x42d3c>
8004b81c:      	lwz 3, 64(31)
8004b820:      	lwz 4, 52(31)
8004b824:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004b828:      	lwz 3, 44(3)
8004b82c:      	li 4, 2
8004b830:      	addi 0, 3, 16415
8004b834:      	rlwinm 3, 0, 0, 0, 26
8004b838:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004b83c:      	stw 3, 56(31)
8004b840:      	lwz 3, 56(31)
8004b844:      	cmplwi	3, 0
8004b848:      	bt	2, 0x8004b874 <_binary__mnt_data_text1_bin_start+0x42b34>
8004b84c:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004b850:      	lwz 3, 64(31)
8004b854:      	lwz 4, 52(31)
8004b858:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004b85c:      	lwz 3, 44(3)
8004b860:      	li 4, 2
8004b864:      	addi 0, 3, 31
8004b868:      	rlwinm 3, 0, 0, 0, 26
8004b86c:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004b870:      	stw 3, 56(31)
8004b874:      	lwz 0, 56(31)
8004b878:      	cmplwi	0, 0
8004b87c:      	bf	2, 0x8004b8a4 <_binary__mnt_data_text1_bin_start+0x42b64>
8004b880:      	lwz 3, 64(31)
8004b884:      	lwz 4, 52(31)
8004b888:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004b88c:      	lwz 3, 44(3)
8004b890:      	li 4, 1
8004b894:      	addi 0, 3, 31
8004b898:      	rlwinm 3, 0, 0, 0, 26
8004b89c:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004b8a0:      	stw 3, 56(31)
8004b8a4:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004b8a8:      	lwz 5, 64(31)
8004b8ac:      	mr	4, 31
8004b8b0:      	lwz 6, 52(31)
8004b8b4:      	bl 0x8004d564 <_binary__mnt_data_text1_bin_start+0x44824>
8004b8b8:      	lwz 0, 44(31)
8004b8bc:      	cmplwi	0, 0
8004b8c0:      	bt	2, 0x8004b8ec <_binary__mnt_data_text1_bin_start+0x42bac>
8004b8c4:      	lis 4, -32763
8004b8c8:      	lwz 3, 64(31)
8004b8cc:      	addi 6, 4, -17776
8004b8d0:      	lwz 4, 52(31)
8004b8d4:      	lwz 5, 56(31)
8004b8d8:      	mr	7, 31
8004b8dc:      	bl 0x8004c770 <_binary__mnt_data_text1_bin_start+0x43a30>
8004b8e0:      	li 0, 1
8004b8e4:      	stb 0, 80(31)
8004b8e8:      	b 0x8004b910 <_binary__mnt_data_text1_bin_start+0x42bd0>
8004b8ec:      	lwz 3, 64(31)
8004b8f0:      	li 6, 0
8004b8f4:      	lwz 4, 52(31)
8004b8f8:      	li 7, 0
8004b8fc:      	lwz 5, 56(31)
8004b900:      	bl 0x8004c770 <_binary__mnt_data_text1_bin_start+0x43a30>
8004b904:      	stw 3, 72(31)
8004b908:      	li 0, 0
8004b90c:      	stb 0, 80(31)
8004b910:      	li 0, 1
8004b914:      	li 3, 0
8004b918:      	stw 0, 40(31)
8004b91c:      	b 0x8004ba7c <_binary__mnt_data_text1_bin_start+0x42d3c>
8004b920:      	lbz 0, 80(31)
8004b924:      	cmplwi	0, 0
8004b928:      	bf	2, 0x8004b964 <_binary__mnt_data_text1_bin_start+0x42c24>
8004b92c:      	lwz 0, 72(31)
8004b930:      	cmplwi	0, 0
8004b934:      	bf	2, 0x8004b950 <_binary__mnt_data_text1_bin_start+0x42c10>
8004b938:      	lwz 3, 56(31)
8004b93c:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004b940:      	li 0, 4
8004b944:      	li 3, 0
8004b948:      	stw 0, 40(31)
8004b94c:      	b 0x8004ba7c <_binary__mnt_data_text1_bin_start+0x42d3c>
8004b950:      	li 0, 2
8004b954:      	stw 0, 40(31)
8004b958:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004b95c:      	mr	4, 31
8004b960:      	bl 0x8004d424 <_binary__mnt_data_text1_bin_start+0x446e4>
8004b964:      	li 3, 0
8004b968:      	b 0x8004ba7c <_binary__mnt_data_text1_bin_start+0x42d3c>
8004b96c:      	lwz 0, 56(31)
8004b970:      	addi 5, 31, 68
8004b974:      	li 3, 3
8004b978:      	li 4, 1
8004b97c:      	stw 0, 68(31)
8004b980:      	bl 0x804823b8 <_binary__mnt_data_text1_bin_start+0x479678>
8004b984:      	stw 3, 76(31)
8004b988:      	li 4, 43
8004b98c:      	li 5, 0
8004b990:      	li 6, 0
8004b994:      	lwz 3, 76(31)
8004b998:      	bl 0x8047dfa0 <_binary__mnt_data_text1_bin_start+0x475260>
8004b99c:      	cmpwi	3, 0
8004b9a0:      	bt	2, 0x8004b9b8 <_binary__mnt_data_text1_bin_start+0x42c78>
8004b9a4:      	lis 3, -32681
8004b9a8:      	lwz 4, 76(31)
8004b9ac:      	addi 3, 3, -6136
8004b9b0:      	bl 0x8047a2d4 <_binary__mnt_data_text1_bin_start+0x471594>
8004b9b4:      	stw 3, 60(31)
8004b9b8:      	li 0, 4
8004b9bc:      	stw 0, 40(31)
8004b9c0:      	b 0x8004ba78 <_binary__mnt_data_text1_bin_start+0x42d38>
8004b9c4:      	lbz 0, 88(31)
8004b9c8:      	cmplwi	0, 0
8004b9cc:      	bt	2, 0x8004ba78 <_binary__mnt_data_text1_bin_start+0x42d38>
8004b9d0:      	li 0, 4
8004b9d4:      	stw 0, 40(31)
8004b9d8:      	b 0x8004ba78 <_binary__mnt_data_text1_bin_start+0x42d38>
8004b9dc:      	lwz 3, -31576(13)
8004b9e0:      	lwz 4, -31600(13)
8004b9e4:      	bl 0x803ad96c <_binary__mnt_data_text1_bin_start+0x3a4c2c>
8004b9e8:      	cmpwi	3, 0
8004b9ec:      	bt	2, 0x8004ba08 <_binary__mnt_data_text1_bin_start+0x42cc8>
8004b9f0:      	lwz 3, 56(31)
8004b9f4:      	cmplwi	3, 0
8004b9f8:      	bt	2, 0x8004ba08 <_binary__mnt_data_text1_bin_start+0x42cc8>
8004b9fc:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004ba00:      	li 0, 0
8004ba04:      	stw 0, 56(31)
8004ba08:      	lwz 3, 76(31)
8004ba0c:      	cmplwi	3, 0
8004ba10:      	bt	2, 0x8004ba1c <_binary__mnt_data_text1_bin_start+0x42cdc>
8004ba14:      	li 4, 0
8004ba18:      	bl 0x8048229c <_binary__mnt_data_text1_bin_start+0x47955c>
8004ba1c:      	cmplwi	30, 0
8004ba20:      	bt	2, 0x8004ba2c <_binary__mnt_data_text1_bin_start+0x42cec>
8004ba24:      	lwz 0, 60(31)
8004ba28:      	stw 0, 0(30)
8004ba2c:      	lwz 12, 44(31)
8004ba30:      	cmplwi	12, 0
8004ba34:      	bt	2, 0x8004ba64 <_binary__mnt_data_text1_bin_start+0x42d24>
8004ba38:      	lwz 3, 60(31)
8004ba3c:      	lwz 4, 48(31)
8004ba40:      	mtctr 12
8004ba44:      	bctrl
8004ba48:      	lwz 4, 64(31)
8004ba4c:      	lwz 3, 84(4)
8004ba50:      	addi 0, 3, -1
8004ba54:      	stw 0, 84(4)
8004ba58:      	lwz 3, 11572(13)
8004ba5c:      	addi 0, 3, -1
8004ba60:      	stw 0, 11572(13)
8004ba64:      	lhz 0, 4(31)
8004ba68:      	li 3, 1
8004ba6c:      	ori 0, 0, 1
8004ba70:      	sth 0, 4(31)
8004ba74:      	b 0x8004ba7c <_binary__mnt_data_text1_bin_start+0x42d3c>
8004ba78:      	li 3, 0
8004ba7c:      	lmw 30, 8(1)

# ===== TOneFileAsync<RpClump>::StateMachine [8004BBD4..8004BE9C) =====
8004bbd4:      	stwu 1, -16(1)
8004bbd8:      	mflr 0
8004bbdc:      	stw 0, 20(1)
8004bbe0:      	stmw 30, 8(1)
8004bbe4:      	mr	31, 3
8004bbe8:      	mr	30, 4
8004bbec:      	lwz 0, 40(3)
8004bbf0:      	cmpwi	0, 2
8004bbf4:      	bt	2, 0x8004bd84 <_binary__mnt_data_text1_bin_start+0x43044>
8004bbf8:      	bf	0, 0x8004bc0c <_binary__mnt_data_text1_bin_start+0x42ecc>
8004bbfc:      	cmpwi	0, 0
8004bc00:      	bt	2, 0x8004bc1c <_binary__mnt_data_text1_bin_start+0x42edc>
8004bc04:      	bf	0, 0x8004bd38 <_binary__mnt_data_text1_bin_start+0x42ff8>
8004bc08:      	b 0x8004be94 <_binary__mnt_data_text1_bin_start+0x43154>
8004bc0c:      	cmpwi	0, 4
8004bc10:      	bt	2, 0x8004bdf8 <_binary__mnt_data_text1_bin_start+0x430b8>
8004bc14:      	bf	0, 0x8004be94 <_binary__mnt_data_text1_bin_start+0x43154>
8004bc18:      	b 0x8004bde0 <_binary__mnt_data_text1_bin_start+0x430a0>
8004bc1c:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004bc20:      	bl 0x8004b13c <_binary__mnt_data_text1_bin_start+0x423fc>
8004bc24:      	cmpwi	3, 16
8004bc28:      	bt	0, 0x8004bc34 <_binary__mnt_data_text1_bin_start+0x42ef4>
8004bc2c:      	li 3, 0
8004bc30:      	b 0x8004be98 <_binary__mnt_data_text1_bin_start+0x43158>
8004bc34:      	lwz 3, 64(31)
8004bc38:      	lwz 4, 52(31)
8004bc3c:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004bc40:      	lwz 3, 44(3)
8004bc44:      	li 4, 2
8004bc48:      	addi 0, 3, 16415
8004bc4c:      	rlwinm 3, 0, 0, 0, 26
8004bc50:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004bc54:      	stw 3, 56(31)
8004bc58:      	lwz 3, 56(31)
8004bc5c:      	cmplwi	3, 0
8004bc60:      	bt	2, 0x8004bc8c <_binary__mnt_data_text1_bin_start+0x42f4c>
8004bc64:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004bc68:      	lwz 3, 64(31)
8004bc6c:      	lwz 4, 52(31)
8004bc70:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004bc74:      	lwz 3, 44(3)
8004bc78:      	li 4, 2
8004bc7c:      	addi 0, 3, 31
8004bc80:      	rlwinm 3, 0, 0, 0, 26
8004bc84:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004bc88:      	stw 3, 56(31)
8004bc8c:      	lwz 0, 56(31)
8004bc90:      	cmplwi	0, 0
8004bc94:      	bf	2, 0x8004bcbc <_binary__mnt_data_text1_bin_start+0x42f7c>
8004bc98:      	lwz 3, 64(31)
8004bc9c:      	lwz 4, 52(31)
8004bca0:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004bca4:      	lwz 3, 44(3)
8004bca8:      	li 4, 1
8004bcac:      	addi 0, 3, 31
8004bcb0:      	rlwinm 3, 0, 0, 0, 26
8004bcb4:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004bcb8:      	stw 3, 56(31)
8004bcbc:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004bcc0:      	lwz 5, 64(31)
8004bcc4:      	mr	4, 31
8004bcc8:      	lwz 6, 52(31)
8004bccc:      	bl 0x8004d564 <_binary__mnt_data_text1_bin_start+0x44824>
8004bcd0:      	lwz 0, 44(31)
8004bcd4:      	cmplwi	0, 0
8004bcd8:      	bt	2, 0x8004bd04 <_binary__mnt_data_text1_bin_start+0x42fc4>
8004bcdc:      	lis 4, -32763
8004bce0:      	lwz 3, 64(31)
8004bce4:      	addi 6, 4, -16724
8004bce8:      	lwz 4, 52(31)
8004bcec:      	lwz 5, 56(31)
8004bcf0:      	mr	7, 31
8004bcf4:      	bl 0x8004c770 <_binary__mnt_data_text1_bin_start+0x43a30>
8004bcf8:      	li 0, 1
8004bcfc:      	stb 0, 80(31)
8004bd00:      	b 0x8004bd28 <_binary__mnt_data_text1_bin_start+0x42fe8>
8004bd04:      	lwz 3, 64(31)
8004bd08:      	li 6, 0
8004bd0c:      	lwz 4, 52(31)
8004bd10:      	li 7, 0
8004bd14:      	lwz 5, 56(31)
8004bd18:      	bl 0x8004c770 <_binary__mnt_data_text1_bin_start+0x43a30>
8004bd1c:      	stw 3, 72(31)
8004bd20:      	li 0, 0
8004bd24:      	stb 0, 80(31)
8004bd28:      	li 0, 1
8004bd2c:      	li 3, 0
8004bd30:      	stw 0, 40(31)
8004bd34:      	b 0x8004be98 <_binary__mnt_data_text1_bin_start+0x43158>
8004bd38:      	lbz 0, 80(31)
8004bd3c:      	cmplwi	0, 0
8004bd40:      	bf	2, 0x8004bd7c <_binary__mnt_data_text1_bin_start+0x4303c>
8004bd44:      	lwz 0, 72(31)
8004bd48:      	cmplwi	0, 0
8004bd4c:      	bf	2, 0x8004bd68 <_binary__mnt_data_text1_bin_start+0x43028>
8004bd50:      	lwz 3, 56(31)
8004bd54:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004bd58:      	li 0, 4
8004bd5c:      	li 3, 0
8004bd60:      	stw 0, 40(31)
8004bd64:      	b 0x8004be98 <_binary__mnt_data_text1_bin_start+0x43158>
8004bd68:      	li 0, 2
8004bd6c:      	stw 0, 40(31)
8004bd70:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004bd74:      	mr	4, 31
8004bd78:      	bl 0x8004d424 <_binary__mnt_data_text1_bin_start+0x446e4>
8004bd7c:      	li 3, 0
8004bd80:      	b 0x8004be98 <_binary__mnt_data_text1_bin_start+0x43158>
8004bd84:      	lwz 0, 56(31)
8004bd88:      	addi 5, 31, 68
8004bd8c:      	li 3, 3
8004bd90:      	li 4, 1
8004bd94:      	stw 0, 68(31)
8004bd98:      	bl 0x804823b8 <_binary__mnt_data_text1_bin_start+0x479678>
8004bd9c:      	stw 3, 76(31)
8004bda0:      	li 4, 16
8004bda4:      	li 5, 0
8004bda8:      	li 6, 0
8004bdac:      	lwz 30, 84(31)
8004bdb0:      	lwz 3, 76(31)
8004bdb4:      	bl 0x8047dfa0 <_binary__mnt_data_text1_bin_start+0x475260>
8004bdb8:      	cmpwi	3, 0
8004bdbc:      	bt	2, 0x8004bdd4 <_binary__mnt_data_text1_bin_start+0x43094>
8004bdc0:      	mr	3, 30
8004bdc4:      	bl 0x8048e114 <_binary__mnt_data_text1_bin_start+0x4853d4>
8004bdc8:      	lwz 3, 76(31)
8004bdcc:      	bl 0x804586e8 <_binary__mnt_data_text1_bin_start+0x44f9a8>
8004bdd0:      	stw 3, 60(31)
8004bdd4:      	li 0, 4
8004bdd8:      	stw 0, 40(31)
8004bddc:      	b 0x8004be94 <_binary__mnt_data_text1_bin_start+0x43154>
8004bde0:      	lbz 0, 88(31)
8004bde4:      	cmplwi	0, 0
8004bde8:      	bt	2, 0x8004be94 <_binary__mnt_data_text1_bin_start+0x43154>
8004bdec:      	li 0, 4
8004bdf0:      	stw 0, 40(31)
8004bdf4:      	b 0x8004be94 <_binary__mnt_data_text1_bin_start+0x43154>
8004bdf8:      	lwz 3, -31584(13)
8004bdfc:      	lwz 4, -31600(13)
8004be00:      	bl 0x803ad96c <_binary__mnt_data_text1_bin_start+0x3a4c2c>
8004be04:      	cmpwi	3, 0
8004be08:      	bt	2, 0x8004be24 <_binary__mnt_data_text1_bin_start+0x430e4>
8004be0c:      	lwz 3, 56(31)
8004be10:      	cmplwi	3, 0
8004be14:      	bt	2, 0x8004be24 <_binary__mnt_data_text1_bin_start+0x430e4>
8004be18:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004be1c:      	li 0, 0
8004be20:      	stw 0, 56(31)
8004be24:      	lwz 3, 76(31)
8004be28:      	cmplwi	3, 0
8004be2c:      	bt	2, 0x8004be38 <_binary__mnt_data_text1_bin_start+0x430f8>
8004be30:      	li 4, 0
8004be34:      	bl 0x8048229c <_binary__mnt_data_text1_bin_start+0x47955c>
8004be38:      	cmplwi	30, 0
8004be3c:      	bt	2, 0x8004be48 <_binary__mnt_data_text1_bin_start+0x43108>
8004be40:      	lwz 0, 60(31)
8004be44:      	stw 0, 0(30)
8004be48:      	lwz 12, 44(31)
8004be4c:      	cmplwi	12, 0
8004be50:      	bt	2, 0x8004be80 <_binary__mnt_data_text1_bin_start+0x43140>
8004be54:      	lwz 3, 60(31)
8004be58:      	lwz 4, 48(31)
8004be5c:      	mtctr 12
8004be60:      	bctrl
8004be64:      	lwz 4, 64(31)
8004be68:      	lwz 3, 84(4)
8004be6c:      	addi 0, 3, -1
8004be70:      	stw 0, 84(4)
8004be74:      	lwz 3, 11572(13)
8004be78:      	addi 0, 3, -1
8004be7c:      	stw 0, 11572(13)
8004be80:      	lhz 0, 4(31)
8004be84:      	li 3, 1
8004be88:      	ori 0, 0, 1
8004be8c:      	sth 0, 4(31)
8004be90:      	b 0x8004be98 <_binary__mnt_data_text1_bin_start+0x43158>
8004be94:      	li 3, 0
8004be98:      	lmw 30, 8(1)

# ===== TOneFileAsync<RwTexDictionary>::StateMachine [8004BFF0..8004C2D4) =====
8004bff0:      	stwu 1, -16(1)
8004bff4:      	mflr 0
8004bff8:      	stw 0, 20(1)
8004bffc:      	stmw 30, 8(1)
8004c000:      	mr	31, 3
8004c004:      	mr	30, 4
8004c008:      	lwz 0, 40(3)
8004c00c:      	cmpwi	0, 2
8004c010:      	bt	2, 0x8004c1a0 <_binary__mnt_data_text1_bin_start+0x43460>
8004c014:      	bf	0, 0x8004c028 <_binary__mnt_data_text1_bin_start+0x432e8>
8004c018:      	cmpwi	0, 0
8004c01c:      	bt	2, 0x8004c038 <_binary__mnt_data_text1_bin_start+0x432f8>
8004c020:      	bf	0, 0x8004c154 <_binary__mnt_data_text1_bin_start+0x43414>
8004c024:      	b 0x8004c2ac <_binary__mnt_data_text1_bin_start+0x4356c>
8004c028:      	cmpwi	0, 4
8004c02c:      	bt	2, 0x8004c210 <_binary__mnt_data_text1_bin_start+0x434d0>
8004c030:      	bf	0, 0x8004c2ac <_binary__mnt_data_text1_bin_start+0x4356c>
8004c034:      	b 0x8004c1f8 <_binary__mnt_data_text1_bin_start+0x434b8>
8004c038:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004c03c:      	bl 0x8004b13c <_binary__mnt_data_text1_bin_start+0x423fc>
8004c040:      	cmpwi	3, 16
8004c044:      	bt	0, 0x8004c050 <_binary__mnt_data_text1_bin_start+0x43310>
8004c048:      	li 3, 0
8004c04c:      	b 0x8004c2b0 <_binary__mnt_data_text1_bin_start+0x43570>
8004c050:      	lwz 3, 64(31)
8004c054:      	lwz 4, 52(31)
8004c058:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004c05c:      	lwz 3, 44(3)
8004c060:      	li 4, 2
8004c064:      	addi 0, 3, 16415
8004c068:      	rlwinm 3, 0, 0, 0, 26
8004c06c:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004c070:      	stw 3, 56(31)
8004c074:      	lwz 3, 56(31)
8004c078:      	cmplwi	3, 0
8004c07c:      	bt	2, 0x8004c0a8 <_binary__mnt_data_text1_bin_start+0x43368>
8004c080:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004c084:      	lwz 3, 64(31)
8004c088:      	lwz 4, 52(31)
8004c08c:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004c090:      	lwz 3, 44(3)
8004c094:      	li 4, 2
8004c098:      	addi 0, 3, 31
8004c09c:      	rlwinm 3, 0, 0, 0, 26
8004c0a0:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004c0a4:      	stw 3, 56(31)
8004c0a8:      	lwz 0, 56(31)
8004c0ac:      	cmplwi	0, 0
8004c0b0:      	bf	2, 0x8004c0d8 <_binary__mnt_data_text1_bin_start+0x43398>
8004c0b4:      	lwz 3, 64(31)
8004c0b8:      	lwz 4, 52(31)
8004c0bc:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004c0c0:      	lwz 3, 44(3)
8004c0c4:      	li 4, 1
8004c0c8:      	addi 0, 3, 31
8004c0cc:      	rlwinm 3, 0, 0, 0, 26
8004c0d0:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004c0d4:      	stw 3, 56(31)
8004c0d8:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004c0dc:      	lwz 5, 64(31)
8004c0e0:      	mr	4, 31
8004c0e4:      	lwz 6, 52(31)
8004c0e8:      	bl 0x8004d564 <_binary__mnt_data_text1_bin_start+0x44824>
8004c0ec:      	lwz 0, 44(31)
8004c0f0:      	cmplwi	0, 0
8004c0f4:      	bt	2, 0x8004c120 <_binary__mnt_data_text1_bin_start+0x433e0>
8004c0f8:      	lis 4, -32763
8004c0fc:      	lwz 3, 64(31)
8004c100:      	addi 6, 4, -15676
8004c104:      	lwz 4, 52(31)
8004c108:      	lwz 5, 56(31)
8004c10c:      	mr	7, 31
8004c110:      	bl 0x8004c770 <_binary__mnt_data_text1_bin_start+0x43a30>
8004c114:      	li 0, 1
8004c118:      	stb 0, 80(31)
8004c11c:      	b 0x8004c144 <_binary__mnt_data_text1_bin_start+0x43404>
8004c120:      	lwz 3, 64(31)
8004c124:      	li 6, 0
8004c128:      	lwz 4, 52(31)
8004c12c:      	li 7, 0
8004c130:      	lwz 5, 56(31)
8004c134:      	bl 0x8004c770 <_binary__mnt_data_text1_bin_start+0x43a30>
8004c138:      	stw 3, 72(31)
8004c13c:      	li 0, 0
8004c140:      	stb 0, 80(31)
8004c144:      	li 0, 1
8004c148:      	li 3, 0
8004c14c:      	stw 0, 40(31)
8004c150:      	b 0x8004c2b0 <_binary__mnt_data_text1_bin_start+0x43570>
8004c154:      	lbz 0, 80(31)
8004c158:      	cmplwi	0, 0
8004c15c:      	bf	2, 0x8004c198 <_binary__mnt_data_text1_bin_start+0x43458>
8004c160:      	lwz 0, 72(31)
8004c164:      	cmplwi	0, 0
8004c168:      	bf	2, 0x8004c184 <_binary__mnt_data_text1_bin_start+0x43444>
8004c16c:      	lwz 3, 56(31)
8004c170:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004c174:      	li 0, 4
8004c178:      	li 3, 0
8004c17c:      	stw 0, 40(31)
8004c180:      	b 0x8004c2b0 <_binary__mnt_data_text1_bin_start+0x43570>
8004c184:      	li 0, 2
8004c188:      	stw 0, 40(31)
8004c18c:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004c190:      	mr	4, 31
8004c194:      	bl 0x8004d424 <_binary__mnt_data_text1_bin_start+0x446e4>
8004c198:      	li 3, 0
8004c19c:      	b 0x8004c2b0 <_binary__mnt_data_text1_bin_start+0x43570>
8004c1a0:      	lwz 0, 56(31)
8004c1a4:      	addi 5, 31, 68
8004c1a8:      	li 3, 3
8004c1ac:      	li 4, 1
8004c1b0:      	stw 0, 68(31)
8004c1b4:      	bl 0x804823b8 <_binary__mnt_data_text1_bin_start+0x479678>
8004c1b8:      	stw 3, 76(31)
8004c1bc:      	li 4, 22
8004c1c0:      	li 5, 0
8004c1c4:      	li 6, 0
8004c1c8:      	lwz 3, 76(31)
8004c1cc:      	bl 0x8047dfa0 <_binary__mnt_data_text1_bin_start+0x475260>
8004c1d0:      	cmpwi	3, 0
8004c1d4:      	bt	2, 0x8004c1ec <_binary__mnt_data_text1_bin_start+0x434ac>
8004c1d8:      	lwz 3, 76(31)
8004c1dc:      	bl 0x8048500c <_binary__mnt_data_text1_bin_start+0x47c2cc>
8004c1e0:      	stw 3, 60(31)
8004c1e4:      	lwz 3, 60(31)
8004c1e8:      	bl 0x802e0bb0 <_binary__mnt_data_text1_bin_start+0x2d7e70>
8004c1ec:      	li 0, 4
8004c1f0:      	stw 0, 40(31)
8004c1f4:      	b 0x8004c2ac <_binary__mnt_data_text1_bin_start+0x4356c>
8004c1f8:      	lbz 0, 88(31)
8004c1fc:      	cmplwi	0, 0
8004c200:      	bt	2, 0x8004c2ac <_binary__mnt_data_text1_bin_start+0x4356c>
8004c204:      	li 0, 4
8004c208:      	stw 0, 40(31)
8004c20c:      	b 0x8004c2ac <_binary__mnt_data_text1_bin_start+0x4356c>
8004c210:      	lwz 3, -31592(13)
8004c214:      	lwz 4, -31600(13)
8004c218:      	bl 0x803ad96c <_binary__mnt_data_text1_bin_start+0x3a4c2c>
8004c21c:      	cmpwi	3, 0
8004c220:      	bt	2, 0x8004c23c <_binary__mnt_data_text1_bin_start+0x434fc>
8004c224:      	lwz 3, 56(31)
8004c228:      	cmplwi	3, 0
8004c22c:      	bt	2, 0x8004c23c <_binary__mnt_data_text1_bin_start+0x434fc>
8004c230:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004c234:      	li 0, 0
8004c238:      	stw 0, 56(31)
8004c23c:      	lwz 3, 76(31)
8004c240:      	cmplwi	3, 0
8004c244:      	bt	2, 0x8004c250 <_binary__mnt_data_text1_bin_start+0x43510>
8004c248:      	li 4, 0
8004c24c:      	bl 0x8048229c <_binary__mnt_data_text1_bin_start+0x47955c>
8004c250:      	cmplwi	30, 0
8004c254:      	bt	2, 0x8004c260 <_binary__mnt_data_text1_bin_start+0x43520>
8004c258:      	lwz 0, 60(31)
8004c25c:      	stw 0, 0(30)
8004c260:      	lwz 12, 44(31)
8004c264:      	cmplwi	12, 0
8004c268:      	bt	2, 0x8004c298 <_binary__mnt_data_text1_bin_start+0x43558>
8004c26c:      	lwz 3, 60(31)
8004c270:      	lwz 4, 48(31)
8004c274:      	mtctr 12
8004c278:      	bctrl
8004c27c:      	lwz 4, 64(31)
8004c280:      	lwz 3, 84(4)
8004c284:      	addi 0, 3, -1
8004c288:      	stw 0, 84(4)
8004c28c:      	lwz 3, 11572(13)
8004c290:      	addi 0, 3, -1
8004c294:      	stw 0, 11572(13)
8004c298:      	lhz 0, 4(31)
8004c29c:      	li 3, 1
8004c2a0:      	ori 0, 0, 1
8004c2a4:      	sth 0, 4(31)
8004c2a8:      	b 0x8004c2b0 <_binary__mnt_data_text1_bin_start+0x43570>
8004c2ac:      	li 3, 0
8004c2b0:      	lmw 30, 8(1)
8004c2b4:      	lwz 0, 20(1)
8004c2b8:      	mtlr 0
8004c2bc:      	addi 1, 1, 16
8004c2c0:      	blr
8004c2c4:      	stw 3, 72(4)
8004c2c8:      	li 0, 0
8004c2cc:      	stb 0, 80(4)
8004c2d0:      	blr

# ===== TOneFileAsync<void>::StateMachine [8004C408..8004C6C4) =====
8004c408:      	stwu 1, -16(1)
8004c40c:      	mflr 0
8004c410:      	stw 0, 20(1)
8004c414:      	stmw 30, 8(1)
8004c418:      	mr	31, 3
8004c41c:      	mr	30, 4
8004c420:      	lwz 0, 40(3)
8004c424:      	cmpwi	0, 2
8004c428:      	bt	2, 0x8004c5b8 <_binary__mnt_data_text1_bin_start+0x43878>
8004c42c:      	bf	0, 0x8004c440 <_binary__mnt_data_text1_bin_start+0x43700>
8004c430:      	cmpwi	0, 0
8004c434:      	bt	2, 0x8004c450 <_binary__mnt_data_text1_bin_start+0x43710>
8004c438:      	bf	0, 0x8004c56c <_binary__mnt_data_text1_bin_start+0x4382c>
8004c43c:      	b 0x8004c69c <_binary__mnt_data_text1_bin_start+0x4395c>
8004c440:      	cmpwi	0, 4
8004c444:      	bt	2, 0x8004c600 <_binary__mnt_data_text1_bin_start+0x438c0>
8004c448:      	bf	0, 0x8004c69c <_binary__mnt_data_text1_bin_start+0x4395c>
8004c44c:      	b 0x8004c5e8 <_binary__mnt_data_text1_bin_start+0x438a8>
8004c450:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004c454:      	bl 0x8004b13c <_binary__mnt_data_text1_bin_start+0x423fc>
8004c458:      	cmpwi	3, 16
8004c45c:      	bt	0, 0x8004c468 <_binary__mnt_data_text1_bin_start+0x43728>
8004c460:      	li 3, 0
8004c464:      	b 0x8004c6a0 <_binary__mnt_data_text1_bin_start+0x43960>
8004c468:      	lwz 3, 64(31)
8004c46c:      	lwz 4, 52(31)
8004c470:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004c474:      	lwz 3, 44(3)
8004c478:      	li 4, 2
8004c47c:      	addi 0, 3, 16415
8004c480:      	rlwinm 3, 0, 0, 0, 26
8004c484:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004c488:      	stw 3, 56(31)
8004c48c:      	lwz 3, 56(31)
8004c490:      	cmplwi	3, 0
8004c494:      	bt	2, 0x8004c4c0 <_binary__mnt_data_text1_bin_start+0x43780>
8004c498:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004c49c:      	lwz 3, 64(31)
8004c4a0:      	lwz 4, 52(31)
8004c4a4:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004c4a8:      	lwz 3, 44(3)
8004c4ac:      	li 4, 2
8004c4b0:      	addi 0, 3, 31
8004c4b4:      	rlwinm 3, 0, 0, 0, 26
8004c4b8:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004c4bc:      	stw 3, 56(31)
8004c4c0:      	lwz 0, 56(31)
8004c4c4:      	cmplwi	0, 0
8004c4c8:      	bf	2, 0x8004c4f0 <_binary__mnt_data_text1_bin_start+0x437b0>
8004c4cc:      	lwz 3, 64(31)
8004c4d0:      	lwz 4, 52(31)
8004c4d4:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
8004c4d8:      	lwz 3, 44(3)
8004c4dc:      	li 4, 1
8004c4e0:      	addi 0, 3, 31
8004c4e4:      	rlwinm 3, 0, 0, 0, 26
8004c4e8:      	bl 0x80049bb8 <_binary__mnt_data_text1_bin_start+0x40e78>
8004c4ec:      	stw 3, 56(31)
8004c4f0:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004c4f4:      	lwz 5, 64(31)
8004c4f8:      	mr	4, 31
8004c4fc:      	lwz 6, 52(31)
8004c500:      	bl 0x8004d564 <_binary__mnt_data_text1_bin_start+0x44824>
8004c504:      	lwz 0, 44(31)
8004c508:      	cmplwi	0, 0
8004c50c:      	bt	2, 0x8004c538 <_binary__mnt_data_text1_bin_start+0x437f8>
8004c510:      	lis 4, -32763
8004c514:      	lwz 3, 64(31)
8004c518:      	addi 6, 4, -14668
8004c51c:      	lwz 4, 52(31)
8004c520:      	lwz 5, 56(31)
8004c524:      	mr	7, 31
8004c528:      	bl 0x8004c770 <_binary__mnt_data_text1_bin_start+0x43a30>
8004c52c:      	li 0, 1
8004c530:      	stb 0, 80(31)
8004c534:      	b 0x8004c55c <_binary__mnt_data_text1_bin_start+0x4381c>
8004c538:      	lwz 3, 64(31)
8004c53c:      	li 6, 0
8004c540:      	lwz 4, 52(31)
8004c544:      	li 7, 0
8004c548:      	lwz 5, 56(31)
8004c54c:      	bl 0x8004c770 <_binary__mnt_data_text1_bin_start+0x43a30>
8004c550:      	stw 3, 72(31)
8004c554:      	li 0, 0
8004c558:      	stb 0, 80(31)
8004c55c:      	li 0, 1
8004c560:      	li 3, 0
8004c564:      	stw 0, 40(31)
8004c568:      	b 0x8004c6a0 <_binary__mnt_data_text1_bin_start+0x43960>
8004c56c:      	lbz 0, 80(31)
8004c570:      	cmplwi	0, 0
8004c574:      	bf	2, 0x8004c5b0 <_binary__mnt_data_text1_bin_start+0x43870>
8004c578:      	lwz 0, 72(31)
8004c57c:      	cmplwi	0, 0
8004c580:      	bf	2, 0x8004c59c <_binary__mnt_data_text1_bin_start+0x4385c>
8004c584:      	lwz 3, 56(31)
8004c588:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004c58c:      	li 0, 4
8004c590:      	li 3, 0
8004c594:      	stw 0, 40(31)
8004c598:      	b 0x8004c6a0 <_binary__mnt_data_text1_bin_start+0x43960>
8004c59c:      	li 0, 2
8004c5a0:      	stw 0, 40(31)
8004c5a4:      	bl 0x8004b164 <_binary__mnt_data_text1_bin_start+0x42424>
8004c5a8:      	mr	4, 31
8004c5ac:      	bl 0x8004d424 <_binary__mnt_data_text1_bin_start+0x446e4>
8004c5b0:      	li 3, 0
8004c5b4:      	b 0x8004c6a0 <_binary__mnt_data_text1_bin_start+0x43960>
8004c5b8:      	lwz 0, 56(31)
8004c5bc:      	addi 5, 31, 68
8004c5c0:      	li 3, 3
8004c5c4:      	li 4, 1
8004c5c8:      	stw 0, 68(31)
8004c5cc:      	bl 0x804823b8 <_binary__mnt_data_text1_bin_start+0x479678>
8004c5d0:      	stw 3, 76(31)
8004c5d4:      	li 0, 4
8004c5d8:      	lwz 3, 56(31)
8004c5dc:      	stw 3, 60(31)
8004c5e0:      	stw 0, 40(31)
8004c5e4:      	b 0x8004c69c <_binary__mnt_data_text1_bin_start+0x4395c>
8004c5e8:      	lbz 0, 88(31)
8004c5ec:      	cmplwi	0, 0
8004c5f0:      	bt	2, 0x8004c69c <_binary__mnt_data_text1_bin_start+0x4395c>
8004c5f4:      	li 0, 4
8004c5f8:      	stw 0, 40(31)
8004c5fc:      	b 0x8004c69c <_binary__mnt_data_text1_bin_start+0x4395c>
8004c600:      	lwz 3, -31600(13)
8004c604:      	mr	4, 3
8004c608:      	bl 0x803ad96c <_binary__mnt_data_text1_bin_start+0x3a4c2c>
8004c60c:      	cmpwi	3, 0
8004c610:      	bt	2, 0x8004c62c <_binary__mnt_data_text1_bin_start+0x438ec>
8004c614:      	lwz 3, 56(31)
8004c618:      	cmplwi	3, 0
8004c61c:      	bt	2, 0x8004c62c <_binary__mnt_data_text1_bin_start+0x438ec>
8004c620:      	bl 0x80049b5c <_binary__mnt_data_text1_bin_start+0x40e1c>
8004c624:      	li 0, 0
8004c628:      	stw 0, 56(31)
8004c62c:      	lwz 3, 76(31)
8004c630:      	cmplwi	3, 0
8004c634:      	bt	2, 0x8004c640 <_binary__mnt_data_text1_bin_start+0x43900>
8004c638:      	li 4, 0
8004c63c:      	bl 0x8048229c <_binary__mnt_data_text1_bin_start+0x47955c>
8004c640:      	cmplwi	30, 0
8004c644:      	bt	2, 0x8004c650 <_binary__mnt_data_text1_bin_start+0x43910>
8004c648:      	lwz 0, 60(31)
8004c64c:      	stw 0, 0(30)
8004c650:      	lwz 12, 44(31)
8004c654:      	cmplwi	12, 0
8004c658:      	bt	2, 0x8004c688 <_binary__mnt_data_text1_bin_start+0x43948>
8004c65c:      	lwz 3, 60(31)
8004c660:      	lwz 4, 48(31)
8004c664:      	mtctr 12
8004c668:      	bctrl
8004c66c:      	lwz 4, 64(31)
8004c670:      	lwz 3, 84(4)
8004c674:      	addi 0, 3, -1
8004c678:      	stw 0, 84(4)
8004c67c:      	lwz 3, 11572(13)
8004c680:      	addi 0, 3, -1
8004c684:      	stw 0, 11572(13)
8004c688:      	lhz 0, 4(31)
8004c68c:      	li 3, 1
8004c690:      	ori 0, 0, 1
8004c694:      	sth 0, 4(31)
8004c698:      	b 0x8004c6a0 <_binary__mnt_data_text1_bin_start+0x43960>
8004c69c:      	li 3, 0
8004c6a0:      	lmw 30, 8(1)
8004c6a4:      	lwz 0, 20(1)
8004c6a8:      	mtlr 0
8004c6ac:      	addi 1, 1, 16
8004c6b0:      	blr
8004c6b4:      	stw 3, 72(4)
8004c6b8:      	li 0, 0
8004c6bc:      	stb 0, 80(4)
8004c6c0:      	blr

# ===== SharedBeginReadHelper [8004D564..8004D700) =====
8004d564:      	stwu 1, -32(1)
8004d568:      	mflr 0
8004d56c:      	stw 0, 36(1)
8004d570:      	stmw 29, 20(1)
8004d574:      	mr	29, 3
8004d578:      	mr	30, 4
8004d57c:      	bl 0x8004d50c <_binary__mnt_data_text1_bin_start+0x447cc>
8004d580:      	clrlwi.	0, 3, 24
8004d584:      	bt	2, 0x8004d5a0 <_binary__mnt_data_text1_bin_start+0x44860>
8004d588:      	bl 0x80014b90 <_binary__mnt_data_text1_bin_start+0xbe50>
8004d58c:      	lis 5, -32693
8004d590:      	mr	4, 29
8004d594:      	addi 6, 5, -12944
8004d598:      	li 5, 1
8004d59c:      	bl 0x8025d580 <_binary__mnt_data_text1_bin_start+0x254840>
8004d5a0:      	bl 0x80050c40 <_binary__mnt_data_text1_bin_start+0x47f00>
8004d5a4:      	li 0, 1
8004d5a8:      	mr	3, 29
8004d5ac:      	stb 0, 11584(13)
8004d5b0:      	bl 0x8004d53c <_binary__mnt_data_text1_bin_start+0x447fc>
8004d5b4:      	mr	31, 3
8004d5b8:      	b 0x8004d5e0 <_binary__mnt_data_text1_bin_start+0x448a0>
8004d5bc:      	lwz 0, 0(31)
8004d5c0:      	cmplw	0, 30
8004d5c4:      	bf	2, 0x8004d5dc <_binary__mnt_data_text1_bin_start+0x4489c>
8004d5c8:      	li 0, 0
8004d5cc:      	stb 0, 11584(13)
8004d5d0:      	bl 0x80050bf8 <_binary__mnt_data_text1_bin_start+0x47eb8>
8004d5d4:      	li 3, 0
8004d5d8:      	b 0x8004d610 <_binary__mnt_data_text1_bin_start+0x448d0>
8004d5dc:      	addi 31, 31, 4
8004d5e0:      	mr	3, 29
8004d5e4:      	bl 0x8004d4d8 <_binary__mnt_data_text1_bin_start+0x44798>
8004d5e8:      	cmplw	31, 3
8004d5ec:      	bf	2, 0x8004d5bc <_binary__mnt_data_text1_bin_start+0x4487c>
8004d5f0:      	stw 30, 8(1)
8004d5f4:      	mr	3, 29
8004d5f8:      	addi 4, 1, 8
8004d5fc:      	bl 0x8004d624 <_binary__mnt_data_text1_bin_start+0x448e4>
8004d600:      	li 0, 0
8004d604:      	stb 0, 11584(13)
8004d608:      	bl 0x80050bf8 <_binary__mnt_data_text1_bin_start+0x47eb8>
8004d60c:      	li 3, 1
8004d610:      	lmw 29, 20(1)
8004d614:      	lwz 0, 36(1)
8004d618:      	mtlr 0
8004d61c:      	addi 1, 1, 32
8004d620:      	blr
8004d624:      	stwu 1, -16(1)
8004d628:      	mflr 0
8004d62c:      	stw 0, 20(1)
8004d630:      	bl 0x8004d644 <_binary__mnt_data_text1_bin_start+0x44904>
8004d634:      	lwz 0, 20(1)
8004d638:      	mtlr 0
8004d63c:      	addi 1, 1, 16
8004d640:      	blr
8004d644:      	stwu 1, -16(1)
8004d648:      	mflr 0
8004d64c:      	stw 0, 20(1)
8004d650:      	stmw 30, 8(1)
8004d654:      	mr	30, 3
8004d658:      	mr	31, 4
8004d65c:      	bl 0x8004d4f8 <_binary__mnt_data_text1_bin_start+0x447b8>
8004d660:      	mr	4, 3
8004d664:      	mr	3, 30
8004d668:      	mr	6, 31
8004d66c:      	li 5, 1
8004d670:      	bl 0x8004d88c <_binary__mnt_data_text1_bin_start+0x44b4c>
8004d674:      	lmw 30, 8(1)
8004d678:      	lwz 0, 20(1)
8004d67c:      	mtlr 0
8004d680:      	addi 1, 1, 16
8004d684:      	blr
8004d688:      	stwu 1, -16(1)
8004d68c:      	mflr 0
8004d690:      	stw 0, 20(1)
8004d694:      	stw 31, 12(1)
8004d698:      	mr	31, 3
8004d69c:      	bl 0x8004d6ec <_binary__mnt_data_text1_bin_start+0x449ac>
8004d6a0:      	mr	3, 31
8004d6a4:      	li 4, 128
8004d6a8:      	bl 0x8004de14 <_binary__mnt_data_text1_bin_start+0x450d4>
8004d6ac:      	mr	3, 31
8004d6b0:      	bl 0x8004d6cc <_binary__mnt_data_text1_bin_start+0x4498c>
8004d6b4:      	lwz 0, 20(1)
8004d6b8:      	mr	3, 31
8004d6bc:      	lwz 31, 12(1)
8004d6c0:      	mtlr 0
8004d6c4:      	addi 1, 1, 16
8004d6c8:      	blr
8004d6cc:      	stwu 1, -16(1)
8004d6d0:      	mflr 0
8004d6d4:      	stw 0, 20(1)
8004d6d8:      	bl 0x8004d3cc <_binary__mnt_data_text1_bin_start+0x4468c>
8004d6dc:      	lwz 0, 20(1)
8004d6e0:      	mtlr 0
8004d6e4:      	addi 1, 1, 16
8004d6e8:      	blr
8004d6ec:      	stwu 1, -16(1)
8004d6f0:      	mflr 0
8004d6f4:      	stw 0, 20(1)
8004d6f8:      	stw 31, 12(1)
8004d6fc:      	mr	31, 3

# ===== Typed vtable wrappers/destructors [8004D05C..8004D344) =====
8004d05c:      	stwu 1, -16(1)
8004d060:      	mflr 0
8004d064:      	stw 0, 20(1)
8004d068:      	stmw 30, 8(1)
8004d06c:      	mr.	30, 3
8004d070:      	mr	31, 4
8004d074:      	bt	2, 0x8004d09c <_binary__mnt_data_text1_bin_start+0x4435c>
8004d078:      	lis 5, -32686
8004d07c:      	li 4, 0
8004d080:      	addi 0, 5, -6380
8004d084:      	stw 0, 24(30)
8004d088:      	bl 0x8004ef18 <_binary__mnt_data_text1_bin_start+0x461d8>
8004d08c:      	extsh. 0, 31
8004d090:      	bf	1, 0x8004d09c <_binary__mnt_data_text1_bin_start+0x4435c>
8004d094:      	mr	3, 30
8004d098:      	bl 0x803a1334 <_binary__mnt_data_text1_bin_start+0x3985f4>
8004d09c:      	mr	3, 30
8004d0a0:      	lmw 30, 8(1)
8004d0a4:      	lwz 0, 20(1)
8004d0a8:      	mtlr 0
8004d0ac:      	addi 1, 1, 16
8004d0b0:      	blr
8004d0b4:      	stwu 1, -16(1)
8004d0b8:      	mflr 0
8004d0bc:      	li 4, 0
8004d0c0:      	stw 0, 20(1)
8004d0c4:      	bl 0x8004ae60 <_binary__mnt_data_text1_bin_start+0x42120>
8004d0c8:      	lwz 0, 20(1)
8004d0cc:      	mtlr 0
8004d0d0:      	addi 1, 1, 16
8004d0d4:      	blr
8004d0d8:      	stwu 1, -16(1)
8004d0dc:      	mflr 0
8004d0e0:      	stw 0, 20(1)
8004d0e4:      	stmw 30, 8(1)
8004d0e8:      	mr.	30, 3
8004d0ec:      	mr	31, 4
8004d0f0:      	bt	2, 0x8004d118 <_binary__mnt_data_text1_bin_start+0x443d8>
8004d0f4:      	lis 5, -32686
8004d0f8:      	li 4, 0
8004d0fc:      	addi 0, 5, -6408
8004d100:      	stw 0, 24(30)
8004d104:      	bl 0x8004ef18 <_binary__mnt_data_text1_bin_start+0x461d8>
8004d108:      	extsh. 0, 31
8004d10c:      	bf	1, 0x8004d118 <_binary__mnt_data_text1_bin_start+0x443d8>
8004d110:      	mr	3, 30
8004d114:      	bl 0x803a1334 <_binary__mnt_data_text1_bin_start+0x3985f4>
8004d118:      	mr	3, 30
8004d11c:      	lmw 30, 8(1)
8004d120:      	lwz 0, 20(1)
8004d124:      	mtlr 0
8004d128:      	addi 1, 1, 16
8004d12c:      	blr
8004d130:      	stwu 1, -16(1)
8004d134:      	mflr 0
8004d138:      	li 4, 0
8004d13c:      	stw 0, 20(1)
8004d140:      	bl 0x8004b3a0 <_binary__mnt_data_text1_bin_start+0x42660>
8004d144:      	lwz 0, 20(1)
8004d148:      	mtlr 0
8004d14c:      	addi 1, 1, 16
8004d150:      	blr
8004d154:      	stwu 1, -16(1)
8004d158:      	mflr 0
8004d15c:      	stw 0, 20(1)
8004d160:      	stmw 30, 8(1)
8004d164:      	mr.	30, 3
8004d168:      	mr	31, 4
8004d16c:      	bt	2, 0x8004d194 <_binary__mnt_data_text1_bin_start+0x44454>
8004d170:      	lis 5, -32686
8004d174:      	li 4, 0
8004d178:      	addi 0, 5, -6436
8004d17c:      	stw 0, 24(30)
8004d180:      	bl 0x8004ef18 <_binary__mnt_data_text1_bin_start+0x461d8>
8004d184:      	extsh. 0, 31
8004d188:      	bf	1, 0x8004d194 <_binary__mnt_data_text1_bin_start+0x44454>
8004d18c:      	mr	3, 30
8004d190:      	bl 0x803a1334 <_binary__mnt_data_text1_bin_start+0x3985f4>
8004d194:      	mr	3, 30
8004d198:      	lmw 30, 8(1)
8004d19c:      	lwz 0, 20(1)
8004d1a0:      	mtlr 0
8004d1a4:      	addi 1, 1, 16
8004d1a8:      	blr
8004d1ac:      	stwu 1, -16(1)
8004d1b0:      	mflr 0
8004d1b4:      	li 4, 0
8004d1b8:      	stw 0, 20(1)
8004d1bc:      	bl 0x8004b7bc <_binary__mnt_data_text1_bin_start+0x42a7c>
8004d1c0:      	lwz 0, 20(1)
8004d1c4:      	mtlr 0
8004d1c8:      	addi 1, 1, 16
8004d1cc:      	blr
8004d1d0:      	stwu 1, -16(1)
8004d1d4:      	mflr 0
8004d1d8:      	stw 0, 20(1)
8004d1dc:      	stmw 30, 8(1)
8004d1e0:      	mr.	30, 3
8004d1e4:      	mr	31, 4
8004d1e8:      	bt	2, 0x8004d210 <_binary__mnt_data_text1_bin_start+0x444d0>
8004d1ec:      	lis 5, -32686
8004d1f0:      	li 4, 0
8004d1f4:      	addi 0, 5, -6464
8004d1f8:      	stw 0, 24(30)
8004d1fc:      	bl 0x8004ef18 <_binary__mnt_data_text1_bin_start+0x461d8>
8004d200:      	extsh. 0, 31
8004d204:      	bf	1, 0x8004d210 <_binary__mnt_data_text1_bin_start+0x444d0>
8004d208:      	mr	3, 30
8004d20c:      	bl 0x803a1334 <_binary__mnt_data_text1_bin_start+0x3985f4>
8004d210:      	mr	3, 30
8004d214:      	lmw 30, 8(1)
8004d218:      	lwz 0, 20(1)
8004d21c:      	mtlr 0
8004d220:      	addi 1, 1, 16
8004d224:      	blr
8004d228:      	stwu 1, -16(1)
8004d22c:      	mflr 0
8004d230:      	li 4, 0
8004d234:      	stw 0, 20(1)
8004d238:      	bl 0x8004bbd4 <_binary__mnt_data_text1_bin_start+0x42e94>
8004d23c:      	lwz 0, 20(1)
8004d240:      	mtlr 0
8004d244:      	addi 1, 1, 16
8004d248:      	blr
8004d24c:      	stwu 1, -16(1)
8004d250:      	mflr 0
8004d254:      	stw 0, 20(1)
8004d258:      	stmw 30, 8(1)
8004d25c:      	mr.	30, 3
8004d260:      	mr	31, 4
8004d264:      	bt	2, 0x8004d28c <_binary__mnt_data_text1_bin_start+0x4454c>
8004d268:      	lis 5, -32686
8004d26c:      	li 4, 0
8004d270:      	addi 0, 5, -6492
8004d274:      	stw 0, 24(30)
8004d278:      	bl 0x8004ef18 <_binary__mnt_data_text1_bin_start+0x461d8>
8004d27c:      	extsh. 0, 31
8004d280:      	bf	1, 0x8004d28c <_binary__mnt_data_text1_bin_start+0x4454c>
8004d284:      	mr	3, 30
8004d288:      	bl 0x803a1334 <_binary__mnt_data_text1_bin_start+0x3985f4>
8004d28c:      	mr	3, 30
8004d290:      	lmw 30, 8(1)
8004d294:      	lwz 0, 20(1)
8004d298:      	mtlr 0
8004d29c:      	addi 1, 1, 16
8004d2a0:      	blr
8004d2a4:      	stwu 1, -16(1)
8004d2a8:      	mflr 0
8004d2ac:      	li 4, 0
8004d2b0:      	stw 0, 20(1)
8004d2b4:      	bl 0x8004bff0 <_binary__mnt_data_text1_bin_start+0x432b0>
8004d2b8:      	lwz 0, 20(1)
8004d2bc:      	mtlr 0
8004d2c0:      	addi 1, 1, 16
8004d2c4:      	blr
8004d2c8:      	stwu 1, -16(1)
8004d2cc:      	mflr 0
8004d2d0:      	stw 0, 20(1)
8004d2d4:      	stmw 30, 8(1)
8004d2d8:      	mr.	30, 3
8004d2dc:      	mr	31, 4
8004d2e0:      	bt	2, 0x8004d308 <_binary__mnt_data_text1_bin_start+0x445c8>
8004d2e4:      	lis 5, -32686
8004d2e8:      	li 4, 0
8004d2ec:      	addi 0, 5, -6520
8004d2f0:      	stw 0, 24(30)
8004d2f4:      	bl 0x8004ef18 <_binary__mnt_data_text1_bin_start+0x461d8>
8004d2f8:      	extsh. 0, 31
8004d2fc:      	bf	1, 0x8004d308 <_binary__mnt_data_text1_bin_start+0x445c8>
8004d300:      	mr	3, 30
8004d304:      	bl 0x803a1334 <_binary__mnt_data_text1_bin_start+0x3985f4>
8004d308:      	mr	3, 30
8004d30c:      	lmw 30, 8(1)
8004d310:      	lwz 0, 20(1)
8004d314:      	mtlr 0
8004d318:      	addi 1, 1, 16
8004d31c:      	blr
8004d320:      	stwu 1, -16(1)
8004d324:      	mflr 0
8004d328:      	li 4, 0
8004d32c:      	stw 0, 20(1)
8004d330:      	bl 0x8004c408 <_binary__mnt_data_text1_bin_start+0x436c8>
8004d334:      	lwz 0, 20(1)
8004d338:      	mtlr 0
8004d33c:      	addi 1, 1, 16
8004d340:      	blr
