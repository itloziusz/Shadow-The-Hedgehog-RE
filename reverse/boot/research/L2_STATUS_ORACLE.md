# PAL boot L2 status polls: fresh synthetic oracle

> Historical selected-path observation. Checkpoint38's separately proven
> **closed native owner** now connects this slice; see
> `NATIVE_L2_COMPLETION_38.md` and its independent audits. This older capture
> still establishes no physical completion/timing or retail input.

Scope: the two `L2CR[L2IP]` polls in PAL GUPP8P `main.dol`,
`0x80372678..0x80372684` and `0x803726B4..0x803726C0`, plus their
immediate producers and consumers. This is an independent selected-path
checkpoint, **not connected native execution** or retail IPL proof. Original
DOL SHA-256: `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.

## Binary and control dependencies

`BINARY_CACHE_HANDLER.md` contains every raw word in `0x80372640..D4`.
The full PAL byte gate was rerun against the read-only DOL: **305 words,
56 direct branches, 14 SPR operands passed**. Text section 1 maps VA
`0x80008D40` to DOL file `0x2600`; relevant raw anchors are:

| VA / file offset | BE word | Exact operation |
|---|---|---|
| `8037266C` / `36BF2C` | `4BFFE499` | `bl 80370B04`, write request `r3` to SPR 1017 (`L2CR`). |
| `80372678` / `36BF38` | `4BFFE485` | `bl 80370AFC`, fresh `mfspr r3,1017` read. |
| `8037267C` / `36BF3C` | `546007FE` | `r0 = r3 & 1`. |
| `80372680` / `36BF40` | `28000000` | `cmplwi r0,0`, setting CR0. |
| `80372684` / `36BF44` | `4082FFF4` | If CR0 is unequal, return to `80372678`; otherwise advance to `80372688`. |
| `80372688` / `36BF48` | `4BFFE475` | Fresh `L2CR` read; the loop's earlier `r3` is not reused. |
| `8037268C` / `36BF4C` | `546302D2` | `r3 &= 0xFFDFFFFF`, clearing numeric L2I `0x00200000`. |
| `80372690` / `36BF50` | `4BFFE475` | Write resulting `r3` to `L2CR`. |
| `803726B4` / `36BF74` | `4BFFE449` | Fresh `L2CR` read, after the software clear. |
| `803726B8` / `36BF78` | `546007FE` | `r0 = r3 & 1`. |
| `803726BC` / `36BF7C` | `28000000` | `cmplwi r0,0`, setting CR0. |
| `803726C0` / `36BF80` | `4082FFE8` | If unequal, branch to `803726A8` (logger, then new read); if equal, restore helper frame at `803726C4`. |

The two `bne` displacements are signed `-0xC` and `-0x18` from the branch
instruction addresses, respectively. The first poll's source is the read
at `0x80372678`, whose prior write request is `0x8037266C`. The second
poll's source is the read at `0x803726B4`, whose prior guest write is
`0x80372690`. In either case the `mfspr` accessor returns a **live** SPR
readback; the PPC code does not determine it solely from the last write.
The first loop has no software timeout. The second loop's only additional
work on a nonzero read is the logger call before retry.

## Same-run synthetic HLE measurements

I ran `reverse/boot/tools/capture_dolphin_rsp.py` with the documented
startup-only synthetic disc SHA-256
`a8a526ef7006a2d2a8d41f536ef30b208ff197c185c05e9b2276e6e93cb41a8e`
and unique GDB port `21717`. Its fresh-user output is the ignored local
`build/l2_status_capture_20260930.json`; the capture tool checked the full
disc digest and recorded the Dolphin executable digest. Ordered breakpoints
ran from `0x80003154` through all rows below. At each breakpoint the
instruction at PC has **not yet executed**.

| PC | MSR | L2CR readback | r3 | r0 | CR | Observation |
|---|---:|---:|---:|---:|---:|---|
| `80372894` | `2032` | `00000000` | `0011C464` | `00004000` | `40000000` | Before initial L2 enable test. |
| `803728A0` | `2032` | `00000000` | `00000000` | `00000000` | `20000000` | Initial L2E branch falls through. |
| `8037266C` | `0030` | `00000000` | `00200000` | `803728D4` | `20000000` | Before guest L2I request write. |
| `80372678` | `0030` | `00200000` | `00200000` | `803728D4` | `20000000` | After request, before first fresh read. |
| `80372684` | `0030` | `00200000` | `00200000` | `00000000` | `20000000` | First read yields L2IP=0; next checkpoint `80372688` confirms fallthrough. |
| `80372690` | `0030` | `00200000` | `00000000` | `00000000` | `20000000` | Fresh read was masked; before explicit guest clear write. |
| `803726B4` | `0030` | `00000000` | `80560000` | `00000000` | `20000000` | After clear write, before second fresh read. |
| `803726C0` | `0030` | `00000000` | `00000000` | `00000000` | `20000000` | Second read yields L2IP=0; next checkpoint `803726C4` confirms fallthrough. |
| `803728DC` | `2032` | `00000000` | `00002032` | `803728D4` | `20000000` | Caller restored MSR; before its fresh L2CR read. |
| `803728EC` | `2032` | `80000000` | `80000000` | `80000000` | `20000000` | Caller wrote L2E on this selected path. |

This capture demonstrates **zero visits to either back-edge on this one
synthetic run**. It does not show that L2IP is always zero. In particular,
the first `L2CR=0x00200000` is a request-visible readback with bit 0 clear;
it gives no cycle-accurate completion or cache-tag proof. The guest writes
zero at `0x80372690` before the second zero readback, but a real hardware
status transition between reads is still an open possibility.

## Native boundary and falsifiers

No native continuation may collapse either poll into a constant result or
substitute the last software write for a fresh hardware read. A platform
hook would need to expose a sequence of readbacks and ordered write effects,
including the possibility of nonzero L2IP and the second loop's logger.
The read at `0x803728DC` is the first downstream consumer of helper output;
it cannot be projected from the helper's last `r3` without a readback rule.

**Remaining retail proof:** obtain the pre-entry retail IPL `L2CR` and MSR,
capture each read at both loop heads through status transitions, compare
the second poll after guest L2I clear, and establish cache-tag/ordering
consequences that later consumers observe. The synthetic startup-only disc
and GDB register snapshots cannot supply those facts. Until then, this
region remains binary/selected-HLE validated, with portable native
semantics **UNKNOWN**.
