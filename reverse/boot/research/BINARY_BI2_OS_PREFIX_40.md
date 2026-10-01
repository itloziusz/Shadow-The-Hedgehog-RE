# PAL BI2/OS native prefix: raw 95-word ledger

**PROVEN raw identity:** PAL DOL SHA256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`. Original bytes are read directly through its section map. Independent instruction, CFG and machine-effect checks are in `BI2_CONNECTED_AUDIT_40.md` and `BI2_NATIVE_ADVERSARIAL_40.md`. Connected native scope and tests are in `NATIVE_BI2_COMPLETION_40.md`. This is a curated continuation record. The last TBU word is pinned but remains unexecuted. Reused EE words remain in `BINARY_CACHE_HANDLER.md`.

D-form immediate fields retain signed and raw values; X/XL/M/I/B fields retain dependencies. SPR8=LR; SPR9=CTR; TBR269=unresolved high time-base input. Direct target fields are independently checked by `agent_handler_bi2.py` and the shared Markdown gate (95 words, 19 direct branches, 4 SPR encodings). BO13 retains the BO12 equality predicate with a prediction hint; a focused mutation rejects changes to the actual predicate. Confidence: PROVEN bytes/conditional ISA effects; STRONG connected closed-owner semantics; UNKNOWN retail/timing state.

| VA | File offset | Raw bytes / BE word | Encoding fields | Independent Capstone decode |
|---|---:|---|---|---|
| `80003140` | `000140` | `38 00 00 01` / `38000001` | D op14 D/S0 A0 imm0001 signed1 | `li r0, 1` |
| `80003144` | `000144` | `98 0D 5A F0` / `980D5AF0` | D op38 D/S0 A13 imm5AF0 signed23280 | `stb r0, 0x5af0(r13)` |
| `80003148` | `000148` | `4E 80 00 20` / `4E800020` | XL op19 D/S/BO20 A/BI0 B0 XO16 Rc/LK0 | `blr ` |
| `80003188` | `000188` | `80 C6 00 00` / `80C60000` | D op32 D/S6 A6 imm0000 signed0 | `lwz r6, 0(r6)` |
| `8000318C` | `00018C` | `28 06 00 00` / `28060000` | D op10 D/S0 A6 imm0000 signed0 | `cmplwi r6, 0` |
| `80003190` | `000190` | `41 82 00 0C` / `4182000C` | B op16 disp+12 AA0 LK0 target8000319C BO12 BI2 → `beq 8000319C` | `beq 0x8000319c` |
| `80003194` | `000194` | `80 E6 00 0C` / `80E6000C` | D op32 D/S7 A6 imm000C signed12 | `lwz r7, 0xc(r6)` |
| `80003198` | `000198` | `48 00 00 24` / `48000024` | I op18 disp+36 AA0 LK0 target800031BC → `b 800031BC` | `b 0x800031bc` |
| `8000319C` | `00019C` | `3C A0 80 00` / `3CA08000` | D op15 D/S5 A0 imm8000 signed-32768 | `lis r5, -0x8000` |
| `800031A0` | `0001A0` | `38 A5 00 34` / `38A50034` | D op14 D/S5 A5 imm0034 signed52 | `addi r5, r5, 0x34` |
| `800031A4` | `0001A4` | `80 A5 00 00` / `80A50000` | D op32 D/S5 A5 imm0000 signed0 | `lwz r5, 0(r5)` |
| `800031A8` | `0001A8` | `28 05 00 00` / `28050000` | D op10 D/S0 A5 imm0000 signed0 | `cmplwi r5, 0` |
| `800031AC` | `0001AC` | `41 82 00 4C` / `4182004C` | B op16 disp+76 AA0 LK0 target800031F8 BO12 BI2 → `beq 800031F8` | `beq 0x800031f8` |
| `800031B0` | `0001B0` | `3C E0 80 00` / `3CE08000` | D op15 D/S7 A0 imm8000 signed-32768 | `lis r7, -0x8000` |
| `800031B4` | `0001B4` | `38 E7 30 E8` / `38E730E8` | D op14 D/S7 A7 imm30E8 signed12520 | `addi r7, r7, 0x30e8` |
| `800031B8` | `0001B8` | `80 E7 00 00` / `80E70000` | D op32 D/S7 A7 imm0000 signed0 | `lwz r7, 0(r7)` |
| `800031BC` | `0001BC` | `38 A0 00 00` / `38A00000` | D op14 D/S5 A0 imm0000 signed0 | `li r5, 0` |
| `800031C0` | `0001C0` | `28 07 00 02` / `28070002` | D op10 D/S0 A7 imm0002 signed2 | `cmplwi r7, 2` |
| `800031C4` | `0001C4` | `41 82 00 24` / `41820024` | B op16 disp+36 AA0 LK0 target800031E8 BO12 BI2 → `beq 800031E8` | `beq 0x800031e8` |
| `800031C8` | `0001C8` | `28 07 00 03` / `28070003` | D op10 D/S0 A7 imm0003 signed3 | `cmplwi r7, 3` |
| `800031CC` | `0001CC` | `38 A0 00 01` / `38A00001` | D op14 D/S5 A0 imm0001 signed1 | `li r5, 1` |
| `800031D0` | `0001D0` | `41 82 00 18` / `41820018` | B op16 disp+24 AA0 LK0 target800031E8 BO12 BI2 → `beq 800031E8` | `beq 0x800031e8` |
| `800031D4` | `0001D4` | `28 07 00 04` / `28070004` | D op10 D/S0 A7 imm0004 signed4 | `cmplwi r7, 4` |
| `800031D8` | `0001D8` | `40 82 00 20` / `40820020` | B op16 disp+32 AA0 LK0 target800031F8 BO4 BI2 → `bne 800031F8` | `bne 0x800031f8` |
| `800031DC` | `0001DC` | `38 A0 00 02` / `38A00002` | D op14 D/S5 A0 imm0002 signed2 | `li r5, 2` |
| `800031E0` | `0001E0` | `4B FF FF 61` / `4BFFFF61` | I op18 disp-160 AA0 LK1 target80003140 → `bl 80003140` | `bl 0x80003140` |
| `800031E4` | `0001E4` | `48 00 00 14` / `48000014` | I op18 disp+20 AA0 LK0 target800031F8 → `b 800031F8` | `b 0x800031f8` |
| `800031E8` | `0001E8` | `3C C0 80 3A` / `3CC0803A` | D op15 D/S6 A0 imm803A signed-32710 | `lis r6, -0x7fc6` |
| `800031EC` | `0001EC` | `38 C6 F8 E0` / `38C6F8E0` | D op14 D/S6 A6 immF8E0 signed-1824 | `addi r6, r6, -0x720` |
| `800031F0` | `0001F0` | `7C C8 03 A6` / `7CC803A6` | X op31 D/S/BO6 A/BI8 B0 XO467 Rc/LK0 SPR8 | `mtlr r6` |
| `800031F4` | `0001F4` | `4E 80 00 21` / `4E800021` | XL op19 D/S/BO20 A/BI0 B0 XO16 Rc/LK1 | `blrl ` |
| `800031F8` | `0001F8` | `3C C0 80 00` / `3CC08000` | D op15 D/S6 A0 imm8000 signed-32768 | `lis r6, -0x8000` |
| `800031FC` | `0001FC` | `38 C6 00 F4` / `38C600F4` | D op14 D/S6 A6 imm00F4 signed244 | `addi r6, r6, 0xf4` |
| `80003200` | `000200` | `80 A6 00 00` / `80A60000` | D op32 D/S5 A6 imm0000 signed0 | `lwz r5, 0(r6)` |
| `80003204` | `000204` | `28 05 00 00` / `28050000` | D op10 D/S0 A5 imm0000 signed0 | `cmplwi r5, 0` |
| `80003208` | `000208` | `41 A2 00 50` / `41A20050` | B op16 disp+80 AA0 LK0 target80003258 BO13 BI2 → `beq 80003258` | `beq 0x80003258` |
| `8000320C` | `00020C` | `80 C5 00 08` / `80C50008` | D op32 D/S6 A5 imm0008 signed8 | `lwz r6, 8(r5)` |
| `80003210` | `000210` | `28 06 00 00` / `28060000` | D op10 D/S0 A6 imm0000 signed0 | `cmplwi r6, 0` |
| `80003214` | `000214` | `41 A2 00 44` / `41A20044` | B op16 disp+68 AA0 LK0 target80003258 BO13 BI2 → `beq 80003258` | `beq 0x80003258` |
| `80003218` | `000218` | `7C C5 32 14` / `7CC53214` | X op31 D/S/BO6 A/BI5 B6 XO266 Rc/LK0 | `add r6, r5, r6` |
| `8000321C` | `00021C` | `81 C6 00 00` / `81C60000` | D op32 D/S14 A6 imm0000 signed0 | `lwz r14, 0(r6)` |
| `80003220` | `000220` | `28 0E 00 00` / `280E0000` | D op10 D/S0 A14 imm0000 signed0 | `cmplwi r14, 0` |
| `80003224` | `000224` | `41 82 00 34` / `41820034` | B op16 disp+52 AA0 LK0 target80003258 BO12 BI2 → `beq 80003258` | `beq 0x80003258` |
| `80003228` | `000228` | `39 E6 00 04` / `39E60004` | D op14 D/S15 A6 imm0004 signed4 | `addi r15, r6, 4` |
| `8000322C` | `00022C` | `7D C9 03 A6` / `7DC903A6` | X op31 D/S/BO14 A/BI9 B0 XO467 Rc/LK0 SPR9 | `mtctr r14` |
| `80003230` | `000230` | `38 C6 00 04` / `38C60004` | D op14 D/S6 A6 imm0004 signed4 | `addi r6, r6, 4` |
| `80003234` | `000234` | `80 E6 00 00` / `80E60000` | D op32 D/S7 A6 imm0000 signed0 | `lwz r7, 0(r6)` |
| `80003238` | `000238` | `7C E7 2A 14` / `7CE72A14` | X op31 D/S/BO7 A/BI7 B5 XO266 Rc/LK0 | `add r7, r7, r5` |
| `8000323C` | `00023C` | `90 E6 00 00` / `90E60000` | D op36 D/S7 A6 imm0000 signed0 | `stw r7, 0(r6)` |
| `80003240` | `000240` | `42 00 FF F0` / `4200FFF0` | B op16 disp-16 AA0 LK0 target80003230 BO16 BI0 → `bdnz 80003230` | `bdnz 0x80003230` |
| `80003244` | `000244` | `3C A0 80 00` / `3CA08000` | D op15 D/S5 A0 imm8000 signed-32768 | `lis r5, -0x8000` |
| `80003248` | `000248` | `38 A5 00 34` / `38A50034` | D op14 D/S5 A5 imm0034 signed52 | `addi r5, r5, 0x34` |
| `8000324C` | `00024C` | `55 E7 00 34` / `55E70034` | M op21 S15 A7 SH0 MB0 ME26 Rc0 | `rlwinm r7, r15, 0, 0, 0x1a` |
| `80003250` | `000250` | `90 E5 00 00` / `90E50000` | D op36 D/S7 A5 imm0000 signed0 | `stw r7, 0(r5)` |
| `80003254` | `000254` | `48 00 00 0C` / `4800000C` | I op18 disp+12 AA0 LK0 target80003260 → `b 80003260` | `b 0x80003260` |
| `80003258` | `000258` | `39 C0 00 00` / `39C00000` | D op14 D/S14 A0 imm0000 signed0 | `li r14, 0` |
| `8000325C` | `00025C` | `39 E0 00 00` / `39E00000` | D op14 D/S15 A0 imm0000 signed0 | `li r15, 0` |
| `80003260` | `000260` | `48 36 D9 91` / `4836D991` | I op18 disp+3594640 AA0 LK1 target80370BF0 → `bl 80370BF0` | `bl 0x80370bf0` |
| `80003264` | `000264` | `48 36 DC 05` / `4836DC05` | I op18 disp+3595268 AA0 LK1 target80370E68 → `bl 80370E68` | `bl 0x80370e68` |
| `80370BF0` | `36A4B0` | `3C 80 80 00` / `3C808000` | D op15 D/S4 A0 imm8000 signed-32768 | `lis r4, -0x8000` |
| `80370BF4` | `36A4B4` | `38 04 00 40` / `38040040` | D op14 D/S0 A4 imm0040 signed64 | `addi r0, r4, 0x40` |
| `80370BF8` | `36A4B8` | `3C 60 80 37` / `3C608037` | D op15 D/S3 A0 imm8037 signed-32713 | `lis r3, -0x7fc9` |
| `80370BFC` | `36A4BC` | `90 0D 5A 18` / `900D5A18` | D op36 D/S0 A13 imm5A18 signed23064 | `stw r0, 0x5a18(r13)` |
| `80370C00` | `36A4C0` | `38 63 0C 60` / `38630C60` | D op14 D/S3 A3 imm0C60 signed3168 | `addi r3, r3, 0xc60` |
| `80370C04` | `36A4C4` | `3C 03 80 00` / `3C038000` | D op15 D/S0 A3 imm8000 signed-32768 | `addis r0, r3, -0x8000` |
| `80370C08` | `36A4C8` | `90 04 00 48` / `90040048` | D op36 D/S0 A4 imm0048 signed72 | `stw r0, 0x48(r4)` |
| `80370C0C` | `36A4CC` | `38 00 00 01` / `38000001` | D op14 D/S0 A0 imm0001 signed1 | `li r0, 1` |
| `80370C10` | `36A4D0` | `90 0D 5A 1C` / `900D5A1C` | D op36 D/S0 A13 imm5A1C signed23068 | `stw r0, 0x5a1c(r13)` |
| `80370C14` | `36A4D4` | `4E 80 00 20` / `4E800020` | XL op19 D/S/BO20 A/BI0 B0 XO16 Rc/LK0 | `blr ` |
| `80370E68` | `36A728` | `7C 08 02 A6` / `7C0802A6` | X op31 D/S/BO0 A/BI8 B0 XO339 Rc/LK0 SPR8 | `mflr r0` |
| `80370E6C` | `36A72C` | `90 01 00 04` / `90010004` | D op36 D/S0 A1 imm0004 signed4 | `stw r0, 4(r1)` |
| `80370E70` | `36A730` | `94 21 FF E8` / `9421FFE8` | D op37 D/S1 A1 immFFE8 signed-24 | `stwu r1, -0x18(r1)` |
| `80370E74` | `36A734` | `93 E1 00 14` / `93E10014` | D op36 D/S31 A1 imm0014 signed20 | `stw r31, 0x14(r1)` |
| `80370E78` | `36A738` | `93 C1 00 10` / `93C10010` | D op36 D/S30 A1 imm0010 signed16 | `stw r30, 0x10(r1)` |
| `80370E7C` | `36A73C` | `93 A1 00 0C` / `93A1000C` | D op36 D/S29 A1 imm000C signed12 | `stw r29, 0xc(r1)` |
| `80370E80` | `36A740` | `80 0D 5A 40` / `800D5A40` | D op32 D/S0 A13 imm5A40 signed23104 | `lwz r0, 0x5a40(r13)` |
| `80370E84` | `36A744` | `3C 60 80 58` / `3C608058` | D op15 D/S3 A0 imm8058 signed-32680 | `lis r3, -0x7fa8` |
| `80370E88` | `36A748` | `3B E3 6C 40` / `3BE36C40` | D op14 D/S31 A3 imm6C40 signed27712 | `addi r31, r3, 0x6c40` |
| `80370E8C` | `36A74C` | `2C 00 00 00` / `2C000000` | D op11 D/S0 A0 imm0000 signed0 | `cmpwi r0, 0` |
| `80370E90` | `36A750` | `3C 60 80 56` / `3C608056` | D op15 D/S3 A0 imm8056 signed-32682 | `lis r3, -0x7faa` |
| `80370E94` | `36A754` | `3B C3 10 F8` / `3BC310F8` | D op14 D/S30 A3 imm10F8 signed4344 | `addi r30, r3, 0x10f8` |
| `80370E98` | `36A758` | `40 82 04 94` / `40820494` | B op16 disp+1172 AA0 LK0 target8037132C BO4 BI2 → `bne 8037132C` | `bne 0x8037132c` |
| `80370E9C` | `36A75C` | `38 00 00 01` / `38000001` | D op14 D/S0 A0 imm0001 signed1 | `li r0, 1` |
| `80370EA0` | `36A760` | `90 0D 5A 40` / `900D5A40` | D op36 D/S0 A13 imm5A40 signed23104 | `stw r0, 0x5a40(r13)` |
| `80370EA4` | `36A764` | `48 00 87 A5` / `480087A5` | I op18 disp+34724 AA0 LK1 target80379648 → `bl 80379648` | `bl 0x80379648` |
| `80379628` | `372EE8` | `7C 6D 42 E6` / `7C6D42E6` | X op31 D/S/BO3 A/BI13 B8 XO371 Rc/LK0 TBR269 | `mftbu r3` |
| `80379648` | `372F08` | `7C 08 02 A6` / `7C0802A6` | X op31 D/S/BO0 A/BI8 B0 XO339 Rc/LK0 SPR8 | `mflr r0` |
| `8037964C` | `372F0C` | `90 01 00 04` / `90010004` | D op36 D/S0 A1 imm0004 signed4 | `stw r0, 4(r1)` |
| `80379650` | `372F10` | `94 21 FF E0` / `9421FFE0` | D op37 D/S1 A1 immFFE0 signed-32 | `stwu r1, -0x20(r1)` |
| `80379654` | `372F14` | `93 E1 00 1C` / `93E1001C` | D op36 D/S31 A1 imm001C signed28 | `stw r31, 0x1c(r1)` |
| `80379658` | `372F18` | `93 C1 00 18` / `93C10018` | D op36 D/S30 A1 imm0018 signed24 | `stw r30, 0x18(r1)` |
| `8037965C` | `372F1C` | `93 A1 00 14` / `93A10014` | D op36 D/S29 A1 imm0014 signed20 | `stw r29, 0x14(r1)` |
| `80379660` | `372F20` | `4B FF CA BD` / `4BFFCABD` | I op18 disp-13636 AA0 LK1 target8037611C → `bl 8037611C` | `bl 0x8037611c` |
| `80379664` | `372F24` | `7C 7F 1B 78` / `7C7F1B78` | X op31 D/S/BO3 A/BI31 B3 XO444 Rc/LK0 | `mr r31, r3` |
| `80379668` | `372F28` | `4B FF FF C1` / `4BFFFFC1` | I op18 disp-64 AA0 LK1 target80379628 → `bl 80379628` | `bl 0x80379628` |
