# MISSION_PARAMETER_MAP — where every mission parameter comes from, per stage

Companion to `MISSION_SYSTEM.md`. Tables in §4–§7 are generated from the DOL + `files/nukkoro2.inf` by
`python tools/agent_mission_table.py [stages|rows]` (re-run it to refresh; it reads only the DOL, the `.inf` file and the
toolkit caches). Confidence tags: PROVEN / STRONG / LIKELY / UNKNOWN. Class names are RTTI originals; function names in
*italics* are semantic guesses.

## 1. Pipeline of a parameter (PROVEN)

```
 DOL stage table 0x804C5AE8           DOL defaults fn_8016CB48              files/nukkoro2.inf
 [idx].+0x20+0xC*slot = key  ─┐       key -> {type, param, 1, 0}  ─┐       [<name>] MISSIONCOUNT_x : A B ─┐
                              │       (fn_8016C8EC inserts into     │       TGameDefaultParam parser         │
                              │        MissionManager+0x30 map)     │       fn_801E030C -> handler -> fn_801DF470
                              │                                     └──────── map[key] = {type, param, A, B} (fn_8016C690)
                              └─ fn_80176FC8 ─> fn_8016E4EC ─> fn_8016B768 factory: switch(type) -> class ctor(args from desc)
```

| parameter | origin | first consumer | conf |
|---|---|---|---|
| mission key per stage/slot | stage table +0x20+0xC·slot (hard mode: +0x40) | fn_80176FC8 (0x80176FE0), fn_8016E4EC 0x8016E550 / 0x8016E58C | PROVEN |
| mission class (type 0–7) | fn_8016CB48 default descriptor | fn_8016B768 0x8016B7A4 + jump table 0x8052C930 | PROVEN |
| EnemyMission team (0 GUN / 1 Eggman / 2 Black Arms) | fn_8016CB48 descriptor param | factory 0x8016B8B8 → EnemyMission+0x58 (0x801F3E10) → vf0B 0x801F3CBC | PROVEN |
| success count / time limit | nukkoro2.inf first number (default 1) | factory: Count 0x8016B860, Enemy 0x8016B8C0, Timer 0x8016B90C, TimerGoal 0x8016B974, TimerCount 0x8016B9FC, Ring 0x8016BA48 | PROVEN |
| fail count / TimerCount time | nukkoro2.inf second number (default 0) | factory: Count 0x8016B868 (→ +0xC), TimerCount 0x8016B9DC (float time) | PROVEN |
| next stage per slot | stage table +0x1C+0xC·slot | fn_80205010 0x802050CC; fn_802D1EC8 0x802D1F58/0x802D1F60/0x802D1F68 | PROVEN |
| enemy team of a live enemy | table 0x804CD310[kind] | EnemyParamCommon::vf00 0x801A68A0 | PROVEN |
| enemies available | SET files (cmn + nrm/hrd + ds1) | fn_801A2BA0 census | PROVEN |

Not a source: SET object parameters do not carry mission counts (GOALRING has only `color`; MissionClearCollision has only
slot + radius) (PROVEN from the SET schema, `data/setobj_catalog.txt`).

---------------------------------------------------------------------------------------------------------------------

## 2. Stage table — `.rodata 0x804C5AE8`, 59 (0x3B) records × 0x50 bytes

Record index == stage index (`.bss 0x8057E80C`, game-state block +0xAC); stage id is at `.bss 0x8057E808` (+0xA8).
Lookups: fn_80177020 (*StageTable_FindById*, −1 = current id), fn_80176FE8 (*StageTable_IndexOfId*),
fn_80176FC8 (*StageTable_GetMissionKey(index, slot)*).

| offset | word | size | field | evidence | confidence |
|---|---|---|---|---|---|
| 0x00 | w0 | 4 | stage id (100 = Westopolis …) | fn_80177020 0x8017704C; fn_802D2010 | PROVEN |
| 0x04 | w1 | 4 | char* name = nukkoro section name ("City1", …) | fn_801E030C 0x801E03F4 (strcmp vs `[name]`) | PROVEN |
| 0x08 | w2 | 4 | flags: bit0 boss stage (skipped/forced route), bit3 → fn_8016F6C0 stores 50 instead of 0 (LIKELY start rings), bit4 (fn_80205058) ; 0x20 on EVENT entries, 0x6 on 2P | fn_802D1EC8 0x802D1F24; fn_800BB4F4; fn_8016F6F8 | PROVEN (reads), meanings LIKELY |
| 0x0C..0x18 | w3–w6 | 4×4 | per-stage code pointers (often 0) | +0xC called @0x80178780 in fn_801783D4 | PROVEN (call), meaning UNKNOWN |
| 0x1C | w7 | 4 | Dark: next stage **index** (−2 = none/end) | fn_802D1EC8 0x802D1F58; fn_80205010 0x802050CC (slot 0) | PROVEN |
| 0x20 | w8 | 4 | Dark: mission key (−1 = no mission) | fn_80176FC8 (slot 0) | PROVEN |
| 0x24 | w9 | 4 | Dark: id 0x2xx (lead: event/movie id) | no consumer found | UNKNOWN |
| 0x28 | w10 | 4 | Normal: next stage index | fn_802D1EC8 0x802D1F60 | PROVEN |
| 0x2C | w11 | 4 | Normal: mission key | fn_80176FC8 (slot 1) | PROVEN |
| 0x30 | w12 | 4 | Normal: id 0x2xx | – | UNKNOWN |
| 0x34 | w13 | 4 | Hero: next stage index | fn_802D1EC8 0x802D1F68 | PROVEN |
| 0x38 | w14 | 4 | Hero: mission key | fn_80176FC8 (slot 2) | PROVEN |
| 0x3C | w15 | 4 | Hero: id 0x2xx | – | UNKNOWN |
| 0x40 | w16 | 4 | hard-mode mission key (single mission, slot 1) | fn_8016E4EC 0x8016E550; fn_801DF470 0x801DF514 | PROVEN |
| 0x44 | w17 | 4 | id 0x7F9…0x80E (−1 on boss/2P/event) | – | UNKNOWN |
| 0x48 | w18 | 4 | id 0x226… | – | UNKNOWN |
| 0x4C | w19 | 4 | small int 0–7 | – | UNKNOWN |

Slot order **0 Dark, 1 Normal, 2 Hero** is PROVEN (MISSIONCOUNT_D/N/H → slot 0/1/2 in fn_801DF3F8/fn_801DF420/fn_801DF448;
fn_8016B330 "Dark"/"Hero" suffix by slot; Westopolis next indices 6/7/8 = stages 200/201/202).

---------------------------------------------------------------------------------------------------------------------

## 3. `nukkoro2.inf` format (TGameDefaultParam) — PROVEN

- Loaded at boot: fn_80048EFC (RenderWare init callback) → fn_801E258C → fn_801E04EC("nukkoro.inf") @0x801E2680 and
  fn_801E04EC("nukkoro2.inf") @0x801E2690. Plain text, Shift-JIS comments, CRLF.
- fn_801E030C (*TGameDefaultParam::ParseLine*): a line starting with `[` selects a section: `[ALL]` → +0x2C50 = −1, otherwise
  the stage-table index whose name (+0x04) equals the bracket text (loop over 0x3B entries). `#` lines are comments. Other lines
  are matched against 27 command names (+0x2C54 table) and dispatched through boost-bound member functions (command table
  `.data 0x8053E700`, 12-byte entries; registered by fn_801E0C68).
- Numbers: fn_801E0144 splits after `:` and fn_801E01D0 parses decimal, `-`, and `0x` hex (hex loop only accepts digits 0–9).
- Mission commands (all need a stage section; ignored under `[ALL]`):

