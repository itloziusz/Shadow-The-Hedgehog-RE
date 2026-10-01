# HANDOFF — Shadow the Hedgehog (GC) gameplay recovery

**Read this first if you are resuming.** (Other agents such as Codex: start with `Read this to continue Codex.md`.) Updated at each checkpoint. Newest checkpoint on top.

---

## Checkpoint 28 — 2026-10-01 (bounded native boot crosses sync)

- Read `reverse/boot/PROGRESS.md` checkpoint 37 and
  `research/NATIVE_SYNC_COMPLETION_37.md`. The new entry-owned immutable
  native runner crosses sync, connects GQR/FPR state and both HID0 branch
  consumers, stopping before live L2CR at **`0x80372894`**.
- Three controlled pre-entry experiments match 5,763 state fields and 444
  known stack bytes; 166 checked words are mutation-gated. Full Release CTest
  passed **49/49**. Old HLE HID0 includes DCFI; the new backend rejects it.
  The historical request-only probe preserves its `80371730` stop.
  Standalone `build_and_run.ps1` also passed **27/27** and stg0100 Dark → 6.
- Recognizer profiles are separate; reviewed advancement follows control flow,
  including lower addresses, and requires matching previous-stop evidence.
  Next: L2CR producer/completion/polls and error/logger paths. Retail inputs,
  excluded hardware modes, physical timing and first-frame/pixel parity remain
  UNKNOWN. All gameplay, decline and runtime gates remain required.

## Checkpoint 27 — 2026-09-30 (boot hidden-state capture and FPR projection)

- Continue boot work from `reverse/boot/PROGRESS.md` checkpoint 36. The
  connected probe still stops before sync at `0x80371730`; the new read-only
  reference export measures cache reset/refill, GQRs, HID2 and both FPR lanes.
- A bounded native 74-word FPR projection matches 4,896 raw fields from six
  HLE experiments. It is not connected across the unresolved barrier.
  A reserved FPSCR bit bug and debugger E-prefixed-data/EOF issues were
  reproduced, fixed and regression-tested. Full Release CTest is **48/48**.
- Gameplay behavior and exact mission routes are unchanged. Retail handoff,
  physical bus/cache ordering, exceptional FP and first-frame parity remain
  UNKNOWN. Next: prove the native immutable-code completion contract, then
  connect GQR/FPR state and compare downstream consumers.

## Checkpoint 26 — 2026-09-30 (six-agent boot wall audit)

- Six independent boot workstreams checked the `sync`/GQR tail, HID0 ICFI
  completion, paired FPR lanes, L2 status polls, the first OS clock helper,
  and the current native prefix adversarially. Their bounded evidence is
  indexed in `reverse/boot/ARCHAEOLOGY_INDEX.md`; the connected native stop
  remains **before** `0x80371730`. A write-request log is insufficient to
  cross `sync` without completed cache/barrier state and later HID0 readback.
- The adversarial audit reproduced a genuine error: a user-mode MSR passed
  native `mfmsr` at `0x80003400`. The wrapper now declines before that word;
  the HID2 read accessor separately declines user mode. A direct negative
  CLI gate plus API cases prevent recurrence. Full Release CTest passed
  **44/44**; ordinary supplied-HLE boot still stops at `0x80371730`.
- Synthetic HLE checkpoints exposed PS0 but not PS1, and observed both L2IP
  polls clear in one run. Retail PS1/cache timing and the OS time-base plus
  low-memory offset remain UNKNOWN. Gameplay code and stg0100 route were not
  changed in this checkpoint. See `reverse/boot/PROGRESS.md` checkpoint 30.

## Checkpoint 25 — 2026-09-30 (boot HID0 ICFI request)

- The connected native boot probe now accepts an explicitly observed HID0
  word, executes the byte-pinned `0x8037172C` call and HID0 ICFI request leaf,
  and stops before `sync` at `0x80371730`. The issued value is not assumed
  to be the later self-cleared hardware readback. `reverse/boot/PROGRESS.md`
  checkpoint 29 records the instruction boundary and first missing effect.
- The full root Release gate passed 43/43 CTest. The new exact-output case
  compares the synthetic/HLE state at `0x80371730`; the C++ regression
  mutates every leaf word and tests a distinct HID0 input. The standalone
  gameplay 27/27 and stg0100 route were validated in checkpoint 24 and are
  unaffected by this boot-only step.

## Checkpoint 24 — 2026-09-30 (boot HID2 boundary and standalone gameplay gate)

- The canonical boot probe in `reverse/boot/` now runs from PAL DOL entry
  through the exact HID2 OR/write request and stops at `0x8037172C`, before
  the HID0 read. This uses explicitly supplied synthetic/HLE MSR and HID2;
  later hardware readback and retail IPL state remain UNKNOWN. Six separate
  binary-first notes cover HID2, HID0 ICFI, FPR lanes, L2 polls, CRT memory
  and constructor indices 12–15. `reverse/boot/PROGRESS.md` checkpoint 28
  gives the exact proof boundary and next failure.
- A CRT return-state audit corrected r4 and r31 in its static note. A new
  binary-derived negative gate prevents the incorrect values returning.
  Full-tree Release CTest passed 42/42.
- The standalone `build_and_run.ps1` accepts `SHADOW_GAME_FILES_DIR` for
  read-only extracted assets outside this public repository. With that path
  set, it passed 27/27 gameplay CTest and stg0100 Dark 35/35, next stage
  index 6. No gameplay simulation semantics changed in this checkpoint.

## Checkpoint 23 — 2026-09-30 (strict boot execution frontier)

