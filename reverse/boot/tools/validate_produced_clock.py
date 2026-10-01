"""Differential test of raw-derived elapsed work, never a boot promotion gate.

Only original entry inputs and RTC epoch enter the native candidate. Observed
frontier cycles, instruction counts, queue state and TB outputs are witnesses,
never production inputs. The separately pinned source audit must pass first.
The event owner and equivalence to physical Gekko elapsed time remain open.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys

from boot_state_diff import (FIELD_NAMES, compare_fields, compare_memory,
                             hex_field, parse_native_state, reference_fields)
from capture_dolphin_rsp import sha256

U64 = (1 << 64) - 1
PAL_DOL_SHA256 = 'fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af'
PREFIX_INSIDE_COLLAPSED_UNITS = [
    0x80376120, 0x80376128, 0x8037612C, 0x80376148, 0x8037614C,
    0x80376158, 0x80376160, 0x80376164, 0x8037964C, 0x80379650,
    0x80379654, 0x80379658, 0x8037965C, 0x80376120, 0x80376128,
    0x8037612C, 0x80379668]


def require(ok, why):
    if not ok:
        raise ValueError(why)


def pin_original_dol(path):
    # Native region word gates deliberately support mutation tests. This
    # original-reference comparison must separately bind the complete input.
    actual = sha256(path)
    require(actual == PAL_DOL_SHA256, 'original PAL DOL identity changed')
    return actual


def checked_inputs(report):
    """Decline controls lacking an original-entry source/readback relationship."""
    require(report.get('capture_complete') is True, 'incomplete capture')
    ext = report['region_extension']
    require(ext['sampler_mode'] == 'continuous' and
            ext['producer_mode'] == 'rtc_initial', 'wrong producer/sampling schedule')
    require(report['sole_internal_stop'].lower() == '80373ac4' and
            ext['stop'].lower() == '80373ac4', 'wrong closed endpoint')
    require(not report['controlled_midchain_writes'] and not ext['midchain_writes'],
            'midchain state forcing')
    for key in ('epoch_controls', 'clock_preentry_writes', 'clock_plan', 'bi2_preentry_writes'):
        require(not ext[key], 'unwitnessed control: ' + key)
    require(ext['requested_epoch'] is None, 'unwitnessed epoch forcing')
    states = report['checkpoints']
    require(len(states) == 2 and states[0]['pc'].lower() == '80003154' and
            states[1]['pc'].lower() == '80373ac4', 'unexpected breakpoint schedule')
    entry, end = states
    original = report['original_unmodified_entry']
    for key in ('clock_producer', 'clock_low_words', 'bi2_blob', 'bi2_pointer'):
        require(entry[key] == original[key], 'unexplained entry writer: ' + key)
    producer = entry['clock_producer']
    require(producer == report['pause_source_readbacks'][0] ==
            report['pause_source_readbacks'][1], 'wall pause changed clock source')
    require(hex_field(producer['cpu_hz'], 8, 'frequency') == 486000000 and
            hex_field(producer['epoch_cycles'], 16, 'epoch phase') == 0 and
            hex_field(producer['rtc_offset'], 16, 'RTC offset') == 0 and
            hex_field(producer['cached_tb'], 16, 'initial cache') == 0 and
            hex_field(producer['exceptions'], 8, 'pending exceptions') == 0,
            'unexplained initial clock state')
    epoch = hex_field(producer['epoch_value'], 16, 'RTC-produced epoch')
    seconds, remainder = divmod(epoch, 40500000)
    require(not remainder and seconds <= 0xffffffff, 'epoch lacks exact RTC producer')
    require(len(entry['bi2_blob']) == 0x4000 and
            not any(bytes.fromhex(entry['bi2_blob'])[8:16]), 'BI2 path has no continuous witness')
    require(len(entry['clock_globals']) == 2 and all(int(x, 16) == 0 for x in entry['clock_globals']),
            'initial clock globals unexplained')
    require(len(entry['clock_low_words']) == 5, 'low-memory clock shape')
    for s in states:
        reference_fields(s)
        require(s['clock_low_words'] == entry['clock_low_words'] and
                s['bi2_blob'] == entry['bi2_blob'], 'unexplained constant/BI2 mutation')
        for key in ('epoch_cycles', 'epoch_value', 'cpu_hz', 'rtc_offset', 'exceptions'):
            require(s['clock_producer'][key] == producer[key], 'producer mutation: ' + key)
    fields = [entry[k] for k in ('msr', 'hid0', 'hid2', 'cr', 'architectural_xer', 'ctr', 'fpscr')]
    fields += entry['ps0'] + entry['ps1'] + entry['gqr'] + ['805f1f30']
    fields += [entry['fpr_source'][n:n+8] for n in range(0, 32, 8)]
    fields += [entry['l2cr'], entry['handler_slot'], entry['bi2_pointer'], entry['bi2_blob']]
    return entry, end, seconds, epoch, ' '.join(fields) + '\n'


def ppc_checkpoints(path):
    """Stream only complete six-row snapshots from the all-kind event evidence."""
    result = []
    group = None
    with path.open(encoding='utf-8') as stream:
        for line in stream:
            parts = line.rstrip('\n').split('\t')
            require(len(parts) == 16, 'event shape differs')
            kind = parts[2]
            if kind == 'ppc':
                require(group is None, 'unterminated PPC snapshot')
                group = {'pc': int(parts[4], 16), 'lr': int(parts[5], 16),
                         'msr': int(parts[6], 16), 'exceptions': int(parts[7], 16),
                         'cycles': int(parts[9], 16), 'gpr': []}
            elif kind in ('gpr0', 'gpr8', 'gpr16', 'gpr24'):
                require(group is not None and len(group['gpr']) == int(kind[3:]),
                        'PPC GPR occurrence order differs')
                group['gpr'] += [int(p, 16) for p in parts[4:12]]
            elif kind == 'ppc-special':
                require(group is not None and len(group['gpr']) == 32 and
                        int(parts[4], 16) == group['pc'], 'PPC special occurrence differs')
                group.update(zip(('cr', 'xer', 'ctr', 'fpscr'),
                                 (int(p, 16) for p in parts[5:9])))
                result.append(group)
                group = None
    require(group is None and result, 'PPC snapshot evidence absent/truncated')
    return result


def word_effects(path):
    """Ordered original stores, including only this new clock/return slice."""
    pcs = {0x80370EA8, 0x80370EAC, 0x80373AB8, 0x80373ABC}
    result = []
    with path.open(encoding='utf-8') as stream:
        for line in stream:
            parts = line.rstrip('\n').split('\t')
            require(len(parts) == 14, 'memory evidence shape differs')
            if parts[1] == 'w' and int(parts[4], 16) in pcs:
                require(int(parts[9], 16) == 4 and int(parts[11], 16) == 1,
                        'memory evidence width/validity differs')
                result.append([int(parts[n], 16) for n in (4, 10, 12)])
    return result


def clock_phase_states(path, endpoint):
    """Retired phase before the next instruction, not the prior exit observer.

    A block-ending exit is observed BEFORE the source loop retires its local
    work. Reading that exit as the following checkpoint would be a phase bug.
    The final partial block instead uses the actual closed endpoint snapshot.
    """
    active = False
    enters = []
    with path.open(encoding='ascii') as stream:
        for line in stream:
            row = line.split()
            require(len(row) == 15, 'instruction evidence shape differs')
            if row[1] == 'enter' and row[2] == '80379628':
                require(not active, 'unexplained continuous clock retry')
                active = True
            if active and row[1] == 'enter':
                enters.append((int(row[2], 16), int(row[8], 16), int(row[13], 16), int(row[14], 16)))
    require(enters, 'clock phase evidence absent')
    final = (int(endpoint['pc'], 16), int(endpoint['clock_producer']['cycles'], 16),
             int(endpoint['msr'], 16), int(endpoint['clock_producer']['exceptions'], 16))
    return enters[1:] + [final]


def match_region(states, events, begin, end):
    """A missing inner-unit checkpoint cannot borrow a later call's state."""
    require(0 <= begin <= end <= len(states), 'invalid native region bounds')
    cursor = begin
    pairs, missing = [], []
    for event in events:
        found = next((n for n in range(cursor, end) if int(states[n][0], 16) == event['pc']), None)
        if found is None:
            missing.append(event['pc'])
        else:
            pairs.append((found, event))
            cursor = found + 1
    return pairs, missing


