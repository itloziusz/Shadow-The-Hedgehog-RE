# Open questions: terrain contact and remaining P1/P2 probes (2026-09-30)

Source: PAL GUPP8P `../sys/main.dol`, queried with existing `tools/q.py` and `tools/ppc.py` caches. Addresses below are DOL virtual addresses. This is static evidence, not a runtime trace. No original function name is claimed unless RTTI supplies it.

## P1 #4: terrain collision and Player+0xA4 contact bits

- **PROVEN** `Player_TerrainCollide` at 0x80097658–0x80097D6C clears Player+0xA4 bit 0 at 0x80097680–0x80097690 and bit 4 at 0x80097694–0x800976A0. It calls the broad collision resolver `fn_80098ABC` at 0x80097770, 0x80097928, and 0x80097C20 with `r6=0x40000`. It also calls `fn_80097DB4` at 0x80097884, 0x800979E4, and 0x80097B7C. After position/collision adjustments, it calls `fn_800988A8` at 0x80097C94, writes the resolved position into Player+0xD0 at 0x80097C98–0x80097CA0, and sets or clears grounded Player+0xA8 bit 2 at 0x80097CB8/0x80097CCC. The meaning of `r6=0x40000` within the lower collision layer is **UNKNOWN**.
- **PROVEN** `fn_80097DB4` at 0x80097EA0–0x80097EC4 calls `fn_80021E24` with callback 0x80099454, returning a collision-result pointer or null. It takes the result object from result+0x14 at 0x80097EC8; `fn_800980D8` reads this object's float +0x18 at 0x80097ED0. Nonpositive values become -1.0 at 0x80097EE0–0x80097EEC. On a positive result it optionally copies two vectors at 0x80097EF0–0x80097F24. It writes the low 28 flag bits from `fn_80026588` to an optional output pointer at 0x80097FB0–0x80097FC4, and the high 4 flag bits to another at 0x80097FC8–0x80097FDC. No hit returns -1.0 at 0x80098000–0x8009800C. Calling this a *raycast* is **LIKELY**, based on return distance and vectors; exact query shape and ownership are **UNKNOWN**.
- **PROVEN** `fn_80026588` (0x80026588–0x800265A4) reads `object+0xC`, then `[object+0xC]+0x1C`, or returns 0 when the inner pointer is null. In the collision loop, `fn_80098ABC` calls it at 0x80098F70 on a collision candidate/result. These are *collision candidate flags*; whether +0x1C is an authored terrain/material field is **LIKELY**, not directly established.
- **PROVEN** Per candidate in `fn_80098ABC`, `clrlwi r23,r3,4` at 0x80098F78 keeps the low 28 bits returned by `fn_80026588`. The following PPC masks and calls to `BitSet32` (0x80014E1C) set Player+0xA4 bits as follows. Here "source bit" is the ordinary least-significant-bit index in that low-28 flag word:

  | Candidate-flag test | source mask | contact bit set | set call | confidence |
  |---|---:|---:|---|---|
  | `rlwinm. ...,0,0x10,0x10` @0x80098FBC | 0x00008000 (bit 15) | 0 | 0x80098FD4 | PROVEN |
  | `rlwinm. ...,0,0x1D,0x1D` @0x80098FD8 | 0x00000004 (bit 2) | 1 | 0x80098FEC | PROVEN |
  | `rlwinm. ...,0,0x1A,0x1A` @0x80098FF0 | 0x00000020 (bit 5) | 2 | 0x80099004 | PROVEN |
  | `rlwinm. ...,0,0x15,0x15` @0x80099008 | 0x00000400 (bit 10) | 4 | 0x8009901C | PROVEN |

  The bit-0 write is conditional on further normal/motion checks at 0x80098F74–0x80098FB8; the other three are after those checks. The loop uses 0x80021934 to obtain candidate results at 0x80098C90, then iterates via 0x80098D28/0x80099190. Which authored surfaces carry these masks is **UNKNOWN**.
- **PROVEN** The separate `fn_800988A8` query path calls `fn_80097DB4` at 0x80098988 and, if needed, again at 0x80098A30. Its successful-hit branch (float result >= 0) tests the low-28 output word at stack+0x1C. At 0x80098A48–0x80098A60 source bit 2 sets Player+0xA4 bit 1. At 0x80098A64–0x80098A7C source bit 5 sets Player+0xA4 bit 3. The geometric purpose of this extra probe is **UNKNOWN**.
- **PROVEN** Controller `fn_8009F17C` tests Player+0xA4 bits 2 and 3, clears each after detection, and sends one DeadCommand (id 8) if either occurred: 0x8009F198–0x8009F240. Contact bit 2 clears Player+0xA8 flag 0x13 at 0x8009F1C0–0x8009F1C8; bit 3 sets flag 0x13 at 0x8009F1F8–0x8009F204. Thus the two lethal contacts select a distinct death-state flag, though their game-world labels are **UNKNOWN**.
- **PROVEN** Controller `fn_8009F29C` tests and clears contact bit 1 at 0x8009F2B8–0x8009F2D8, then constructs/sends DamageCommand (id 7, float power from 0x805F3B80) at 0x8009F2E8–0x8009F318. The DOL float at 0x805F3B80 is 10.0 (independently read with `dol.py`); the damage-command constructor receives that value at 0x8009F2F8–0x8009F300.
- **STRONG** A direct material-bit-to-player-command chain now exists: candidate flag bit 2 → contact bit 1 → DamageCommand; candidate flag bit 5 → contact bit 2 or 3 → DeadCommand, with the contact-bit choice depending on which collision probe produced the hit. This is supported by independent disassemblies of the resolver and controller. It does **not** yet establish complete collision geometry, surface taxonomy, or all contact-bit writers.

