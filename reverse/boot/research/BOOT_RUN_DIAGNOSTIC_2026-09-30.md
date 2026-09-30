# Native PAL boot probe rerun and error record — 2026-09-30

**Input:** read-only PAL GUPP8P `main.dol`, SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
**Build:** MSVC Release from the public repository. The command below uses
two **caller-supplied** values observed in a pinned startup-only Dolphin/HLE
run. They are scenario inputs, not values found in the DOL or retail IPL
defaults. The diagnostic executable does not run Dolphin.

```text
shadow_boot_probe <read-only-PAL-main.dol> \
  --observed-msr 00002032 --observed-hid2 E0000000
```

## Actual execution result

The process exited **0**, with no crash, failed assertion, invalid pointer,
unresolved indirect branch, or divergent modeled value. Its connected
checkpoints were `0x80003158`, `0x80003400`, `0x80371714`,
`0x80370BA8`, and the final stop **before** `0x80371724`.

| At `0x80371724` | Native output | Fresh same-run PPC/HLE capture | Scope |
|---|---:|---:|---|
| LR / r0 | `80371724` / `80003414` | same | byte-decoded `bl`, `mflr`, `blr` |
| r1 / r3 / r31 | `8060C5E8` / `E0000000` / `8000315C` | same | r3 uses the explicit HID2 input |
| MSR | `00002032` | same | native MSR field; other MSR side effects not compared |
| first stack write event | BE32 `80003414` at `8060C5F4` | those bytes at the accessor entry | ordered `stw` output record |
| second stack write event | BE32 `8060C5F0` at `8060C5E8` | those bytes at the accessor entry | ordered `stwu` output record |

The reference capture used synthetic disc SHA-256
`a8a526ef7006a2d2a8d41f536ef30b208ff197c185c05e9b2276e6e93cb41a8e`
and Dolphin executable SHA-256
`db536025ee02190c20ccb9bfa94ee36ce28bb996f2e88574e46734b353bdc526`.
Its ignored `build/hid2_leaf_capture_20260930.json` contains ordered
checkpoints at entry, register-helper return, wrapper entry, paired-setup
entry, HID2 accessor entry and accessor return. The separate fresh
`build/paired_entry_capture_20260930.json` confirms the two stack words at
the accessor entry. Neither capture recreates retail IPL state.

## Errors, first stop, and limits

- **Current execution/validation errors:** none in this bounded run. The
  boot-only suite passed **12/12 CTest**, and the complete public Release
  suite passed **41/41 CTest** after the same implementation.
- **Deliberate first unimplemented instruction:** DOL file offset `0x36AFE4`,
  VA `0x80371724`, bytes `64 63 A0 00` / word `0x6463A000`,
  `oris r3,r3,0xA000`. The following `bl` calls the HID2 writer at
  `0x80370BB0`. The native probe stops before that OR; it has not written
  HID2, issued HID0[ICFI], set GQRs, seeded FPRs, run cache setup, returned
  to CRT, or reached the game transition.
- **Missing proof for further equivalence:** incoming retail HID2/MSR and
  complete pre-entry memory; HID2 post-write readback and Gekko ordering;
  HID0 ICFI/cache consequence; PS1/FPSCR state; later consumers. The
  `StartupState` model does not carry CR/XER/CTR/FPRs, and its two stack
  effects are ordered write records, not yet applied to a native memory
  object and checked at a downstream read. No parity claim covers those
  fields or effects.
- **Previously diagnosed gate error, fixed:** the old raw-note checker could
  replace the required FP-enable OR at `0x80003404` with an unrelated valid
  DOL word while preserving counts. Exact required VA ranges and
  `boot_pal_raw_coverage_negative` now reject that substitution. See
  [adversarial audit](ADVERSARIAL_BOOT_AUDIT.md).

**Next run loop:** decode and validate `0x80371724` from the DOL, preserve
the supplied HID2 provenance, resolve the write accessor and its actual
readback/consumer effects, implement only those proven transitions, then
rerun from `0x80003154` and the complete regression suite.
