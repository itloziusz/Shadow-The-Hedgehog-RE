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


def raw_motifs(rows: list[dict]) -> list[dict]:
    found = []
    # Raw local motifs retain registers/masks/dependencies; they do not turn
    # hardware completion or a function family into validated behavior.
    raw=[int(row["raw"],16) for row in rows]
    for index in range(max(0,len(raw)-4)):
        a,b,c,d,e=raw[index:index+5]
        original=(a>>21)&31;temporary=(b>>16)&31
        if (a&~0x03E00000==0x7C0000A6 and
            b>>26==21 and (b>>21)&31==original and (b>>11)&31==0 and (b>>6)&31==17 and (b>>1)&31==15 and not b&1 and
            c&~0x03E00000==0x7C000124 and (c>>21)&31==temporary and
            d>>26==21 and (d>>21)&31==original and (d>>16)&31==original and (d>>11)&31==17 and (d>>6)&31==31 and (d>>1)&31==31 and not d&1 and
            e==0x4E800020 and original!=temporary):
            found.append({"id":"interrupt_mask_exchange","status":"STRUCTURAL_MATCH",
                "hypothesis":"clear EE and return its prior one-bit value",
                "evidence":[f"raw five-word dependency chain at {rows[index]['pc']}","exact MSR clear mask and rotate-to-bit0; distinct registers"],
                "contradictions":["privilege and pending interrupt state are not provided"],
                "validation_needed":["compare live MSR/return word for both EE inputs","prove closed native delivery scope"]})
    for index in range(max(0,len(raw)-9)):
        group=raw[index:index+10];stores=group[:1]+group[2:9]
        source=(stores[0]>>21)&31;base=(stores[0]>>16)&31
        decrement=group[1];counter=(decrement>>21)&31;branch=group[9]
        if (all(w>>26==(37 if n==7 else 36) and (w>>21)&31==source and (w>>16)&31==base and (w&0xFFFF)==4*(n+1)
                for n,w in enumerate(stores)) and base!=0 and source!=base and
            decrement>>26==13 and (decrement>>16)&31==counter and decrement&0xFFFF==0xFFFF and counter not in (source,base) and
            branch==0x4082FFDC):
            found.append({"id":"eight_word_fill_group","status":"STRUCTURAL_MATCH",
                "hypothesis":"ascending eight-word fill with decremented CR0 loop counter",
                "evidence":[f"raw ten-word motif at {rows[index]['pc']}","eight same-value stores at offsets4..32; final stwu; exact CR0 backward edge"],
                "contradictions":["fill value, allocation bounds, zero/tail paths and carry are not supplied"],
                "validation_needed":["slice incoming value/count/address","compare full written range and boundary canaries","validate carry and next consumer"]})
    for index in range(max(0,len(raw)-5)):
        match=stable_timebase_sampler(raw[index:index+6],int(rows[index]['pc'],16))
        if match:
            found.append({"id":"stable_timebase_sampler","status":"STRUCTURAL_MATCH",
                "hypothesis":"retry high/low/high time-base sampling until both high words agree",
                "evidence":[f"raw six-word dependency chain at {rows[index]['pc']}",str(match)],
                "contradictions":["live tick producer, units, offset, interruption and timing consumers remain unknown"],
                "validation_needed":["capture both equal/retry branches and rollover","trace tick/offset provenance and downstream consumers","compare portable clock contract without fixed reference values"]})
    for index in range(max(0,len(raw)-4)):
        match=low_timebase_deadline(raw[index:index+5],int(rows[index]['pc'],16))
        if match:
            found.append({"id":"low_timebase_deadline","status":"STRUCTURAL_MATCH",
                "hypothesis":"retain a starting low tick; poll unsigned modulo32 elapsed ticks against an immediate threshold",
                "evidence":[f"raw five-word dependency chain at {rows[index]['pc']}",str(match)],
                "contradictions":["source phase/frequency, progress, counter wrap interval and hardware consumer are not supplied"],
                "validation_needed":["capture predicate on either side of threshold and wrap","trace device state and producer scheduling","preserve reference tick units; no host spin assumption"]})
    return found


