"""Scriptable boot recognizer: batch -> inspect -> validate -> promote -> rescan."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
from collections import Counter
from pathlib import Path

from fingerprints import detectors, fingerprint, minimal_experiments, similarity
from machine import DolImage, PAL_SHA256, analyze, hx
from store import Store

ROOT = Path(__file__).resolve().parents[3]
SEEDS = Path(__file__).with_name("seeds.json")


def record(db, image, identity, start, end, origin, family, status, evidence):
    analysis = analyze(image, start, end)
    fp = fingerprint(analysis)
    hypotheses = detectors(analysis)
    db.upsert(identity, start, end, origin, family, status, evidence, analysis, fp, hypotheses)
    return analysis, fp, hypotheses


def bootstrap(db, image):
    seed = json.loads(SEEDS.read_text(encoding="utf-8"))
    if seed["binary_sha256"] != image.sha256:
        raise ValueError("seed evidence belongs to a different DOL")
    for item in seed["regions"]:
        record(db, image, item["id"], int(item["start"], 16), int(item["end"], 16),
               "seed", item["family"], item["status"], item["evidence"])
    return len(seed["regions"])


def first_return_window(image, start, max_words=128):
    """Heuristic extent, never a certified function boundary."""
    from machine import decode
    for index in range(max_words):
        pc = start + 4 * index
        if not image.is_text(pc):
            break
        ins = decode(image.word(pc, text=True), pc)
        if ins.m == "blr":
            return pc + 4, "first_return_heuristic"
        if ins.m == ".word":
            return pc + 4, "unknown_opcode_window"
        if ins.m in ("b", "ba") and not start <= (ins.target or 0) <= pc:
            return pc + 4, "external_tail_branch_window"
    return start + 4 * min(max_words, 24), "capped_window"


def scan_constructors(db, image, first: int, stop: int):
    if not 0 <= first < stop <= 282:
        raise ValueError("constructor index range must be within 0..281")
    found = []
    for index in range(first, stop):
        table_addr = 0x804AAC60 + 4 * index
        target = image.word(table_addr, text=False)
        if not image.is_text(target):
            found.append({"index": index, "status": "UNKNOWN", "reason": "table target is not DOL text",
                          "table_word": hx(target)})
            continue
        end, boundary = first_return_window(image, target)
        identity = f"ctor_{index}"
        analysis, _, hypotheses = record(
            db, image, identity, target, end, "candidate", "unknown_constructor_table_target",
            "UNKNOWN", [f"static table word {hx(table_addr)} -> {hx(target)}; live table may differ",
                        f"boundary={boundary}"],)
        found.append({"index": index, "id": identity, "start": hx(target), "end": hx(end),
                      "boundary": boundary, "detectors": [h["id"] for h in hypotheses],
                      "unknown_words": len(analysis["unknown_instructions"])})
    return found


def frontier(db, image, name="connected_pal_boot"):
    current = db.frontier(name)
    start = current["stop_pc"]
    if start == 0x80371730:
        identity, end = "frontier_sync_gqr", 0x80371768
    elif start == 0x80372894:
        identity, end = "frontier_l2cr_call", 0x803728A4
    elif start == 0x80372904:
        identity, end = "frontier_handler_call", 0x80372908
    else:
        # No generic extent is inferred from a PC alone. Add a byte-backed
        # range before analyzing a future frontier.
        raise ValueError(f"no checked bounded region for frontier {hx(start)}")
    analysis, fp, hypotheses = record(
        db, image, identity, start, end, "frontier",
        "unknown_boot_frontier", "UNKNOWN",
        ["reverse/boot/PROGRESS.md: explicit profile-specific connected stop",
         "reverse/boot/research/NATIVE_L2_COMPLETION_38.md" if start == 0x80372904 else
         "reverse/boot/research/NATIVE_SYNC_COMPLETION_37.md" if start == 0x80372894
         else "reverse/boot/research/SYNC_GQR_CHAIN.md: raw tail only"])
    return {"connected_stop": hx(start), "next_native_checkpoint": None,
            "raw_range": [analysis["start"], analysis["end"]],
            "normalized_sha256": fp["normalized_sha256"],
            "hypotheses": hypotheses, "effects": analysis["effects"],
            "unknown_words": analysis["unknown_instructions"],
            "unsupported_semantics": analysis["unsupported_semantics"],
            "minimal_experiments": minimal_experiments(analysis),
            "classification": "STRUCTURAL_ONLY; no boot-frontier advance"}


def run_native_probe(exe: Path, dol: Path, observed: dict[str, str | None], entry: Path | None = None):
    args = [str(exe), str(dol)]
    if entry:
        if any(observed.values()):
            raise ValueError("native entry fixture cannot be mixed with request-only observations")
        args.append(str(entry))
    for name in ("msr", "hid2", "hid0"):
        value = observed[name]
        if value is None:
            if any(observed[later] is not None for later in
                   ("hid2", "hid0")[("msr", "hid2", "hid0").index(name):]):
                raise ValueError("observed native inputs must be a contiguous prefix")
            break
        if not re.fullmatch(r"[0-9A-Fa-f]{8}", value):
            raise ValueError(f"observed {name} must be eight hex digits")
        args += ["--observed-" + name, value]
    completed = subprocess.run(args, text=True, capture_output=True, timeout=30,
                               check=False)
    if completed.returncode:
        raise RuntimeError(f"native boot probe failed: {completed.stderr.strip()}")
    stops = re.findall(r"^STOP pc=0x([0-9A-F]{8})\b(.*)$", completed.stdout, re.M)
    if len(stops) != 1:
        raise ValueError("native probe did not report exactly one stop")
    fields = dict(re.findall(r"\b([a-z][a-z0-9]*)=0x([0-9A-F]{8})\b", stops[0][1]))
    if entry:
        lines = [line.split() for line in completed.stdout.splitlines() if re.match(r"^[0-9a-f]{8} ", line)]
        if not lines or len(lines[-1]) not in (115, 118) or int(lines[-1][0], 16) != int(stops[0][0], 16):
            raise ValueError("native entry probe missing full stop state")
        fields = {key: lines[-1][n] for n, key in enumerate(("pc", "msr", "lr", "cr", "xer", "fpscr"))}
        fields.update({f"r{n}": lines[-1][6+n] for n in range(32)})
        fields.update({"ctr": lines[-1][102], "hid0": lines[-1][103], "hid2": lines[-1][104]})
        if len(lines[-1]) == 118:
            fields["l2cr"] = lines[-1][115]
    writes = [dict(re.findall(r"\b([a-z]+)=0x([0-9A-F]{8})\b", line))
              for line in completed.stdout.splitlines() if line.startswith("WRITE ")]
    for line in completed.stdout.splitlines():
        if line.startswith(("COMMITTED_STACK ", "COMMITTED_L2 ")):
            parts = line.split()
            if len(parts) != 3 or any(not re.fullmatch(r"[0-9a-fA-F]{8}", p) for p in parts[1:]):
                raise ValueError("malformed native committed-memory readback")
            writes.append({"addr": "0x" + parts[1].upper(), "value": "0x" + parts[2].upper(),
                           "provenance": "applied native stack bytes; printed readback"})
    return {"stop_pc": "0x" + stops[0][0], "known_fields": fields,
            "ordered_stack_writes": writes,
            "provenance": "native C++ from explicit entry fixture; bounded immutable backend" if entry
            else "native C++ run; hardware inputs explicitly caller supplied",
            "entry_fixture_sha256": hashlib.sha256(entry.read_bytes()).hexdigest() if entry else None,
            "observed_input_names": [k for k, v in observed.items() if v is not None]}


def rescan(db, threshold: float = 0.35):
    db.clear_matches()
    references = [r for r in db.rows(origin="seed")]
    candidates = [r for r in db.rows() if r["origin"] in ("candidate", "frontier")]
    matches = {}
    for candidate in candidates:
        cfp = json.loads(candidate["fingerprint_json"])
        ranked = []
        for reference in references:
            if (candidate["start"], candidate["end"]) == (reference["start"], reference["end"]):
                continue  # a body cannot validate itself under a second identity
            result = similarity(cfp, json.loads(reference["fingerprint_json"]))
            if result["score"] >= threshold:
                contradictions = []
                if cfp["unknown_words"] or cfp["unsupported_semantics"] or cfp["unresolved_edges"]:
                    contradictions.append("candidate has unproven instruction semantics")
                if candidate["origin"] == "frontier":
                    contradictions.append("sync/ICFI completion lacks native state validation")
                db.set_match(candidate["id"], reference["id"], result, contradictions)
                ranked.append({"reference": reference["id"], "family": reference["family"],
                               "reference_status": reference["status"],
                               "score": result["score"], "components": result["components"],
                               "contradictions": contradictions})
        matches[candidate["id"]] = sorted(ranked, key=lambda x: -x["score"])[:5]
    return matches


def clusters(db, threshold: float = 0.60):
    rows = list(db.rows(origin="candidate"))
    fps = {r["id"]: json.loads(r["fingerprint_json"]) for r in rows}
    distances = {}

    def score(a, b):
        key = tuple(sorted((a, b)))
        if key not in distances:
            distances[key] = similarity(fps[a], fps[b])["score"]
        return distances[key]

    # Complete linkage prevents A~B~C from implying A~C when A and C differ.
    groups = []
    for row in sorted(rows, key=lambda r: r["id"]):
        identity = row["id"]
        group = next((g for g in groups if all(score(identity, other) >= threshold
                                                for other in g)), None)
        if group is None:
            groups.append([identity])
        else:
            group.append(identity)
    result = []
    for group in groups:
        if len(group) < 2:
            continue
        pair_scores = [score(a, b) for i, a in enumerate(group) for b in group[i + 1:]]
        shared = set(fps[group[0]]["coarse_ngrams"])
        for identity in group[1:]:
            shared &= set(fps[identity]["coarse_ngrams"])
        result.append({"members": group, "minimum_pair_score": min(pair_scores),
                       "common_coarse_ngrams": sorted(shared)[:16],
                       "status": "STRUCTURAL_MATCH",
                       "contradictions": ["heuristic function extents", "live constructor table may be modified",
                                          "similarity does not prove common behavior"]})
    return sorted(result, key=lambda g: (-len(g["members"]), g["members"]))


def learned_hits(db, patterns):
    hits = []
    source_starts = {row["id"]: (row["start"], row["end"])
                     for row in db.rows(origin="seed")}
    for row in db.rows(origin="candidate"):
        fp = json.loads(row["fingerprint_json"])
        tokens = set(fp["coarse_ngrams"])
        for pattern in patterns:
            if any(source_starts.get(source) == (row["start"], row["end"])
                   for source in pattern["sources"]):
                continue
            if set(pattern["tokens"]) <= tokens:
                hits.append({"candidate": row["id"], "family_hypothesis": pattern["family"],
                             "pattern_id": pattern["id"], "support": pattern["support"],
                             "status": "STRUCTURAL_MATCH",
                             "validation_needed": "compare ordered effects and reject decoys; no automatic promotion"})
    return hits


def family_rankings(db, patterns):
    """Aggregate shared-token coverage; never interpret it as confidence."""
    families = {}
    for pattern in patterns:
        families.setdefault(pattern["family"], []).append(pattern)
    seeds = {}
    for row in db.rows(origin="seed"):
        seeds.setdefault(row["family"], []).append(row)
    ranked = []
    for candidate in db.rows(origin="candidate"):
        fp = json.loads(candidate["fingerprint_json"])
        tokens = set(fp["coarse_ngrams"])
        for family, group in families.items():
            references = seeds[family]
            if any((candidate["start"], candidate["end"]) ==
                   (reference["start"], reference["end"]) for reference in references):
                continue
            matched = sum(set(pattern["tokens"]) <= tokens for pattern in group)
            if matched != len(group):
                continue  # partial individual n-grams are not a family hit
            seed_fps = [json.loads(reference["fingerprint_json"]) for reference in references]
            ranked.append({
                "candidate": candidate["id"], "family_hypothesis": family,
                "shared_ngrams_matched": matched, "shared_ngrams_total": len(group),
                "exact_normalized_sequence": all(
                    fp["normalized_sha256"] == seed_fp["normalized_sha256"]
                    for seed_fp in seed_fps),
                "exact_effect_counts": all(fp["effect_kinds"] == seed_fp["effect_kinds"]
                                           for seed_fp in seed_fps),
                "status": "STRUCTURAL_MATCH",
                "validation_needed": "live dispatch; before/after memory; downstream consumer",
            })
    return sorted(ranked, key=lambda hit: (hit["family_hypothesis"], hit["candidate"]))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dol", type=Path, required=True, help="read-only pinned PAL main.dol")
    parser.add_argument("--db", type=Path, default=ROOT / "build" / "boot-recognizer.sqlite")
    parser.add_argument("--report", type=Path, default=ROOT / "build" / "boot-recognizer-report.json")
    parser.add_argument("--probe-exe", type=Path, help="run the current native boot prefix first")
    parser.add_argument("--native-entry", type=Path, help="explicit pre-entry input file for the immutable native probe")
    parser.add_argument("--frontier-name", choices=("connected_pal_boot", "connected_immutable_native_boot"),
                        default="connected_pal_boot", help="keep different supported input/backend profiles separate")
    for name in ("msr", "hid2", "hid0"):
        parser.add_argument("--observed-" + name, help="explicit 8-hex-digit reference input")
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("bootstrap")
    scan = sub.add_parser("scan")
    scan.add_argument("--constructors", default="16:48", help="half-open table index range")
    sub.add_parser("frontier")
    sub.add_parser("rescan")
    sub.add_parser("batch")
    inspect = sub.add_parser("inspect")
    inspect.add_argument("identity", help="region id, e.g. ctor_16 or frontier_sync_gqr")
    promote = sub.add_parser("promote")
    promote.add_argument("identity")
    promote.add_argument("family")
    promote.add_argument("evidence", type=Path, help="JSON with four validation passes and addresses")
    sub.add_parser("report-db")
    advance = sub.add_parser("advance-frontier", help="explicit reviewed five-pass evidence; never automatic")
    advance.add_argument("evidence", type=Path)
    args = parser.parse_args(argv)
    image = DolImage(args.dol)
    db = Store(args.db)
    db.run(image.sha256, args.command)
    output = {"dol_sha256": image.sha256, "command": args.command,
              "frontier_name": args.frontier_name,
              "frontier_before": hx(db.frontier(args.frontier_name)["stop_pc"])}
    observed = {name: getattr(args, "observed_" + name) for name in ("msr", "hid2", "hid0")}
    if args.probe_exe:
        if args.native_entry and args.frontier_name != "connected_immutable_native_boot":
            raise ValueError("explicit native profile requires its separate frontier name")
        output["native_probe"] = run_native_probe(args.probe_exe, args.dol, observed, args.native_entry)
    elif any(observed.values()):
        raise ValueError("observed values require --probe-exe")
    elif args.native_entry:
        raise ValueError("native-entry requires --probe-exe")
    if args.command in ("bootstrap", "batch"):
        output["seed_count"] = bootstrap(db, image)
    if args.command in ("scan", "batch"):
        first, stop = (int(v) for v in (args.constructors if args.command == "scan" else "16:48").split(":"))
        output["scanned_constructors"] = scan_constructors(db, image, first, stop)
    if args.command in ("frontier", "batch"):
        output["frontier_analysis"] = frontier(db, image, args.frontier_name)
        if "native_probe" in output:
            output["frontier_analysis"]["entry_from_native_probe"] = (
                output["native_probe"] if output["native_probe"]["stop_pc"] ==
                output["frontier_analysis"]["connected_stop"] else
                {"status": "probe stopped earlier; no reached entry-state claim",
                 "probe_stop": output["native_probe"]["stop_pc"]})
    if args.command == "promote":
        evidence = json.loads(args.evidence.read_text(encoding="utf-8"))
        db.promote(args.identity, args.family, evidence)
        output["promoted"] = args.identity
    if args.command == "advance-frontier":
        evidence = json.loads(args.evidence.read_text(encoding="utf-8"))
        if evidence["frontier_name"] != args.frontier_name or evidence["binary_sha256"] != image.sha256:
            raise ValueError("frontier evidence profile/binary mismatch")
        db.advance_frontier(int(evidence["next_stop"], 16), evidence, name=args.frontier_name)
    if args.command == "inspect":
        row = db.get(args.identity)
        analysis = json.loads(row["analysis_json"])
        output["region"] = {"id": row["id"], "family": row["family"],
                            "status": row["status"], "evidence": json.loads(row["evidence_json"]),
                            "analysis": analysis,
                            "fingerprint": json.loads(row["fingerprint_json"]),
                            "hypotheses": json.loads(row["hypotheses_json"]),
                            "experiments": minimal_experiments(analysis),
                            "portable_cpp": None if row["status"] != "VALIDATED" else
                            "See separately validated native module; automatic code generation is disabled"}
    if args.command in ("rescan", "batch", "scan", "bootstrap", "promote"):
        output["learned_patterns"] = db.learn()
        output["matches"] = rescan(db)
        output["clusters"] = clusters(db)
        output["learned_hits"] = learned_hits(db, output["learned_patterns"])
        output["family_rankings"] = family_rankings(db, output["learned_patterns"])
    counts = Counter(r["status"] for r in db.rows())
    output["status_counts"] = dict(counts)
    output["frontier_after"] = hx(db.frontier(args.frontier_name)["stop_pc"])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(output, indent=2) + "\n", encoding="utf-8")
    print(f"DOL {image.sha256} | {args.command} | regions {sum(counts.values())} | statuses {dict(counts)}")
    if "clusters" in output:
        print(f"unknown clusters {len(output['clusters'])}: " + "; ".join(
            f"{len(c['members'])} members (minimum pair {c['minimum_pair_score']:.4f}): "
            f"{', '.join(c['members'][:5])}" + (", ..." if len(c['members']) > 5 else "")
            for c in output["clusters"][:5]))
        print(f"learned patterns {len(output['learned_patterns'])}; structural hits {len(output['learned_hits'])}")
        print(f"complete shared-pattern family matches {len(output['family_rankings'])}")
    if "frontier_analysis" in output:
        f = output["frontier_analysis"]
        print(f"frontier {f['connected_stop']} | detectors {[h['id'] for h in f['hypotheses']]} "
              f"| next native checkpoint {f['next_native_checkpoint']}")
    print(f"report {args.report}")


if __name__ == "__main__":
    main()
