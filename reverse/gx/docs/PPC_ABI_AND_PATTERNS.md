# PPC EABI / Gekko patterns that matter for GX recovery

## Calling convention baseline

For ordinary PowerPC EABI calls, start with this model:

- `r3..r10`: first eight integer/pointer argument words.
- `f1..f8`: first eight floating-point arguments.
- `r3` (and sometimes `r4` for multiword values): return value.
- `r1`: stack pointer; EABI stack alignment is 8 bytes.
- `r2`: `.sdata2` / small read-only data anchor under EABI small-data conventions.
- `r13`: `.sdata` / `.sbss` small-data anchor.
- LR/CTR are control-flow state and must be represented explicitly in the front end.

Do not apply a generic desktop PowerPC SysV ABI blindly. GameCube code is commonly built with Metrowerks and uses EABI/SDA conventions heavily.

## Why this matters to GX calls

A call such as:

```c
GXSetVtxAttrFmt(vtxfmt, attr, cnt, type, frac);
```

normally exposes its scalar arguments in `r3..r7` at the call boundary. Therefore a symbol/fingerprint hit is only half of the lift: the decompiler must recover argument expressions reaching those registers.

For matrix/array APIs, pointers often arrive in a GPR while matrix elements were prepared through FPR/paired-single code. Keep pointer provenance separate from the data it points to.

## SDA/SDA2

Treat `offset(r13)` and `offset(r2)` as semantic global references whenever their anchors are known from boot/runtime state or map/relocation evidence. Do not flatten them into anonymous absolute constants.

This is especially important for:

- SDK GX context/shadow-state pointers,
- static matrices and projection constants,
- texture object templates,
- state tables and callback pointers.

## Gekko paired singles

The Gekko adds paired-single instructions and quantized load/store operations (`ps_*`, `psq_l`, `psq_st`, indexed/update variants). `psq_l/psq_st` interpretation depends on GQR state and the W/I fields.

Production front end requirements:

1. decode Gekko opcodes correctly (do not trust a disassembler mode that mistakes them for unrelated newer PowerPC instructions),
2. track all GQR writes and quantization modes,
3. propagate two-lane values independently,
4. preserve lane swaps/merges and rounding/quantization,
5. never coerce a paired-single operation to scalar `float` merely to make output prettier.

## Direct WGPIPE write patterns

The write-gather pipe is at `0xCC008000`. Common base construction forms include:

```asm
lis   r12, 0xCC01
stb   r3, -0x8000(r12)   ; 0xCC008000
sth   r4, -0x8000(r12)
stw   r5, -0x8000(r12)
stfs  f1, -0x8000(r12)
```

Also recognize:

- `lis` + `addi`, `ori`, `oris`,
- a WGPIPE base hoisted into a nonvolatile GPR across a loop,
- base copied by `mr`/`or`,
- indexed stores where both base/index are proven,
- wrapper helpers that contain only FIFO stores.

A store is a FIFO event only if effective-address analysis proves exactly `0xCC008000`.

## Inline GX vertex submission

Many vertex helpers are inline macros/functions, so there may be no call target to name. The PPC can reduce directly to stores such as:

```text
stfs -> WGPIPE
stfs -> WGPIPE
stfs -> WGPIPE
```

That is not enough to call it a position. The current VCD/VAT state and primitive attribute cursor must prove that those 12 bytes occupy a direct XYZ/F32 position slot.

## Suggested SSA facts per instruction

Carry at least:

```text
GPR value:   known constant | symbolic expression | unknown
FPR value:   scalar/pair symbolic expression + raw bit provenance
address:     base object + offset where possible
memory load: endian + width + source range/relocation
store:       effective address + width + value + source PC
branch:      target set + LR/CTR effects
```

For graphics recovery, exact bit provenance is more important than aggressive algebraic simplification.

## Function call summaries

For a recognized GX function store:

```text
symbol
SDK revision / fingerprint evidence
input registers used
memory reads/writes
CP/BP/XF state effects
FIFO events emitted
GX context dirty flags changed
```

This permits interprocedural reconstruction without pretending that every GX source call maps one-to-one to immediate FIFO bytes.