- Ran the connected native `shadow_boot_probe` again from PAL DOL entry. It
  still stops correctly at PC/LR `0x80003158`, with r1 `0x8060C5F0`, r2
  `0x805FA780`, r13 `0x805EC500`. This is the **last fully validated connected
  checkpoint**, not a newly booted game. `reverse/boot/PROGRESS.md` records
  the first stop, root cause, evidence and next comparison in the requested
  run → trace → fix → rerun workflow. A structurally translated pre-clock
  event candidate was withdrawn because no original-state and end-to-end
  gates could validate it across the hardware/CRT gap.
- New `reverse/boot/research/APPLOADER_PATH.md` proves normal completed
  apploader state 6 calls zero-fill on the DOL BSS envelope at `0x81200DA0`
  **before** loading sections. This initially zeroes the FPR source
  `0x805F1F30..3F`. A later IPL-dependent FST read can overlap that gap, so
  the final pre-entry bytes remain UNKNOWN. `OS_STARTUP.md` maps the later
  first unresolved time-base/MSR/low-memory call at `0x80370EA4` and device
  waits; no OS success stub was introduced.
- Fixed a genuine arithmetic bug in the archived hardware helper:
  `mtfsf 0xFF,f0` at `0x80370DFC` derives FEX/VX summary bits instead of
  copying them from the source low word. IBM Gekko/NXP manuals and a focused
  executable `boot_fpscr_arithmetic` regression cover a finite counterexample.
  Exceptional PS1 encodings now decline rather than silently preserving raw
  source bits. This helper remains disconnected from the validated entry prefix.
- Automatic approval review rejected an agent's proposed
  `reverse/boot/research/FPR_SEMANTICS.md` write under an earlier
  gameplay-only path restriction. The agent did not retry it. The essential
  findings are in this checkpoint, existing boot code comments and the
  in-thread report. The full MSVC Release build and **33/33 CTest** passed
  after integration; the direct boot probe was rerun and stopped at the same
  validated `0x80003158` boundary.
- Next connected boot gate requires a same-run original entry snapshot of
  MSR/HID0/HID2/L2CR, IPL word `[0x80000028]`, final FPR source bytes, and
  `0x80003400` branch/write effects. No value is installed from a static
  candidate merely to advance the PC. Later OS, constructor, event, runtime
  and pixel gates remain required.

---

## Checkpoint 22 — 2026-09-30 (boot frontiers and application loop)

- Three independent PAL DOL/disc audits now live in `reverse/boot/research/`:
  `PREENTRY_STATE.md`, `HARDWARE_PATH.md`, and `CRT_TO_GAME.md`. The original
  apploader requires IPL low-memory inputs; header word `[0x430]=0x803E74E0`
  overlaps loaded DOL code and is **not** a proven final FST base. Hardware
  path `0x80003400` reads bytes `0x805F1F38` then `0x805F1F30` **before** CRT
  zeroes that region; the DOL does not supply them. The constructor table has
  282 nonzero in-text targets, but their runtime effects remain UNKNOWN.
- Independently confirmed that `0x800510C0` is the static application event
  loop. Its recurring `0x800511E0..0x80051218` phase is now native C++17 with
  explicit event dispatcher, service, and exit-word hooks.
  `reverse/boot/research/APPLICATION_LOOP.md` records the exact source path through event `0x12`,
  `Game_FrameStep` and the conditional exit setter. Initialization, actual
  callback state, displayed frames and pixel parity are still UNKNOWN.
- Corrected a real diagnostic error in archived `experimental_native_boot`:
  handler store `0x803733C0` targets `0x80586CB4`, not `0x80580000`. A fixture
  regression now pins the caller selector and store-address instructions.
  Corrected stale FST-address and event-loop wording in the boot documents.
- Public MSVC Release build and **32/32 CTest** passed against the exact
  read-only PAL DOL and game data (27 gameplay, asset, runtime, three boot).
  The added loop fixture pins source instruction words and tests zero, one,
  three and dispatch-triggered exit passes. Gameplay route and pixel-gate
  requirements are preserved; no pixel parity is claimed.
- Next: obtain an independent original-path preentry snapshot and branch trace
  described in the research notes. Without the 16 pre-CRT FPR source bytes and
  IPL/low-memory branch inputs, extending past `0x80003400` as an exact native
  boot would invent state. Continue bounded boot and gameplay recovery while
  retaining that explicit decline.

---

## Checkpoint 21 — 2026-09-30 (public native boot entry foundation)

- The canonical public repository now builds `reverse/boot/shadow_boot_foundation`
  as C++17 plus a fixture-backed `shadow_boot_probe`. **PROVEN** for PAL GUPP8P:
  DOL entry `0x80003154` branches to the 36-instruction register helper
  `0x800032B0..0x8000333C`; the native slice yields PC/LR `0x80003158`,
  r1 `0x8060C5F0`, r2 `0x805FA780`, r13 `0x805EC500`, then stops before
  `0x80003400`. It does not claim hardware, CRT or application boot parity.
- Fixed two validation gaps in the old standalone proof: it checked only the
  first/last helper instruction and accepted extra populated DOL section
  slots. The library now pins all 36 words and all 18 section slots; tests
  mutate each helper word, entry/boundary words, a section descriptor and
  absent slots. The exact PAL DOL SHA-256 is checked at each content-backed
  CTest run. The former standalone file was removed to avoid duplicate code.
- Public MSVC Release build and **31/31 CTest** passed against read-only game
  files (27 gameplay, asset, runtime, two boot). The stg0100 exact-route and
  no-duplicate-spawn gates remain green. Gameplay code and its original
  27-suite build script are unchanged by this checkpoint.
- Next boot frontier: independently establish original pre-entry low-memory
  state and the effects of `0x80003400` before extending the native startup
  path. See `reverse/boot/OPEN_QUESTIONS.md`. Continue gameplay world,
  collision and remaining enemy work without inventing those services.

---

## Checkpoint 20 — 2026-09-30 (GUN Robot and BK Worm simulator adapters)

