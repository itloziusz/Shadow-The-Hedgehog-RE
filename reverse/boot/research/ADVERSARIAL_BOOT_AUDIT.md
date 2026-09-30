# Adversarial audit of the PAL binary-first boot prefix

Scope: commit `31a3337`, the SHA-pinned PAL GUPP8P `main.dol`, and the raw
boot/FPR/cache/CRT notes. I checked the C++ transition boundaries against
the entry/helper/wrapper words and separately tested whether the published
raw-note verifier fails closed when a required instruction is omitted. This
is an audit of the current proof, not a claim of native hardware parity.

## Concrete gate gap: row count does not prove region coverage

**PROVEN:** DOL VA `0x80003404`, file offset `0x000404`, contains
`60 00 20 00` / `0x60002000`, `ori r0,r0,0x2000`. This sets the FP bit
candidate between `mfmsr r0` at `0x80003400` and `mtmsr r0` at
`0x80003408`; omitting it would change the machine-state transition for an
incoming MSR without that bit. DOL VA `0x80003424`, offset `0x000424`,
contains the unrelated valid word `3C A0 FF FF` / `0x3CA0FFFF`.

At audited commit `31a3337`, `verify_binary_note.py` checked each *listed*
row's VA-to-file mapping, bytes, word, and any annotated branch target/SPR
number, then compared only row, branch, and SPR **counts**. It did not pin the
set of required VAs. I
replaced the `0x80003404` row in an in-memory copy of
`BINARY_BOOT_PREFIX.md` with a properly formatted `0x80003424` row and an
arbitrary annotation. The verifier returned the same tuple for both texts:

```text
original: (78, 8, 17, 0)
missing 80003404, added 80003424: (78, 8, 17, 0)
```

Thus the then-current `boot_pal_raw_prefix` CTest gate would pass a note missing a
required semantic instruction, even though the DOL SHA and every remaining
row are correct. The same structural weakness applies to the FPR, cache,
and CRT raw gates. This does **not** imply the currently committed notes are
missing that word; the current `BINARY_BOOT_PREFIX.md` contains it.

The required fix was to pin an independent expected VA set or explicit
inclusive spans plus sparse leaves for each note, rejecting both missing and
extra addresses. A negative test had to make the substitution fail. These
changes now establish the claimed coverage in addition to byte
identity. Opcode/operand semantics still require their independent ISA
checks; matching an address set alone would not prove them.

## Checks that did not expose a separate contradiction

* Re-read `0x80003154` (`0x4800015D`), `0x80003158`
  (`0x480002A9`), and `0x80003400..0x80003410` from the DOL. The
  direct targets, LR transitions, and commit `31a3337`'s bounded C++ stop
  at `0x80371714` match those words, conditional on caller-supplied MSR.
* The register helper's 36 pinned words account for r0, r3..r12,
  r14..r31 zeroing and explicit r1/r2/r13 setup. Its `blr` uses the LR
  from the entry `bl`; no memory effect is claimed by the native helper.
* The raw FPR and cache/CRT tables state hardware and HLE limits rather
  than asserting native parity. HID0 `0x0800` is treated as an ICFI command
  word, with subsequent readback kept distinct. No additional native
  section is marked proven by the raw tables.

## Fix status at audit time

The coordinator added explicit inclusive expected code/data VA ranges to the
raw-note gate and a negative test that performs the substitution above. I
independently ran that negative test against the PAL DOL: it printed
`PASS omitted FP-enable OR rejected despite same raw/count gate totals`.
I also rechecked all four published tables against their exact expected VA
sets; boot, FPR, cache, and CRT returned respectively `(78,8,17,0)`,
`(74,1,1,0)`, `(305,56,14,0)`, and `(111,25,0,41)`. These checks establish
the specific coverage fix. The coordinator subsequently reported a full
Release build and **39/39 CTest pass** after the fix. **Status: fixed and
regression gated.**

No evidence in this audit warrants treating synthetic/HLE observations as
retail IPL state. The coordinator's separate native run reached
`0x80370BA8` with stack effects compared to a fresh HLE capture; that
checkpoint is independent of the raw-note coverage fix and does not imply
completion of the hardware path.
