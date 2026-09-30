# ===== LandA child callback/helpers [800463A8..800466B4) =====
800463a8:      	stwu 1, -32(1)
800463ac:      	mflr 0
800463b0:      	stw 0, 36(1)
800463b4:      	stmw 29, 20(1)
800463b8:      	mr	30, 4
800463bc:      	mr	29, 3
800463c0:      	lwz 31, 188(4)
800463c4:      	bl 0x8003a868 <_binary__mnt_data_text1_bin_start+0x31b28>
800463c8:      	lis 3, -32681
800463cc:      	slwi 0, 31, 3
800463d0:      	addi 3, 3, 17120
800463d4:      	add 3, 3, 0
800463d8:      	stw 29, 4(3)
800463dc:      	lwz 31, 188(30)
800463e0:      	bl 0x8003a868 <_binary__mnt_data_text1_bin_start+0x31b28>
800463e4:      	lis 3, -32681
800463e8:      	slwi 5, 31, 3
800463ec:      	addi 4, 3, 17120
800463f0:      	li 0, 5
800463f4:      	lwzx 3, 4, 5
800463f8:      	addi 3, 3, 1
800463fc:      	stwx 3, 4, 5
80046400:      	stw 0, 180(30)
80046404:      	lmw 29, 20(1)
80046408:      	lwz 0, 36(1)
8004640c:      	mtlr 0
80046410:      	addi 1, 1, 32
80046414:      	blr
80046418:      	lwz 6, 168(4)
8004641c:      	li 5, 5
80046420:      	li 0, 12
80046424:      	stw 3, 184(6)
80046428:      	lwz 3, 168(4)
8004642c:      	stw 5, 0(3)
80046430:      	stw 0, 180(4)
80046434:      	blr
80046438:      	stwu 1, -32(1)
8004643c:      	mflr 0
80046440:      	stw 0, 36(1)
80046444:      	stmw 29, 20(1)
80046448:      	mr	31, 4
8004644c:      	lwz 0, 8(4)
80046450:      	lwz 5, 4(4)
80046454:      	mulli 4, 0, 12
80046458:      	addi 29, 4, 4
8004645c:      	add 29, 5, 29
80046460:      	stw 3, 0(29)
80046464:      	lwz 30, 0(29)
80046468:      	cmplwi	30, 0
8004646c:      	bt	2, 0x800464b0 <_binary__mnt_data_text1_bin_start+0x3d770>
80046470:      	bl 0x80013bc4 <_binary__mnt_data_text1_bin_start+0xae84>
80046474:      	lwz 6, 0(31)
80046478:      	mr	4, 3
8004647c:      	mr	5, 30
80046480:      	addi 3, 1, 8
80046484:      	addi 7, 6, 104
80046488:      	li 6, 0
8004648c:      	li 8, 0
80046490:      	li 9, 0
80046494:      	bl 0x8004fbd0 <_binary__mnt_data_text1_bin_start+0x46e90>
80046498:      	lhz 0, 8(1)
8004649c:      	sth 0, 8(29)
800464a0:      	lha 0, 10(1)
800464a4:      	sth 0, 10(29)
800464a8:      	lwz 3, 0(29)
800464ac:      	bl 0x80038148 <_binary__mnt_data_text1_bin_start+0x2f408>
800464b0:      	lwz 4, 0(31)
800464b4:      	li 5, 0
800464b8:      	lwz 3, 8(31)
800464bc:      	li 0, 15
800464c0:      	li 6, 0
800464c4:      	add 3, 4, 3
800464c8:      	stb 5, 196(3)
800464cc:      	mtctr 0
800464d0:      	lwz 3, 0(31)
800464d4:      	addi 0, 6, 196
800464d8:      	lbzx 0, 3, 0
800464dc:      	cmplwi	0, 0
800464e0:      	bt	2, 0x800464f0 <_binary__mnt_data_text1_bin_start+0x3d7b0>
800464e4:      	mr	3, 31
800464e8:      	bl 0x803a1334 <_binary__mnt_data_text1_bin_start+0x3985f4>
800464ec:      	b 0x80046518 <_binary__mnt_data_text1_bin_start+0x3d7d8>
800464f0:      	addi 6, 6, 1
800464f4:      	bdnz 0x800464d0 <_binary__mnt_data_text1_bin_start+0x3d790>
800464f8:      	lwz 4, 4(31)
800464fc:      	li 5, 4
80046500:      	li 0, 10
80046504:      	mr	3, 31
80046508:      	stw 5, 0(4)
8004650c:      	lwz 4, 0(31)
80046510:      	stw 0, 180(4)
80046514:      	bl 0x803a1334 <_binary__mnt_data_text1_bin_start+0x3985f4>
80046518:      	lmw 29, 20(1)
8004651c:      	lwz 0, 36(1)
80046520:      	mtlr 0
80046524:      	addi 1, 1, 32
80046528:      	blr
8004652c:      	stwu 1, -64(1)
80046530:      	mflr 0
80046534:      	stw 0, 68(1)
80046538:      	stmw 21, 20(1)
8004653c:      	mr	24, 3
80046540:      	mr	25, 4
80046544:      	mr	26, 5
80046548:      	mr	27, 6
8004654c:      	addi 3, 24, 196
80046550:      	li 21, 0
80046554:      	li 4, 0
80046558:      	li 5, 15
8004655c:      	bl 0x8000540c <_binary__mnt_data_text1_bin_size+0x7fb634ec>
80046560:      	lis 3, -32686
80046564:      	li 30, 0
80046568:      	addi 31, 3, -7384
8004656c:      	li 23, 0
80046570:      	li 22, 0
80046574:      	lwzx 4, 31, 22
80046578:      	mr	3, 26
8004657c:      	li 5, 2
80046580:      	bl 0x8004c878 <_binary__mnt_data_text1_bin_start+0x43b38>
80046584:      	mr.	29, 3
80046588:      	bf	1, 0x80046684 <_binary__mnt_data_text1_bin_start+0x3d944>
8004658c:      	addi 28, 23, 4
80046590:      	mr	3, 26
80046594:      	mr	4, 29
80046598:      	add 28, 25, 28
8004659c:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
800465a0:      	mr	21, 3
800465a4:      	bl 0x8003a868 <_binary__mnt_data_text1_bin_start+0x31b28>
800465a8:      	mr	3, 21
800465ac:      	addi 4, 2, -31428
800465b0:      	bl 0x803ad710 <_binary__mnt_data_text1_bin_start+0x3a49d0>
800465b4:      	mr.	21, 3
800465b8:      	bt	2, 0x800465ec <_binary__mnt_data_text1_bin_start+0x3d8ac>
800465bc:      	addi 3, 2, -31428
800465c0:      	bl 0x803adc08 <_binary__mnt_data_text1_bin_start+0x3a4ec8>
800465c4:      	add 3, 21, 3
800465c8:      	lbz 0, 0(3)
800465cc:      	lbz 4, 1(3)
800465d0:      	extsb 3, 0
800465d4:      	addi 0, 3, -48
800465d8:      	extsb 3, 4
800465dc:      	mulli 0, 0, 10
800465e0:      	add 3, 3, 0
800465e4:      	addi 3, 3, -48
800465e8:      	b 0x800465f0 <_binary__mnt_data_text1_bin_start+0x3d8b0>
800465ec:      	li 3, 0
800465f0:      	stw 3, 4(28)
800465f4:      	lwz 21, 4(28)
800465f8:      	cmpwi	21, 0
800465fc:      	bt	2, 0x80046638 <_binary__mnt_data_text1_bin_start+0x3d8f8>
80046600:      	bl 0x8003a868 <_binary__mnt_data_text1_bin_start+0x31b28>
80046604:      	cmpwi	21, 0
80046608:      	bf	1, 0x8004662c <_binary__mnt_data_text1_bin_start+0x3d8ec>
8004660c:      	cmpwi	21, 10
80046610:      	bf	0, 0x8004662c <_binary__mnt_data_text1_bin_start+0x3d8ec>
80046614:      	lis 3, -32681
80046618:      	slwi 4, 21, 3
8004661c:      	addi 0, 3, 17120
80046620:      	add 3, 0, 4
80046624:      	lwz 3, 4(3)
80046628:      	b 0x80046630 <_binary__mnt_data_text1_bin_start+0x3d8f0>
8004662c:      	li 3, 0
80046630:      	bl 0x8048e114 <_binary__mnt_data_text1_bin_start+0x4853d4>
80046634:      	b 0x80046644 <_binary__mnt_data_text1_bin_start+0x3d904>
80046638:      	bl 0x8003a868 <_binary__mnt_data_text1_bin_start+0x31b28>
8004663c:      	lwz 3, 6148(3)
80046640:      	bl 0x8048e114 <_binary__mnt_data_text1_bin_start+0x4853d4>
80046644:      	li 3, 12
80046648:      	bl 0x803a1380 <_binary__mnt_data_text1_bin_start+0x398640>
8004664c:      	stw 24, 0(3)
80046650:      	mr	6, 3
80046654:      	lis 4, -32764
80046658:      	li 8, 1
8004665c:      	stw 25, 4(6)
80046660:      	addi 5, 4, 25656
80046664:      	mr	3, 26
80046668:      	mr	4, 29
8004666c:      	stw 30, 8(6)
80046670:      	lwz 7, 8(6)
80046674:      	addi 0, 7, 196
80046678:      	stbx 8, 24, 0
8004667c:      	bl 0x8004b318 <_binary__mnt_data_text1_bin_start+0x425d8>
80046680:      	li 21, 1
80046684:      	addi 30, 30, 1
80046688:      	addi 22, 22, 24
8004668c:      	cmpwi	30, 15
80046690:      	addi 23, 23, 12
80046694:      	bt	0, 0x80046574 <_binary__mnt_data_text1_bin_start+0x3d834>
80046698:      	stw 27, 188(25)
8004669c:      	mr	3, 21
800466a0:      	lmw 21, 20(1)
800466a4:      	lwz 0, 68(1)
800466a8:      	mtlr 0
800466ac:      	addi 1, 1, 64
800466b0:      	blr

