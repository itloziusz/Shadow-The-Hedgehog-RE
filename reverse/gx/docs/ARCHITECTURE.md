# Proposed production architecture: GXRecover

## Pipeline

```text
DOL/REL/RAM image
  -> Gekko decoder + CFG
  -> SSA/value provenance
  -> SDK call recognizer ----\
                              +-> GX event IR
  -> WGPIPE/MMIO recognizer -/
  -> FIFO packetizer + CP/BP/XF shadow state
  -> display-list resolver
  -> vertex stream decoder (VCD/VAT/array base/stride)
  -> semantic lifter
  -> C/C++ emitter + evidence database
```

## Front end

Do not make the graphics pass responsible for discovering all functions. Feed it a mature GameCube code-analysis layer (DTK/Ghidra/DolRecomp-like CFG information) or implement equivalent function/relocation discovery.

Required Gekko semantics include paired singles, quantized loads/stores, GQRs, big-endian memory, LR/CTR branches, SDA/SDA2, and Metrowerks calling patterns.

## GX Event IR

Use a lossless IR instead of directly printing C:

```text
CallGX(symbol, args[], pc_range, confidence)
FifoWrite(width, expr, pc)
CPWrite(reg, value_expr)
BPWrite(reg, value24_expr)
XFWrite(base, words[])
IndexedXF(kind, index, addr, size)
CallDL(address_expr, size_expr)
BeginPrimitive(primitive, vat, vertex_count)
VertexAttribute(slot, mode, format, raw_exprs[])
UnknownGX(bytes/events, reason)
```

The IR must accept symbolic values. Static recovery from PPC will often know packet structure before it knows every runtime float.

## State model

Maintain separate domains:

- CP: VCD, 8 VATs, array bases/strides, matrix indices.
- XF: matrices, viewport, projection, lighting, texgen/channel state.
- BP: genmode, scissor, depth, blend, TEV, indirect texturing, texture state, copy/EFB state.

Use persistent/immutable snapshots or versioned state IDs. At CFG joins, merge each field with a three-state lattice (`same known`, `different/unknown`, `uninitialized`).

## Function summaries

For each recovered function, compute a GX summary:

- state reads/writes
- FIFO writes
- nested GX calls
- whether it opens/closes a primitive
- whether it is state-transparent
- preconditions on incoming VCD/VAT

This allows interprocedural reconstruction when a helper function submits only positions or colors.

## Display lists

Resolve a display list when address+size are known and bytes are available in the image. Treat the list like an inline GX command subroutine with state effects. Cache decode results by `(address, size, interpretation_state_key)`, not only address.

## Lifting policy

Prefer exact low-level C over wrong high-level C.

Good fallback:

```c
GX_RAW_BP(0x41, value);
GX_FIFO_U32(expr);
```

Bad fallback:

```c
GX_SetBlendMode(...); // guessed from an incomplete BP value
```

High-level setters should be emitted only if the complete observed state transition is equivalent to that API call and no hidden SDK shadow-state behavior is lost.

## Validation

1. Unit test packet decoding from synthetic vectors.
2. Differential test CP/BP/XF state against a trusted implementation using generated command streams.
3. Compile recovered C and capture its FIFO stream.
4. Byte-compare original vs regenerated FIFO for deterministic paths.
5. For symbolic paths, compare packet/state traces over recorded runtime inputs.
6. Add negative tests specifically designed to defeat superficial recognizers.

A useful success criterion is not “the C looks plausible”; it is “the regenerated GX event trace is equivalent under the same inputs.”
