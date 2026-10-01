"""Independent raw PAL TB inventory; original input is opened read-only.

This does not execute guest code or assert that static call sites are reachable.
Only outputs under this repository are permitted. Direct edge and special
register decoding use raw bitfields; the shared decoder is display-only.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'gameplay' / 'tools'))
from dol import Dol
from ppc import decode

TB = {268: 'TBL-read', 269: 'TBU-read', 284: 'TBL-write', 285: 'TBU-write'}
LOW_MEMORY = {0x800030d8, 0x800030dc, 0x800000f8, 0x800000fc}
EXPECTED_SHA256 = 'fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af'
EXPECTED_TB = {
    0x80376ebc: 0x7cac42e6, 0x80376ec0: 0x7ccc42e6,
    0x80379628: 0x7c6d42e6, 0x8037962c: 0x7c8c42e6,
    0x80379630: 0x7cad42e6, 0x80379640: 0x7c6c42e6,
    0x8039f5bc: 0x7d4c42e6, 0x8039f5c0: 0x7d6d42e6,
    0x8039f75c: 0x7f1c43a6, 0x8039f760: 0x7f3d43a6,
    0x804035d4: 0x7c6d42e6, 0x804035d8: 0x7c8c42e6,
    0x804035dc: 0x7c0d42e6,
}


def special(word):
    if word >> 26 != 31:
        return None
    xo = (word >> 1) & 1023
    number = ((word >> 16) & 31) | (((word >> 11) & 31) << 5)
    if xo not in (339, 371, 467) or number not in TB:
        return None
    # mftb has XO371. mfspr/ mtspr also included: illegal/suspicious forms
    # remain explicit, rather than being silently presented as valid TB ops.
    return dict(xo=xo, spr_tbr=number, register=(word >> 21) & 31,
                rc=word & 1, form={339: 'mfspr', 371: 'mftb', 467: 'mtspr'}[xo],
                candidate=TB[number])


def branch(pc, word):
    if word >> 26 == 18:
        disp = word & 0x03fffffc
        if disp & 0x02000000:
            disp -= 0x04000000
        return dict(target=(disp if word & 2 else pc + disp) & 0xffffffff,
                    lk=bool(word & 1), aa=bool(word & 2), kind='I')
    if word >> 26 == 16:
        disp = word & 0xfffc
        if disp & 0x8000:
            disp -= 0x10000
        return dict(target=(disp if word & 2 else pc + disp) & 0xffffffff,
                    lk=bool(word & 1), aa=bool(word & 2), kind='B',
                    bo=(word >> 21) & 31, bi=(word >> 16) & 31)
    return None


def words(dol):
    for section in dol.sections:
        if not section.is_text:
            continue
        for pc in range(section.addr, section.end - 3, 4):
            yield pc, dol.u32(pc)


def direct_edges(dol, target):
    return [(pc, word, edge) for pc, word in words(dol)
            if (edge := branch(pc, word)) is not None and edge['target'] == target]


def dump(dol, start, count):
    for pc in range(start, start + 4 * count, 4):
        word = dol.u32(pc)
        if word is None:
            break
        section = dol.find(pc)
        print(f'{pc:08X} +{section.file_off + pc-section.addr:06X} {word:08X} {decode(word, pc).text()}')


def local_lowmem(dol):
    """Constants produced inside uninterrupted straight-line text blocks.

    State resets after every branch, call, return and unsupported instruction.
    No entry constant, predecessor join or read value is fabricated. This
    intentionally misses references; results are static candidates awaiting CFG
    validation, especially when another edge enters the middle of a block.
    """
    out = []
    for section in dol.text:
        known = {}
        for pc in range(section.addr, section.end - 3, 4):
            word = dol.u32(pc)
            ins = decode(word, pc)
            op = word >> 26
            if ins.kind in ('load', 'store'):
                mem = next((x for x in ins.ops if x[0] == 'm'), None)
                if mem and (mem[2] == 0 or mem[2] in known):
                    base, proof = (0, []) if mem[2] == 0 else known[mem[2]]
                    ea = (base + mem[1]) & 0xffffffff
                    if ea in LOW_MEMORY:
                        out.append(dict(pc=f'{pc:08X}', word=f'{word:08X}',
                                        effective_address=f'{ea:08X}', display=ins.text(),
                                        address_producer=[f'{x:08X}' for x in proof]))
                if ins.kind == 'load':
                    if ins.m == 'lmw':
                        for r in range(ins.rd, 32): known.pop(r, None)
                    else: known.pop(ins.rd, None)
                if ins.upd: known.pop(ins.ra, None)
                continue
            if ins.kind in ('branch', 'call', 'ret') or ins.m == '.word':
                known.clear(); continue
            if op in (14, 15):
                val = (word & 0xffff) - (0x10000 if word & 0x8000 else 0)
                if op == 15: val <<= 16
                if ins.ra == 0: known[ins.rd] = (val & 0xffffffff, [pc])
                elif ins.ra in known:
                    prev, proof = known[ins.ra]
                    known[ins.rd] = ((prev + val) & 0xffffffff, proof + [pc])
                else: known.pop(ins.rd, None)
            elif op in (24, 25):
                val = (word & 0xffff) << (16 if op == 25 else 0)
                if ins.rd in known:
                    prev, proof = known[ins.rd]
                    known[ins.ra] = (prev | val, proof + [pc])
                else: known.pop(ins.ra, None)
            elif ins.m == 'mr':
                if ins.rd in known: known[ins.ra] = known[ins.rd][0], known[ins.rd][1] + [pc]
                else: known.pop(ins.ra, None)
            elif ins.kind == 'cmp' or ins.kind == 'fp' or ins.kind == 'ps' or ins.m in ('nop', 'sync', 'isync', 'eieio'):
                pass
            elif ins.kind == 'sys':
                known.clear()
            else:
                # Conservative: avoid accidental constants surviving a write
                # whose destination is not represented by ordinary rd.
                known.clear()
    return out


def inventory(dol):
    rows = list(words(dol))
    specials = []
    edges = []
    # Broad lexical displacement candidates are NOT effective-address proofs.
    candidates = []
    for pc, word in rows:
        op = special(word)
        if op:
            specials.append(dict(pc=f'{pc:08X}', word=f'{word:08X}', **op))
        edge = branch(pc, word)
        if edge:
            edges.append(dict(pc=pc, word=word, **edge))
        if word >> 26 in (32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
                          46, 47, 48, 49, 50, 51, 52, 53, 54, 55):
            if (word & 0xffff) in (0x30d8, 0x30dc, 0xf8, 0xfc):
                candidates.append(dict(pc=f'{pc:08X}', word=f'{word:08X}',
                                       lexical_displacement=f'{word & 0xffff:04X}',
                                       display=decode(word, pc).text()))
    targets = {0x80379620, 0x80379628, 0x80379648, 0x804035d4}
    # Addresses of every independently recognized instruction also searchable.
    targets.update(int(r['pc'], 16) for r in specials)
    direct = {f'{target:08X}': [dict(pc=f"{e['pc']:08X}", word=f"{e['word']:08X}",
                                      lk=e['lk'], aa=e['aa'], kind=e['kind'])
                                  for e in edges if e['target'] == target]
              for target in sorted(targets)}
    pointer_words = {f'{target:08X}': [] for target in targets}
    for section in dol.sections:
        for addr in range(section.addr, section.end - 3, 4):
            value = dol.u32(addr)
            if value in targets:
                pointer_words[f'{value:08X}'].append(dict(address=f'{addr:08X}', section=section.key,
                                                         is_text=section.is_text))
    return dict(sha256=hashlib.sha256(dol.raw).hexdigest(), text_word_count=len(rows),
                sections=[dict(key=s.key, start=f'{s.addr:08X}', end=f'{s.end:08X}',
                               file_offset=f'{s.file_off:06X}', text=s.is_text) for s in dol.sections],
                raw_timebase_special_register_forms=specials,
                direct_edges_to_sampled_targets=direct,
                exact_aligned_pointer_words=pointer_words,
                lexical_low_memory_displacement_candidates=candidates,
                straight_line_constant_address_candidates=local_lowmem(dol),
                scope='all aligned text words; direct raw I/B edges only; candidate pointer words do not prove dynamic indirect targets')


def check(dol, result):
    if result['sha256'] != EXPECTED_SHA256:
        raise ValueError('PAL authority digest mismatch')
    observed = {int(r['pc'], 16): int(r['word'], 16)
                for r in result['raw_timebase_special_register_forms']}
    if observed != EXPECTED_TB:
        raise ValueError('whole-text TB inventory changed')
    for target, expected in ((0x80379628, 39), (0x80379640, 12), (0x80379648, 27)):
        got = sum(r['lk'] for r in result['direct_edges_to_sampled_targets'][f'{target:08X}'])
        if got != expected:
            raise ValueError(f'direct BL inventory changed at {target:08X}: {got}')
    # Decoder falsification: destination changes are retained, unsupported
    # primary/XO/TBR forms are not silently presented as a known valid opcode.
    for word in EXPECTED_TB.values():
        op = special(word)
        assert op is not None
        assert special(word ^ (1 << 26)) is None
        changed = special(word ^ (1 << 21))
        assert changed is not None and changed['register'] == (op['register'] ^ 1)
        changed = special(word | 1)
        assert changed is not None and changed['rc'] == 1
    assert special(0x7c6d42e6 ^ (3 << 11)) is None
    assert branch(0x80379638, 0x4082fff0)['target'] == 0x80379628
    assert branch(0x80379668, 0x4bffffc1)['target'] == 0x80379628
    assert branch(0x80051420, 0x48328209)['target'] == 0x80379628
    # Falsify decimal-rate pattern guesses at every /125000 transition
    # possible for the input word shifted right by two, plus /1000 edges.
    # Both multipliers are formed with signed addi, e.g.431C0000-217D.
    rates = {0, 1, 40500000, 0x3fffffff}
    for divisor in (125000, 1000):
        limit = 0x3fffffff // divisor
        ks = range(limit + 1) if divisor == 125000 else (*range(20), limit-1, limit)
        for k in ks:
            for delta in (-1, 0, 1):
                value = divisor * k + delta
                if 0 <= value <= 0x3fffffff: rates.add(value)
    for rate in rates:
        assert (rate * 0x431bde83) >> 47 == rate // 125000
        assert (rate * 0x10624dd3) >> 38 == rate // 1000
    print('PASS independent PAL raw TB inventory, direct call counts and decoder falsification')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('dol', type=Path)
    p.add_argument('--report', type=Path)
    p.add_argument('--dis', type=lambda x: int(x, 16))
    p.add_argument('--count', type=int, default=64)
    p.add_argument('--lowmem', action='store_true')
    p.add_argument('--callers', type=lambda x: int(x, 16))
    p.add_argument('--caller-context', type=int, default=0)
    p.add_argument('--check', action='store_true')
    a = p.parse_args()
    d = Dol(a.dol)
    if a.callers is not None:
        for pc, word, edge in direct_edges(d, a.callers):
            print(f'{pc:08X} {word:08X} {edge}')
            if a.caller_context:
                dump(d, pc + 4, a.caller_context)
    elif a.lowmem:
        print(json.dumps(local_lowmem(d), indent=2))
    elif a.dis is not None:
        dump(d, a.dis, a.count)
    else:
        result = inventory(d)
        if a.check:
            check(d, result)
            if a.report is None:
                return
        if a.report:
            if not a.report.resolve().is_relative_to(ROOT.resolve()):
                raise ValueError('output must remain inside repository')
            a.report.parent.mkdir(parents=True, exist_ok=True)
            a.report.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
        print(json.dumps({k: result[k] for k in ('sha256', 'text_word_count',
              'raw_timebase_special_register_forms', 'direct_edges_to_sampled_targets',
              'exact_aligned_pointer_words')}, indent=2))


if __name__ == '__main__':
    main()
