# Enemy alert / activation paths (open question from GUN_SOLDIER.md §11.x)

Question: 54 of 187 placed GUN soldiers have `SearchWidth = 0`, so `EnemySensor_IsInSearchArea` 0x801A6C7C
(`distXZ² < SearchWidth²`) can never succeed. How else can an enemy be alerted or activated?
Answer (STRONG): **nothing else in `main.dol` alerts a SoldierCommon/EnemyWalker/Flyer enemy.** Zero-width soldiers are
passive by design. Evidence below; confidence per row. Helpers: `tools/agent_enemy2_cmdsend.py`,
`tools/agent_enemy_vscan.py`, `tools/agent_enemy2_census.py`.

## 1. Who can request "target found" (AI slot 0x0F)?

Whole-DOL scan of virtual calls through an AI vptr (`agent_enemy_vscan.py 44 --vp 18`) — every call site of AI slot 0x0F:

| Site | Function | Trigger | Conf |
|---|---|---|---|
| 0x801895D4 | `EnemyAIState_SoldierCommonAI_WaitAction_Standing::Update` | sensor | PROVEN |
| 0x80188BF0 | `EnemyAIState_SoldierCommonAI_WaitAction_Moving::Update` | sensor | PROVEN |
| 0x8019A330 / 0x80199EC0 | `EnemyAIState_EnemyWalkerAI_WaitAction_{Standing,Moving}::Update` | `Enemy_DetectNearestPlayer` 0x8019B2C0 = sensor | PROVEN |
| 0x80183FCC | `EnemyAIState_FlyerCommonAI_WaitFloating::Update` | sensor and ActionType == 2 | PROVEN |
| 0x8019C09C | `EnemyAIState_EnemyBaseAI_Wait::Update` (GunBigfoot, BkGiant, …) | sensor (not traced further) | STRONG |
| 0x8028FB4C / 0x802B3A14 | EggPierrot / BkChaos `…_Wait::Update` | family code (not in scope) | — |
| 0x8019E11C | `EnemyBaseAI::HandleMessage` case **0x205** | CharCommand type 0x205 (see §2) | PROVEN |
| 0x800A0CD4 / 0x801FD744 | player input / NPC code — not enemy AI objects | — | PROVEN (not relevant) |

The sensor itself is written only at spawn: the only writers of the squared width (sensor+0x3C) are
`EnemySensor_SetupFromSetParams` 0x801A6DB8 (from `TEnemySetBase_InitFromSetSlot`) and the BkLarva generator
`fn_801899E0` (`q.py off E4 W`, `q.py off 3C W`). The detection target is always the nearest **player**
(`EnemyBaseAI::GetTargetPosition` 0x8019DF7C, the AI target tracker `fn_8019FB08` uses it) — enemies never target other
factions. PROVEN.

## 2. AI message commands 0x201–0x20C

`EnemyBaseAI::HandleMessage` 0x8019E058, jump table 0x8053B76C: type 0x201+*k* → AI slot 0x0B+*k* (0x201 Idle, 0x202
initial state, 0x203 ReturnToWait, 0x204/0x206/0x207 slots 0x0E/0x10/0x11 (base: Wait; EggPawn slot 0x10 =
StartAttack), **0x205 OnTargetFound**, 0x208 Guard, 0x209 WaitFinishDamaged, 0x20A OnDead, 0x20B nop, 0x20C
OnRescued/Recover), then sets the handled flag. PROVEN.

**Senders: none.** `agent_enemy2_cmdsend.py`: all 36 call sites of `CharCommand::CharCommand` 0x8006E57C pass a
constant type outside 0x201–0x20C, except the pass-through subclass ctors `Player::PlayerCommand`, `Player::Npc::NpcCommand`
and `GadgetCommand`, whose own callers only pass 0x101–0x137 / 0x401–0x403 / 0x301–0x304; no data pointer references the
ctor; the `CharCommand` vptr is written only by its ctor/dtor; no immediate 0x201–0x20C appears next to command code; the
disc has no REL modules (`sys/main.dol` only). → the 0x2xx interface is unreachable in the shipped game (STRONG; LIKELY a
debug/editor hook).

## 3. Other messages that change enemy AI state (none of them alerts)