# ===== LandALoadManager state machine [800466B4..80046A60) =====
800466b4:      	stwu 1, -96(1)
800466b8:      	mflr 0
800466bc:      	stw 0, 100(1)
800466c0:      	stmw 29, 84(1)
800466c4:      	mr	30, 3
800466c8:      	lwz 0, 180(3)
800466cc:      	cmplwi	0, 12
800466d0:      	bt	1, 0x80046a4c <_binary__mnt_data_text1_bin_start+0x3dd0c>
800466d4:      	lis 4, -32686
800466d8:      	slwi 0, 0, 2
800466dc:      	addi 4, 4, -6972
800466e0:      	lwzx 0, 4, 0
800466e4:      	mtctr 0
800466e8:      	bctr
800466ec:      	li 0, 6
800466f0:      	stw 0, 180(30)
800466f4:      	lwz 4, 168(30)
800466f8:      	li 0, 2
800466fc:      	li 3, 88
80046700:      	stw 0, 0(4)
80046704:      	bl 0x803a1380 <_binary__mnt_data_text1_bin_start+0x398640>
80046708:      	mr.	0, 3
8004670c:      	bt	2, 0x80046720 <_binary__mnt_data_text1_bin_start+0x3d9e0>
80046710:      	li 4, 0
80046714:      	li 5, 0
80046718:      	bl 0x8004cfd8 <_binary__mnt_data_text1_bin_start+0x44298>
8004671c:      	mr	0, 3
80046720:      	stw 0, 172(30)
80046724:      	lwz 3, 172(30)
80046728:      	cmplwi	3, 0
8004672c:      	bt	2, 0x80046774 <_binary__mnt_data_text1_bin_start+0x3da34>
80046730:      	addi 4, 30, 104
80046734:      	li 5, 0
80046738:      	li 6, 0
8004673c:      	li 7, 0
80046740:      	li 8, 0
80046744:      	bl 0x8004cc94 <_binary__mnt_data_text1_bin_start+0x43f54>
80046748:      	clrlwi.	0, 3, 24
8004674c:      	bt	2, 0x8004675c <_binary__mnt_data_text1_bin_start+0x3da1c>
80046750:      	li 0, 7
80046754:      	stw 0, 180(30)
80046758:      	b 0x80046a4c <_binary__mnt_data_text1_bin_start+0x3dd0c>
8004675c:      	lwz 3, 168(30)
80046760:      	li 4, 5
80046764:      	li 0, 12
80046768:      	stw 4, 0(3)
8004676c:      	stw 0, 180(30)
80046770:      	b 0x80046a4c <_binary__mnt_data_text1_bin_start+0x3dd0c>
80046774:      	li 0, 12
80046778:      	stw 0, 180(30)
8004677c:      	b 0x80046a4c <_binary__mnt_data_text1_bin_start+0x3dd0c>
80046780:      	lwz 3, 172(30)
80046784:      	bl 0x8004cb20 <_binary__mnt_data_text1_bin_start+0x43de0>
80046788:      	clrlwi.	0, 3, 24
8004678c:      	bt	2, 0x80046a4c <_binary__mnt_data_text1_bin_start+0x3dd0c>
80046790:      	li 0, 2
80046794:      	stw 0, 180(30)
80046798:      	b 0x80046a4c <_binary__mnt_data_text1_bin_start+0x3dd0c>
8004679c:      	lwz 3, 172(30)
800467a0:      	lwz 31, 52(3)
800467a4:      	b 0x800468f0 <_binary__mnt_data_text1_bin_start+0x3dbb0>
800467a8:      	lwz 3, 172(30)
800467ac:      	lwz 4, 184(30)
800467b0:      	bl 0x8004ca50 <_binary__mnt_data_text1_bin_start+0x43d10>
800467b4:      	mr	29, 3
800467b8:      	bl 0x8003a868 <_binary__mnt_data_text1_bin_start+0x31b28>
800467bc:      	mr	3, 29
800467c0:      	addi 4, 2, -31428
800467c4:      	bl 0x803ad710 <_binary__mnt_data_text1_bin_start+0x3a49d0>
800467c8:      	mr.	29, 3
800467cc:      	bt	2, 0x80046800 <_binary__mnt_data_text1_bin_start+0x3dac0>
800467d0:      	addi 3, 2, -31428
800467d4:      	bl 0x803adc08 <_binary__mnt_data_text1_bin_start+0x3a4ec8>
800467d8:      	add 3, 29, 3
800467dc:      	lbz 0, 0(3)
800467e0:      	lbz 4, 1(3)
800467e4:      	extsb 3, 0
800467e8:      	addi 0, 3, -48
800467ec:      	extsb 3, 4
800467f0:      	mulli 0, 0, 10
800467f4:      	add 3, 3, 0
800467f8:      	addi 3, 3, -48
800467fc:      	b 0x80046804 <_binary__mnt_data_text1_bin_start+0x3dac4>
80046800:      	li 3, 0
80046804:      	stw 3, 188(30)
80046808:      	lwz 3, 184(30)
8004680c:      	addi 0, 3, 1
80046810:      	stw 0, 184(30)
80046814:      	lwz 29, 188(30)
80046818:      	cmpwi	29, 0
8004681c:      	bt	2, 0x800468f0 <_binary__mnt_data_text1_bin_start+0x3dbb0>
80046820:      	bl 0x8003a868 <_binary__mnt_data_text1_bin_start+0x31b28>
80046824:      	lis 3, -32681
80046828:      	slwi 0, 29, 3
8004682c:      	addi 3, 3, 17120
80046830:      	lwzx 0, 3, 0
80046834:      	cmpwi	0, 0
80046838:      	bf	2, 0x800468d0 <_binary__mnt_data_text1_bin_start+0x3db90>
8004683c:      	li 3, 88
80046840:      	bl 0x803a1380 <_binary__mnt_data_text1_bin_start+0x398640>
80046844:      	mr.	0, 3
80046848:      	bt	2, 0x8004685c <_binary__mnt_data_text1_bin_start+0x3db1c>
8004684c:      	li 4, 0
80046850:      	li 5, 0
80046854:      	bl 0x8004cfd8 <_binary__mnt_data_text1_bin_start+0x44298>
80046858:      	mr	0, 3
8004685c:      	stw 0, 176(30)
80046860:      	lwz 0, 176(30)
80046864:      	cmplwi	0, 0
80046868:      	bt	2, 0x800468f0 <_binary__mnt_data_text1_bin_start+0x3dbb0>
8004686c:      	lis 3, -32693
80046870:      	lwz 6, 188(30)
80046874:      	addi 4, 3, -13636
80046878:      	addi 5, 30, 40
8004687c:      	addi 3, 1, 8
80046880:      	crclr	6
80046884:      	bl 0x803aa248 <_binary__mnt_data_text1_bin_start+0x3a1508>
80046888:      	lwz 3, 176(30)
8004688c:      	addi 4, 1, 8
80046890:      	li 5, 0
80046894:      	li 6, 0
80046898:      	li 7, 0
8004689c:      	li 8, 0
800468a0:      	bl 0x8004cc94 <_binary__mnt_data_text1_bin_start+0x43f54>
800468a4:      	clrlwi.	0, 3, 24
800468a8:      	bt	2, 0x800468b8 <_binary__mnt_data_text1_bin_start+0x3db78>
800468ac:      	li 0, 3
800468b0:      	stw 0, 180(30)
800468b4:      	b 0x80046a4c <_binary__mnt_data_text1_bin_start+0x3dd0c>
800468b8:      	lwz 3, 176(30)
800468bc:      	li 4, 1
800468c0:      	bl 0x8004cf90 <_binary__mnt_data_text1_bin_start+0x44250>
800468c4:      	li 0, 0
800468c8:      	stw 0, 176(30)
800468cc:      	b 0x800468f0 <_binary__mnt_data_text1_bin_start+0x3dbb0>
800468d0:      	lwz 29, 188(30)
800468d4:      	bl 0x8003a868 <_binary__mnt_data_text1_bin_start+0x31b28>
800468d8:      	lis 3, -32681
800468dc:      	slwi 5, 29, 3
800468e0:      	addi 4, 3, 17120
800468e4:      	lwzx 3, 4, 5
800468e8:      	addi 0, 3, 1
800468ec:      	stwx 0, 4, 5
800468f0:      	lwz 3, 16(31)
800468f4:      	lwz 4, 184(30)
800468f8:      	addi 0, 3, 2
800468fc:      	cmpw	4, 0
80046900:      	bt	0, 0x800467a8 <_binary__mnt_data_text1_bin_start+0x3da68>
80046904:      	li 0, 8
80046908:      	stw 0, 180(30)
8004690c:      	b 0x80046a4c <_binary__mnt_data_text1_bin_start+0x3dd0c>
80046910:      	lwz 3, 176(30)
80046914:      	bl 0x8004cb20 <_binary__mnt_data_text1_bin_start+0x43de0>
80046918:      	clrlwi.	0, 3, 24
8004691c:      	bt	2, 0x80046a4c <_binary__mnt_data_text1_bin_start+0x3dd0c>
80046920:      	lwz 29, 188(30)
80046924:      	bl 0x8003a868 <_binary__mnt_data_text1_bin_start+0x31b28>
80046928:      	cmpwi	29, 0
8004692c:      	bf	1, 0x80046950 <_binary__mnt_data_text1_bin_start+0x3dc10>
80046930:      	cmpwi	29, 10
80046934:      	bf	0, 0x80046950 <_binary__mnt_data_text1_bin_start+0x3dc10>
80046938:      	lis 3, -32681
8004693c:      	slwi 0, 29, 3
80046940:      	addi 3, 3, 17120
80046944:      	add 3, 3, 0
80046948:      	lwz 0, 4(3)
8004694c:      	b 0x80046954 <_binary__mnt_data_text1_bin_start+0x3dc14>
80046950:      	li 0, 0
80046954:      	cmplwi	0, 0
80046958:      	bf	2, 0x80046974 <_binary__mnt_data_text1_bin_start+0x3dc34>
8004695c:      	lis 4, -32764
80046960:      	lwz 3, 176(30)
80046964:      	addi 5, 4, 25512
80046968:      	mr	6, 30
8004696c:      	li 4, 2
80046970:      	bl 0x8004bf68 <_binary__mnt_data_text1_bin_start+0x43228>
80046974:      	li 0, 4
80046978:      	stw 0, 180(30)
8004697c:      	b 0x80046a4c <_binary__mnt_data_text1_bin_start+0x3dd0c>
80046980:      	lwz 3, 176(30)
80046984:      	li 4, 1
80046988:      	bl 0x8004cf90 <_binary__mnt_data_text1_bin_start+0x44250>
8004698c:      	li 3, 0
80046990:      	li 0, 2
80046994:      	stw 3, 176(30)
80046998:      	stw 0, 180(30)
8004699c:      	b 0x80046a4c <_binary__mnt_data_text1_bin_start+0x3dd0c>
800469a0:      	lwz 4, 168(30)
800469a4:      	lwz 5, 172(30)
800469a8:      	lwz 6, 192(30)
800469ac:      	bl 0x8004652c <_binary__mnt_data_text1_bin_start+0x3d7ec>
800469b0:      	clrlwi.	0, 3, 24
800469b4:      	bt	2, 0x800469c4 <_binary__mnt_data_text1_bin_start+0x3dc84>
800469b8:      	li 0, 9
800469bc:      	stw 0, 180(30)
800469c0:      	b 0x80046a4c <_binary__mnt_data_text1_bin_start+0x3dd0c>
800469c4:      	li 0, 10
800469c8:      	stw 0, 180(30)
800469cc:      	b 0x80046a4c <_binary__mnt_data_text1_bin_start+0x3dd0c>
800469d0:      	lwz 29, 172(30)
800469d4:      	addi 4, 2, -31436
800469d8:      	li 5, 2
800469dc:      	mr	3, 29
800469e0:      	bl 0x8004c878 <_binary__mnt_data_text1_bin_start+0x43b38>
800469e4:      	mr.	4, 3
800469e8:      	bf	1, 0x80046a08 <_binary__mnt_data_text1_bin_start+0x3dcc8>
800469ec:      	lis 5, -32764
800469f0:      	mr	3, 29
800469f4:      	addi 5, 5, 25624
800469f8:      	mr	6, 30
800469fc:      	bl 0x8004b318 <_binary__mnt_data_text1_bin_start+0x425d8>
80046a00:      	li 4, 1
80046a04:      	b 0x80046a18 <_binary__mnt_data_text1_bin_start+0x3dcd8>
80046a08:      	lwz 3, 168(30)
80046a0c:      	li 0, 5
80046a10:      	li 4, 0
80046a14:      	stw 0, 0(3)
80046a18:      	clrlwi.	0, 4, 24
80046a1c:      	bt	2, 0x80046a2c <_binary__mnt_data_text1_bin_start+0x3dcec>
80046a20:      	li 0, 11
80046a24:      	stw 0, 180(30)
80046a28:      	b 0x80046a4c <_binary__mnt_data_text1_bin_start+0x3dd0c>
80046a2c:      	li 0, 12
80046a30:      	stw 0, 180(30)
80046a34:      	b 0x80046a4c <_binary__mnt_data_text1_bin_start+0x3dd0c>
80046a38:      	li 0, 0
80046a3c:      	stw 0, 180(30)
80046a40:      	lhz 0, 4(30)
80046a44:      	ori 0, 0, 1
80046a48:      	sth 0, 4(30)
80046a4c:      	lmw 29, 84(1)
80046a50:      	lwz 0, 100(1)
80046a54:      	mtlr 0
80046a58:      	addi 1, 1, 96
80046a5c:      	blr