- Connected native `GunRobotAI` (SET 0x0068, HP 8, team GUN) and `BkWormAI`
  (SET 0x0090, HP 12, team Black Arms) to the stage simulator. Their SET
  parameter indices and HP/team values come from `data/setobj_catalog.txt`,
  0x804CD678 and 0x804CD310. Walker caution, target tracking, terrain RNG,
  worm movement/effects and animation remain explicit harness hooks; this is
  decision integration, not world or attack parity.
- Added native-spawn regressions: stg0301 has 11 native GUN Robots; stg0201
  has 9 native BK Worms. The full `build_and_run.ps1` gate passes **27/27
  CTest**, including all ten exact stage routes and stg0100 Dark 35/35 → index
  6. Independent census: stg0100, stg0301, stg0401 and stg0404 now have zero
  stub enemies. Remaining stubs across the ten routes are BK WingLarge,
  Egg Pierrot, BK Giant, GUN Bigfoot, BK Chaos and BK Ninja.
- Next: recover those remaining families and, more importantly, the missing
  movement, collision, path and world engine services. Public commit
  `f5b7f05` contains this checkpoint; its full multi-module gate passed
  **29/29 CTest** against read-only game data.

---

## Checkpoint 19 — 2026-09-30 (BK WingSmall simulator adapter)

- Connected SET 0x008F `BK_WINGSMALL` to the bounded native `BkWingSmallAI`
  decision module in `src/sim/stage_sim.cpp`. The harness reads `AppearType`
  and `ActionType` (SET params 7/8), `FloatWidth` (param 10), builds the shared
  search sensor, and uses proven type-7 HP 2 / Black Arms team
  (`data/setobj_catalog.txt`, 0x804CD678/0x804CD310). No path geometry is
  fabricated: `EnemyPath.one`, activation application, motion, flight and
  attack displacement remain explicitly labelled harness hooks.
- Added `stage_sim_stg0100_wing_native` to catch a return to the stub factory.
  **25/25 CTest** pass; stg0100 now reports 74 native spawns, zero stubs,
  18 BK WingSmall native spawns and zero duplicate GUN Beetle SET spawns.
  Dark 35/35 still routes to index 6. All other exact-route gates pass.
- Next: connect GUN Robot/BK Worm world adapters where stage data exercises
  them; reconstruct the missing movement, collision and path services from
  evidence. The clean initial public repository is published, with a follow-up
  pending for this new simulator integration.

---

## Checkpoint 18 — 2026-09-30 (three bounded enemy AIs; portable test gate)

- Added PROVEN bounded decision slices for original RTTI `GunRobotAI`
  (`GUN_ROBOT` 0x0068; 0x80186308..0x80186990), `BkWingSmallAI`
  (`BK_WINGSMALL` 0x008F; 0x80181D3C..0x801825D0), and `BkWormAI`
  (`BK_WORM` 0x0090; 0x80182964..0x8018393C). Each has a family document,
  explicit engine-service hooks and a Release-active focused test. They are
  linked into the native library; the stage simulator still uses stubs for
  these families until its world-service adapters are connected.
- Independent cross-compiler check caught a GUN Robot `switch` case variable
  whose initialization crossed case labels. Scoped case 1 with braces; the
  recovered branch and target position call are unchanged. The MSVC Release
  build and all **24/24 CTest** suites now pass. stg0100 Dark 35/35 still routes
  to index 6. The no-respawn gate now counts duplicate GUN Beetle spawns per
  SET slot (zero), so adding native families cannot mask that regression.
- Merged 25 new names from `notes/{gunrobot,bkwingsmall,bkworm}_symbols.csv`;
  regenerated `GAMEPLAY_SYMBOLS.csv` (1,344 curated; 8,594 exported data rows).
- Next: connect the three family adapters to the simulator with labelled
  world hooks, continue player/world reconstruction, and publish the verified
  source plus evidence to the new canonical repository without game binaries.

---

## Checkpoint 17 — 2026-09-30 (SET flag 0x8 re-arm correction)

- **PROVEN correction:** runtime flag 0x8 is written at normal kill/detach
  (`SetSlot_MarkKilledAndDetach` 0x800C9EEC) and at GunSoldier spawn
  (`SetSlot_SetFlag8` 0x800C9C44), but it is not a spawn veto. The scanner
  0x800CACA4..0x800CACF4 preserves 0x8 when an external 0x40 re-arm trigger
  reenables the record. A normally killed record stays down because detach
  clears enable 0x1 and supplies no re-arm trigger. The flag's full gameplay
  meaning remains UNKNOWN beyond its proven checkpoint-save mapping to 0x200.
- Independently rechecked the scanner branches and added a Release-active
  `test_set_task` edge case: killed record does not spawn on the next scan;
  explicit 0x40 re-arm retains 0x8, enables the record, and permits the next
  scan to spawn it. Updated the data-format, enemy, open-question and current
  continuation docs to remove the stale "0x8 means do not respawn" claim.
- The full `build_and_run.ps1` gate after this test passed **21/21 CTest**;
  stg0100 Dark 35/35 → index 6. Historical checkpoint 9 below records the
  older interpretation and is superseded by this correction.

---

## Checkpoint 16 — 2026-09-30 (normal enemy death cannot respawn)

- Independent simulator audit found that GUN Beetle death deleted its task without
  `TEnemySetBase::DetachKilled`, so its still-enabled SET record spawned again. stg0100
  showed 62 native spawns after Beetle integration; the expected stage-owned count is
  56. This could inflate scripted mission counts despite a green route test.
- **PROVEN fix:** `SetData::MarkKilledAndDetach` follows
  `SetSlot_MarkKilledAndDetach` 0x800C9EEC → `SetSlot_DisableAndDetach` 0x800C9F1C →
  `SetSlot_Detach` 0x800C9FD0. It sets runtime flag 0x8, clears enabled/alive/despawn,
  detaches the object and removes it from the link group. The stage-owned misc buffer
  path excludes the original's separate dynamic-slot cleanup. `SimStatus` invokes this
  at the normal enemy death edge 0x801A8C40 for type != 0; GunSoldier keeps its slot
  while entering Hinshi. The scripted attacker now advances past a detached killed slot.
