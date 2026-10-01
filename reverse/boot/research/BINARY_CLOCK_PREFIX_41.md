# Binary clock and next-input boundary, checkpoint41 research
Authority: pinned PAL DOL SHA256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
**PROVEN bytes/decoding:** 71 exact words below. Original bytes, independent opcode fields, existing Gekko decoder and Capstone are cross-checked. **STRONG research projection** of the admitted debugger-step interval reaches before80373AC4. **UNKNOWN production clock:** connected checkpoint40 remains before80379628. Broader regions include unexecuted stop/successor words; they do not imply runtime implementation.
| VA | File offset | Bytes / BE word | Opcode fields, assembly and successors |
|---|---|---|---|
| `80379628` | `372EE8` | `7C 6D 42 E6` / `7C6D42E6` | op=31; RD/RS=3; RA=13; RB=8; imm=42E6; XO=371; Rc=0; TBR269; Capstone `mftbu r3` |
| `8037962C` | `372EEC` | `7C 8C 42 E6` / `7C8C42E6` | op=31; RD/RS=4; RA=12; RB=8; imm=42E6; XO=371; Rc=0; TBR268; Capstone `mftb r4, 0x10c` |
| `80379630` | `372EF0` | `7C AD 42 E6` / `7CAD42E6` | op=31; RD/RS=5; RA=13; RB=8; imm=42E6; XO=371; Rc=0; TBR269; Capstone `mftbu r5` |
| `80379634` | `372EF4` | `7C 03 28 00` / `7C032800` | op=31; RD/RS=0; RA=3; RB=5; imm=2800; XO=0; Rc=0; Capstone `cmpw r3, r5` |
| `80379638` | `372EF8` | `40 82 FF F0` / `4082FFF0` | op=16; RD/RS=4; RA=2; RB=31; imm=FFF0; Capstone `bne 0x80379628`; BO=4; BI=2; BD=FFF0; AA=0; LK=0; → `bne 80379628` |
| `8037963C` | `372EFC` | `4E 80 00 20` / `4E800020` | op=19; RD/RS=20; RA=0; RB=0; imm=0020; Capstone `blr ` |
| `80379648` | `372F08` | `7C 08 02 A6` / `7C0802A6` | op=31; RD/RS=0; RA=8; RB=0; imm=02A6; XO=339; Rc=0; SPR8; Capstone `mflr r0` |
| `8037964C` | `372F0C` | `90 01 00 04` / `90010004` | op=36; RD/RS=0; RA=1; RB=0; imm=0004; Capstone `stw r0, 4(r1)` |
| `80379650` | `372F10` | `94 21 FF E0` / `9421FFE0` | op=37; RD/RS=1; RA=1; RB=31; imm=FFE0; Capstone `stwu r1, -0x20(r1)` |
| `80379654` | `372F14` | `93 E1 00 1C` / `93E1001C` | op=36; RD/RS=31; RA=1; RB=0; imm=001C; Capstone `stw r31, 0x1c(r1)` |
| `80379658` | `372F18` | `93 C1 00 18` / `93C10018` | op=36; RD/RS=30; RA=1; RB=0; imm=0018; Capstone `stw r30, 0x18(r1)` |
| `8037965C` | `372F1C` | `93 A1 00 14` / `93A10014` | op=36; RD/RS=29; RA=1; RB=0; imm=0014; Capstone `stw r29, 0x14(r1)` |
| `80379660` | `372F20` | `4B FF CA BD` / `4BFFCABD` | op=18; RD/RS=31; RA=31; RB=25; imm=CABD; Capstone `bl 0x8037611c`; LI=03FFCABC; AA=0; LK=1; → `bl 8037611C` |
| `80379664` | `372F24` | `7C 7F 1B 78` / `7C7F1B78` | op=31; RD/RS=3; RA=31; RB=3; imm=1B78; XO=444; Rc=0; Capstone `mr r31, r3` |
| `80379668` | `372F28` | `4B FF FF C1` / `4BFFFFC1` | op=18; RD/RS=31; RA=31; RB=31; imm=FFC1; Capstone `bl 0x80379628`; LI=03FFFFC0; AA=0; LK=1; → `bl 80379628` |
| `8037966C` | `372F2C` | `3C C0 80 00` / `3CC08000` | op=15; RD/RS=6; RA=0; RB=16; imm=8000; Capstone `lis r6, -0x8000` |
| `80379670` | `372F30` | `80 A6 30 DC` / `80A630DC` | op=32; RD/RS=5; RA=6; RB=6; imm=30DC; Capstone `lwz r5, 0x30dc(r6)` |
| `80379674` | `372F34` | `80 06 30 D8` / `800630D8` | op=32; RD/RS=0; RA=6; RB=6; imm=30D8; Capstone `lwz r0, 0x30d8(r6)` |
| `80379678` | `372F38` | `7F A5 20 14` / `7FA52014` | op=31; RD/RS=29; RA=5; RB=4; imm=2014; XO=10; Rc=0; Capstone `addc r29, r5, r4` |
| `8037967C` | `372F3C` | `7F C0 19 14` / `7FC01914` | op=31; RD/RS=30; RA=0; RB=3; imm=1914; XO=138; Rc=0; Capstone `adde r30, r0, r3` |
| `80379680` | `372F40` | `7F E3 FB 78` / `7FE3FB78` | op=31; RD/RS=31; RA=3; RB=31; imm=FB78; XO=444; Rc=0; Capstone `mr r3, r31` |
| `80379684` | `372F44` | `4B FF CA C1` / `4BFFCAC1` | op=18; RD/RS=31; RA=31; RB=25; imm=CAC1; Capstone `bl 0x80376144`; LI=03FFCAC0; AA=0; LK=1; → `bl 80376144` |
| `80379688` | `372F48` | `7F A4 EB 78` / `7FA4EB78` | op=31; RD/RS=29; RA=4; RB=29; imm=EB78; XO=444; Rc=0; Capstone `mr r4, r29` |
| `8037968C` | `372F4C` | `7F C3 F3 78` / `7FC3F378` | op=31; RD/RS=30; RA=3; RB=30; imm=F378; XO=444; Rc=0; Capstone `mr r3, r30` |
| `80379690` | `372F50` | `80 01 00 24` / `80010024` | op=32; RD/RS=0; RA=1; RB=0; imm=0024; Capstone `lwz r0, 0x24(r1)` |
| `80379694` | `372F54` | `83 E1 00 1C` / `83E1001C` | op=32; RD/RS=31; RA=1; RB=0; imm=001C; Capstone `lwz r31, 0x1c(r1)` |
| `80379698` | `372F58` | `83 C1 00 18` / `83C10018` | op=32; RD/RS=30; RA=1; RB=0; imm=0018; Capstone `lwz r30, 0x18(r1)` |
| `8037969C` | `372F5C` | `83 A1 00 14` / `83A10014` | op=32; RD/RS=29; RA=1; RB=0; imm=0014; Capstone `lwz r29, 0x14(r1)` |
| `803796A0` | `372F60` | `38 21 00 20` / `38210020` | op=14; RD/RS=1; RA=1; RB=0; imm=0020; Capstone `addi r1, r1, 0x20` |
| `803796A4` | `372F64` | `7C 08 03 A6` / `7C0803A6` | op=31; RD/RS=0; RA=8; RB=0; imm=03A6; XO=467; Rc=0; SPR8; Capstone `mtlr r0` |
| `803796A8` | `372F68` | `4E 80 00 20` / `4E800020` | op=19; RD/RS=20; RA=0; RB=0; imm=0020; Capstone `blr ` |
| `8037611C` | `36F9DC` | `7C 60 00 A6` / `7C6000A6` | op=31; RD/RS=3; RA=0; RB=0; imm=00A6; XO=83; Rc=0; Capstone `mfmsr r3` |
| `80376120` | `36F9E0` | `54 64 04 5E` / `5464045E` | op=21; RD/RS=3; RA=4; RB=0; imm=045E; Capstone `rlwinm r4, r3, 0, 0x11, 0xf` |
| `80376124` | `36F9E4` | `7C 80 01 24` / `7C800124` | op=31; RD/RS=4; RA=0; RB=0; imm=0124; XO=146; Rc=0; Capstone `mtmsr r4` |
| `80376128` | `36F9E8` | `54 63 8F FE` / `54638FFE` | op=21; RD/RS=3; RA=3; RB=17; imm=8FFE; Capstone `rlwinm r3, r3, 0x11, 0x1f, 0x1f` |
| `8037612C` | `36F9EC` | `4E 80 00 20` / `4E800020` | op=19; RD/RS=20; RA=0; RB=0; imm=0020; Capstone `blr ` |
| `80376144` | `36FA04` | `2C 03 00 00` / `2C030000` | op=11; RD/RS=0; RA=3; RB=0; imm=0000; Capstone `cmpwi r3, 0` |
| `80376148` | `36FA08` | `7C 80 00 A6` / `7C8000A6` | op=31; RD/RS=4; RA=0; RB=0; imm=00A6; XO=83; Rc=0; Capstone `mfmsr r4` |
| `8037614C` | `36FA0C` | `41 82 00 0C` / `4182000C` | op=16; RD/RS=12; RA=2; RB=0; imm=000C; Capstone `beq 0x80376158`; BO=12; BI=2; BD=000C; AA=0; LK=0; → `beq 80376158` |
| `80376150` | `36FA10` | `60 85 80 00` / `60858000` | op=24; RD/RS=4; RA=5; RB=16; imm=8000; Capstone `ori r5, r4, 0x8000` |
| `80376154` | `36FA14` | `48 00 00 08` / `48000008` | op=18; RD/RS=0; RA=0; RB=0; imm=0008; Capstone `b 0x8037615c`; LI=00000008; AA=0; LK=0; → `b 8037615C` |
| `80376158` | `36FA18` | `54 85 04 5E` / `5485045E` | op=21; RD/RS=4; RA=5; RB=0; imm=045E; Capstone `rlwinm r5, r4, 0, 0x11, 0xf` |
| `8037615C` | `36FA1C` | `7C A0 01 24` / `7CA00124` | op=31; RD/RS=5; RA=0; RB=0; imm=0124; XO=146; Rc=0; Capstone `mtmsr r5` |
| `80376160` | `36FA20` | `54 83 8F FE` / `54838FFE` | op=21; RD/RS=4; RA=3; RB=17; imm=8FFE; Capstone `rlwinm r3, r4, 0x11, 0x1f, 0x1f` |
| `80376164` | `36FA24` | `4E 80 00 20` / `4E800020` | op=19; RD/RS=20; RA=0; RB=0; imm=0020; Capstone `blr ` |
| `80370EA8` | `36A768` | `90 8D 5A 54` / `908D5A54` | op=36; RD/RS=4; RA=13; RB=11; imm=5A54; Capstone `stw r4, 0x5a54(r13)` |
| `80370EAC` | `36A76C` | `90 6D 5A 50` / `906D5A50` | op=36; RD/RS=3; RA=13; RB=11; imm=5A50; Capstone `stw r3, 0x5a50(r13)` |
| `80370EB0` | `36A770` | `48 00 52 6D` / `4800526D` | op=18; RD/RS=0; RA=0; RB=10; imm=526D; Capstone `bl 0x8037611c`; LI=0000526C; AA=0; LK=1; → `bl 8037611C` |
| `80370EB4` | `36A774` | `38 7F 00 50` / `387F0050` | op=14; RD/RS=3; RA=31; RB=0; imm=0050; Capstone `addi r3, r31, 0x50` |
| `80370EB8` | `36A778` | `48 00 2B FD` / `48002BFD` | op=18; RD/RS=0; RA=0; RB=5; imm=2BFD; Capstone `bl 0x80373ab4`; LI=00002BFC; AA=0; LK=1; → `bl 80373AB4` |
| `80373AB4` | `36D374` | `7C 08 02 A6` / `7C0802A6` | op=31; RD/RS=0; RA=8; RB=0; imm=02A6; XO=339; Rc=0; SPR8; Capstone `mflr r0` |
| `80373AB8` | `36D378` | `90 01 00 04` / `90010004` | op=36; RD/RS=0; RA=1; RB=0; imm=0004; Capstone `stw r0, 4(r1)` |
| `80373ABC` | `36D37C` | `94 21 FF F8` / `9421FFF8` | op=37; RD/RS=1; RA=1; RB=31; imm=FFF8; Capstone `stwu r1, -8(r1)` |
| `80373AC0` | `36D380` | `3C A0 80 00` / `3CA08000` | op=15; RD/RS=5; RA=0; RB=16; imm=8000; Capstone `lis r5, -0x8000` |
| `80373AC4` | `36D384` | `80 85 30 F0` / `808530F0` | op=32; RD/RS=4; RA=5; RB=6; imm=30F0; Capstone `lwz r4, 0x30f0(r5)` |
| `80373AC8` | `36D388` | `7C 05 20 40` / `7C052040` | op=31; RD/RS=0; RA=5; RB=4; imm=2040; XO=32; Rc=0; Capstone `cmplw r5, r4` |
| `80373ACC` | `36D38C` | `41 81 00 10` / `41810010` | op=16; RD/RS=12; RA=1; RB=0; imm=0010; Capstone `bgt 0x80373adc`; BO=12; BI=1; BD=0010; AA=0; LK=0; → `bgt 80373ADC` |
| `80373AD0` | `36D390` | `38 A0 00 1C` / `38A0001C` | op=14; RD/RS=5; RA=0; RB=0; imm=001C; Capstone `li r5, 0x1c` |
| `80373AD4` | `36D394` | `4B C9 1A 21` / `4BC91A21` | op=18; RD/RS=30; RA=9; RB=3; imm=1A21; Capstone `bl 0x800054f4`; LI=03C91A20; AA=0; LK=1; → `bl 800054F4` |
| `80373AD8` | `36D398` | `48 00 00 0C` / `4800000C` | op=18; RD/RS=0; RA=0; RB=0; imm=000C; Capstone `b 0x80373ae4`; LI=0000000C; AA=0; LK=0; → `b 80373AE4` |
| `80373ADC` | `36D39C` | `38 00 00 00` / `38000000` | op=14; RD/RS=0; RA=0; RB=0; imm=0000; Capstone `li r0, 0` |
| `80373AE0` | `36D3A0` | `90 03 00 00` / `90030000` | op=36; RD/RS=0; RA=3; RB=0; imm=0000; Capstone `stw r0, 0(r3)` |
| `80373AE4` | `36D3A4` | `80 01 00 0C` / `8001000C` | op=32; RD/RS=0; RA=1; RB=0; imm=000C; Capstone `lwz r0, 0xc(r1)` |
| `80373AE8` | `36D3A8` | `38 21 00 08` / `38210008` | op=14; RD/RS=1; RA=1; RB=0; imm=0008; Capstone `addi r1, r1, 8` |
| `80373AEC` | `36D3AC` | `7C 08 03 A6` / `7C0803A6` | op=31; RD/RS=0; RA=8; RB=0; imm=03A6; XO=467; Rc=0; SPR8; Capstone `mtlr r0` |
| `80373AF0` | `36D3B0` | `4E 80 00 20` / `4E800020` | op=19; RD/RS=20; RA=0; RB=0; imm=0020; Capstone `blr ` |
| `80376EBC` | `37077C` | `7C AC 42 E6` / `7CAC42E6` | op=31; RD/RS=5; RA=12; RB=8; imm=42E6; XO=371; Rc=0; TBR268; Capstone `mftb r5, 0x10c` |
| `80376EC0` | `370780` | `7C CC 42 E6` / `7CCC42E6` | op=31; RD/RS=6; RA=12; RB=8; imm=42E6; XO=371; Rc=0; TBR268; Capstone `mftb r6, 0x10c` |
| `80376EC4` | `370784` | `7C E5 30 50` / `7CE53050` | op=31; RD/RS=7; RA=5; RB=6; imm=3050; XO=40; Rc=0; Capstone `subf r7, r5, r6` |
| `80376EC8` | `370788` | `28 07 11 24` / `28071124` | op=10; RD/RS=0; RA=7; RB=2; imm=1124; Capstone `cmplwi r7, 0x1124` |
| `80376ECC` | `37078C` | `41 80 FF F4` / `4180FFF4` | op=16; RD/RS=12; RA=0; RB=31; imm=FFF4; Capstone `blt 0x80376ec0`; BO=12; BI=0; BD=FFF4; AA=0; LK=0; → `blt 80376EC0` |

## State/control contract

TBU/TBL/TBU reads are distinct samples; compare is signed32 plus XER.SO; inequality retries. No mftb memory fence exists. addc/adde replace CA only and leave CR0 intact; restoration compares savedEE and preserves other currentMSR bits. Offsets are loaded lowDC then highD8. Return saves/restores use actual owned stack aliases. The caller writes low805F1F54 then high805F1F50, disables EE again, and allocates the eight-byte next frame, overwriting old clock slots only after restoration. At80373AC4 the unprovided800030F0 load has not executed.

Polling at80376EBC..ECC retains its first low sample and resamples the second. Threshold is **0x1124=4388 ticks**, condition unsigned modulo32 elapsed<4388, retry to80376EC0. This downstream device interval is not connected to boot here.

See CLOCK_RESEARCH_41.md, CLOCK_SOURCE_AUDIT_41.md, CLOCK_BINARY_AUDIT_41.md and CLOCK_ADVERSARIAL_41.md for producer evidence, consumer scope, exclusions and validation.