| command | handler | fn_801DF470 args | key used | effect |
|---|---|---|---|---|
| `MISSIONCOUNT_D : A B` | fn_801DF3F8 (table entry 0x8053E804) | slot 0, cmd 0x15 | table +0x20 | desc.count = A, desc.failCount = B |
| `MISSIONCOUNT_N : A B` | fn_801DF420 (0x8053E7F8) | slot 1, cmd 0x14 | table +0x2C | same |
| `MISSIONCOUNT_H : A B` | fn_801DF448 (0x8053E7EC) | slot 2, cmd 0x13 | table +0x38 | same |
| `MISSIONCOUNT_HARD : A B` | fn_801DF3D0 (0x8053E840) | slot 1, cmd 0x1A | table +0x40 | same |

- fn_801DF470 (*TGameDefaultParam::SetMissionCount*): requires a per-stage block (+0x2C50 ≠ −1), exactly 2 numbers, and an
  existing descriptor; keeps `type/param`, replaces `count/failCount` (0x801DF530..0x801DF55C), writes with fn_8016C690.
- Meaning of A/B per class (file header comment in `nukkoro.inf` + factory code): Count/Enemy/Ring → A = number to reach;
  Timer/TimerGoal → A = seconds; TimerCount → A = number, **B = seconds**; Count → B = failure threshold (0 = no failure);
  Enemy → B ignored (forced 0). GoalMission ignores both.
- Keys are shared between stages (see §7) → the **last** section that writes a key wins (e.g. `[GUN2P_0802]` for key 0x28;
  `[City1]` also configures Practice, whose Dark/Hero keys are 2/1).

Other stage-section commands in the same file (not mission-related, listed for completeness): PLAYER, STARTSPD, STARTDEMO,
RANK_H/N/D, GOALEVENTPOS_H/N/D/X, START2P, MIPMAPK; global: ROM, DEBUGWHO, PREDVDLIST, HEAPINFO, LOGFILE, LOGREPORT, LANGUAGE,
SPEECH, CHECKSIZE, REGION, AREA.

---------------------------------------------------------------------------------------------------------------------

## 4. Stage table dump (all 59 records)

Triples are `(next stage index, mission key, +0x24/+0x30/+0x3C id)`.