- **Regression:** `test_set_task` checks detached flags, link membership and no spawn on
  the next scan. An extra stg0100 CTest asserts 56 native / 18 stub spawns so the
  simulator cannot silently count a duplicate beetle. Final `build_and_run.ps1`
  passed **21/21 CTest**, stg0100 Dark 35/35 → index 6. Direct stg0201 Hero 60/60 →
  index 11 and stg0404 Hero 50/50 → index 32 also remain green.

---

## Checkpoint 15 — 2026-09-30 (Egg Pawn, GUN Beetle, player slices; 10 routes)

- **PROVEN partial GUN Beetle AI:** `src/GunBeetleAI.cpp` implements WaitFloating,
  MoveOnPath, Caution and Attack decisions, the inclusive ActionType-2 firing window
  (0x80193AD4), strict 1/3 s timers and same-frame death request. Its engine path
  geometry, float motion, weapon mount and activation application remain hooks.
  The independent audit removed an incorrect AppearType gate from the window helper.
- **PROVEN partial Egg Pawn AI:** `src/EggPawnAI.cpp` implements AppearType/wait dispatch,
  Caution, home return, Weapon/Tuki/Dash attack loops and shield-dependent motion IDs
  (0x8017CB50, 0x8017CBD8, 0x8017CC20). Movement, motion, target and terrain
  services remain hooks. The independent audit changed the movement-result hook into
  `AdvanceTowardMoveTarget` because `Enemy::MoveTowardHome` 0x8019B0C8 advances and tests
  in one call. Both families have focused CTest suites and labelled simulator factories.
- **PROVEN bounded player updates:** `StepGroundExit`, `StepFallLanding`,
  `StepJumpLandingGrace` and `StepHomingAttack` cover DOL branch/timer slices from
  0x80083B7C, 0x80080B88, 0x80087680 and 0x80085648. World, collision and
  target services remain explicit inputs; see `PLAYER_GAMEPLAY_RECOVERY.md` §5.5.
- **Genuine lifetime fix:** native `Enemy::~Enemy` now mirrors
  `TEnemySetTask::~TEnemySetTask` 0x801A705C..0x801A7098 by detaching its SET slot
  only when it still owns it. `test_enemy_framework` checks both normal teardown
  and a slot reattached to a replacement object. This removes the freed-object pointer
  exposed when GUN Beetle requested deletion.
- Merged three Egg Pawn symbol names (`notes/eggpawn_symbols.csv`), then regenerated
  `GAMEPLAY_SYMBOLS.csv`: 1,319 curated and 8,593 exported data rows.
- **Verification:** final `build_and_run.ps1` passed **20/20 CTest** in Release and
  stg0100 Dark 35/35 → stage index 6. All ten documented enemy-count stage simulator
  runs clear; each exact next-stage route is a CTest assertion. stg0201 Hero 60/60 →
  index 11 and stg0404 Hero 50/50 → index 32 still pass. The simulator still uses
  labelled stubs for remaining families, so these are integration gates, not game parity.
- Next: recover remaining enemy families and player/world engine services; terrain
  geometry, global byte 0x8057E828 and stage-table unknown words remain UNKNOWN.

---

## Checkpoint 14 — 2026-09-30 (larva Release test repaired)

- Independent audit found that `test_bklarva.cpp` used standard `assert`, which MSVC Release
  removes under `NDEBUG`; the CTest entry could pass without evaluating its checks. Replaced
  all larva assertions with a persistent `CHECK` counter and nonzero failure exit. Direct
  Release executable prints `BkLarvaGenerator native tests passed` with checks active.
- Re-ran `build_and_run.ps1`: **11/11 CTest pass**, exact simulator route gates pass, and
  stg0100 Dark 35/35 → index 6. The player agent may still edit its assigned files; rerun the
  full gate after all agent changes finish.

---

## Checkpoint 13 — 2026-09-30 (exact simulator route gate)

- CTest's three `stage_sim` cases previously required only process success. Added
  `PASS_REGULAR_EXPRESSION` checks for the exact documented Dark/Hero route and next-stage
  indices 6/11/32 in `src/CMakeLists.txt`. This catches a cleared mission routed to the
  wrong destination. Re-ran `build_and_run.ps1`: **11/11 pass**, stg0100 Dark 35/35 → index 6.
- No gameplay semantics changed. Egg Pawn, GUN Beetle and player behavior workstreams remain
  in flight; integrate each only after evidence and independent review.

---

## Checkpoint 12 — 2026-09-30 (patrol, larva, BK Soldier native decisions)

- **PROVEN patrol:** `GunSoldierAI::UpdatePatrol` now follows `WaitAction_Moving::Update`
  0x80188B98 over E+0x288 points. The four substeps, point choices, timer order, spawn-yaw
  wait turn (E+0x64), and live shield-dependent WaitType 1 motion have focused tests. The dead
  `PatrolStep()` interface was removed. Display, locomotion and RNG effects remain engine hooks.
- **PROVEN larva scope:** `BkLarvaGenerator` SET 0x0091 reads `Num`, creates one child per
  update (0x80189C88), and reports collision/command defeats through EnemyManager at
  0x8018A9CC/0x8018ABE0. Its different delayed/immediate death paths are tested; stage gating,
  world trigger, motion and collision delivery remain labelled hooks. Added `enemy/BK_LARVA.md`.
  stg0201 Hero 60/60 clears → index 11 (stg0302); stg0404 Hero 50/50 clears → index 32
  (stg0412). Both routes are permanent CTest gates. `SimTaskCleanup` fixes teardown order so
  actor tasks do not outlive their referenced SetData/EnemyManager.
