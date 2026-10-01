# OPEN_QUESTIONS — prioritized list of unresolved gameplay facts

Each item names where the evidence stops. Detailed context lives in the linked document.
Priority: P1 blocks faithful native behaviour · P2 affects tuning/fidelity · P3 cosmetic/secondary.

## P1
| # | Question | Where it stops | Doc |
|---|---|---|---|
| 1 | ~~How are SearchWidth = 0 enemies alerted?~~ **Answered (STRONG):** they are not — the sensor is the only alert path; no code builds AI commands 0x201–0x20C, AnnounceAttack is only acknowledged, hits return the enemy to its wait state. Remaining: confirm in-game (runtime trace) | enemy/ENEMY_ALERT_PATHS.md |
| 2 | ~~Damage values~~ **Answered (PROVEN):** hit rule fn_800C6A4C (level > defense), damage = `CharaColliAttack+0x34` → `AttackCallbackParam+0x50`; homing attack 2, Chaos Blast 12, guns 2–6… Remaining: SatelliteLaser damage, unnamed laser fire intervals, WEAPON_CONTAINER index path (LIKELY) | WEAPONS_AND_TARGETING.md |
| 3 | ~~GunSoldier patrol logic~~ **Answered (PROVEN):** `WaitAction_Moving::Update` 0x80188B98 and point table E+0x288 are in native `GunSoldierAI::UpdatePatrol`; substeps, point selection, timers and wait motions have focused boundary tests. Engine display, movement, RNG and mutable shield state still require explicit body services. | enemy/GUN_SOLDIER.md §5.5; `src/GunSoldierAI.cpp` |
| 4 | Terrain collision internals (fn_80097658, fn_80097DB4, fn_80098ABC) and contact bits +0xA4 | **Partly answered (PROVEN):** collision candidate bits 15/2/5/10 map to contact bits 0/1/2/4 at 0x80098FBC..0x8009901C; a second probe maps bits 2/5 to contact 1/3. Contact 1 sends damage; 2/3 send death. Exact geometry, query masks and authored surface meanings remain UNKNOWN. | notes/open_questions_terrain_contact.md; PLAYER_GAMEPLAY_RECOVERY.md §8 |
| 5 | SET record flag **0x8** — **corrected (PROVEN control):** written by `DetachKilled` 0x800C9EEC and by GunSoldier at spawn; checkpoint save maps it to 0x200. The scanner 0x800CACA4..0x800CACF4 preserves 0x8 during explicit 0x40 re-arm, so 0x8 is **not** itself a no-respawn veto. Killed records stay down because detachment clears enable 0x1 and supplies no re-arm trigger. Native tests cover both cases. The flag's full gameplay intent beyond checkpoint persistence remains UNKNOWN. | STAGE_GAMEPLAY_DATA_FORMATS.md; enemy/GUN_SOLDIER.md; `src/tests/test_set_task.cpp` |
| 5b | ~~`BkLarvaGenerator` census and mission gate~~ **Answered (PROVEN for native scope):** SET 0x0091 `Num`, one-child-per-update spawn at 0x80189C88, collision life event at 0x8018A9CC, and distinct command-14 death are reconstructed; stg0201/0404 clear in `stage_sim`. Stage gating, spatial trigger, motion and collision delivery remain engine hooks/UNKNOWN. | enemy/BK_LARVA.md; `src/BkLarva.cpp`; `src/sim/stage_sim.cpp` |

## P2
| # | Question | Where it stops | Doc |
|---|---|---|---|
| 6 | Global byte **0x8057E828** (forces enemy AI Idle, blocks karma loss, read by player input/bosses) | no writer found by constant xref | enemy/GUN_SOLDIER.md, COLLECTIBLES_AND_SCORING.md |
| 7 | ~~Setter of player state flags 0x22/0x23~~ **Answered (PROVEN):** RTTI `Awake` ctor 0x800A51F0 selects row 0/1 of DOL table 0x804B39D4 and sets 0x23 for gauge 1 / 0x22 for gauge 0 at 0x800A53A8..B4; Update 0x800A4D2C clears the flag when its gauge empties. Other effect lifetime details remain open. | notes/open_questions_terrain_contact.md; PLAYER_GAMEPLAY_RECOVERY.md |
| 8 | Parameters of LightDash, Grind, hangs, DriveVehicle, ChaosBlast behaviours | Update addresses listed, constants not extracted | PLAYER_GAMEPLAY_RECOVERY.md §4.3 |
| 9 | Stage-table words +0x24/+0x30/+0x3C (0x2xx ids), +0x44..+0x4C, hook pointers +0x0C..+0x18 | no consumer found | MISSION_PARAMETER_MAP.md §2 |
| 10 | StageState mode value 9 that suppresses mission clear/fail messages | UNKNOWN | MISSION_SYSTEM.md §6.1 |
| 11 | Consumers of logical R/L/Z buttons | weapon control region suspected | PLAYER_GAMEPLAY_RECOVERY.md |
| 12 | `SetObjDesc` fields +0x10/+0x14/+0x1A/+0x1C (0x80/0x200/0x800; 1/2/4; 10/5) | values listed only | STAGE_GAMEPLAY_DATA_FORMATS.md §4 |
| 13 | What "ds1" SET files represent (always loaded; exempt from the global suppression bit) | loader behaviour only | STAGE_GAMEPLAY_DATA_FORMATS.md §1 |

## P3
| # | Question | Doc |
|---|---|---|
| 14 | Mission message contents (voice lines, HUD pop-ups) | MISSION_SYSTEM.md §7 |
| 15 | Behavior vtable slot 6; Control vslots 1/2; CharCommand vslot 3 | PLAYER_GAMEPLAY_RECOVERY.md |
| 16 | World-unit scale (metres per unit) | — |
| 17 | Library code bands 0x8040C040–0x804226BC and 0x80440040–0x80472350 (RenderWare toolkits?) | GAMEPLAY_DOL_MAP.md §4 |

## Resolved during integration (kept for traceability)
- SET spawn/despawn ranges are measured from **camera** units 0/1, not the player (fn_80009548 / fn_80010244 / fn_80169FC8).
- Karma gauge 0 = hero (Chaos Control), gauge 1 = dark (Chaos Blast).
- SET param type codes 1 Sint32, 2 Uint32, 3 Hex, 4 Single (the DOL's own "Sample" object).
- Stage number ↔ stage index = row of the stage table 0x804C5AE8.
- Enemy type ids 0–14 (except 9) resolved by constant propagation through base-ctor chains.

## Boot coordination, research42c

Gameplay questions above are unchanged. **PROVEN reference / conditional
native work:** original apploader and prefix work now produce the correct
40.5MHz source clock phase through80373AC4 without observed frontier counts.
Fresh source-bound DTK lifecycle now reaches the GPU AllowSleep effect;
enabled dumping fails at the earlier live system dump-state read. Original
Entry/callee receipts support11 native words/10 ordered stores through
before812003B8. The frame model owns no scheduler and refuses a connection
from current declared-only Complete event owners.
**UNKNOWN native ownership:** live GPU worker/async/CP effects,
Movie/frame-step/achievement lifetime, general log/WAV ingress, SI poll/input
and progressive device/memory state. Production remains before80379628.
Fresh GC memory Clear is the source of zero at800030F0; restored/retail
ownership, ordered copy extents and live alarm/DEC/callback consumers remain
gated. See `../reverse/boot/research/CLOCK_PRODUCTION_RESEARCH_42.md` and boot
`OPEN_QUESTIONS.md` for exact addresses, evidence and next tests.