| Type | Command | Sender(s) | Enemy effect (`HandleMessage`) | Conf |
|---|---|---|---|---|
| 0x10 | RecoverCommand | `HealingBlast_SendRecover` 0x80108294 | AddHP, first-heal score, AI slot 0x16 (GunSoldier: Thanks; others: `EnemyBaseAI_Recover` = passive 60 s) | PROVEN |
| 0x14 | VacuumCommand | `fn_80341638` (PMF-bound callback, owner class UNKNOWN) | counts as a kill; type 0 → respawnable detach, others → DetachKilled; delete | PROVEN |
| 0x16 | AnnounceAttackCommand | `fn_80060F80`, `fn_800626D4`, `fn_800627C4`, `fn_800852B4` (player code), `fn_8023C688` | **handled flag only** (case 0x8019E23C) | PROVEN |
| 0x19 | GenerateCommand | `SET GENERATOR` (SET id 0x2593, params ComId/Id/num/delay/scoreId) update `fn_8027CBD0` → `fn_800C9B24(slot,cmd)` | `EnemyBaseAI_WarpWait` (alpha 0, status slot 0x1D(0,0) = collision off (LIKELY), 0.5 s) → AI slot 0x0C = initial wait state | PROVEN |
| 0x1A | BindCommand | CaptureCage code `fn_800F3458`, `fn_800F38F0` | only within 10 frames of activation (frame counter 0x8057E7F0 − E+0xF8 < 10): S bit 0xE, `CaptureWait` | PROVEN |
| 0x1B | ReleaseCommand | `fn_800F3458`, `CaptureCage::vf02` 0x800F3FA8 | S bit 0xE, if not dead: unlock + **AI slot 0x0D (ReturnToWait)** — note: GUN_SOLDIER.md §6 lists "Release → Caution"; the call at 0x8019E474 is `[[AI+0x18]+0x3C]` = slot 0x0D | PROVEN |
| 0x07 | DamageCommand | — | not handled by enemies (damage arrives via CharaColli) | PROVEN |

The `SET GENERATOR` also **re-arms** its target record (sets record flag 0x40 when the slot is not alive, 0x8027CCE8),
which is the only enemy-respawn path found for a record that was killed (flag 0x8, enable bit cleared). Its slot
predicate `fn_8027CFB8` tests record flag 0x40000. STRONG (mechanics) / LIKELY (design intent).

## 4. Link-group events

`EnemyManager_OnEnemyLifeEvent` 0x801A250C decrements the alive count of the dead enemy's link group (SET record+0x22)
and at 0 calls `LinkEvent_Set(LinkEventTable_Get(), link, 1)` 0x80169460 (table of 256 × {value, flags}).
Readers (`LinkEvent_Get` 0x80169430, 59 call sites in 50 functions) are gimmicks: Door/SetBaseDoor, Elevator, Cage,
Weight, CircusBall, HintRing, RegularRing, SetCollision, CommandCollision (self-removes via `SetSlot_DisableAndDetach`
when its link fires), ElecCircuit, ElecCristalWall(Switch), ElecBarrier, ElecFan, ElecPanel, DefenseShield,
ElecAccessPanel, ElecSearchLight, Bomb, BombingWall, BreakRoad/FallRoad, Footing*, Magma, BAGunShip, SetSeOneShot,
ArtWork::BreakableOrnament, HintCollision, EscapePlaneRail, **SetGenerator** … — **no enemy class**. So a cleared link
group can start a `SET GENERATOR` (which then warps enemies in via GenerateCommand, §3), but never alerts an existing
enemy. `SetData_RebuildLinkList` only relinks SET slots when the scanner re-arms a record. STRONG.

## 5. Damage does not alert a waiting SoldierCommon/Walker enemy

Flinch → `EnemyBaseAI_WaitFinishDamaged` → motion end → AI slot 0x0D (ReturnToWait); shield block → Guard → 0x0D.
Both return to the wait state, which re-tests only the sensor. (Only a rescued GunSoldier in `Thanks` turns hostile on
damage.) PROVEN.

## 6. Census and conclusion

| Family | Records | SearchWidth = 0 | Where / notes |
|---|---:|---:|---|
| GUN_SOLDIER 0x64 | 187 | 54 | stg0100 19, stg0504 31, stg0600 3, stg0404 1; all `cmn`, link id 0; 45 of them also SearchRange = SearchHeight = 0; AppearType 0‑3 only |
| BK_SOLDIER 0x8D | 402 | 19 | stg0404/0501/0503/0600; AppearType 0/1 |
| EGG_PAWN 0x79 | 498 | 3 | stg0602 hrd; AppearType 0 |
| GUN_BEETLE 0x65 | 313 | 13 | stg0100 2, stg0504 11; 12 floaters (ActionType 1 ones still run the spark cycle, which ignores the sensor); 1 path beetle (param 9 = 8) that is never revealed |

Conclusion (STRONG): a zero-width enemy stands or patrols forever, can be damaged and killed (kills count normally),
but no code path in `main.dol` switches it to Caution/attack. The only unexplored channel is the dead 0x2xx message
interface.
