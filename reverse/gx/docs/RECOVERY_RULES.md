# PPC -> GX recovery rules (fail-closed)

## 1. Separate SDK calls from inline FIFO writes

Many GX state APIs are ordinary functions and can be recovered from `bl` targets when symbols/fingerprints are known. Vertex submission helpers are commonly inline and collapse into volatile stores to the write-gather pipe.

Use two recognizers:

- **Call recognizer**: function address -> SDK symbol/version confidence -> EABI argument recovery.
- **WGPIPE recognizer**: PPC effective-address/value analysis -> symbolic FIFO write events.

Do not require one path to imitate the other.

## 2. EABI argument state

For normal PowerPC EABI calls, model GPR/FPR argument registers and the stack. Keep `r2` (.sdata2) and `r13` (.sdata) semantics explicit because SDK/game code frequently loads constants/state through SDA addressing.

A function-symbol match alone is not enough: recover the call-site arguments and preserve unknown expressions.

## 3. WGPIPE address proof

A store is a GX FIFO candidate only when the effective address is proven to equal `0xCC008000`.

Recognize at minimum:

- `lis/addis` + signed `addi`
- `lis/addis` + `ori/oris`
- register copies (`mr`/`or`)
- compiler-hoisted WGPIPE base registers
- indexed stores when both address inputs are proven

Do not classify stores to a merely similar `0xCC00xxxx` address as FIFO writes; CP/PE/PI MMIO occupies nearby space.

## 4. Width is semantic

Preserve every store width:

- `stb` -> 8 bits
- `sth` -> 16 bits
- `stw` -> 32 bits
- `stfs` -> IEEE single 32-bit payload

A 32-bit store is not interchangeable with four byte stores for lifting purposes even if a test byte stream happens to match.

## 5. Build a symbolic FIFO stream

Represent each event as roughly:

```text
FifoWrite {
    pc,
    width_bits,
    value_expr,
    value_known_bits,
    provenance
}
```

Packetization must operate on this stream, not on guessed source calls.

If a command byte is unknown, stop semantic packetization at that point or fork a bounded state hypothesis. Never pick an opcode because it makes later bytes look convenient.

## 6. Maintain CP state as a known/unknown lattice

Track VCD low/high and all 8 VAT A/B/C sets, array bases/strides, and matrix indices. Each bitfield should be able to be known, partially known, or unknown.

A primitive can be decoded only when its selected VAT and the relevant VCD fields prove its per-vertex byte layout.

## 7. Lift vertex helpers only after state proof

Example: three consecutive `stfs` writes are **not** automatically `GX_Position3f32`.

They become a position only if:

- the decoder is inside a primitive,
- POS is the next expected attribute,
- VCD says POS is direct,
- selected VAT says XYZ + F32,
- exactly 12 bytes are consumed in the correct slot.

The same store sequence in a normal, tangent/binormal, or texcoord slot means something else.

## 8. Preserve display-list state effects

A display list is not an isolated blob. CP/BP/XF state changes inside it can affect what follows. Decode lists with an explicit incoming state and return an outgoing state. Memoization/precompilation keys must include all state that changes interpretation, especially VCD/VAT.

## 9. Version-aware SDK fingerprinting

Keep separate fingerprint databases for SDK revisions/build flags. The 2001 and 2004 SDK decomp repositories are useful anchors, but do not assume byte-identical GX implementations across every retail game.

Suggested evidence levels:

- A: exact reloc/symbol or byte-identical known library object
- B: normalized CFG + constants + call graph match
- C: semantic register-write equivalence
- D: heuristic resemblance only — never auto-rename

## 10. Output both semantic C and raw evidence

Every lifted line should retain a machine-readable provenance record: source PCs, original PPC words, FIFO events, incoming GX state, and the rule that justified the lift. That makes false positives auditable.
