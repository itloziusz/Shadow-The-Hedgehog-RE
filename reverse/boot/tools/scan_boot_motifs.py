"""Whole-text raw motif scan, bounded analysis and UNKNOWN database candidates.

The shared recognizer detectors retain bitfields/register dependencies. A
match is never promoted or connected merely because it resembles solved code.
"""
import argparse
import json
from pathlib import Path
import sys
from collections import Counter

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'recognizer'))
from machine import DolImage, analyze
from fingerprints import raw_motifs, detectors, fingerprint
from store import Store

WIDTHS={'interrupt_mask_exchange':5,'eight_word_fill_group':10,'stable_timebase_sampler':6}


def scan(dol,db_path,report):
    root=Path(__file__).resolve().parents[3]/'build'
    for p in (db_path,report):
        if not p.resolve().is_relative_to(root.resolve()):raise ValueError('generated outputs must stay in build/')
    image=DolImage(dol);db=Store(db_path);db.run(image.sha256,'whole_text_raw_motifs');hits=[];scanned=0
    for section in image.sections:
        if not section.text:continue
        raw=image.read(section.address,section.end-section.address)
        rows=[dict(raw=raw[n:n+4].hex(),pc=f'0x{section.address+n:08X}') for n in range(0,len(raw),4)]
        scanned+=len(rows)
        for match in raw_motifs(rows):
            pc=int(match['evidence'][0].split()[-1],16);end=pc+4*WIDTHS[match['id']]
            analysis=analyze(image,pc,end);identity=f"raw_{match['id']}_{pc:08X}"
            db.upsert(identity,pc,end,'candidate','unknown_raw_motif','UNKNOWN',
                      [f"whole original text raw scan; {match['id']}; caller/input/semantic status unproven"],
                      analysis,fingerprint(analysis),detectors(analysis))
            hits.append(dict(id=identity,start=f'{pc:08x}',end=f'{end:08x}',hypothesis=match,
                             status=db.get(identity)['status'],unsupported_semantics=analysis['unsupported_semantics']))
    result=dict(binary_sha256=image.sha256,scanned_instruction_words=scanned,counts=dict(Counter(h['hypothesis']['id'] for h in hits)),
                candidates=hits,policy='raw structural matches; no automatic promotion or frontier advance')
    report.parent.mkdir(parents=True,exist_ok=True);report.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(f"Raw scan {scanned} words; matches {result['counts']}; all new candidates remain UNKNOWN")
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('dol','db_path','report'):parser.add_argument(name,type=Path)
    scan(**vars(parser.parse_args()))