# ===== LandALoadManager destructor [80046A60..80046AF0) =====
80046a60:      	stwu 1, -16(1)
80046a64:      	mflr 0
80046a68:      	stw 0, 20(1)
80046a6c:      	stmw 30, 8(1)
80046a70:      	mr.	30, 3
80046a74:      	mr	31, 4
80046a78:      	bt	2, 0x80046ad8 <_binary__mnt_data_text1_bin_start+0x3dd98>
80046a7c:      	lis 3, -32686
80046a80:      	addi 0, 3, -6908
80046a84:      	stw 0, 24(30)
80046a88:      	lwz 3, 172(30)
80046a8c:      	cmplwi	3, 0
80046a90:      	bt	2, 0x80046a9c <_binary__mnt_data_text1_bin_start+0x3dd5c>
80046a94:      	li 4, 1
80046a98:      	bl 0x8004cf90 <_binary__mnt_data_text1_bin_start+0x44250>
80046a9c:      	lwz 3, 176(30)
80046aa0:      	cmplwi	3, 0
80046aa4:      	bt	2, 0x80046ab0 <_binary__mnt_data_text1_bin_start+0x3dd70>
80046aa8:      	li 4, 1
80046aac:      	bl 0x8004cf90 <_binary__mnt_data_text1_bin_start+0x44250>
80046ab0:      	bl 0x80014b90 <_binary__mnt_data_text1_bin_start+0xbe50>
80046ab4:      	mr	4, 30
80046ab8:      	bl 0x8025d4c8 <_binary__mnt_data_text1_bin_start+0x254788>
80046abc:      	mr	3, 30
80046ac0:      	li 4, 0
80046ac4:      	bl 0x8004ef18 <_binary__mnt_data_text1_bin_start+0x461d8>
80046ac8:      	extsh. 0, 31
80046acc:      	bf	1, 0x80046ad8 <_binary__mnt_data_text1_bin_start+0x3dd98>
80046ad0:      	mr	3, 30
80046ad4:      	bl 0x803a1334 <_binary__mnt_data_text1_bin_start+0x3985f4>
80046ad8:      	mr	3, 30
80046adc:      	lmw 30, 8(1)
80046ae0:      	lwz 0, 20(1)
80046ae4:      	mtlr 0
80046ae8:      	addi 1, 1, 16
80046aec:      	blr

