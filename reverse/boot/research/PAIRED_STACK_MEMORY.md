# Paired-setup stack bytes before the ICFI/`sync` wall

Original input: PAL GUPP8P `main.dol`, SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
This note covers only the two reached stack stores at `0x80371718/1C` and
their eventual read at `0x80371758`. **PROVEN-BINARY** refers to DOL bytes
and PPC instruction effects; **OBSERVED-HLE** refers to the pinned synthetic
startup capture, not retail IPL or physical cache behavior.

| VA / DOL file | Word | PROVEN-BINARY effect |
|---|---|---|
| `80371714` / `36AFD4` | `7C0802A6` | `mflr r0`: r0 receives outer return LR `0x80003414` on the reached path. |
| `80371718` / `36AFD8` | `90010004` | `stw r0,4(r1)`: BE32 write to old SP+4. |
| `8037171C` / `36AFDC` | `9421FFF8` | `stwu r1,-8(r1)`: BE32 backchain write to old SP−8, then r1 becomes old SP−8. |
| `80371758` / `36B018` | `8001000C` | `lwz r0,12(r1)`: read the *current* BE32 word at old SP+4; this has not executed in connected native boot. |

The register helper sets old SP=`0x8060C5F0`. Hence the first store writes
`80 00 34 14` at `0x8060C5F4..F7`, and the update-form store writes
`80 60 C5 F0` at `0x8060C5E8..EB`. The intervening eight bytes are not
initialized by this prologue. The archived same-run HLE snapshot at
`0x80371724` in ignored `build/boot-paired-setup-capture.json` shows both
four-byte sequences in its stack window; its path also reaches `0x80371764`
and `0x80003414`. This checks the selected HLE stack effect, not retail
pre-entry RAM or asynchronous writes.

The native `PairedSetupStackMemory` now applies both stores in order to a
16-byte guest-addressed window. Every byte carries a validity bit, so a
load from an unwritten slot declines instead of assuming zero. Its BE32
load reads the current bytes; a later writer can change the saved return
word. The focused regression checks both byte sequences, aligned and
unwritten declines, a changed saved word, and preservation of the memory
through the HID2 and HID0 request boundaries. This addresses the previous
write-event-only gap in the connected prefix.

**UNKNOWN / stop:** This bounded window is not a full memory model.
`0x80371730` `sync`, ICFI completion, post-command HID0 readback and eight
GQR writes remain unresolved as connected state. The native probe still
stops before `sync`; it does not yet consume the saved LR at `0x80371758`.
When that barrier is proven, the tail must load the saved word from the
applied bytes rather than copy the earlier LR or assume an unmodified stack.