- **PROVEN BK Soldier decision scope:** `BkSoldierAI` reconstructs AppearType dispatch,
  detection, weapon split, strict 30-unit melee transitions, distinct base/BK GoHome routes,
  and ranged update-before-sensor ordering (0x8017FDC0, 0x8017F870). Its animation,
  weapons, patrol movement and death effects remain explicit engine hooks. The stage simulator
  now uses it for SET 0x008D, HP 4 from 0x804CD678[5], and labels its body as a harness.
- **Verification:** the final `build_and_run.ps1` after all these edits passed **11/11 CTest**;
  stg0100 Dark 35/35 → index 6 (stg0200). Direct stage_sim runs confirmed the stg0201/0404
  counts and routes above. `symbols_curated.csv` has 1,316 data rows and
  `GAMEPLAY_SYMBOLS.csv` has 8,593 data rows after merging the terrain and larva fragments.
- Next: complete Egg Pawn and GUN Beetle native decision modules (in flight); integrate and
  test them without claiming unrecovered motion/weapon systems. Full player behavior transitions,
  terrain geometry, global byte 0x8057E828 and stage-table unknown words remain open.

---

## Checkpoint 11 — 2026-09-30 (terrain contact and Chaos-ready setter)

- Independent DOL checks confirmed the new `notes/open_questions_terrain_contact.md` findings:
  collision candidate bits 15/2/5/10 → Player+0xA4 contact 0/1/2/4 at 0x80098FBC..0x8009901C;
  contact 1 leads to DamageCommand at 0x8009F2B8..0x8009F318. Geometry and surface meanings stay UNKNOWN.
- PROVEN flag setter: original RTTI `Awake` ctor 0x800A51F0 selects table 0x804B39D4 row 0/1,
  setting player flag 0x23 for gauge 1 or 0x22 for gauge 0 at 0x800A53A8..B4. Its Update
  0x800A4D2C clears the selected flag at gauge exhaustion. The table words and setter/clear sites
  were independently read from the DOL. `OPEN_QUESTIONS.md` #7 is answered and #4 narrowed.
- Merged `notes/open_questions_symbols.csv` (4 new names) and regenerated `GAMEPLAY_SYMBOLS.csv`:
  1,311 curated names, 8,590 exported rows. No C++ changed in this checkpoint; the earlier
  checkpoint's 7/7 build and stg0100 clear remain the latest gate.

---

## Checkpoint 10 — 2026-09-30 (Codex continuation; player Jump hold)

- Re-read the full gameplay handoff, README, open questions and toolkit instructions in order. Baseline
  `build_and_run.ps1` passed 7/7 suites and stg0100 Dark → stage index 6.
- PROVEN partial native `Jump::Update`: `ApplyJumpHold` in `src/PlayerPhysics.*` follows
  0x80087384..0x800873F0. Behavior bit 4 and the **old** nonnegative timer gate one frame of
  character-table +4 thrust (Shadow 270 from 0x804B1628 + 14×0xC + 4); then the timer loses dt.
  The original 0x805F381C comparison value is 0.0. This is one step, not the full Jump behavior.
  Boundary tests cover timer crossing zero and disabled hold.
- After the C++ change, `build_and_run.ps1` again passed 7/7 and stg0100 Dark → stg0200.
- Patrol, BkLarvaGenerator and terrain/open-question investigations are active in separate gameplay
  files; integrate their evidence and run the gate again when they finish. The user's gameplay-only
  rule leaves the earlier GX reconstruction untouched.

---

## Checkpoint 9 — 2026-09-30 (end of session; all background work finished)

- Enemy generalization integrated: `enemy/BK_SOLDIER.md`, `EGG_PAWN.md`, `GUN_BEETLE.md`, `ENEMY_ALERT_PATHS.md`
  (sensor is the only alert path → SearchWidth 0 = passive, STRONG); SET flag 0x8 = "do not respawn" (STRONG).
- Weapons/damage/vehicles integrated: rewritten `WEAPONS_AND_TARGETING.md` (hit rule, damage values, weapon
  registry, ammo, lock-on) and new `VEHICLES.md`; DamageCommand payload added to `GAMEPLAY_COMMANDS.md`.
- Symbols: 168 + 648 merged → 1 307 curated (tool: `tools/merge_symbols.py`).
- Fix: GUN_SOLDIER.md ReleaseCommand → ReturnToWait (slot 0x0D), verified.
- `stage_sim` now uses the proven homing-attack damage 2.0; `build_and_run.ps1`: 7/7 pass, stg0100 Dark clears in 9.4 s.
- Next steps: see `Read this to continue Codex.md` §7.

---

## Checkpoint 8 — 2026-09-30

- **User rules now:** (1) do nothing outside `gameplay/`; (2) **build and run the reconstructed code from time to
  time** → `powershell -ExecutionPolicy Bypass -File gameplay\build_and_run.ps1` (build → 7 CTest suites →
  `stage_sim` stg0100). Run it after every C++ change.
- New: `src/sim/stage_sim.cpp` (integration simulator; harness stubs labelled). stg0100 Dark mission clears natively
  (35 kills → GoalAction → stg0200). 8/10 enemy-count missions clear; stg0201/0404 need `BkLarvaGenerator`
  (added to OPEN_QUESTIONS).
- `StageFlow.{hpp,cpp}` (StageState request/dispatch verified against 0x801762D0 / 0x80174974).
- Build dir moved into `gameplay/build` (all tests pass there).

---

## Checkpoint 7 — 2026-09-30

- `PLAYER_GAMEPLAY_RECOVERY.md` delivered (creation, per-frame order, 0x29C layout, 35 behaviors with all slot
  addresses, physics constants+consumers, damage/rings/Super, input). Player symbols merged → **491 curated**.