# ===== LandALoadManager constructor [80046AF0..80046BB4) =====
80046af0:      	stwu 1, -32(1)
80046af4:      	mflr 0
80046af8:      	stw 0, 36(1)
80046afc:      	stmw 28, 16(1)
80046b00:      	mr	28, 3
80046b04:      	mr	29, 5
80046b08:      	mr	30, 6
80046b0c:      	mr	31, 7
80046b10:      	bl 0x8004f014 <_binary__mnt_data_text1_bin_start+0x462d4>
80046b14:      	lis 4, -32686
80046b18:      	lis 3, -32693
80046b1c:      	addi 0, 4, -6908
80046b20:      	mr	5, 29
80046b24:      	stw 0, 24(28)
80046b28:      	addi 4, 3, -13616
80046b2c:      	mr	6, 29
80046b30:      	mr	7, 30
80046b34:      	lwz 0, -31664(13)
80046b38:      	addi 3, 28, 104
80046b3c:      	stw 0, 0(28)
80046b40:      	crclr	6
80046b44:      	bl 0x803aa248 <_binary__mnt_data_text1_bin_start+0x3a1508>
80046b48:      	mr	4, 29
80046b4c:      	addi 3, 28, 40
80046b50:      	bl 0x803adb50 <_binary__mnt_data_text1_bin_start+0x3a4e10>
80046b54:      	stw 31, 168(28)
80046b58:      	li 5, 1
80046b5c:      	li 3, 0
80046b60:      	li 0, 2
80046b64:      	stw 5, 180(28)
80046b68:      	lwz 4, 168(28)
80046b6c:      	stw 5, 0(4)
80046b70:      	stw 3, 172(28)
80046b74:      	stw 3, 176(28)
80046b78:      	stw 0, 184(28)
80046b7c:      	stw 3, 188(28)
80046b80:      	stw 30, 192(28)
80046b84:      	bl 0x80014b90 <_binary__mnt_data_text1_bin_start+0xbe50>
80046b88:      	lis 5, -32693
80046b8c:      	mr	4, 28
80046b90:      	addi 6, 5, -13600
80046b94:      	li 5, 0
80046b98:      	bl 0x8025d580 <_binary__mnt_data_text1_bin_start+0x254840>
80046b9c:      	mr	3, 28
80046ba0:      	lmw 28, 16(1)
80046ba4:      	lwz 0, 36(1)
80046ba8:      	mtlr 0
80046bac:      	addi 1, 1, 32
80046bb0:      	blr

