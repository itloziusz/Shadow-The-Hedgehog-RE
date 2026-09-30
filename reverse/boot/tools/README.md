# Boot checkpoint capture

`capture_dolphin_rsp.py` starts a caller-supplied Dolphin executable in
interpreter/debug mode, stops at DOL entry `0x80003154`, then takes ordered
PPC GDB/RSP breakpoint snapshots. Supply the executable, disc, a fresh user
directory and JSON output path. All paths are arguments; keep capture output
under the ignored repository `build/` directory. The script checks the full
disc SHA-256 before launch and records both disc and executable digests.

```text
python reverse/boot/tools/capture_dolphin_rsp.py <Dolphin.exe> <disc.iso> \
  <repo-build/user-dir> <repo-build/capture.json> \
  --expect-disc-sha256 <full-disc-sha256> --port <free-port> \
  --checkpoints 80003158 80003400 80371714
```

The documented startup-only synthetic disc has SHA-256
`a8a526ef7006a2d2a8d41f536ef30b208ff197c185c05e9b2276e6e93cb41a8e`.
Its embedded PAL `sys/` regions were checked byte-for-byte, but it has no
ordinary game-file payload. Captures from it are **synthetic HLE startup
evidence**. They cannot establish retail IPL handoff, physical PS1/cache
consequences, or a playable game boot. See `../research/ASM_HARDWARE_VALIDATION.md`
for checked branch/state observations and limits.

The native C++ boot code does not call this tool or Dolphin. The JSON is a
validation artifact and is intentionally excluded from version control.

## Hidden-state capture (checkpoint 36)

`make_boot_oracle_disc.py <read-only-sys-dir> <build/startup.iso>` checks the
five original SYS hashes and reproduces the exact archived startup-only disc.
The output is sparse on Windows, contains no ordinary game-file payload and
must stay in this repository's ignored `build/`.

`build_readonly_boot_oracle.py <oracle-source> <existing-oracle-build>
<build/private-oracle> --msvc <matching-MSVC-dir> --sdk-root <Windows-Kits-dir>
--sdk-version <matching-version>` reuses an existing MSVC build's read-only
objects/archives and compiles one copied GDB translation unit. It adds only
reads for PS1, GQRs, HID2, cache metadata and composed live XER. It records
source/executable/link-input hashes and copies runtime dependencies, including
`qt.conf`. No native target links the oracle. No download/install is performed.

```text
python -B reverse/boot/tools/capture_boot_machine_state.py \
  <build/private-oracle/Dolphin.exe> <build/startup.iso> \
  <build/fresh-user-dir> <build/capture.json> --port <free-port>

python -B reverse/boot/tools/validate_fpr_capture.py \
  <read-only-main.dol> <build/reverse/boot/Release/shadow_boot_fpr_projection.exe> \
  <build/capture.json> <build/validation-work>
```

Capture requires a fresh user directory and pins the startup disc and
instrumented executable. It single-steps the HID0 command, sync, paired load,
scalar load and first scalar move, alongside ordered breakpoints. It records
both lanes of all 32 FPRs at every checkpoint. `--finite-sources` or
`--source-bits <32-hex-digits>` performs an explicitly labelled live-BSS data
experiment at `0x80370CDC`; it does not patch DOL/instruction bytes or SPRs.
`--stock` captures only original exposed fields for an unchanged-executable
cross-check. Use a byte-identical copy of that executable in the private build
directory with its dependencies, preserving the original directory read-only.

Validation serializes observed input state and compares eight native
checkpoints, 816 raw fields per run. It is an **unconnected bounded projection**;
neither this comparison nor a HLE sync no-op advances the native boot frontier.
See `../research/HIDDEN_BOOT_STATE_36.md` for findings, exact hashes and limits.

The RSP client now accepts valid E-prefixed data such as `E0030000`, rejects
short memory replies, terminates on EOF and assembles fragmented checksum
bytes. `tests/test_rsp.py` is a host-only regression test for these bugs.

`verify_binary_note.py` rereads every instruction row in a binary-first
Markdown note from the SHA-pinned PAL DOL. It independently checks the DOL
header mapping, four raw bytes, big-endian word, any `bl` displacement and
any numbered SPR field, then prints counts. It writes no files:

```text
python reverse/boot/tools/verify_binary_note.py <main.dol> \
  reverse/boot/research/BINARY_BOOT_PREFIX.md \
  --expected-words 78 --expected-branches 8 --expected-sprs 17 \
  --required-code-range 80003154:80003158 \
  --required-code-range 800032B0:8000333C \
  --required-code-range 80003400:80003420 \
  --required-code-range 80370BA8:80370BBC \
  --required-code-range 80371714:80371764 \
  --required-code-range 803725F4:80372600
```

The same gate covers the bounded FPR, paired setup, cache/handler and CRT
notes through their `boot_pal_raw_*` cases. Every direct I/B branch row must
have a verified target; the CRT case also checks 41 descriptor words with
`--expected-data-words 41`. This is a byte/ISA gate, not an execution-parity
gate. `boot_pal_raw_coverage_negative` proves that replacing a required word
with an unrelated valid DOL word fails despite identical row totals.
