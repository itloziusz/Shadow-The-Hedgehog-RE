"""Symbol database: merges curated names (GAMEPLAY_SYMBOLS.csv) with
automatically derived names (RTTI vtable ownership, ctor/dtor detection).

Name provenance is kept explicit:
  csv      curated semantic name (confidence in the CSV row)
  vt       'Class::vfNN'  -- function occupies slot NN of Class's vtable
           (Class = most-base class whose vtable holds it).  The *class*
           name is original (RTTI); the method name is not.
  ctor/dtor  'Class::Class' / 'Class::~Class' derived from vptr stores
Original Sega/Sonic Team function names are NOT known; only class names are.
"""
import os
import csv
import json
from collections import defaultdict
from dol import DATA_DIR

ROOT = os.path.normpath(os.path.join(DATA_DIR, '..'))
CSV_PATH = os.path.join(ROOT, 'symbols_curated.csv')


def load_classes():
    p = os.path.join(DATA_DIR, 'classes.json')
    with open(p) as f:
        return json.load(f)


def class_depths(classes):
    memo = {}

    def dep(ti, stack=()):
        if ti in memo:
            return memo[ti]
        if ti in stack or ti not in classes:
            return 0
        bs = classes[ti]['bases']
        v = 0 if not bs else 1 + max(dep(b['typeinfo'], stack + (ti,)) for b in bs)
        memo[ti] = v
        return v
    for ti in classes:
        dep(ti)
    return memo


def auto_names(classes=None):
    classes = classes or load_classes()
    depths = class_depths(classes)
    slot_users = defaultdict(list)     # func -> [(depth, class, slot, vt)]
    vt_class = {}
    for ti, c in classes.items():
        for vt in c['vtables']:
            vt_class[int(vt['addr'], 16)] = c['name']
            for i, s in enumerate(vt['slots']):
                slot_users[int(s, 16)].append((depths.get(ti, 0), c['name'], i, vt['addr'], vt['adjust']))
    names = {}
    thunks = {}
    try:
        import program as P
        prog = P.load()
        for f in slot_users:
            w0, w1 = prog.word(f), prog.word(f + 4)
            # MWCC this-adjust thunk: addi r3,r3,-N ; b target
            if w0 is not None and (w0 >> 16) == 0x3863 and w1 is not None and (w1 >> 26) == 18 and not (w1 & 3):
                imm = w0 & 0xFFFF
                imm = imm - 0x10000 if imm & 0x8000 else imm
                li = w1 & 0x03FFFFFC
                if li & 0x02000000:
                    li -= 0x04000000
                thunks[f] = (imm, (f + 4 + li) & 0xFFFFFFFF)
    except Exception:
        pass
    for f, users in slot_users.items():
        if f in thunks:
            continue
        users.sort()
        d, cname, slot, vt, adj = users[0]
        names[f] = ('%s::vf%02X' % (cname, slot) if adj == 0 else
                    '%s::vf%02X@vt%s' % (cname, slot, vt), 'vt')
    for f, (imm, tgt) in thunks.items():
        tn = names.get(tgt, ('fn_%08X' % tgt,))[0]
        names[f] = ('thunk%d_%s' % (imm, tn), 'thunk')
    # ctor / dtor from vptr writers
    for ti, c in classes.items():
        for vt in c['vtables']:
            if vt['adjust'] != 0:
                continue
            slots = set(int(s, 16) for s in vt['slots'])
            for w in vt['vptr_writers']:
                if not w['base'].startswith("('a', 3)"):
                    continue
                f = int(w['func'], 16)
                short = c['name'].split('<')[0].split('::')[-1]
                if f in slots:
                    names[f] = ('%s::~%s' % (c['name'], short), 'dtor')
                elif f not in names or names[f][1] not in ('dtor', 'csv'):
                    names[f] = ('%s::%s' % (c['name'], short), 'ctor')
    return names, vt_class


def load_csv():
    out = {}
    if not os.path.exists(CSV_PATH):
        return out
    with open(CSV_PATH, newline='') as f:
        for row in csv.DictReader(f):
            try:
                a = int(row['address'], 16)
            except (ValueError, KeyError):
                continue
            out[a] = row
    return out


class Symbols:
    def __init__(self):
        self.classes = load_classes()
        self.auto, self.vt_class = auto_names(self.classes)
        self.csv = load_csv()
        self.by_name = {}
        for a, (n, k) in self.auto.items():
            self.by_name.setdefault(n, a)
        for a, r in self.csv.items():
            self.by_name[r['current_name']] = a

    def name(self, a):
        r = self.csv.get(a)
        if r:
            return r['current_name']
        t = self.auto.get(a)
        if t:
            return t[0]
        return None

    def label(self, a):
        n = self.name(a)
        return n if n else 'fn_%08X' % a

    def lookup(self, s):
        try:
            return int(s, 16)
        except ValueError:
            return self.by_name.get(s)


_S = None


def get_symbols():
    global _S
    if _S is None:
        _S = Symbols()
    return _S