- Karma gauges labeled with code evidence: gauge 0 = hero (ChaosControl drains it), gauge 1 = dark (ChaosBlast
  −10000); `KarmaGauge_Add` semantics (no gain while any gauge is full) in COLLECTIBLES_AND_SCORING.md.
- Pipeline doc: player path (layers 7→8→13) + time bases (+0x18 player dt, +0x20 object dt, +0x24 slow task).
- Native: `PlayerBehavior.{hpp,cpp}` (fn_800A82FC transition semantics, Super remap) — 6 suites pass (clang+MSVC).
- **Running now:** enemy generalization (→ `enemy/BK_SOLDIER.md`, `EGG_PAWN.md`, `GUN_BEETLE.md`,
  `notes/enemy2_symbols.csv`, plus the zero-width alert question) and weapons/damage/vehicles (→ rewritten
  `WEAPONS_AND_TARGETING.md`, new `VEHICLES.md`, `notes/weapon_symbols.csv`). When they land: merge symbols
  (same merge snippet as before: append rows whose address is not yet in `symbols_curated.csv`), re-export,
  update ENEMY_BEHAVIOR_INDEX status table and README.

---

## Checkpoint 6 — 2026-09-30

- **Both milestone traces complete**: `enemy/GUN_SOLDIER.md` (14-step chain; game's own enum strings at 0x80539464:
  WeaponType NONE/KNIFE/GUN/MACHINEGUN/RIFLE/GRENADE/MISSILE, AppearType STAND/LINEAR_MOVE/TRIANGLE_MOVE/RANDOM_MOVE/
  OFFSETPOS/HINSHI, WaitType RADIO_CONTACT/ATTACK/HIDE/KAMAE) and `MISSION_SYSTEM.md` §9.
- Symbols: enemy (129) + mission (98) merged → 308 curated; `GAMEPLAY_SYMBOLS.csv` re-exported.
- **Correction (PROVEN):** SET spawn/despawn ranges are measured from **camera units 0/1** (`CameraManager_Get`
  0x80009548, `CameraManager_GetUnitPos` 0x80010244, `Range_IsAnyCameraWithin` 0x80169FC8), not the player. Docs fixed.
- Enemy type ids fully resolved (0 GunSoldier … 14 EggShadowAndroid; 9 BkLarva LIKELY) with max HP/team tables
  (ENEMY_BEHAVIOR_INDEX.md §2.1).
- Native: `PlayerPhysics.{hpp,cpp}` (fn_80093BBC integrator verified by hand; tuning constants with addresses).
  5 test suites pass on clang + MSVC.
- `README.md` added as folder entry point; `tools/README.md` lists all extractors/generators.
- Player deliverable (`PLAYER_GAMEPLAY_RECOVERY.md`, `notes/player_symbols.csv`) being written by the player
  investigator; notes in `notes/player_trace.md` are complete. Merge its symbols when it lands.

---

## Checkpoint 5 — 2026-09-30

- **Enemy milestone: traced** (`notes/enemy_gunsoldier_trace.md`; deliverable `enemy/GUN_SOLDIER.md` being written by
  the enemy investigator + `notes/enemy_symbols.csv` → merge into `symbols_curated.csv`, then `tools/export_symbols.py`).
  Key: HP 0 converts the soldier into an injured "Hinshi" soldier (never truly dies); AI state machine at AI+0x28.
- **Correction applied:** SET flag 0x8 is set at spawn by GunSoldier (ctor post-init) → renamed `SetSlot_SetFlag8`
  (was "MarkDestroyed") in CSV, docs and C++ (`kSetFlag8`).
- **Open item (important):** 54/187 placed GUN soldiers have SearchWidth = 0 → their sensor (`fn_801A6C7C`) can never
  fire, yet soldiers engage in-game → another alert path exists (AI message 0x205 → OnTargetFound? group alert?). No
  constant-arg sender of 0x205 found yet.
- New native C++: `EnemyAI.{hpp,cpp}` (state machine, EnemyBaseAI pre-update priorities, exact sensor math from
  fn_801A6DB8/fn_801A6C7C/fn_801A6D00, sin/cos convention proven), `GunSoldierAI.{hpp,cpp}` (all GunSoldier states;
  patrol + motion ids are explicit engine hooks, NOT reconstructed). Tests: 4 suites (`test_gunsoldier_ai` uses a real
  stg0100 soldier) — all pass on clang and MSVC.
- `CHECKPOINTS_AND_RESTART.md` (SavePoint → Checkpoint_Save fn_80119350; ContinueAction restore order; flags
  0x200/0x400/0x2000000). `WEAPONS_AND_TARGETING.md`, `COLLECTIBLES_AND_SCORING.md`, `GAMEPLAY_COMMANDS.md` done.
- Still running: player investigation (→ `notes/player_trace.md`).

---

## Checkpoint 4 — 2026-09-30

- **Mission milestone: DONE.** `MISSION_SYSTEM.md` (26-step Westopolis Dark "defeat 35 GUN soldiers" chain),
  `MISSION_PARAMETER_MAP.md` (all 59 stages), symbols merged into `symbols_curated.csv` (176 curated rows).
  Independently re-verified: `files/nukkoro2.inf` `[City1] MISSIONCOUNT_D : 35 0 / _H : 45 0`, loader strings.
- **Native C++ now covers**: Task scheduler, SET system, enemy lifecycle (`EnemyFramework.{hpp,cpp}`), missions
  (`Mission.{hpp,cpp}` + `StageTable.generated.cpp` from `tools/gen_stage_table_cpp.py`). Tests (clang + MSVC via
  `src/CMakeLists.txt`, build dir in scratchpad): `test_set_task`, `test_enemy_framework`, `test_mission` — all pass.
  `test_mission` runs the full chain on real data (census GUN 36 / BA 45; 35th kill → GoalAction → stg0200).