| idx | addr | id | name | flags | Dark (next,key,+0x24) | Normal (next,key,+0x30) | Hero (next,key,+0x3C) | +0x40 hard key | +0x44 | +0x48 | +0x4C |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | 0x804C5AE8 | 0 | Practice | 0x0 | -2, 2, -1 | -2, 0, -1 | -2, 1, -1 | 0 | 0x7F9 | -1 | 0 |
| 1 | 0x804C5B38 | 1 | Practice01 | 0x0 | -2, -1, -1 | -2, -1, -1 | -2, -1, -1 | -1 | 0x7F9 | -1 | 0 |
| 2 | 0x804C5B88 | 2 | Practice02 | 0x0 | -2, -1, -1 | -2, -1, -1 | -2, -1, -1 | -1 | 0x7F9 | -1 | 0 |
| 3 | 0x804C5BD8 | 3 | Practice03 | 0x0 | -2, -1, -1 | -2, -1, -1 | -2, -1, -1 | -1 | 0x7F9 | -1 | 0 |
| 4 | 0x804C5C28 | 5 | EVENT | 0x20 | -2, -1, -1 | -2, -1, -1 | -2, -1, -1 | -1 | 0x7F9 | -1 | 0 |
| 5 | 0x804C5C78 | 100 | City1 | 0x0 | 6, 1, 0x229 | 7, 0, 0x227 | 8, 2, 0x228 | 0 | 0x7F9 | 0x226 | 2 |
| 6 | 0x804C5CC8 | 200 | Circuit | 0x0 | 9, 3, 0x249 | 9, -1, -1 | 10, 0, 0x248 | 0 | 0x7FE | 0x247 | 3 |
| 7 | 0x804C5D18 | 201 | Canyon1 | 0x0 | 9, 4, 0x23C | 10, 0, 0x23A | 11, 5, 0x23B | 0 | 0x7FC | 0x239 | 3 |
| 8 | 0x804C5D68 | 202 | Highway | 0x0 | 28, 0, 0x22F | 28, -1, -1 | 28, 6, 0x22E | 0 | 0x7FA | 0x22D | 0 |
| 9 | 0x804C5DB8 | 300 | HorrorCastle | 0x0 | 29, 7, 0x257 | 29, 0, 0x255 | 29, 8, 0x256 | 0 | 0x800 | 0x254 | 0 |
| 10 | 0x804C5E08 | 301 | PrisonIsland | 0x0 | 13, 9, 0x265 | 14, 0, 0x263 | 15, 10, 0x264 | 0 | 0x802 | 0x262 | 4 |
| 11 | 0x804C5E58 | 302 | Circus | 0x0 | 14, 11, 0x25E | 15, 0, 0x25C | 16, 12, 0x25D | 0 | 0x801 | 0x25B | 4 |
| 12 | 0x804C5EA8 | 400 | City2 | 0x0 | 17, 13, 0x235 | 18, -1, -1 | 18, 14, 0x234 | 37 | 0x7FB | 0x233 | 0 |
| 13 | 0x804C5EF8 | 401 | ARKPast1 | 0x0 | 30, 15, 0x279 | 30, 0, 0x277 | 30, 16, 0x278 | 38 | 0x805 | 0x276 | 0 |
| 14 | 0x804C5F48 | 402 | canyon2 | 0x0 | 18, 17, 0x243 | 19, 0, 0x241 | 20, 18, 0x242 | 0 | 0x7FD | 0x240 | 0 |
| 15 | 0x804C5F98 | 403 | eWorld | 0x0 | 31, 19, 0x250 | 31, 0, 0x24E | 31, 20, 0x24F | 39 | 0x7FF | 0x24D | 0 |
| 16 | 0x804C5FE8 | 404 | Ruins | 0x0 | 32, 0, 0x26B | 32, -1, -1 | 32, 21, 0x26A | 0 | 0x803 | 0x269 | 0 |
| 17 | 0x804C6038 | 500 | ARKRuins1 | 0x0 | 33, 22, 0x27F | 33, 0, 0x27E | 33, -1, -1 | 0 | 0x806 | 0x27D | 0 |
| 18 | 0x804C6088 | 501 | Sky | 0x0 | 22, 23, 0x299 | 23, 0, 0x297 | 24, 24, 0x298 | 0 | 0x80A | 0x296 | 5 |
| 19 | 0x804C60D8 | 502 | Jungle | 0x0 | 34, 25, 0x272 | 34, 0, 0x270 | 34, 26, 0x271 | 0 | 0x804 | 0x26F | 0 |
| 20 | 0x804C6128 | 503 | Space | 0x0 | 24, 27, 0x286 | 25, 0, 0x284 | 26, 28, 0x285 | 0 | 0x807 | 0x283 | 5 |
| 21 | 0x804C6178 | 504 | ARKPast2 | 0x0 | 25, -1, -1 | 25, 0, 0x28B | 26, 29, 0x28C | 0 | 0x808 | 0x28A | 0 |
| 22 | 0x804C61C8 | 600 | GunsBase | 0x0 | 36, 30, 0x29F | 36, -1, -1 | 35, 0, 0x29E | 0 | 0x80B | 0x29D | 6 |
| 23 | 0x804C6218 | 601 | DoomsBase1 | 0x0 | 38, 31, 0x2AB | 37, -1, -1 | 37, 0, 0x2AA | 0 | 0x80D | 0x2A9 | 6 |
| 24 | 0x804C6268 | 602 | EggmansBase | 0x0 | 39, 32, 0x2A5 | 39, -1, -1 | 39, 0, 0x2A4 | 0 | 0x80C | 0x2A3 | 6 |
| 25 | 0x804C62B8 | 603 | ARKRuins2 | 0x0 | 40, 33, 0x292 | 40, -1, -1 | 41, 34, 0x291 | 0 | 0x809 | 0x290 | 6 |
| 26 | 0x804C6308 | 604 | DoomsBase2 | 0x0 | 43, 35, 0x2B1 | 43, -1, -1 | 42, 0, 0x2B0 | 0 | 0x80E | 0x2AF | 6 |
| 27 | 0x804C6358 | 700 | DoomsCore | 0x10 | 44, -1, -1 | 44, -1, -1 | 44, 36, 0x2B4 | 0 | 0x80D | -1 | 0 |
| 28 | 0x804C63A8 | 210 | BossBK1 | 0x1 | 10, -1, 0x2B5 | 11, -1, 0x2B5 | 11, -1, 0x2B5 | -1 | 0xFFFFFFFF | -1 | 3 |
| 29 | 0x804C63F8 | 310 | BossEggMeka1 | 0x1 | 12, -1, 0x2B7 | 13, -1, 0x2B7 | 14, -1, 0x2B7 | -1 | 0xFFFFFFFF | -1 | 4 |
| 30 | 0x804C6448 | 410 | BossGUN1 | 0x1 | 17, -1, 0x2BA | 18, -1, 0x2BA | 19, -1, 0x2BA | -1 | 0xFFFFFFFF | -1 | 0 |
| 31 | 0x804C6498 | 411 | BossEggMeka2 | 0x1 | 19, -1, 0x2B8 | 20, -1, 0x2B8 | 21, -1, 0x2B8 | -1 | 0xFFFFFFFF | -1 | 0 |
| 32 | 0x804C64E8 | 412 | BossBK2 | 0x1 | 20, -1, 0x2B6 | 21, -1, 0x2B6 | 21, -1, 0x2B6 | -1 | 0xFFFFFFFF | -1 | 0 |
| 33 | 0x804C6538 | 510 | BossGUN2 | 0x1 | 22, -1, 0x2BB | 23, -1, 0x2BB | 23, -1, 0x2BB | -1 | 0xFFFFFFFF | -1 | 5 |
| 34 | 0x804C6588 | 511 | BossEggMeka3 | 0x1 | 23, -1, 0x2B9 | 24, -1, 0x2B9 | 25, -1, 0x2B9 | -1 | 0xFFFFFFFF | -1 | 5 |
| 35 | 0x804C65D8 | 610 | BossBlack_0610 | 0x1 | -2, -1, 0x2BC | -2, -1, 0x2BC | -2, -1, 0x2BC | -1 | 0xFFFFFFFF | -1 | 7 |
| 36 | 0x804C6628 | 611 | BossSonic_0611 | 0x1 | -2, -1, 0x2BD | -2, -1, 0x2BD | -2, -1, 0x2BD | -1 | 0xFFFFFFFF | -1 | 7 |
| 37 | 0x804C6678 | 612 | BossEggmanRobbo_0612 | 0x1 | -2, -1, 0x2BE | -2, -1, 0x2BE | -2, -1, 0x2BE | -1 | 0xFFFFFFFF | -1 | 7 |
| 38 | 0x804C66C8 | 613 | BossSonic_0613 | 0x1 | -2, -1, 0x2BF | -2, -1, 0x2BF | -2, -1, 0x2BF | -1 | 0xFFFFFFFF | -1 | 7 |
| 39 | 0x804C6718 | 614 | BossEggmanRobbo_0614 | 0x1 | -2, -1, 0x2C0 | -2, -1, 0x2C0 | -2, -1, 0x2C0 | -1 | 0xFFFFFFFF | -1 | 7 |
| 40 | 0x804C6768 | 615 | BossEggmanRobbo_0615 | 0x1 | -2, -1, 0x2C1 | -2, -1, 0x2C1 | -2, -1, 0x2C1 | -1 | 0xFFFFFFFF | -1 | 7 |
| 41 | 0x804C67B8 | 616 | BossBlack_0616 | 0x1 | -2, -1, 0x2C2 | -2, -1, 0x2C2 | -2, -1, 0x2C2 | -1 | 0xFFFFFFFF | -1 | 7 |
| 42 | 0x804C6808 | 617 | BossBlack_0617 | 0x1 | -2, -1, 0x2C3 | -2, -1, 0x2C3 | -2, -1, 0x2C3 | -1 | 0xFFFFFFFF | -1 | 7 |
| 43 | 0x804C6858 | 618 | BossSonic_0618 | 0x1 | -2, -1, 0x2C4 | -2, -1, 0x2C4 | -2, -1, 0x2C4 | -1 | 0xFFFFFFFF | -1 | 7 |
| 44 | 0x804C68A8 | 710 | BossLast | 0x19 | -2, -1, 0x2C5 | -2, -1, 0x2C5 | -2, -1, 0x2C5 | -1 | 0xFFFFFFFF | -1 | 0 |
| 45 | 0x804C68F8 | 800 | GUN2P_0800 | 0x6 | -2, -1, -1 | -2, 40, -1 | -2, -1, -1 | -1 | 0xFFFFFFFF | -1 | 0 |
| 46 | 0x804C6948 | 801 | GUN2P_0801 | 0x6 | -2, -1, -1 | -2, 40, -1 | -2, -1, -1 | -1 | 0xFFFFFFFF | -1 | 0 |
| 47 | 0x804C6998 | 802 | GUN2P_0802 | 0x6 | -2, -1, -1 | -2, 40, -1 | -2, -1, -1 | -1 | 0xFFFFFFFF | -1 | 0 |
| 48 | 0x804C69E8 | 901 | EVENT0901 | 0x20 | -2, -1, -1 | -2, -1, -1 | -2, -1, -1 | -1 | 0xFFFFFFFF | -1 | 0 |
| 49 | 0x804C6A38 | 902 | EVENT0902 | 0x20 | -2, -1, -1 | -2, -1, -1 | -2, -1, -1 | -1 | 0xFFFFFFFF | -1 | 0 |
| 50 | 0x804C6A88 | 903 | EVENT0903 | 0x20 | -2, -1, -1 | -2, -1, -1 | -2, -1, -1 | -1 | 0xFFFFFFFF | -1 | 0 |
| 51 | 0x804C6AD8 | 904 | EVENT0904 | 0x20 | -2, -1, -1 | -2, -1, -1 | -2, -1, -1 | -1 | 0xFFFFFFFF | -1 | 0 |
| 52 | 0x804C6B28 | 905 | EVENT0905 | 0x20 | -2, -1, -1 | -2, -1, -1 | -2, -1, -1 | -1 | 0xFFFFFFFF | -1 | 0 |
| 53 | 0x804C6B78 | 906 | EVENT0906 | 0x20 | -2, -1, -1 | -2, -1, -1 | -2, -1, -1 | -1 | 0xFFFFFFFF | -1 | 0 |
| 54 | 0x804C6BC8 | 907 | EVENT0907 | 0x20 | -2, -1, -1 | -2, -1, -1 | -2, -1, -1 | -1 | 0xFFFFFFFF | -1 | 0 |
| 55 | 0x804C6C18 | 908 | EVENT0908 | 0x20 | -2, -1, -1 | -2, -1, -1 | -2, -1, -1 | -1 | 0xFFFFFFFF | -1 | 0 |
| 56 | 0x804C6C68 | 909 | EVENT0909 | 0x20 | -2, -1, -1 | -2, -1, -1 | -2, -1, -1 | -1 | 0xFFFFFFFF | -1 | 0 |
| 57 | 0x804C6CB8 | 910 | EVENT0910 | 0x20 | -2, -1, -1 | -2, -1, -1 | -2, -1, -1 | -1 | 0xFFFFFFFF | -1 | 0 |
| 58 | 0x804C6D08 | 911 | EVENT0911 | 0x20 | -2, -1, -1 | -2, -1, -1 | -2, -1, -1 | -1 | 0xFFFFFFFF | -1 | 0 |


---------------------------------------------------------------------------------------------------------------------

## 5. Default descriptor map (fn_8016CB48 → fn_8016C8EC; PROVEN)

Every descriptor starts as `{type, param, count = 1, failCount = 0}`. `param` is only used by EnemyMission (team).