### Native reconstruction boundary

The resolver has broadphase/candidate traversal (`fn_80021934` at 0x80098C90), per-candidate normals/positions (0x80098D28–0x80098F68), optional callbacks/side effects (0x80099020–0x80099154), and iterative adjustment (branch back at 0x80099284). **UNKNOWN:** exact geometry representation, query masks, callbacks, ordering ties, and authored surface semantics. A native collision implementation from these snippets alone would invent behaviour. Use explicit terrain-query engine hooks until those pieces are recovered.

## P2 #6: global byte 0x8057E828

- **PROVEN** The byte is read via global block 0x8057E760+0xC8 by `fn_8006F8E8` at 0x8006F920–0x8006F930 to skip a player-control path when nonzero. Existing xref results also list readers at 0x800A1DE0 (UserInput), 0x8019EBF0 (EnemyBaseAI pre-update), and 0x8016F514 (KarmaGauge_Add), among others.
- **UNKNOWN** The writer, event that enables/disables it, and exact global meaning. `q.py xref 0x8057E828` returned only read sites, so that search does not prove no writer: a bulk initialization, indirect pointer, or relative store may evade constant xrefs. No native setter should be asserted.

## P2 #7: player flags 0x22/0x23 — setter recovered

- **PROVEN** DOL table 0x804B39D4 has two 0x20-byte rows. Row 0 starts with bit index 0x23 and has gauge index 1 at +0x14; row 1 starts with bit index 0x22 and has gauge index 0 at +0x14. Independently read the DOL words with `dol.py`: row 0 `{0x23,0x4B,0x54,0x1A,0x3,1,0,0x804B39AC}`; row 1 `{0x22,0x3F,0x49,0x19,0x4,0,1,0x804B39C0}`. Meanings of the other fields are **UNKNOWN**.
- **PROVEN** In `Player_CheckChaosPowerReady`, gauge 1 full at 0x800AF814–0x800AF830 calls `fn_800A2FE8`; gauge 0 full at 0x800AF838–0x800AF850 calls `fn_800A2FC0`. The first path reaches `fn_800A3830` and passes `r7=0` to `fn_800A51F0` at 0x800A38A4. The second reaches `fn_800A37A4` and passes `r7=1` at 0x800A3818.
- **PROVEN** `fn_800A51F0` is constructor of the original RTTI class `@unnamed@PlayerEffect_cpp@::Awake` (typeinfo 0x805E59D0; vtable 0x80522A94). It selects `table + 0x20*r7` at 0x800A5280–0x800A5290, then reads row+0 and calls `PlayerFlags_Set(player+0xA8, bit)` at 0x800A53A8–0x800A53B4. Thus **gauge 1/full → flag 0x23**, **gauge 0/full → flag 0x22**. The prior constant-bit scan missed the setter because `r4` is loaded from a table. This directly answers the setter portion of OPEN_QUESTIONS #7.
- **PROVEN** The same vtable's Update slot (0x80522AA0) is `fn_800A4D2C`. It selects the same row at 0x800A4D48–0x800A4D64, uses row+0x14 as gauge index at 0x800A4DC0 and 0x800A4DD8, and clears the row+0 bit when the selected gauge current value is <= 0 at 0x800A4DF4–0x800A4E20. `Player_ClearChaosFlags` also explicitly clears 0x23/0x22 at 0x80079618/0x80079624. Full task lifetime and effects are outside this probe.
- **PROVEN** The corresponding input readers are `UserInput_BuildButtonCommands` at 0x800A0324 (0x23) and 0x800A0360 (0x22). The Chaos-ready check itself gates on both at 0x800AF7F0/0x800AF808. The flags mean a ready effect is present, as inferred from constructor/task ownership and those gate sites: **STRONG** semantic label, not an original enum name.
## P2 #9: stage-table fields

- **PROVEN** The table base is 0x804C5AE8, row stride 0x50; `MISSION_PARAMETER_MAP.md` enumerates +0x24/+0x30/+0x3C, +0x44/+0x48/+0x4C and hook pointers +0x0C..+0x18 for all 59 rows. This probe has not identified a new consumer.
- **UNKNOWN** Semantics of the 0x2xx IDs, the remaining tail words, and the hook targets beyond the established +0x0C call at 0x80178780. Value-pattern correlation is insufficient to name them.

## Checks and limits

- Independently checked the key branch/call ranges with both annotated `q.py dis` and raw `ppc.py` output (0x80097658, 0x80097DB4, 0x800988A8, 0x80098ABC, 0x8009F17C, 0x8009F29C).
- No C++ or existing docs changed in this workstream. No build/test run here; this is static documentation only and the coordinator runs the shared build gate.