- New docs: `GAMEPLAY_COMMANDS.md` (89 command classes, type ids; `tools/command_catalog.py` → `data/commands.csv`),
  `COLLECTIBLES_AND_SCORING.md` (`tools/scoredata.py` → `data/score_table.csv`: 115 score/karma entries; ScoreCommand
  0x1C handled in `Player_HandleCommand` 0x800A7CE4).
- Still running: enemy trace (→ `notes/enemy_gunsoldier_trace.md`), player trace (→ `notes/player_trace.md`).
  Next: write `enemy/GUN_SOLDIER.md` + update `ENEMY_BEHAVIOR_INDEX.md` status; `PLAYER_GAMEPLAY_RECOVERY.md`;
  merge their symbol names; extend `EnemyFramework` with the AI state dispatch once known.

---

## Checkpoint 3 — 2026-09-30

- **User rule (updated): do NOTHING outside `gameplay/` — no scratchpad/temp,
  no builds elsewhere (build dir = `gameplay/build`), no memory writes.
  Do not modify other folders.** `GekkoForge_RE_Toolchain/` (copied in by the user) is READ-ONLY reference.
  - It targets the same PAL GUPP8P DOL. Its `docs/gameplay-recon-2026-09-29/examples/knowledge_shadow_gupp8p.yaml`
    independently agrees with our Task/scheduler findings and adds anchors: RNG 0x8040FCD0, sin 0x8000F3B0,
    cos 0x8000FFE8, atan2 0x80016260, pad manager 0x8056FE0C (record stride 0x2C: +0 held, +4 not-held,
    +8 pressed, +C released, +10 stick A, +1C stick B), frame delta float 0x805F7198, SDK bands.
  - Its runtime (`runtime/src/gekko/system.cpp`) names the **stage table 0x804C5AE8 (59 × 0x50)** — verified:
    row index == stage index (row 5 = stg0100).
  - Its prebuilt exes support `--headless --boot-stage stg0100 --watch <addr> --dump <addr> <len>` (runtime
    traces), but running them writes files beside the exe → NOT run (would modify that folder). Ask the user
    before using them.
- Mission investigation DONE: `notes/mission_trace.md` (Westopolis Dark "defeat 35 GUN soldiers" end to end;
  counts from `files/nukkoro2.inf`, keys from stage table, EnemyManager 0x80580BD8, MissionManager 0x80576FBC).
  The agent was asked to write `MISSION_SYSTEM.md`, `MISSION_PARAMETER_MAP.md`, `notes/mission_symbols.csv`.
- `WEAPONS_AND_TARGETING.md` (weapon ids 0xC8–0x114 in the SET catalog, info block, class framework).
- `ENEMY_BEHAVIOR_INDEX.md` + `tools/enemy_index.py` (all 27 enemy/boss types: create hook, sizeof, class, AI states).
- New tool helpers: `SetSlot_GetParamPtr` 0x800CA118 (params ptr if schema exists).

---

## Checkpoint 2 — 2026-09-30

Done since checkpoint 1:
- Deliverable docs written: `GAMEPLAY_DOL_MAP.md`, `GAMEPLAY_UPDATE_PIPELINE.md`, `GAMEPLAY_STRUCTURES.md`,
  `STAGE_GAMEPLAY_DATA_FORMATS.md`.
- Symbol DB: `symbols_curated.csv` (hand-maintained, read by tools) → `tools/export_symbols.py` →
  `GAMEPLAY_SYMBOLS.csv` (7.7k rows). Add new names to `symbols_curated.csv`, then re-export.
- `tools/setparse.py`: validates all 130 stage SET files against the DOL schema (26 557/26 567 exact).
- `tools/classify.py`: function families → `data/func_families.json`, `data/family_runs.txt`.
- Stage number→index table (setid.bin bit) derived; `0x8057E80C` = stage index (PROVEN role).
- SET param type codes PROVEN: 1 Sint32, 2 Uint32, 3 Hex, 4 Single.
- **Native C++** (`gameplay/src/`): `Task.{hpp,cpp}` (scheduler + TaskManager layers), `SetSystem.{hpp,cpp}`
  (SET load/scan/spawn/link lists/slot API), `SetObjCatalog.generated.cpp` (from `tools/gen_catalog_cpp.py`),
  test `tests/test_set_task.cpp` — builds with
  `clang++ -std=c++17 -Iinclude Task.cpp SetSystem.cpp SetObjCatalog.generated.cpp tests/test_set_task.cpp`
  and passes on real stg0100 data (or use `src/CMakeLists.txt`).

Still in flight: enemy trace (→ `notes/enemy_gunsoldier_trace.md`), mission trace (→ `notes/mission_trace.md`),
player trace (→ `notes/player_trace.md`). When they land: write `enemy/GUN_SOLDIER.md`, `ENEMY_BEHAVIOR_INDEX.md`,
`MISSION_SYSTEM.md`, `MISSION_PARAMETER_MAP.md`, `PLAYER_GAMEPLAY_RECOVERY.md`; add names to
`symbols_curated.csv`; extend C++ (enemy lifecycle/AI state machine, mission counter).

---

## Checkpoint 1 — 2026-09-30

### Workspace
- Input: `sys/main.dol` (verified DOL; entry 0x80003154; r2=0x805FA780, r13=0x805EC500), stage data in `files/`.
- Everything for this effort lives in `gameplay/`:
  - `gameplay/tools/` — Python toolkit (see `tools/README.md`). Pipeline: `program.py` → `dataflow.py` → `rtti.py` → `setobj_catalog.py` → `classify.py`. Query CLI: `q.py`.
  - `gameplay/data/` — generated caches (`program.pkl`, `dataflow.pkl`, `classes.json`, `setobj_catalog.{json,txt}`, `func_families.json`, `family_runs.txt`, `region_classes.txt`, `strings_raw.txt`, `xref_index.pkl`).
  - `gameplay/notes/` — raw investigation notes from sub-investigations (enemy trace, mission trace) — being written.
  - Deliverable docs (in progress): `GAMEPLAY_DOL_MAP.md`, `GAMEPLAY_STRUCTURES.md`, `GAMEPLAY_UPDATE_PIPELINE.md`, `STAGE_GAMEPLAY_DATA_FORMATS.md`, `ENEMY_BEHAVIOR_INDEX.md`, `enemy/GUN_SOLDIER.md`, `MISSION_SYSTEM.md`, `MISSION_PARAMETER_MAP.md`, `PLAYER_GAMEPLAY_RECOVERY.md`, `GAMEPLAY_SYMBOLS.csv`.