def low_timebase_deadline(words: list[int],pc: int):
    if len(words)!=5:return None
    a,b,sub,cmp,branch=words
    def low(w):
        return w>>26==31 and (w>>1)&1023==371 and not w&1 and (((w>>16)&31)|((w>>6)&0x3E0))==268
    if not low(a) or not low(b):return None
    start,current=(a>>21)&31,(b>>21)&31
    delta=(sub>>21)&31
    if len({start,current,delta})!=3:return None
    if sub>>26!=31 or (sub>>1)&1023!=40 or sub&1 or (sub>>16)&31!=start or (sub>>11)&31!=current:return None
    if cmp>>26!=10 or cmp&0x00600000 or (cmp>>16)&31!=delta or not cmp&0xFFFF:return None
    field=(cmp>>23)&7
    if branch>>26!=16 or (branch>>21)&31 not in (12,13) or (branch>>16)&31!=4*field or branch&3:return None
    disp=branch&0xFFFC
    if disp&0x8000:disp-=0x10000
    if (pc+16+disp)&0xFFFFFFFF!=(pc+4)&0xFFFFFFFF:return None
    return dict(start_register=start,current_register=current,elapsed_register=delta,threshold_ticks=cmp&0xFFFF,
                cr_field=field,retry_target=f'{pc+4:08X}',elapsed_semantics='unsigned subtraction modulo32')


def stable_timebase_sampler(words: list[int],pc: int):
    if len(words)!=6:return None
    def tbr(w):
        if w>>26!=31 or (w>>1)&1023!=371 or w&1:return None
        return ((w>>16)&31)|((w>>6)&0x3E0)
    if [tbr(w) for w in words[:3]]!=[269,268,269]:return None
    high,low,second=[(w>>21)&31 for w in words[:3]]
    if len({high,low,second})!=3:return None
    compare,branch,ret=words[3:]
    if compare>>26!=31 or (compare>>1)&1023!=0 or compare&0x00600001:return None
    field=(compare>>23)&7
    if {(compare>>16)&31,(compare>>11)&31}!={high,second}:return None
    if branch>>26!=16 or (branch>>21)&31 not in (4,5) or (branch>>16)&31!=4*field+2 or branch&3:return None
    displacement=branch&0xFFFC
    if displacement&0x8000:displacement-=0x10000
    if (pc+16+displacement)&0xFFFFFFFF!=pc or ret!=0x4E800020:return None
    return dict(high_register=high,low_register=low,second_high_register=second,cr_field=field,retry_target=f'{pc:08X}')


def detectors(region: dict) -> list[dict]:
    rows, effects = region["instructions"], region["effects"]
    ms = [r["mnemonic"] for r in rows]
    found = raw_motifs(rows)
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
    for match in raw_motifs(region['instructions']):
        if match['id']=='stable_timebase_sampler':
            pc=int(match['evidence'][0].split()[-1],16)
            requests.append({'break_before':f'0x{pc:08X}',
                'read':['three destination GPRs','CR field','live tick producer/frequency','offset words800030D8/DC'],
                'step':'three original time-base reads and compare; observe both EQ-return and retry paths',
                'assert':'trace sample provenance/units and rollover; no fixed tick or guessed offset; downstream consumer parity required'})
    for e in region["effects"]:
        if e["kind"] == "call" and e["indirect"]:
            requests.append({"break_before": e["pc"], "read": ["CTR", "LR", e["target"]],
                             "step": "one indirect transfer", "assert": "actual target in proven set"})
        elif e["kind"] == "barrier" and e["status"] == "unresolved_hardware":
            requests.append({"break_before": e["pc"],
                             "read": ["HID0", "HID2", "MSR", "stack writes", "instruction bytes"],
                             "step": "barrier and next instruction", "read_after": ["HID0", "PC", "stack bytes"],
                             "assert": "ordered state; cache completion requires separate evidence"})
        elif e["kind"] == "call" and e.get("callee_effects") == "UNTRACED":
            requests.append({"break_before": e["target"], "read": ["GPR", "LR", "callee raw bytes", "consumed globals/SPR"],
                             "break_after": e["return_address"], "read_after": ["GPR", "memory delta", "consumed globals/SPR"],
                             "assert": "trace callee and producer of each branch/pointer input; do not supply an assumed return"})
    return requests
