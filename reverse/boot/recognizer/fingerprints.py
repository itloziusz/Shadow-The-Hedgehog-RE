"""Multi-layer structural fingerprints and falsifiable boot detectors."""

from __future__ import annotations

import hashlib
from collections import Counter


def _ngrams(items: list[str], size: int) -> set[str]:
    return {"|".join(items[i:i + size]) for i in range(max(0, len(items) - size + 1))}


def fingerprint(region: dict) -> dict:
    rows = region["instructions"]
    ops = [r["mnemonic"] for r in rows]
    norm = [r["normalized"] for r in rows]
    coarse = [r["coarse_normalized"] for r in rows]
    effects = region["effects"]
    kinds = Counter(e["kind"] for e in effects)
    bases = Counter()
    globals_seen = set()
    for e in effects:
        if e["kind"] in ("load", "store"):
            addr = e["address"]
            bases["SDA2" if "SDA2" in addr else "SDA" if "SDA" in addr else
                  "SP" if "SP" in addr else "CONCRETE" if addr.startswith("K:") else
                  "SYMBOLIC"] += 1
            if addr.startswith("K:"):
                globals_seen.add("0x" + addr[2:])
    shape = [len(b["successors"]) for b in region["cfg"]]
    return {
        "raw_sha256": hashlib.sha256(bytes.fromhex("".join(r["raw"] for r in rows))).hexdigest(),
        "normalized_sha256": hashlib.sha256("\n".join(norm).encode()).hexdigest(),
        "normalized": norm, "mnemonics": ops,
        "norm_ngrams": sorted(_ngrams(norm, 2) | _ngrams(norm, 3)),
        "coarse_ngrams": sorted(_ngrams(coarse, 2) | _ngrams(coarse, 3)),
        "opcode_ngrams": sorted(_ngrams(ops, 2) | _ngrams(ops, 3)),
        "cfg_shape": shape, "effect_kinds": dict(kinds),
        "memory_bases": dict(bases), "concrete_accesses": sorted(globals_seen),
        "direct_calls": sorted({e["target"] for e in effects
                                if e["kind"] == "call" and not e["indirect"]}),
        "indirect_calls": sum(e["kind"] == "call" and e["indirect"] for e in effects),
        "unknown_words": len(region["unknown_instructions"]),
        "unsupported_semantics": len(region["unsupported_semantics"]),
        "external_exits": region["external_exits"],
        "unresolved_edges": region["unresolved_edges"],
    }


def _jaccard(a, b) -> float:
    a, b = set(a), set(b)
    return len(a & b) / len(a | b) if a or b else 0.0


def similarity(a: dict, b: dict) -> dict:
    """Explicit measured structural overlap; never a semantic probability."""
    parts = {
        "precise_ngram": _jaccard(a["norm_ngrams"], b["norm_ngrams"]),
        "coarse_ngram": _jaccard(a["coarse_ngrams"], b["coarse_ngrams"]),
        "opcode_ngram": _jaccard(a["opcode_ngrams"], b["opcode_ngrams"]),
        "memory_effect": _jaccard(
            [f"{k}:{v}" for k, v in a["memory_bases"].items()] +
            [f"{k}:{v}" for k, v in a["effect_kinds"].items()],
            [f"{k}:{v}" for k, v in b["memory_bases"].items()] +
            [f"{k}:{v}" for k, v in b["effect_kinds"].items()]),
        "cfg_shape": _jaccard(enumerate(a["cfg_shape"]), enumerate(b["cfg_shape"])),
    }
    weights = {"precise_ngram": 0.25, "coarse_ngram": 0.25,
               "opcode_ngram": 0.20, "memory_effect": 0.20, "cfg_shape": 0.10}
    return {"score": round(sum(parts[k] * weights[k] for k in weights), 4),
            "components": {k: round(v, 4) for k, v in parts.items()},
            "meaning": "weighted structural overlap, not confidence or runtime parity"}