- Pre-existing, unrelated work in the repo root (asset loader, RE_Content/) — not used as evidence.

### Key proven facts so far (details → docs)
1. **RTTI is present** (MWCC): typeinfo `{name*, bases*}` in .sdata; bases `{ti*, offset}…0`; vtable `{ti*, this_adj, slots…}`; vptr → &vtable.ti, slot i at +8+4i. ~2170 unique ORIGINAL class names (`data/classes.json`). Anonymous namespaces leak source file names (`@unnamed@PlayerShadowWeaponGun_cpp@`).
2. **Runtime helpers**: operator new 0x803A1380, operator delete 0x803A1334, memset 0x8000540C, memcpy 0x800054F4(?), strcmp 0x803AD96C, sprintf 0x803AA248/0x803AA4AC (vsprintf-style), atexit-register 0x803A1210.
3. **Task system**: `Task` (Task::Task 0x8004F014, dtor 0x8004EF18): +0 name*, +4 u16 flags (bit0 kill-request, 0x20 destroying, 0x100 runs-while-paused), +8 prev (head.prev=tail), +C next, +10 parent, +14 firstChild, +18 vptr, +20 u64 profile ticks. Scheduler `fn_8004ECAC(Task*, float dt)` recursively calls vtable slot1 `Update(dt)`, deletes killed tasks via slot0(1).
4. **Frame loop**: `fn_80049078` → `fn_801E2210` (frame step; PAL 50/NTSC 60 select; dt) → `fn_801EB680(taskMgr)` → root. Task manager singleton `fn_8001C4F8` (bss 0x80571C6C); `fn_801EB614(mgr, layer)` returns layer task. Layer order (PROVEN, names in DOL 0x8053ED20): Root→[System, Audio, Debug, Scene→[Manager, Controller, Command, CharaColli, Landscape, Gadget, Vehicle, Enemy(14), Player(13), PostManager, Particle, Editor, Camera], PostSystem, Render].
5. **Stage init**: `StageManager::vf01` → `fn_801783D4` creates SetManagerTask+TStageInitalizeTask (layer 6), CharCommandManagerTask (8), CharaColliManageTask (9), EffectTask (0x10), `fn_8016E4EC` (mission region; likely MissionManagerTask) …
6. **SET layout files** (`stgXXXX_{cmn,nrm|hrd,ds1}.dat`, little-endian): header `"sky2", u32 count, u32 miscSize`; record 0x2C = byteswap format `"ffffffiisccii"` (fn_800CB1A4): pos f32[3], rot f32[3], u32 runtimeFlags(+0x18), u32 flags(+0x1C), u16 id(+0x20), u8 link(+0x22), u8 range×100(+0x23), u32 miscLen(+0x24), u32 miscPtr(+0x28, runtime). Loader `fn_800CBC54`; hard mode flag bss 0x8057E885 selects `_hrd` over `_nrm`.
7. **Object type catalog in DOL**: table 0x8052C1A0 → 286 descriptors {name, loadHook, releaseHook, createHook(+0xC), …, id u16 @+0x18, params*} with named, typed, ranged misc params (`data/setobj_catalog.txt`). setid.bin = per-stage enable bitmasks (fn_800CA7F0).
8. **Spawn**: `fn_800CB97C` (SetManagerTask update) → `fn_800CAA2C/fn_800CAC08` per slot → `fn_800CA8D8` → range test `fn_800CA970` (|pos−player|² ≤ (range·100)²) → descriptor createHook(slot).
9. **GUN Soldier**: createHook `fn_80196E38`: new(0x418) → `GunSoldier::GunSoldier` 0x8019708C(this,1,layer 0xE,slot). Component layout via RTTI (Status@0x110, Disp@0x1A8, Move@0x1E8, SetBase@0x250, AI@0x360, Task@0x3E0, EnemyReferer@0x410). Update: `TEnemySetTask::vf01` 0x801A6EB8 → `TEnemySetBase::vf07` 0x801A7280 lifecycle (state@+8: 1 res wait, 2 init comps, 3 ready wait, 4 run: Status/AI/Disp/Move per frame).

### In flight at this checkpoint
- Background investigation A: GunSoldier AI states / damage / death / despawn → `gameplay/notes/enemy_gunsoldier_trace.md`.
- Background investigation B: Mission system, parameter source, progress & completion → `gameplay/notes/mission_trace.md`.
- If those notes exist, integrate them into the deliverable docs; if missing/partial, redo from the "WHAT TO DETERMINE" lists in this file's companion notes.

### Next steps (priority)
1. Write deliverable docs from facts above (DOL map, structures, pipeline, SET formats).
2. Integrate enemy + mission traces → milestone docs (enemy/GUN_SOLDIER.md, MISSION_SYSTEM.md, MISSION_PARAMETER_MAP.md).
3. Build `GAMEPLAY_SYMBOLS.csv` (curated names; symbols.py already reads it).
4. C++ reconstruction under `gameplay/src/` (Task scheduler, SET loader/spawner, enemy lifecycle) with PPC address annotations.
5. Player mechanics (Player::Behavior::*, 0x8006D274–0x800B85E0), weapons (0x80052368–0x80069BA0).
