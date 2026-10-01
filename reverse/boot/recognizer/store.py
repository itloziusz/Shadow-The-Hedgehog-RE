"""Persistent, provenance-preserving boot fingerprint database."""

import json
import sqlite3
import hashlib
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path


def encoded(value) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"))


class Store:
    def __init__(self, path: Path):
        path = Path(path)
        path.parent.mkdir(parents=True, exist_ok=True)
        self.connection = sqlite3.connect(path)
        self.connection.row_factory = sqlite3.Row
        self.connection.executescript("""
            PRAGMA foreign_keys = ON;
            CREATE TABLE IF NOT EXISTS runs (
                id INTEGER PRIMARY KEY, timestamp TEXT NOT NULL,
                dol_sha256 TEXT NOT NULL, command TEXT NOT NULL);
            CREATE TABLE IF NOT EXISTS regions (
                id TEXT PRIMARY KEY, start INTEGER NOT NULL, end INTEGER NOT NULL,
                origin TEXT NOT NULL, family TEXT NOT NULL, status TEXT NOT NULL,
                evidence_json TEXT NOT NULL, analysis_json TEXT NOT NULL,
                fingerprint_json TEXT NOT NULL, hypotheses_json TEXT NOT NULL,
                updated_at TEXT NOT NULL);
            CREATE TABLE IF NOT EXISTS matches (
                candidate_id TEXT NOT NULL, reference_id TEXT NOT NULL,
                score REAL NOT NULL, components_json TEXT NOT NULL,
                contradictions_json TEXT NOT NULL,
                PRIMARY KEY(candidate_id, reference_id));
            CREATE TABLE IF NOT EXISTS learned_patterns (
                id TEXT PRIMARY KEY, family TEXT NOT NULL, support INTEGER NOT NULL,
                tokens_json TEXT NOT NULL, source_ids_json TEXT NOT NULL,
                false_positive_count INTEGER NOT NULL DEFAULT 0);
            CREATE TABLE IF NOT EXISTS frontier (
                name TEXT PRIMARY KEY, stop_pc INTEGER NOT NULL,
                status TEXT NOT NULL, evidence_json TEXT NOT NULL);
        """)
        self.connection.commit()

    def run(self, sha256: str, command: str):
        self.connection.execute("INSERT INTO runs(timestamp,dol_sha256,command) VALUES(?,?,?)",
                                (datetime.now(timezone.utc).isoformat(), sha256, command))
        self.connection.commit()

    def upsert(self, identity: str, start: int, end: int, origin: str, family: str,
               status: str, evidence: list[str], analysis: dict, fingerprint: dict,
               hypotheses: list[dict]):
        if status not in {"UNKNOWN", "STRUCTURAL_MATCH", "PROBABLE",
                          "STRONGLY_SUPPORTED", "VALIDATED"}:
            raise ValueError("invalid confidence status")
        previous = self.connection.execute("SELECT * FROM regions WHERE id=?",
                                           (identity,)).fetchone()
        # Validation belongs to an exact binary span, not an arbitrary row ID.
        # Rescanning that span cannot replace reviewed evidence or semantics;
        # explicit revalidation belongs in promote(). A changed body needs a
        # new identity and its own proof rather than inheriting VALIDATED.
        if previous and previous["status"] == "VALIDATED":
            prior_analysis = json.loads(previous["analysis_json"])
            prior_fingerprint = json.loads(previous["fingerprint_json"])
            before = (previous["start"], previous["end"], prior_analysis.get("sha256"),
                      prior_fingerprint.get("raw_sha256"))
            after = (start, end, analysis.get("sha256"), fingerprint.get("raw_sha256"))
            if before != after:
                raise ValueError("validated region binary/range changed; new evidence required")
            return
        self.connection.execute("""
            INSERT INTO regions VALUES(?,?,?,?,?,?,?,?,?,?,?)
            ON CONFLICT(id) DO UPDATE SET
              start=excluded.start,end=excluded.end,origin=excluded.origin,
              family=excluded.family,status=excluded.status,
              evidence_json=excluded.evidence_json,analysis_json=excluded.analysis_json,
              fingerprint_json=excluded.fingerprint_json,
              hypotheses_json=excluded.hypotheses_json,updated_at=excluded.updated_at
        """, (identity, start, end, origin, family, status, encoded(evidence),
              encoded(analysis), encoded(fingerprint), encoded(hypotheses),
              datetime.now(timezone.utc).isoformat()))
        self.connection.commit()

    def rows(self, *, origin: str | None = None):
        query = "SELECT * FROM regions" + (" WHERE origin=?" if origin else "")
        return self.connection.execute(query, (origin,) if origin else ()).fetchall()

    def get(self, identity: str):
        row = self.connection.execute("SELECT * FROM regions WHERE id=?", (identity,)).fetchone()
        if row is None:
            raise KeyError(identity)
        return row

    def set_match(self, candidate: str, reference: str, result: dict,
                  contradictions: list[str]):
        self.connection.execute("""INSERT INTO matches VALUES(?,?,?,?,?)
            ON CONFLICT(candidate_id,reference_id) DO UPDATE SET
            score=excluded.score,components_json=excluded.components_json,
            contradictions_json=excluded.contradictions_json""",
            (candidate, reference, result["score"], encoded(result["components"]),
             encoded(contradictions)))
        self.connection.commit()

    def clear_matches(self):
        """A rescan replaces old results; stale hits cannot survive a new seed."""
        self.connection.execute("DELETE FROM matches")
        self.connection.commit()

    def promote(self, identity: str, family: str, evidence: dict):
        required = {"identification", "falsification", "behavioral_validation",
                    "chain_validation", "binary_addresses", "oracle_kind"}
        missing = required - evidence.keys()
        if missing or any(not evidence[key] for key in required):
            raise ValueError(f"VALIDATED promotion lacks required evidence: {sorted(missing)}")
        if evidence.get("contradictions"):
            raise ValueError("unresolved contradictions block VALIDATED promotion")
        self.get(identity)
        self.connection.execute("UPDATE regions SET status='VALIDATED',family=?,evidence_json=? WHERE id=?",
                                (family, encoded(evidence), identity))
        self.connection.commit()

    def frontier(self, name: str = "connected_pal_boot") -> dict:
        row = self.connection.execute("SELECT * FROM frontier WHERE name=?", (name,)).fetchone()
        if row:
            return {"stop_pc": row["stop_pc"], "status": row["status"],
                    "evidence": json.loads(row["evidence_json"])}
        self.connection.execute("INSERT INTO frontier VALUES(?,?,?,?)",
                                (name, 0x80371730, "STOP_BEFORE_INSTRUCTION",
                                 encoded(["reverse/boot/PROGRESS.md checkpoint 31"])))
        self.connection.commit()
        return self.frontier(name)

    def advance_frontier(self, next_pc: int, evidence: dict, *, name="connected_pal_boot"):
        required = {"raw_decode", "data_flow", "reference_state", "native_replay",
                    "regression", "unresolved_side_effects", "previous_stop"}
        if required - evidence.keys() or any(not evidence[k] for k in required - {"unresolved_side_effects"}):
            raise ValueError("frontier advance lacks five-pass evidence")
        if evidence["unresolved_side_effects"]:
            raise ValueError("frontier advance has unresolved side effects")
        current = self.frontier(name)
        if int(evidence["previous_stop"], 16) != current["stop_pc"]:
            raise ValueError("frontier evidence belongs to a different previous stop")
        # Control flow can advance to a lower VA (e.g. sync -> FPR callee).
        # Numeric address ordering is not execution progress. The explicit
        # replay/chain evidence is required, and a repeated stop is rejected.
        if next_pc == current["stop_pc"]:
            raise ValueError("frontier stop did not change")
        self.connection.execute("UPDATE frontier SET stop_pc=?,status=?,evidence_json=? WHERE name=?",
                                (next_pc, "VALIDATED_STOP", encoded(evidence), name))
        self.connection.commit()

    def learn(self) -> list[dict]:
        """Generalize shared normalized bigrams; no automatic semantic promotion."""
        # Materialized patterns are a deterministic projection of current
        # evidence. Removing or demoting a seed must remove its old patterns.
        self.connection.execute("DELETE FROM learned_patterns")
        grouped: dict[str, list] = {}
        for row in self.rows():
            if row["status"] in ("STRONGLY_SUPPORTED", "VALIDATED"):
                grouped.setdefault(row["family"], []).append(row)
        learned = []
        for family, rows in grouped.items():
            if len(rows) < 2:
                continue
            supporters = defaultdict(list)
            for row in rows:
                for token in set(json.loads(row["fingerprint_json"])["coarse_ngrams"]):
                    supporters[token].append(row["id"])
            for token, sources in supporters.items():
                if len(sources) < 2:
                    continue  # one example is not a reusable pattern
                pattern = {"id": "family:" + family + ":" + hashlib.sha256(token.encode()).hexdigest()[:12],
                           "family": family, "support": len(sources), "tokens": [token],
                           "sources": sorted(sources), "status": "STRUCTURAL_PATTERN_ONLY"}
                self.connection.execute("""INSERT INTO learned_patterns(id,family,support,tokens_json,source_ids_json)
                    VALUES(?,?,?,?,?) ON CONFLICT(id) DO UPDATE SET support=excluded.support,
                    tokens_json=excluded.tokens_json,source_ids_json=excluded.source_ids_json""",
                    (pattern["id"], family, len(sources), encoded(pattern["tokens"]),
                     encoded(pattern["sources"])))
                learned.append(pattern)
        self.connection.commit()
        return learned