def detectors(region: dict) -> list[dict]:
    rows, effects = region["instructions"], region["effects"]
    ms = [r["mnemonic"] for r in rows]
    found = []
    writes_by_pc = {e["pc"]: e for e in effects if e["kind"] == "spr_write"}
    gqr_writes = [writes_by_pc.get(r["pc"]) for r in rows[2:10]]
    if (len(rows) >= 10 and ms[:2] == ["sync", "li"] and
            ms[2:10] == ["mtspr"] * 8 and all(gqr_writes) and
            [e["spr"] for e in gqr_writes] == list(range(912, 920)) and
            all(e["value"] == "K:00000000" for e in gqr_writes) and
            not region["unknown_instructions"] and
            not region["unsupported_semantics"] and
            not region["unresolved_edges"]):
        found.append({"id": "sync_gqr_zero_chain", "status": "STRONGLY_SUPPORTED",
                      "hypothesis": "ordered GQR0–7 zero writes after a synchronization barrier",
                      "evidence": ["first two words decode sync; li", "eight distinct SPR912–919 writes use propagated zero"],
                      "contradictions": ["barrier/cache effect remains unresolved"],
                      "validation_needed": ["acknowledge ICFI/sync consequence", "measure GQR readback or downstream paired use",
                                            "compare next native state from DOL entry"]})
    if any(e["kind"] == "call" and e["indirect"] for e in effects):
        targets = [e["target"] for e in effects if e["kind"] == "call" and e["indirect"]]
        found.append({"id": "indirect_dispatch", "status": "STRUCTURAL_MATCH",
                      "hypothesis": "function-pointer or virtual dispatch",
                      "evidence": [f"indirect call target expression {target}" for target in targets],
                      "contradictions": ["target set is not proven from a live memory snapshot"],
                      "validation_needed": ["read producer memory before call", "capture CTR/LR and actual destination"]})
    if (sum(e["kind"] == "store" for e in effects) >= 3 and
            not any(e["kind"] == "call" for e in effects) and
            ms[-1:] == ["blr"] and not region["external_exits"] and
            not region["unresolved_edges"] and
            not region["unknown_instructions"] and not region["unsupported_semantics"]):
        found.append({"id": "store_initializer", "status": "STRUCTURAL_MATCH",
                      "hypothesis": "static data or object-field initializer",
                      "evidence": ["at least three stores", "no decoded calls", "return terminator"],
                      "contradictions": ["range/function boundary supplied heuristically",
                                         "input memory and prior writes not observed"],
                      "validation_needed": ["compare ordered memory delta at entry/return",
                                            "resolve caller/owner before naming constructor"]})
    concrete_stores = sorted({e["address"] for e in effects
                              if e["kind"] == "store" and e["address"].startswith("K:")})
    if ("stmw" in ms and "lmw" in ms and ms[-1:] == ["blr"] and
            len(concrete_stores) >= 16 and not any(e["kind"] == "call" for e in effects) and
            not region["unknown_instructions"] and not region["unsupported_semantics"] and
            not region["external_exits"] and not region["unresolved_edges"]):
        found.append({"id": "bulk_table_propagation", "status": "STRUCTURAL_MATCH",
                      "hypothesis": "multiword register save/restore around global-table field propagation",
                      "evidence": [f"{len(concrete_stores)} distinct concrete store addresses",
                                   "decoded stmw and lmw effects; return terminator; no decoded call"],
                      "contradictions": ["live source values and table target are unobserved",
                                         "matching layout does not prove object type or runtime reachability"],
                      "validation_needed": ["capture live constructor-table pointer",
                                            "compare every ordered read/write at entry and return",
                                            "check destination aliases and later consumers"]})
    if (any(e["kind"] == "branch" and e["target"].startswith("0x") and
            int(e["target"], 16) < int(e["pc"], 16) for e in effects) and
            any(e["kind"] == "load" for e in effects)):
        found.append({"id": "load_poll_loop", "status": "STRUCTURAL_MATCH",
                      "hypothesis": "repeated state poll or data-dependent loop",
                      "evidence": ["back edge and memory read in bounded range"],
                      "contradictions": ["loop condition may have another producer"],
                      "validation_needed": ["slice CR/CTR producer", "observe repeated read values and exit"]})
    if ("lwz" in ms and any(m.startswith("cmp") for m in ms) and
            any(m.startswith("b") and m not in ("b", "blr") for m in ms) and
            any("SDA" in e.get("address", "") for e in effects)):
        found.append({"id": "sda_guard", "status": "STRUCTURAL_MATCH",
                      "hypothesis": "guarded SDA state access",
                      "evidence": ["SDA load", "comparison", "conditional branch"],
                      "contradictions": ["guard purpose and live value unknown"],
                      "validation_needed": ["trace guard writer", "capture branch input and selected edge"]})
    return found


def minimal_experiments(region: dict) -> list[dict]:
    requests = []
    for e in region["effects"]:
        if e["kind"] == "call" and e["indirect"]:
            requests.append({"break_before": e["pc"], "read": ["CTR", "LR", e["target"]],
                             "step": "one indirect transfer", "assert": "actual target in proven set"})
        elif e["kind"] == "barrier" and e["status"] == "unresolved_hardware":
            requests.append({"break_before": e["pc"],
                             "read": ["HID0", "HID2", "MSR", "stack writes", "instruction bytes"],
                             "step": "barrier and next instruction", "read_after": ["HID0", "PC", "stack bytes"],
                             "assert": "ordered state; cache completion requires separate evidence"})
    return requests