| key | type | param(+4) |
|---|---|---|
| 0x00 | 1 GoalMission | 0 |
| 0x01 | 3 EnemyMission | 0 |
| 0x02 | 3 EnemyMission | 2 |
| 0x03 | 2 CountMission | 0 |
| 0x04 | 2 CountMission | 0 |
| 0x05 | 3 EnemyMission | 2 |
| 0x06 | 2 CountMission | 0 |
| 0x07 | 2 CountMission | 0 |
| 0x08 | 2 CountMission | 0 |
| 0x09 | 3 EnemyMission | 0 |
| 0x0A | 2 CountMission | 0 |
| 0x0B | 3 EnemyMission | 0 |
| 0x0C | 7 RingMission | 0 |
| 0x0D | 6 TimerCountMission | 0 |
| 0x0E | 6 TimerCountMission | 0 |
| 0x0F | 3 EnemyMission | 0 |
| 0x10 | 2 CountMission | 0 |
| 0x11 | 2 CountMission | 0 |
| 0x12 | 2 CountMission | 0 |
| 0x13 | 2 CountMission | 0 |
| 0x14 | 2 CountMission | 0 |
| 0x15 | 3 EnemyMission | 2 |
| 0x16 | 2 CountMission | 0 |
| 0x17 | 2 CountMission | 0 |
| 0x18 | 3 EnemyMission | 2 |
| 0x19 | 3 EnemyMission | 0 |
| 0x1A | 2 CountMission | 0 |
| 0x1B | 2 CountMission | 0 |
| 0x1C | 5 TimerGoalMission | 0 |
| 0x1D | 3 EnemyMission | 2 |
| 0x1E | 2 CountMission | 0 |
| 0x1F | 3 EnemyMission | 0 |
| 0x20 | 2 CountMission | 0 |
| 0x21 | 5 TimerGoalMission | 0 |
| 0x22 | 4 TimerMission | 0 |
| 0x23 | 2 CountMission | 0 |
| 0x24 | 5 TimerGoalMission | 0 |
| 0x25 | 2 CountMission | 0 |
| 0x26 | 2 CountMission | 0 |
| 0x27 | 2 CountMission | 0 |
| 0x28 | 4 TimerMission | 0 |


---------------------------------------------------------------------------------------------------------------------

## 6. Effective missions per stage (after nukkoro2.inf; PROVEN)

Only slots with a key ≥ 0 are listed. "Hard/X" = stage table +0x40 (used only when `.bss 0x8057E885` is set).
EnemyMission requirements vs. enemies placed for that team in cmn+nrm+ds1 (`tools/agent_mission_enemycount.py`): 100 D 35/36,
100 H 45/45, 201 H 60/80, 301 D 40/41, 302 D 20/21, 401 D 60/61, 404 H 50/50, 501 H 35/35, 502 D 28/29, 504 H 35/35, 601 D 50/53
(always ≤ placed — STRONG support for the team mapping).