# ===== Stage light filename builder/loader [80048738..800487B8) =====
80048738:      	stwu 1, -48(1)
8004873c:      	mflr 0
80048740:      	lis 6, -32693
80048744:      	mr	5, 4
80048748:      	stw 0, 52(1)
8004874c:      	addi 4, 6, -13544
80048750:      	mr	6, 5
80048754:      	stw 31, 44(1)
80048758:      	mr	31, 3
8004875c:      	addi 3, 1, 8
80048760:      	crclr	6
80048764:      	bl 0x803aa248 <_binary__mnt_data_text1_bin_start+0x3a1508>
80048768:      	mr	3, 31
8004876c:      	addi 5, 1, 8
80048770:      	li 4, 0
80048774:      	bl 0x8004867c <_binary__mnt_data_text1_bin_start+0x3f93c>
80048778:      	clrlwi.	0, 3, 24
8004877c:      	bt	2, 0x800487a0 <_binary__mnt_data_text1_bin_start+0x3fa60>
80048780:      	addi 3, 31, 32
80048784:      	cmplw	3, 3
80048788:      	bt	2, 0x80048798 <_binary__mnt_data_text1_bin_start+0x3fa58>
8004878c:      	mr	4, 3
80048790:      	li 5, 832
80048794:      	bl 0x800054f4 <_binary__mnt_data_text1_bin_size+0x7fb635d4>
80048798:      	li 3, 1
8004879c:      	b 0x800487a4 <_binary__mnt_data_text1_bin_start+0x3fa64>
800487a0:      	li 3, 0
800487a4:      	lwz 0, 52(1)
800487a8:      	lwz 31, 44(1)
800487ac:      	mtlr 0
800487b0:      	addi 1, 1, 48
800487b4:      	blr
