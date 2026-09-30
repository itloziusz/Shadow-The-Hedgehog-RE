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
evidence**. They cannot establish retail IPL handoff, complete PS1/GQR/cache
state, or a playable game boot. See `../research/ASM_HARDWARE_VALIDATION.md`
for checked branch/state observations and limits.

The native C++ boot code does not call this tool or Dolphin. The JSON is a
validation artifact and is intentionally excluded from version control.

`verify_binary_note.py` rereads every instruction row in a binary-first
Markdown note from the SHA-pinned PAL DOL. It independently checks the DOL
header mapping, four raw bytes, big-endian word, any `bl` displacement and
any numbered SPR field, then prints counts. It writes no files:

```text
python reverse/boot/tools/verify_binary_note.py <main.dol> \
  reverse/boot/research/BINARY_BOOT_PREFIX.md \
  --expected-words 78 --expected-branches 8 --expected-sprs 17
```

The same gate covers the bounded FPR, cache/handler and CRT notes through
`boot_pal_raw_fpr_prefix`, `boot_pal_raw_cache_handler` and
`boot_pal_raw_crt_prefix`. Every direct I/B branch row in those notes must
have a verified target; the CRT case also checks 41 descriptor words with
`--expected-data-words 41`. This is a byte/ISA gate, not an execution-parity
gate.