def compare_output(output, prefix_output, report, events, original_effects, phases):
    entry, end, _, epoch, _ = checked_inputs(report)
    lines = output.splitlines()
    work = [s.split() for s in lines if s.startswith('SCOPED_SOURCE_WORK ')]
    require(len(work) == 1 and len(work[0]) == 4, 'missing derived elapsed work')
    app_steps, prefix_work, first_cycles = [hex_field(s, 16, 'derived work') for s in work[0][1:]]
    # The relation is independently established by the fresh scheduler source.
    # Actual steps/costs are NOT computed from reference instruction counts.
    require(first_cycles == 20000 + app_steps + prefix_work, 'derived phase relation differs')
    require(hex_field(entry['clock_producer']['cycles'], 16, 'entry cycles') == 20000 + app_steps,
            'apploader-derived entry phase diverges')
    native = [parse_native_state(s) for s in lines if s[:1].isdigit()]
    prefix = [parse_native_state(s) for s in prefix_output.splitlines() if s[:1].isdigit()]
    require(native and prefix and int(prefix[-1][0], 16) == 0x80379628, 'prefix did not reach first clock')
    require('STOP pc=0x80379628 reason=LIVE_TIME_BASE_UNRESOLVED' in prefix_output.splitlines(),
            'production frontier changed')
    require(lines[-1] == 'RESEARCH_STOP pc=0x80373ac4 reason=NO_NATIVE_CLOCK_PROVIDER',
            'research mislabeled as connected boot')
    require([line for line in lines if line.startswith('SOURCE_PENDING_WORK ')] ==
            ['SOURCE_PENDING_WORK 00000004'], 'unretired continuation work lost or charged')
    clocks = [s.split() for s in lines if s.startswith('CLOCK_RESEARCH ')]
    require(len(clocks) == len(native) and all(len(s) == 6 for s in clocks), 'clock checkpoint shape')
    require(len(phases) == len(clocks), 'clock phase occurrence count differs')
    cached = (epoch + first_cycles // 12) & U64
    total = (cached + int(''.join(entry['clock_low_words'][:2]), 16)) & U64
    global_pair = [0, 0]
    for clock, state, (pc, cycles, msr, exceptions) in zip(clocks, native, phases):
        require(int(clock[1], 16) == pc and int(clock[2], 16) == cycles and
                int(state[1], 16) == msr and exceptions == 0 and
                int(clock[3], 16) == cached, f'continuous phase/state divergence at {pc:08x}')
        if pc == 0x80370EAC:
            global_pair[1] = total & 0xffffffff
        elif pc == 0x80370EB0:
            global_pair[0] = total >> 32
        require([int(v, 16) for v in clock[4:]] == global_pair, 'clock global write timeline differs')
    states = prefix + native
    fields = samples = 0
    cut = next(n for n, e in enumerate(events) if e['pc'] == 0x80379628) + 1
    first_pairs, missing = match_region(states, events[:cut], 0, len(prefix))
    tail_pairs, tail_missing = match_region(states, events[cut:], len(prefix), len(states))
    require(missing == PREFIX_INSIDE_COLLAPSED_UNITS and not tail_missing,
            'unexpected unavailable checkpoint granularity')
    for found, event in first_pairs + tail_pairs:
        pc = event['pc']
        state = states[found]
        indexes = [0, 1, 2, 3, 4, 5] + list(range(6, 38)) + [102]
        expected = [pc, event['msr'], event['lr'], event['cr'], event['xer'], event['fpscr']]
        expected += event['gpr'] + [event['ctr']]
        fields += compare_fields([state[n] for n in indexes], [f'{v:08x}' for v in expected],
                                 [FIELD_NAMES[n] for n in indexes], f'continuous occurrence {found}')
        require(event['exceptions'] == 0, 'unexplained exception in continuous trace')
        if found >= len(prefix):
            clock = clocks[found - len(prefix)]
            require(hex_field(clock[2], 16, 'derived sample phase') == event['cycles'],
                    f'first elapsed phase divergence at {pc:08x}')
            samples += 1
        elif pc == 0x80379628:
            require(first_cycles == event['cycles'], 'first clock phase diverges')
            samples += 1
    last = native[-1]
    fields += compare_fields(last[:113], reference_fields(end), FIELD_NAMES, 'closed continuous endpoint')
    fields += compare_fields([last[n] for n in (115, 118, 119)],
                             [end[k] for k in ('l2cr', 'handler_slot', 'lowmem_44')],
                             ['L2CR', 'handler', 'low44'], 'closed endpoint')
    # Retain the validated prefix's byte ownership; never turn unwritten bytes
    # into known zeros. The two added prologue stores target already owned words.
    require(all(last[117][n] == bit for n, bit in enumerate(prefix[-1][117])),
            'unexpected byte ownership change')
    memory = compare_memory(last[116], last[117], end['l2_stack_bytes'], prefix[-1][117],
                            'closed stack endpoint', 0x8060C570)
    memory += compare_memory(last[113], last[114], end['paired_stack_bytes'],
                             prefix[-1][117][120:136], 'paired endpoint', 0x8060C5E8)
    last_clock = clocks[-1]
    final_cycles = hex_field(last_clock[2], 16, 'derived final cycle')
    require(final_cycles == first_cycles + 40 and
            final_cycles == hex_field(end['clock_producer']['cycles'], 16, 'reference final cycle') and
            hex_field(last_clock[3], 16, 'native cached TB') == cached and
            hex_field(end['clock_producer']['cached_tb'], 16, 'reference cached TB') == cached,
            'continuous clock retirement/cached sample mismatch')
    expected_effects = [[0x80370EA8, 0x805F1F54, total & 0xffffffff],
                        [0x80370EAC, 0x805F1F50, total >> 32],
                        [0x80373AB8, 0x8060C5D4, 0x80370EBC],
                        [0x80373ABC, 0x8060C5C8, 0x8060C5D0]]
    effects = [s.split() for s in lines if s.startswith('BOOT_STORE ')]
    require(len(effects) == 4 and all(len(s) == 5 for s in effects), 'word effect shape differs')
    require([[int(v, 16) for v in s[1:4]] for s in effects] == expected_effects and
            all(s[3] == s[4] for s in effects), 'word effect order/readback differs')
    require(original_effects == expected_effects, 'original ordered stores diverge')
    pair = [f'{total >> 32:08x}', f'{total & 0xffffffff:08x}']
    compare_fields(last_clock[4:], pair, ['clock high', 'clock low'], 'closed clock globals')
    compare_fields(end['clock_globals'], pair, ['clock high', 'clock low'], 'reference clock globals')
    current = [s.split() for s in lines if s.startswith('RESEARCH_GLOBAL_RANGE ')]
    prior = [s.split() for s in prefix_output.splitlines() if s.startswith('CURRENT_GLOBAL_RANGE ')]
    require(len(current) == len(prior) == 3, 'current memory owner shape differs')
    for row, old in zip(current, prior):
        require(len(row) == len(old) == 4 and row[1:3] == old[1:3], 'memory owner relabeled')
        base, length = int(row[1], 16), int(row[2], 16)
        expected = bytearray.fromhex(old[3])
        require(len(expected) == length, 'memory owner truncated')
        for address, value in ((0x805F1F50, total >> 32), (0x805F1F54, total & 0xffffffff)):
            if base <= address < base + length:
                expected[address-base:address-base+4] = value.to_bytes(4, 'big')
        require(row[3] == expected.hex(), 'unexplained current owner delta')
    return dict(matched_event_checkpoints=len(events)-len(missing), state_fields=fields,
                full_endpoint_fields=116, known_endpoint_memory_bytes=memory,
                compared_event_elapsed_samples=samples, complete_clock_phase_checkpoints=len(phases),
                ordered_stores=len(effects),
                app_steps=app_steps, prefix_work=prefix_work, first_cycles=first_cycles,
                final_cycles=final_cycles, unresolved_checkpoint_granularity=[f'{v:08x}' for v in missing])


def falsify_output(output, prefix, capture, events, effects, phases):
    """Try corrupting every endpoint field and every clock phase/cache/global."""
    lines = output.splitlines()
    final = max(n for n, line in enumerate(lines) if line[:1].isdigit())
    rejected = 0
    def reject(changed):
        nonlocal rejected
        try:
            compare_output('\n'.join(changed)+'\n', prefix, capture, events, effects, phases)
        except ValueError:
            rejected += 1
        else:
            raise ValueError('corrupted native state/phase was accepted')
    for field in range(113):
        row = lines[final].split(); row[field] = f'{int(row[field],16)^1:0{len(row[field])}x}'
        changed = list(lines); changed[final] = ' '.join(row); reject(changed)
    for n, line in enumerate(lines):
        if line.startswith('CLOCK_RESEARCH '):
            for field in (1, 2, 3, 4, 5):
                row = line.split(); row[field] = f'{int(row[field],16)^1:0{len(row[field])}x}'
                changed = list(lines); changed[n] = ' '.join(row); reject(changed)
        elif line.startswith('SCOPED_SOURCE_WORK '):
            for field in (1, 2, 3):
                row = line.split(); row[field] = f'{int(row[field],16)^1:016x}'
                changed = list(lines); changed[n] = ' '.join(row); reject(changed)
        elif line.startswith('BOOT_STORE '):
            for field in (1, 2, 3, 4):
                row = line.split(); row[field] = f'{int(row[field],16)^1:08x}'
                changed = list(lines); changed[n] = ' '.join(row); reject(changed)
        elif line.startswith('SOURCE_PENDING_WORK '):
            changed = list(lines); changed[n] = 'SOURCE_PENDING_WORK 00000000'; reject(changed)
    return rejected


def validate(args):
    dol_sha256 = pin_original_dol(args.dol)
    root = Path(__file__).resolve().parents[3] / 'build'
    require(args.work.resolve().is_relative_to(root.resolve()), 'output outside repository build')
    args.work.mkdir(parents=True, exist_ok=True)
    capture = json.loads(args.capture.read_text(encoding='utf-8'))
    entry, _, seconds, _, fixture_text = checked_inputs(capture)
    audit_args = [sys.executable, '-B', str(Path(__file__).with_name('agent_timing_source_42.py')),
                  '--source-root', str(args.source_root), '--library', str(args.library),
                  '--capture', str(args.capture), '--logs', str(args.capture.with_name(capture['event_log'])),
                  '--instructions', str(args.capture.with_name(capture['instruction_trace'])),
                  '--mmio', str(args.capture.with_name(capture['mmio_log']))]
    audit = json.loads(subprocess.run(audit_args, capture_output=True, text=True, check=True).stdout)
    require(audit['mmio_access_audit']['environment_bound'], 'unbound device exclusion')
    fixture = args.work / (args.capture.stem + '.entry.txt')
    fixture.write_text(fixture_text, encoding='ascii')
    producer = entry['clock_producer']
    cmd = [str(args.program.resolve()), str(args.dol.resolve()), str(fixture.resolve()),
           f'{seconds:08x}', producer['cpu_hz'], producer['epoch_cycles'], '-',
           ''.join(entry['clock_low_words'][:2]), producer['cached_tb'], producer['exceptions'],
           'research-produced-work']
    native = subprocess.run(cmd, capture_output=True, text=True, check=True).stdout
    prefix = subprocess.run([str(args.prefix_program.resolve()), str(args.dol.resolve()), str(fixture.resolve())],
                            capture_output=True, text=True, check=True).stdout
    (args.work / (args.capture.stem + '.native.txt')).write_text(native, encoding='ascii')
    (args.work / (args.capture.stem + '.prefix.txt')).write_text(prefix, encoding='ascii')
    events = ppc_checkpoints(args.capture.with_name(capture['event_log']))
    original_effects = word_effects(args.capture.with_name(capture['mmio_log']))
    phases = clock_phase_states(args.capture.with_name(capture['instruction_trace']), capture['checkpoints'][-1])
    report = compare_output(native, prefix, capture, events, original_effects, phases)
    report['output_mutation_declines'] = falsify_output(native, prefix, capture, events, original_effects, phases)
    report.update(scope='source-conditioned produced-work RESEARCH; not native clock/event ownership',
                  production_stop='80379628', research_stop='80373ac4',
                  dol_sha256=dol_sha256,
                  capture_sha256=sha256(args.capture), executable_sha256=sha256(args.program),
                  source_audit=audit)
    (args.work / (args.capture.stem + '.validation.json')).write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({k:v for k,v in report.items() if k != 'source_audit'}, indent=2))
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('dol', 'program', 'prefix_program', 'capture', 'source_root', 'library', 'work'):
        parser.add_argument('--' + name.replace('_', '-'), type=Path, required=True)
    validate(parser.parse_args())