| idx | stage | name | slot | next idx (stage) | key | class | ctor params | count source |
|---|---|---|---|---|---|---|---|---|
| 0 | 0 | Practice | Dark | -2 | 0x02 | EnemyMission | team=Black Arms (SET 0x8C-0x95), required=45 (fail count forced 0) | nukkoro2.inf:22 [City1] MISSIONCOUNT_H |
| 0 | 0 | Practice | Normal | -2 | 0x00 | GoalMission | - | default@8016CB78 |
| 0 | 0 | Practice | Hero | -2 | 0x01 | EnemyMission | team=GUN (SET 0x64-0x77), required=35 (fail count forced 0) | nukkoro2.inf:21 [City1] MISSIONCOUNT_D |
| 0 | 0 | Practice | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 5 | 100 | City1 | Dark | 6 (200) | 0x01 | EnemyMission | team=GUN (SET 0x64-0x77), required=35 (fail count forced 0) | nukkoro2.inf:21 [City1] MISSIONCOUNT_D |
| 5 | 100 | City1 | Normal | 7 (201) | 0x00 | GoalMission | - | default@8016CB78 |
| 5 | 100 | City1 | Hero | 8 (202) | 0x02 | EnemyMission | team=Black Arms (SET 0x8C-0x95), required=45 (fail count forced 0) | nukkoro2.inf:22 [City1] MISSIONCOUNT_H |
| 5 | 100 | City1 | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 6 | 200 | Circuit | Dark | 9 (300) | 0x03 | CountMission | required=1, fail=0 | default@8016CBB4 |
| 6 | 200 | Circuit | Hero | 10 (301) | 0x00 | GoalMission | - | default@8016CB78 |
| 6 | 200 | Circuit | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 7 | 201 | Canyon1 | Dark | 9 (300) | 0x04 | CountMission | required=5, fail=0 | nukkoro2.inf:42 [Canyon1] MISSIONCOUNT_D |
| 7 | 201 | Canyon1 | Normal | 10 (301) | 0x00 | GoalMission | - | default@8016CB78 |
| 7 | 201 | Canyon1 | Hero | 11 (302) | 0x05 | EnemyMission | team=Black Arms (SET 0x8C-0x95), required=60 (fail count forced 0) | nukkoro2.inf:43 [Canyon1] MISSIONCOUNT_H |
| 7 | 201 | Canyon1 | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 8 | 202 | Highway | Dark | 28 (210) | 0x00 | GoalMission | - | default@8016CB78 |
| 8 | 202 | Highway | Hero | 28 (210) | 0x06 | CountMission | required=1, fail=0 | default@8016CBF0 |
| 8 | 202 | Highway | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 9 | 300 | HorrorCastle | Dark | 29 (310) | 0x07 | CountMission | required=5, fail=0 | nukkoro2.inf:64 [HorrorCastle] MISSIONCOUNT_D |
| 9 | 300 | HorrorCastle | Normal | 29 (310) | 0x00 | GoalMission | - | default@8016CB78 |
| 9 | 300 | HorrorCastle | Hero | 29 (310) | 0x08 | CountMission | required=2, fail=0 | nukkoro2.inf:65 [HorrorCastle] MISSIONCOUNT_H |
| 9 | 300 | HorrorCastle | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 10 | 301 | PrisonIsland | Dark | 13 (401) | 0x09 | EnemyMission | team=GUN (SET 0x64-0x77), required=40 (fail count forced 0) | nukkoro2.inf:76 [PrisonIsland] MISSIONCOUNT_D |
| 10 | 301 | PrisonIsland | Normal | 14 (402) | 0x00 | GoalMission | - | default@8016CB78 |
| 10 | 301 | PrisonIsland | Hero | 15 (403) | 0x0A | CountMission | required=5, fail=0 | nukkoro2.inf:77 [PrisonIsland] MISSIONCOUNT_H |
| 10 | 301 | PrisonIsland | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 11 | 302 | Circus | Dark | 14 (402) | 0x0B | EnemyMission | team=GUN (SET 0x64-0x77), required=20 (fail count forced 0) | nukkoro2.inf:88 [Circus] MISSIONCOUNT_D |
| 11 | 302 | Circus | Normal | 15 (403) | 0x00 | GoalMission | - | default@8016CB78 |
| 11 | 302 | Circus | Hero | 16 (404) | 0x0C | RingMission | required rings=400 | nukkoro2.inf:89 [Circus] MISSIONCOUNT_H |
| 11 | 302 | Circus | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 12 | 400 | City2 | Dark | 17 (500) | 0x0D | TimerCountMission | required=5, time=480s | nukkoro2.inf:101 [City2] MISSIONCOUNT_D |
| 12 | 400 | City2 | Hero | 18 (501) | 0x0E | TimerCountMission | required=20, time=480s | nukkoro2.inf:102 [City2] MISSIONCOUNT_H |
| 12 | 400 | City2 | Hard/X |  | 0x25 | CountMission | required=5, fail=0 | nukkoro2.inf:103 [City2] MISSIONCOUNT_HARD |
| 13 | 401 | ARKPast1 | Dark | 30 (410) | 0x0F | EnemyMission | team=GUN (SET 0x64-0x77), required=60 (fail count forced 0) | nukkoro2.inf:109 [ARKPast1] MISSIONCOUNT_D |
| 13 | 401 | ARKPast1 | Normal | 30 (410) | 0x00 | GoalMission | - | default@8016CB78 |
| 13 | 401 | ARKPast1 | Hero | 30 (410) | 0x10 | CountMission | required=10, fail=0 | nukkoro2.inf:108 [ARKPast1] MISSIONCOUNT_H |
| 13 | 401 | ARKPast1 | Hard/X |  | 0x26 | CountMission | required=10, fail=0 | nukkoro2.inf:117 [ARKPast1] MISSIONCOUNT_HARD |
| 14 | 402 | canyon2 | Dark | 18 (501) | 0x11 | CountMission | required=5, fail=0 | nukkoro2.inf:130 [canyon2] MISSIONCOUNT_D |
| 14 | 402 | canyon2 | Normal | 19 (502) | 0x00 | GoalMission | - | default@8016CB78 |
| 14 | 402 | canyon2 | Hero | 20 (503) | 0x12 | CountMission | required=5, fail=0 | nukkoro2.inf:131 [canyon2] MISSIONCOUNT_H |
| 14 | 402 | canyon2 | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 15 | 403 | eWorld | Dark | 31 (411) | 0x13 | CountMission | required=30, fail=0 | nukkoro2.inf:142 [eWorld] MISSIONCOUNT_D |
| 15 | 403 | eWorld | Normal | 31 (411) | 0x00 | GoalMission | - | default@8016CB78 |
| 15 | 403 | eWorld | Hero | 31 (411) | 0x14 | CountMission | required=4, fail=0 | nukkoro2.inf:143 [eWorld] MISSIONCOUNT_H |
| 15 | 403 | eWorld | Hard/X |  | 0x27 | CountMission | required=4, fail=0 | nukkoro2.inf:144 [eWorld] MISSIONCOUNT_HARD |
| 16 | 404 | Ruins | Dark | 32 (412) | 0x00 | GoalMission | - | default@8016CB78 |
| 16 | 404 | Ruins | Hero | 32 (412) | 0x15 | EnemyMission | team=Black Arms (SET 0x8C-0x95), required=50 (fail count forced 0) | nukkoro2.inf:154 [Ruins] MISSIONCOUNT_H |
| 16 | 404 | Ruins | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 17 | 500 | ARKRuins1 | Dark | 33 (510) | 0x16 | CountMission | required=4, fail=0 | nukkoro2.inf:164 [ARKRuins1] MISSIONCOUNT_D |
| 17 | 500 | ARKRuins1 | Normal | 33 (510) | 0x00 | GoalMission | - | default@8016CB78 |
| 17 | 500 | ARKRuins1 | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 18 | 501 | Sky | Dark | 22 (600) | 0x17 | CountMission | required=1, fail=1 | nukkoro2.inf:176 [Sky] MISSIONCOUNT_D |
| 18 | 501 | Sky | Normal | 23 (601) | 0x00 | GoalMission | - | default@8016CB78 |
| 18 | 501 | Sky | Hero | 24 (602) | 0x18 | EnemyMission | team=Black Arms (SET 0x8C-0x95), required=35 (fail count forced 0) | nukkoro2.inf:177 [Sky] MISSIONCOUNT_H |
| 18 | 501 | Sky | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 19 | 502 | Jungle | Dark | 34 (511) | 0x19 | EnemyMission | team=GUN (SET 0x64-0x77), required=28 (fail count forced 0) | nukkoro2.inf:188 [Jungle] MISSIONCOUNT_D |
| 19 | 502 | Jungle | Normal | 34 (511) | 0x00 | GoalMission | - | default@8016CB78 |
| 19 | 502 | Jungle | Hero | 34 (511) | 0x1A | CountMission | required=1, fail=1 | nukkoro2.inf:189 [Jungle] MISSIONCOUNT_H |
| 19 | 502 | Jungle | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 20 | 503 | Space | Dark | 24 (602) | 0x1B | CountMission | required=6, fail=0 | nukkoro2.inf:201 [Space] MISSIONCOUNT_D |
| 20 | 503 | Space | Normal | 25 (603) | 0x00 | GoalMission | - | default@8016CB78 |
| 20 | 503 | Space | Hero | 26 (604) | 0x1C | TimerGoalMission | time=300s | nukkoro2.inf:202 [Space] MISSIONCOUNT_H |
| 20 | 503 | Space | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 21 | 504 | ARKPast2 | Normal | 25 (603) | 0x00 | GoalMission | - | default@8016CB78 |
| 21 | 504 | ARKPast2 | Hero | 26 (604) | 0x1D | EnemyMission | team=Black Arms (SET 0x8C-0x95), required=35 (fail count forced 0) | nukkoro2.inf:212 [ARKPast2] MISSIONCOUNT_H |
| 21 | 504 | ARKPast2 | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 22 | 600 | GunsBase | Dark | 36 (611) | 0x1E | CountMission | required=3, fail=0 | nukkoro2.inf:222 [GunsBase] MISSIONCOUNT_D |
| 22 | 600 | GunsBase | Hero | 35 (610) | 0x00 | GoalMission | - | default@8016CB78 |
| 22 | 600 | GunsBase | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 23 | 601 | DoomsBase1 | Dark | 38 (613) | 0x1F | EnemyMission | team=GUN (SET 0x64-0x77), required=50 (fail count forced 0) | nukkoro2.inf:232 [DoomsBase1] MISSIONCOUNT_D |
| 23 | 601 | DoomsBase1 | Hero | 37 (612) | 0x00 | GoalMission | - | default@8016CB78 |
| 23 | 601 | DoomsBase1 | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 24 | 602 | EggmansBase | Dark | 39 (614) | 0x20 | CountMission | required=5, fail=0 | nukkoro2.inf:241 [EggmansBase] MISSIONCOUNT_D |
| 24 | 602 | EggmansBase | Hero | 39 (614) | 0x00 | GoalMission | - | default@8016CB78 |
| 24 | 602 | EggmansBase | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 25 | 603 | ARKRuins2 | Dark | 40 (615) | 0x21 | TimerGoalMission | time=900s | nukkoro2.inf:252 [ARKRuins2] MISSIONCOUNT_D |
| 25 | 603 | ARKRuins2 | Hero | 41 (616) | 0x22 | TimerMission | time=900s | nukkoro2.inf:253 [ARKRuins2] MISSIONCOUNT_H |
| 25 | 603 | ARKRuins2 | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 26 | 604 | DoomsBase2 | Dark | 43 (618) | 0x23 | CountMission | required=4, fail=0 | nukkoro2.inf:263 [DoomsBase2] MISSIONCOUNT_D |
| 26 | 604 | DoomsBase2 | Hero | 42 (617) | 0x00 | GoalMission | - | default@8016CB78 |
| 26 | 604 | DoomsBase2 | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 27 | 700 | DoomsCore | Hero | 44 (710) | 0x24 | TimerGoalMission | time=600s | nukkoro2.inf:271 [DoomsCore] MISSIONCOUNT_H |
| 27 | 700 | DoomsCore | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 45 | 800 | GUN2P_0800 | Normal | -2 | 0x28 | TimerMission | time=600s | nukkoro2.inf:404 [GUN2P_0802] MISSIONCOUNT_N |
| 46 | 801 | GUN2P_0801 | Normal | -2 | 0x28 | TimerMission | time=600s | nukkoro2.inf:404 [GUN2P_0802] MISSIONCOUNT_N |
| 47 | 802 | GUN2P_0802 | Normal | -2 | 0x28 | TimerMission | time=600s | nukkoro2.inf:404 [GUN2P_0802] MISSIONCOUNT_N |


