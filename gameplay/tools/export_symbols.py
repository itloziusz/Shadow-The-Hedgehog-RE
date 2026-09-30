"""Export GAMEPLAY_SYMBOLS.csv = curated names (symbols_curated.csv) + RTTI-derived names.

Columns: address,current_name,subsystem,confidence,evidence
- curated rows keep their own confidence/evidence
- RTTI-derived rows: class part is ORIGINAL (RTTI), method part is positional
  (vfNN = vtable slot) -> confidence STRONG for class ownership, evidence names
  the derivation (vt slot / ctor / dtor / thunk).
Library families (sdk/runtime/middleware/renderware/csd) are omitted unless curated.
"""
import os
import csv
import json
from symbols import get_symbols, ROOT as GAMEPLAY_ROOT
from dol import DATA_DIR

SKIP = ('sdk_', 'runtime', 'middleware', 'renderware', 'ui_csd', 'audio_gcax')


def main():
    S = get_symbols()
    fam = json.load(open(os.path.join(DATA_DIR, 'func_families.json')))
    rows = {}
    for a, r in S.csv.items():
        rows[a] = [('%08X' % a), r['current_name'], r['subsystem'], r['confidence'], r['evidence']]
    for a, (name, kind) in S.auto.items():
        if a in rows:
            continue
        f = fam.get('%08X' % a, {}).get('family', 'unknown')
        if f.startswith(SKIP):
            continue
        ev = {'vt': 'RTTI vtable slot (class original; method positional)',
              'ctor': 'stores own vtable into this (ctor; class original)',
              'dtor': 'stores own vtable + deletes (dtor; class original)',
              'thunk': 'addi r3,r3,-N; b target (this-adjust thunk)'}[kind]
        rows[a] = ['%08X' % a, name, f, 'STRONG', ev]
    out = os.path.join(GAMEPLAY_ROOT, 'GAMEPLAY_SYMBOLS.csv')
    with open(out, 'w', newline='', encoding='utf-8') as fh:
        w = csv.writer(fh)
        w.writerow(['address', 'current_name', 'subsystem', 'confidence', 'evidence'])
        for a in sorted(rows):
            w.writerow(rows[a])
    print('wrote', out, len(rows), 'rows (curated %d)' % len(S.csv))


if __name__ == '__main__':
    main()