### 6.1 Keys shared by several stages/slots


- 0x00 (GoalMission): Practice/Normal, Practice/Hard, City1/Normal, City1/Hard, Circuit/Hero, Circuit/Hard, Canyon1/Normal, Canyon1/Hard, Highway/Dark, Highway/Hard, HorrorCastle/Normal, HorrorCastle/Hard, PrisonIsland/Normal, PrisonIsland/Hard, Circus/Normal, Circus/Hard, ARKPast1/Normal, canyon2/Normal, canyon2/Hard, eWorld/Normal, Ruins/Dark, Ruins/Hard, ARKRuins1/Normal, ARKRuins1/Hard, Sky/Normal, Sky/Hard, Jungle/Normal, Jungle/Hard, Space/Normal, Space/Hard, ARKPast2/Normal, ARKPast2/Hard, GunsBase/Hero, GunsBase/Hard, DoomsBase1/Hero, DoomsBase1/Hard, EggmansBase/Hero, EggmansBase/Hard, ARKRuins2/Hard, DoomsBase2/Hero, DoomsBase2/Hard, DoomsCore/Hard
- 0x01 (EnemyMission): Practice/Hero, City1/Dark
- 0x02 (EnemyMission): Practice/Dark, City1/Hero
- 0x28 (TimerMission): GUN2P_0800/Normal, GUN2P_0801/Normal, GUN2P_0802/Normal


---------------------------------------------------------------------------------------------------------------------

## 7. All-stage parameter table

Hard-mode rows whose key is 0 (plain GoalMission) are omitted. Value sources: `default@ADDR` = fn_8016C8EC call site in
fn_8016CB48; `nukkoro2.inf:LINE [section] CMD` = override written by fn_801DF470. Consumer addresses: key → fn_80176FC8 /
fn_8016E4EC; class → fn_8016B768 jump table 0x8052C930; team → EnemyMission+0x58 read by EnemyMission::vf0B 0x801F3CBC;
counts → CountMission+0x8 compared in CountMission::vf05 (0x8016B070) / vf06 (0x8016AFBC); next stage → fn_80205010 / fn_802D1EC8.


| Stage | Mission | Parameter | Value/source | Consumer address | Confidence |
|---|---|---|---|---|---|
| 0 Practice | Dark | mission key | 0x02 (stage table 0x804C5AE8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 0 Practice | Dark | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 0 Practice | Dark | enemy team (desc+4) | 2 = Black Arms (SET 0x8C-0x95) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 0 Practice | Dark | required count (desc+8) | 45 (nukkoro2.inf:22 [City1] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 0 Practice | Normal | mission key | 0x00 (stage table 0x804C5AE8 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 0 Practice | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 0 Practice | Hero | mission key | 0x01 (stage table 0x804C5AE8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 0 Practice | Hero | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 0 Practice | Hero | enemy team (desc+4) | 0 = GUN (SET 0x64-0x77) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 0 Practice | Hero | required count (desc+8) | 35 (nukkoro2.inf:21 [City1] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 100 City1 | Dark | mission key | 0x01 (stage table 0x804C5C78 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 100 City1 | Dark | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 100 City1 | Dark | enemy team (desc+4) | 0 = GUN (SET 0x64-0x77) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 100 City1 | Dark | required count (desc+8) | 35 (nukkoro2.inf:21 [City1] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 100 City1 | Dark | next stage on clear | idx 6 = stage 200 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 100 City1 | Normal | mission key | 0x00 (stage table 0x804C5C78 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 100 City1 | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 100 City1 | Normal | next stage on clear | idx 7 = stage 201 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 100 City1 | Hero | mission key | 0x02 (stage table 0x804C5C78 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 100 City1 | Hero | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 100 City1 | Hero | enemy team (desc+4) | 2 = Black Arms (SET 0x8C-0x95) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 100 City1 | Hero | required count (desc+8) | 45 (nukkoro2.inf:22 [City1] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 100 City1 | Hero | next stage on clear | idx 8 = stage 202 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 200 Circuit | Dark | mission key | 0x03 (stage table 0x804C5CC8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 200 Circuit | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 200 Circuit | Dark | required count (desc+8) | 1 (default@8016CBB4) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 200 Circuit | Dark | next stage on clear | idx 9 = stage 300 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 200 Circuit | Hero | mission key | 0x00 (stage table 0x804C5CC8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 200 Circuit | Hero | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 200 Circuit | Hero | next stage on clear | idx 10 = stage 301 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 201 Canyon1 | Dark | mission key | 0x04 (stage table 0x804C5D18 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 201 Canyon1 | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 201 Canyon1 | Dark | required count (desc+8) | 5 (nukkoro2.inf:42 [Canyon1] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 201 Canyon1 | Dark | next stage on clear | idx 9 = stage 300 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 201 Canyon1 | Normal | mission key | 0x00 (stage table 0x804C5D18 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 201 Canyon1 | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 201 Canyon1 | Normal | next stage on clear | idx 10 = stage 301 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 201 Canyon1 | Hero | mission key | 0x05 (stage table 0x804C5D18 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 201 Canyon1 | Hero | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 201 Canyon1 | Hero | enemy team (desc+4) | 2 = Black Arms (SET 0x8C-0x95) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 201 Canyon1 | Hero | required count (desc+8) | 60 (nukkoro2.inf:43 [Canyon1] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 201 Canyon1 | Hero | next stage on clear | idx 11 = stage 302 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 202 Highway | Dark | mission key | 0x00 (stage table 0x804C5D68 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 202 Highway | Dark | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 202 Highway | Dark | next stage on clear | idx 28 = stage 210 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 202 Highway | Hero | mission key | 0x06 (stage table 0x804C5D68 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 202 Highway | Hero | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 202 Highway | Hero | required count (desc+8) | 1 (default@8016CBF0) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 202 Highway | Hero | next stage on clear | idx 28 = stage 210 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 300 HorrorCastle | Dark | mission key | 0x07 (stage table 0x804C5DB8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 300 HorrorCastle | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 300 HorrorCastle | Dark | required count (desc+8) | 5 (nukkoro2.inf:64 [HorrorCastle] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 300 HorrorCastle | Dark | next stage on clear | idx 29 = stage 310 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 300 HorrorCastle | Normal | mission key | 0x00 (stage table 0x804C5DB8 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 300 HorrorCastle | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 300 HorrorCastle | Normal | next stage on clear | idx 29 = stage 310 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 300 HorrorCastle | Hero | mission key | 0x08 (stage table 0x804C5DB8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 300 HorrorCastle | Hero | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 300 HorrorCastle | Hero | required count (desc+8) | 2 (nukkoro2.inf:65 [HorrorCastle] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 300 HorrorCastle | Hero | next stage on clear | idx 29 = stage 310 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 301 PrisonIsland | Dark | mission key | 0x09 (stage table 0x804C5E08 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 301 PrisonIsland | Dark | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 301 PrisonIsland | Dark | enemy team (desc+4) | 0 = GUN (SET 0x64-0x77) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 301 PrisonIsland | Dark | required count (desc+8) | 40 (nukkoro2.inf:76 [PrisonIsland] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 301 PrisonIsland | Dark | next stage on clear | idx 13 = stage 401 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 301 PrisonIsland | Normal | mission key | 0x00 (stage table 0x804C5E08 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 301 PrisonIsland | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 301 PrisonIsland | Normal | next stage on clear | idx 14 = stage 402 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 301 PrisonIsland | Hero | mission key | 0x0A (stage table 0x804C5E08 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 301 PrisonIsland | Hero | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 301 PrisonIsland | Hero | required count (desc+8) | 5 (nukkoro2.inf:77 [PrisonIsland] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 301 PrisonIsland | Hero | next stage on clear | idx 15 = stage 403 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 302 Circus | Dark | mission key | 0x0B (stage table 0x804C5E58 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 302 Circus | Dark | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 302 Circus | Dark | enemy team (desc+4) | 0 = GUN (SET 0x64-0x77) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 302 Circus | Dark | required count (desc+8) | 20 (nukkoro2.inf:88 [Circus] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 302 Circus | Dark | next stage on clear | idx 14 = stage 402 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 302 Circus | Normal | mission key | 0x00 (stage table 0x804C5E58 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 302 Circus | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 302 Circus | Normal | next stage on clear | idx 15 = stage 403 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 302 Circus | Hero | mission key | 0x0C (stage table 0x804C5E58 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 302 Circus | Hero | class (desc.type) | 7 = RingMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 302 Circus | Hero | required count (desc+8) | 400 (nukkoro2.inf:89 [Circus] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 302 Circus | Hero | next stage on clear | idx 16 = stage 404 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 400 City2 | Dark | mission key | 0x0D (stage table 0x804C5EA8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 400 City2 | Dark | class (desc.type) | 6 = TimerCountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 400 City2 | Dark | required count (desc+8) | 5 (nukkoro2.inf:101 [City2] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 400 City2 | Dark | time limit s (desc+0xC) | 480 (nukkoro2.inf:101 [City2] MISSIONCOUNT_D) | TimerCountMission ctor f1 (0x8016B9DC) | PROVEN |
| 400 City2 | Dark | next stage on clear | idx 17 = stage 500 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 400 City2 | Hero | mission key | 0x0E (stage table 0x804C5EA8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 400 City2 | Hero | class (desc.type) | 6 = TimerCountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 400 City2 | Hero | required count (desc+8) | 20 (nukkoro2.inf:102 [City2] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 400 City2 | Hero | time limit s (desc+0xC) | 480 (nukkoro2.inf:102 [City2] MISSIONCOUNT_H) | TimerCountMission ctor f1 (0x8016B9DC) | PROVEN |
| 400 City2 | Hero | next stage on clear | idx 18 = stage 501 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 400 City2 | Hard | mission key | 0x25 (stage table 0x804C5EA8 +0x40) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 400 City2 | Hard | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 400 City2 | Hard | required count (desc+8) | 5 (nukkoro2.inf:103 [City2] MISSIONCOUNT_HARD) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 401 ARKPast1 | Dark | mission key | 0x0F (stage table 0x804C5EF8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 401 ARKPast1 | Dark | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 401 ARKPast1 | Dark | enemy team (desc+4) | 0 = GUN (SET 0x64-0x77) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 401 ARKPast1 | Dark | required count (desc+8) | 60 (nukkoro2.inf:109 [ARKPast1] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 401 ARKPast1 | Dark | next stage on clear | idx 30 = stage 410 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 401 ARKPast1 | Normal | mission key | 0x00 (stage table 0x804C5EF8 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 401 ARKPast1 | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 401 ARKPast1 | Normal | next stage on clear | idx 30 = stage 410 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 401 ARKPast1 | Hero | mission key | 0x10 (stage table 0x804C5EF8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 401 ARKPast1 | Hero | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 401 ARKPast1 | Hero | required count (desc+8) | 10 (nukkoro2.inf:108 [ARKPast1] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 401 ARKPast1 | Hero | next stage on clear | idx 30 = stage 410 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 401 ARKPast1 | Hard | mission key | 0x26 (stage table 0x804C5EF8 +0x40) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 401 ARKPast1 | Hard | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 401 ARKPast1 | Hard | required count (desc+8) | 10 (nukkoro2.inf:117 [ARKPast1] MISSIONCOUNT_HARD) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 402 canyon2 | Dark | mission key | 0x11 (stage table 0x804C5F48 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 402 canyon2 | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 402 canyon2 | Dark | required count (desc+8) | 5 (nukkoro2.inf:130 [canyon2] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 402 canyon2 | Dark | next stage on clear | idx 18 = stage 501 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 402 canyon2 | Normal | mission key | 0x00 (stage table 0x804C5F48 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 402 canyon2 | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 402 canyon2 | Normal | next stage on clear | idx 19 = stage 502 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 402 canyon2 | Hero | mission key | 0x12 (stage table 0x804C5F48 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 402 canyon2 | Hero | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 402 canyon2 | Hero | required count (desc+8) | 5 (nukkoro2.inf:131 [canyon2] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 402 canyon2 | Hero | next stage on clear | idx 20 = stage 503 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 403 eWorld | Dark | mission key | 0x13 (stage table 0x804C5F98 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 403 eWorld | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 403 eWorld | Dark | required count (desc+8) | 30 (nukkoro2.inf:142 [eWorld] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 403 eWorld | Dark | next stage on clear | idx 31 = stage 411 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 403 eWorld | Normal | mission key | 0x00 (stage table 0x804C5F98 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 403 eWorld | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 403 eWorld | Normal | next stage on clear | idx 31 = stage 411 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 403 eWorld | Hero | mission key | 0x14 (stage table 0x804C5F98 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 403 eWorld | Hero | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 403 eWorld | Hero | required count (desc+8) | 4 (nukkoro2.inf:143 [eWorld] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 403 eWorld | Hero | next stage on clear | idx 31 = stage 411 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 403 eWorld | Hard | mission key | 0x27 (stage table 0x804C5F98 +0x40) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 403 eWorld | Hard | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 403 eWorld | Hard | required count (desc+8) | 4 (nukkoro2.inf:144 [eWorld] MISSIONCOUNT_HARD) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 404 Ruins | Dark | mission key | 0x00 (stage table 0x804C5FE8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 404 Ruins | Dark | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 404 Ruins | Dark | next stage on clear | idx 32 = stage 412 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 404 Ruins | Hero | mission key | 0x15 (stage table 0x804C5FE8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 404 Ruins | Hero | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 404 Ruins | Hero | enemy team (desc+4) | 2 = Black Arms (SET 0x8C-0x95) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 404 Ruins | Hero | required count (desc+8) | 50 (nukkoro2.inf:154 [Ruins] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 404 Ruins | Hero | next stage on clear | idx 32 = stage 412 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 500 ARKRuins1 | Dark | mission key | 0x16 (stage table 0x804C6038 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 500 ARKRuins1 | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 500 ARKRuins1 | Dark | required count (desc+8) | 4 (nukkoro2.inf:164 [ARKRuins1] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 500 ARKRuins1 | Dark | next stage on clear | idx 33 = stage 510 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 500 ARKRuins1 | Normal | mission key | 0x00 (stage table 0x804C6038 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 500 ARKRuins1 | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 500 ARKRuins1 | Normal | next stage on clear | idx 33 = stage 510 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 501 Sky | Dark | mission key | 0x17 (stage table 0x804C6088 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 501 Sky | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 501 Sky | Dark | required count (desc+8) | 1 (nukkoro2.inf:176 [Sky] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 501 Sky | Dark | fail count (desc+0xC) | 1 (nukkoro2.inf:176 [Sky] MISSIONCOUNT_D) | CountMission+0xC, CountMission::vf05 0x8016B0A4 -> Fail 0x8016B654 | PROVEN |
| 501 Sky | Dark | next stage on clear | idx 22 = stage 600 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 501 Sky | Normal | mission key | 0x00 (stage table 0x804C6088 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 501 Sky | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 501 Sky | Normal | next stage on clear | idx 23 = stage 601 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 501 Sky | Hero | mission key | 0x18 (stage table 0x804C6088 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 501 Sky | Hero | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 501 Sky | Hero | enemy team (desc+4) | 2 = Black Arms (SET 0x8C-0x95) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 501 Sky | Hero | required count (desc+8) | 35 (nukkoro2.inf:177 [Sky] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 501 Sky | Hero | next stage on clear | idx 24 = stage 602 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 502 Jungle | Dark | mission key | 0x19 (stage table 0x804C60D8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 502 Jungle | Dark | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 502 Jungle | Dark | enemy team (desc+4) | 0 = GUN (SET 0x64-0x77) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 502 Jungle | Dark | required count (desc+8) | 28 (nukkoro2.inf:188 [Jungle] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 502 Jungle | Dark | next stage on clear | idx 34 = stage 511 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 502 Jungle | Normal | mission key | 0x00 (stage table 0x804C60D8 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 502 Jungle | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 502 Jungle | Normal | next stage on clear | idx 34 = stage 511 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 502 Jungle | Hero | mission key | 0x1A (stage table 0x804C60D8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 502 Jungle | Hero | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 502 Jungle | Hero | required count (desc+8) | 1 (nukkoro2.inf:189 [Jungle] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 502 Jungle | Hero | fail count (desc+0xC) | 1 (nukkoro2.inf:189 [Jungle] MISSIONCOUNT_H) | CountMission+0xC, CountMission::vf05 0x8016B0A4 -> Fail 0x8016B654 | PROVEN |
| 502 Jungle | Hero | next stage on clear | idx 34 = stage 511 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 503 Space | Dark | mission key | 0x1B (stage table 0x804C6128 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 503 Space | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 503 Space | Dark | required count (desc+8) | 6 (nukkoro2.inf:201 [Space] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 503 Space | Dark | next stage on clear | idx 24 = stage 602 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 503 Space | Normal | mission key | 0x00 (stage table 0x804C6128 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 503 Space | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 503 Space | Normal | next stage on clear | idx 25 = stage 603 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 503 Space | Hero | mission key | 0x1C (stage table 0x804C6128 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 503 Space | Hero | class (desc.type) | 5 = TimerGoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 503 Space | Hero | time limit s (desc+8) | 300 (nukkoro2.inf:202 [Space] MISSIONCOUNT_H) | TimerMission+0x8 (float), fn_8016E4EC min-time -> stage timer .bss 0x8057E7E8 | PROVEN |
| 503 Space | Hero | next stage on clear | idx 26 = stage 604 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 504 ARKPast2 | Normal | mission key | 0x00 (stage table 0x804C6178 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 504 ARKPast2 | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 504 ARKPast2 | Normal | next stage on clear | idx 25 = stage 603 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 504 ARKPast2 | Hero | mission key | 0x1D (stage table 0x804C6178 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 504 ARKPast2 | Hero | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 504 ARKPast2 | Hero | enemy team (desc+4) | 2 = Black Arms (SET 0x8C-0x95) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 504 ARKPast2 | Hero | required count (desc+8) | 35 (nukkoro2.inf:212 [ARKPast2] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 504 ARKPast2 | Hero | next stage on clear | idx 26 = stage 604 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 600 GunsBase | Dark | mission key | 0x1E (stage table 0x804C61C8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 600 GunsBase | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 600 GunsBase | Dark | required count (desc+8) | 3 (nukkoro2.inf:222 [GunsBase] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 600 GunsBase | Dark | next stage on clear | idx 36 = stage 611 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 600 GunsBase | Hero | mission key | 0x00 (stage table 0x804C61C8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 600 GunsBase | Hero | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 600 GunsBase | Hero | next stage on clear | idx 35 = stage 610 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 601 DoomsBase1 | Dark | mission key | 0x1F (stage table 0x804C6218 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 601 DoomsBase1 | Dark | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 601 DoomsBase1 | Dark | enemy team (desc+4) | 0 = GUN (SET 0x64-0x77) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 601 DoomsBase1 | Dark | required count (desc+8) | 50 (nukkoro2.inf:232 [DoomsBase1] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 601 DoomsBase1 | Dark | next stage on clear | idx 38 = stage 613 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 601 DoomsBase1 | Hero | mission key | 0x00 (stage table 0x804C6218 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 601 DoomsBase1 | Hero | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 601 DoomsBase1 | Hero | next stage on clear | idx 37 = stage 612 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 602 EggmansBase | Dark | mission key | 0x20 (stage table 0x804C6268 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 602 EggmansBase | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 602 EggmansBase | Dark | required count (desc+8) | 5 (nukkoro2.inf:241 [EggmansBase] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 602 EggmansBase | Dark | next stage on clear | idx 39 = stage 614 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 602 EggmansBase | Hero | mission key | 0x00 (stage table 0x804C6268 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 602 EggmansBase | Hero | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 602 EggmansBase | Hero | next stage on clear | idx 39 = stage 614 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 603 ARKRuins2 | Dark | mission key | 0x21 (stage table 0x804C62B8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 603 ARKRuins2 | Dark | class (desc.type) | 5 = TimerGoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 603 ARKRuins2 | Dark | time limit s (desc+8) | 900 (nukkoro2.inf:252 [ARKRuins2] MISSIONCOUNT_D) | TimerMission+0x8 (float), fn_8016E4EC min-time -> stage timer .bss 0x8057E7E8 | PROVEN |
| 603 ARKRuins2 | Dark | next stage on clear | idx 40 = stage 615 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 603 ARKRuins2 | Hero | mission key | 0x22 (stage table 0x804C62B8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 603 ARKRuins2 | Hero | class (desc.type) | 4 = TimerMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 603 ARKRuins2 | Hero | time limit s (desc+8) | 900 (nukkoro2.inf:253 [ARKRuins2] MISSIONCOUNT_H) | TimerMission+0x8 (float), fn_8016E4EC min-time -> stage timer .bss 0x8057E7E8 | PROVEN |
| 603 ARKRuins2 | Hero | next stage on clear | idx 41 = stage 616 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 604 DoomsBase2 | Dark | mission key | 0x23 (stage table 0x804C6308 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 604 DoomsBase2 | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 604 DoomsBase2 | Dark | required count (desc+8) | 4 (nukkoro2.inf:263 [DoomsBase2] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 604 DoomsBase2 | Dark | next stage on clear | idx 43 = stage 618 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 604 DoomsBase2 | Hero | mission key | 0x00 (stage table 0x804C6308 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 604 DoomsBase2 | Hero | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 604 DoomsBase2 | Hero | next stage on clear | idx 42 = stage 617 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 700 DoomsCore | Hero | mission key | 0x24 (stage table 0x804C6358 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 700 DoomsCore | Hero | class (desc.type) | 5 = TimerGoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 700 DoomsCore | Hero | time limit s (desc+8) | 600 (nukkoro2.inf:271 [DoomsCore] MISSIONCOUNT_H) | TimerMission+0x8 (float), fn_8016E4EC min-time -> stage timer .bss 0x8057E7E8 | PROVEN |
| 700 DoomsCore | Hero | next stage on clear | idx 44 = stage 710 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 800 GUN2P_0800 | Normal | mission key | 0x28 (stage table 0x804C68F8 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 800 GUN2P_0800 | Normal | class (desc.type) | 4 = TimerMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 800 GUN2P_0800 | Normal | time limit s (desc+8) | 600 (nukkoro2.inf:404 [GUN2P_0802] MISSIONCOUNT_N) | TimerMission+0x8 (float), fn_8016E4EC min-time -> stage timer .bss 0x8057E7E8 | PROVEN |
| 801 GUN2P_0801 | Normal | mission key | 0x28 (stage table 0x804C6948 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 801 GUN2P_0801 | Normal | class (desc.type) | 4 = TimerMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 801 GUN2P_0801 | Normal | time limit s (desc+8) | 600 (nukkoro2.inf:404 [GUN2P_0802] MISSIONCOUNT_N) | TimerMission+0x8 (float), fn_8016E4EC min-time -> stage timer .bss 0x8057E7E8 | PROVEN |
| 802 GUN2P_0802 | Normal | mission key | 0x28 (stage table 0x804C6998 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 802 GUN2P_0802 | Normal | class (desc.type) | 4 = TimerMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 802 GUN2P_0802 | Normal | time limit s (desc+8) | 600 (nukkoro2.inf:404 [GUN2P_0802] MISSIONCOUNT_N) | TimerMission+0x8 (float), fn_8016E4EC min-time -> stage timer .bss 0x8057E7E8 | PROVEN |
