#!/usr/bin/env python3
"""Independent first-Advance journal audit; never supplies live event inputs.

The model predicts only source-owned queue/timer/VI rows. Callback side effects
not emitted by the observer remain unvalidated. No coordinator helper imports.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
from pathlib import Path
import subprocess


SOURCE_HASHES = {
    'Source/Core/Core/CoreTiming.cpp': '742a2eb94f3db96d85f9ba1637ac1eec77e815b9e0bac1a7b0f574e096d6fac1',
    'Source/Core/Core/CoreTiming.h': 'fdfab0fbb4bdc953a946f746012636b6005a96d875d82ad3263543cacaa7ea7b',
    'Source/Core/Core/PowerPC/Gekko.h': 'fad9f1cbb274f63955b44680627bfab542e1940b24b6f6625ee1653436a6020c',
    'Source/Core/Core/HW/HW.cpp': '173015f302d96ba801efe7c47dbf9bcb572955d6ed116ff61d51215325c151a0',
    'Source/Core/Core/HW/SystemTimers.cpp': '578abca48d1ab5273d44bdf337431ff66817352d6862e6c28c79890969962bf7',
    'Source/Core/Core/HW/VideoInterface.cpp': 'f1dc11d057c8a52e47316580713d27d9e8f6458e84726f45e7c648bdbfd92295',
    'Source/Core/Core/HW/ProcessorInterface.cpp': '62310e34368b1d61fff9b9dee7848ab8f1e0125edef4015f8ab3cd3fbb7e6186',
    'Source/Core/Core/HW/AudioInterface.cpp': 'bc698352d4276b63ba99629adf0987b8e8e252b5034ef76e4701717709d76b71',
    'Source/Core/Core/HW/DVD/DVDInterface.cpp': 'b017c9ce65bd788be55c3310d2a0f4cb8687f2a3a7f0aa674131548e45997da1',
    'Source/Core/Core/HW/DSP.cpp': '110d30677b3bf63e0bd8043377a566d7c3625f312ed76c09824f08796b2e62dd',
    'Source/Core/Core/HW/DSP.h': '9e98d77bd6af4d462a39d75e0532102b3fc942d95e76b8400977f2ca53b2aecc',
    'Source/Core/Core/HW/DSPHLE/DSPHLE.cpp': 'e1059fccb22935e2e71dd93090836e1358c0fd8dc9e087b3ebf4da7b2122cc8c',
    'Source/Core/Core/HW/DSPHLE/DSPHLE.h': '4ad76991b0f777779e72bacf2d8050032b21bf2354a6a904503413992987e6e6',
    'Source/Core/Core/HW/DSPHLE/UCodes/ROM.cpp': '16900823dc3d2b70f5413b030869e0507f3f68a3e6a8755101b94b3b00369814',
    'Source/Core/Core/HW/DSPHLE/MailHandler.cpp': '950b42a3ad61480cc54bf26163d7802f471305c8ce52f30a4099585696af789f',
    'Source/Core/Core/HW/DSPHLE/MailHandler.h': '35a94fd4d1fc7dc7302f8208adeade9a79db01d78fa8b0c0e0fe53e0dee41cc5',
    'Source/Core/Core/Movie.cpp': 'c6dc7711039e9c3c271412311bb5b342f31c39dd95300e35f970d15836b0ba39',
    'Source/Core/Core/Movie.h': '6f799ca407acc14f8d0ceab919aad63220ae7f9ce4c8e6a52f9afc52a9ce0fb7',
    'Source/Core/Core/Core.cpp': '8bf9e90fc56b5b4d7a1e42410ea2e2291f44b5d86fb4e982b3aa68912edf5944',
    'Source/Core/Core/AchievementManager.cpp': '687394d3f9601102b200f22048f52ac2bd6dd2eae976ac07eb3f83c6a435f9d2',
    'Source/Core/Core/AchievementManager.h': '689252000d9b1d9b11f1207e9f3bd3f5ba3ba21b13538fe054d52dcee44b37e5',
    'Source/Core/AudioCommon/Mixer.cpp': '24f91152ffe824019053f89acc77a974992fa9ed19e68b0c94194521987f90a3',
    'Source/Core/AudioCommon/Mixer.h': '612025371230d433dcf62e027d1abd2da8be5c3533a8ed2947a0e3b6f9480bd5',
    'Source/Core/AudioCommon/WaveFile.cpp': 'bd9020f3769c3a4df30932cae3fa7efeb5dddd6f5251ce59221d59edba9d4961',
    'Source/Core/VideoCommon/Fifo.cpp': '971d31caf503e18e9435202aa7f576a0ab281630a127e22a42063ab30795a15e',
    'Source/Core/Common/BlockingLoop.h': '410f0fc12388580664481862d8cbc975539b7bfb56ecdef935d07c255adb3043',
    'Source/Core/Core/Config/MainSettings.cpp': '31349025a4a3d4ed7a5b32e9aa6b4d6aa52c522b2bc85b0e909876f685c78c71',
    'Source/Core/Core/Config/AchievementSettings.cpp': 'eaa04213954068ee4e4168259d4a005b0bd9d59d9c60f5b80e15ab6ca9533f79',
    'Source/Core/DolphinQt/MainWindow.cpp': '2e0b9808bd62f123e5153d1392ecdc388f3879f3d69649cdd15c5ece4ab55871',
    'Externals/rcheevos/rcheevos/src/rc_client.c': 'ec6c68ffa4769b786d3ab3f45065d8872ad8b70afc8e15ac1436ee3910935029',
}


def require(ok, why):
    if not ok:
        raise ValueError(why)


def source_gate(root):
    for name, expected in SOURCE_HASHES.items():
        require(hashlib.sha256((root / name).read_bytes()).hexdigest() == expected,
                f'unreviewed source: {name}')
    return len(SOURCE_HASHES)


KINDS = {'init', 'init-ready', 'enqueue', 'advance-enter', 'advance-clock',
         'dispatch', 'callback-return', 'vt-update', 'vt-rebase', 'vt-advanced',
         'advance-exit', 'schedule-clock', 'clock-read', 'config', 'adjust',
         'cancel', 'force-enter', 'force-exit'}
NATIVE_KINDS = {'init', 'init-ready', 'enqueue', 'advance-enter', 'advance-clock',
                'dispatch', 'callback-return', 'vt-update', 'vt-rebase',
                'vt-advanced', 'advance-exit'}


def expected_rows():
    """Finite equations from fresh GC source, not observed totals or rows."""
    out = []
    hz, unit, pc = 486_000_000, 0x3f800000, 0x81200258
    ais = 1124 * 2
    aid = ais * 3 // 2
    half = (2 * hz // 27_000_000) * 429
    fields = 3 * 6 + 503 + 4
    dma = hz * aid // (108_000_000 * 4 // 32)
    dtk = hz * 6 * 28 * ais // 108_000_000
    userdata = (3 << 32) | 1
    def row(kind, name='-', *v):
        out.append((kind, name, tuple(v) + (0,) * (8-len(v))))
    def clock(kind, g, length, down, sane, fifo, p=pc, extra=0):
        row(kind, '-', g, length, down, sane, fifo, p, unit, extra)
    def enqueue(name, deadline, fifo, data=0, relative=None):
        row('enqueue', name, deadline, fifo, data,
            deadline if relative is None else relative)
    def initial_enqueue(name, deadline, fifo, data=0):
        clock('schedule-clock', 0, 20000, 0, 1, fifo, p=0, extra=deadline)
        clock('clock-read', 0, 20000, 0, 1, fifo, p=0)
        enqueue(name, deadline, fifo, data)
    row('init', '-', 0, 20000, 0, 1, 0, 0, 0, 0)
    # Initial RefreshConfig calls GetTicks before cached inverse is installed.
    row('clock-read', '-', 0, 20000, 0, 1, 0, 0, 0, 0)
    row('config', '-', unit, unit)
    clock('init-ready', 0, 20000, 0, 1, 0, p=0)
    row('adjust', '-', hz, hz)
    initial_enqueue('FinishExecutingCommand', 0, 0, userdata)
    clock('clock-read', 0, 20000, 0, 1, 1, p=0)
    row('cancel', '<null>')
    # Cached TB and decrementer origins each consume one zero tick getter.
    clock('clock-read', 0, 20000, 0, 1, 1, p=0)
    clock('clock-read', 0, 20000, 0, 1, 1, p=0)
    initial_enqueue('GPUSleeper', 0, 1)
    initial_enqueue('VICallback', half, 2)
    initial_enqueue('DSPCallback', 0, 3)
    initial_enqueue('AudioDMACallback', dma, 4)
    initial_enqueue('PatchEngine', half * fields, 5)
    clock('clock-read', 0, 20000, 0, 1, 6, p=0)
    row('config', '-', unit, unit)
    clock('clock-read', 0, 20000, 0, 1, 6, p=0)
    row('config', '-', unit, unit)
    clock('force-enter', 0, 20000, 0, 1, 6, p=0, extra=50)
    clock('force-exit', 0, 20000, 0, 1, 6, p=0)
    clock('advance-enter', 0, 20000, 0, 1, 6)
    clock('advance-clock', 20000, 20000, 0, 1, 6, extra=20000)
    queue = [(0, 0, 'FinishExecutingCommand', userdata),
             (0, 1, 'GPUSleeper', 0), (half, 2, 'VICallback', 0),
             (0, 3, 'DSPCallback', 0), (dma, 4, 'AudioDMACallback', 0),
             (half * fields, 5, 'PatchEngine', 0)]
    fifo = 6
    for time, old_fifo, name, data in sorted(queue):
        if time > 20000:
            continue
        late = 20000-time
        row('dispatch', name, time, old_fifo, data, late)
        if name == 'VICallback':
            clock('clock-read', 20000, 20000, 0, 1, fifo, extra=20000)
            for kind, line in [('vt-update', 0), ('vt-rebase', 0), ('vt-advanced', 1)]:
                row(kind, '-', time, line, 15, 525, 525, pc)
            period = half
        elif name == 'FinishExecutingCommand':
            period = dtk
        else:
            period = hz // 1000
        relative = period-late
        clock('schedule-clock', 20000, 20000, 0, 1, fifo, extra=relative)
        clock('clock-read', 20000, 20000, 0, 1, fifo, extra=20000)
        enqueue(name, 20000+relative, fifo, data, relative)
        fifo += 1
        row('callback-return', name, time, old_fifo, data)
    length = 2*half-20000
    clock('advance-exit', 20000, length, length, 0, fifo)
    return out


def load_journal(path, lines=None):
    rows, boundaries, active, done = [], [], False, False
    last_seq, final, headers = -1, None, 0
    for line in path.open(encoding='utf-8') if lines is None else lines:
        require(not line.startswith('LIMIT'), 'truncated event sink')
        f = line.rstrip('\n').split('\t')
        require(len(f) == 16, 'wrong event schema')
        seq = int(f[0])
        require(seq > last_seq, 'event sequence is not increasing')
        last_seq = seq
        v = tuple(int(x, 16) for x in f[4:12])
        if f[2] == 'event-filter':
            headers += 1
            require(f[3].startswith('kinds=;window=init..timing-prefix-end@80373ac4;'),
                    'filtered/incomplete event lifecycle')
        if f[2] == 'init':
            require(not active and not done, 'second initialization')
            active = True
        if active and f[2] in KINDS:
            rows.append((f[2], f[3], v))
        if active:
            require(f[2] not in {'ts-publish', 'ts-commit', 'ts-move', 'cancel-item', 'adjust-item'},
                    'unowned first-Advance queue ingress/mutation')
        if active and f[2] == 'advance-exit':
            active, done = False, True
        if f[2] == 'boundary':
            boundaries.append(v)
        final = (f[2], v)
    require(headers == 1, 'missing/repeated evidence filter header')
    require(done and not active, 'missing complete first Advance')
    require(final is not None and final[0] == 'timing-prefix-end' and
            final[1][0] == 0x80373ac4, 'missing terminal evidence')
    require([b[0] for b in boundaries] == [0x80003154, 0x80003170],
            'missing explicit PI boundary observations')
    require(all(b[7] == 0x10000 for b in boundaries), 'later PI cause differs')
    return rows, boundaries


def completeness_mutations(path):
    rejected = 0
    for missing in ['event-filter', 'timing-prefix-end']:
        with path.open(encoding='utf-8') as stream:
            lines = (line for line in stream if line.split('\t', 3)[2] != missing)
            try:
                load_journal(path, lines)
            except ValueError:
                rejected += 1
            else:
                raise AssertionError('incomplete evidence admitted')
    return rejected


def compare(rows):
    expected = expected_rows()
    require(len(rows) == len(expected), 'first-Advance lifecycle row count differs')
    for i, (got, want) in enumerate(zip(rows, expected)):
        require(got == want, f'row {i}: {got!r} != {want!r}')
    return len(expected) * 8


def mutation_tests(rows):
    n = 0
    for i in range(len(rows)):
        for col in range(8):
            changed = list(rows)
            kind, name, values = changed[i]
            values = list(values)
            values[col] ^= 1
            changed[i] = (kind, name, tuple(values))
            try:
                compare(changed)
            except ValueError:
                n += 1
            else:
                raise AssertionError('state mutation admitted')
    for i in range(len(rows)):
        changed = rows[:i] + rows[i+1:]
        try:
            compare(changed)
        except ValueError:
            n += 1
        else:
            raise AssertionError('missing lifecycle row admitted')
    # A FIFO-only comparator wrongly dispatches VI before the overdue DSP.
    dispatch = [r[1] for r in rows if r[0] == 'dispatch']
    require(dispatch == ['FinishExecutingCommand', 'GPUSleeper', 'DSPCallback', 'VICallback'],
            'deadline/FIFO ordering changed')
    # Equal emitted timing rows cannot establish absent Movie/host work.
    def movie(frame, lag, polled, recording):
        frame = (frame + 1) & ((1 << 64)-1)
        lag = (lag + (not polled)) & ((1 << 64)-1)
        return frame, lag, False, (frame, lag) if recording else (0, 0)
    require(movie(0, 0, False, False) != movie(0, 0, True, False),
            'Movie polled perturbation failed')
    require(movie(0, 0, False, False) != movie(0, 0, False, True),
            'Movie recording perturbation failed')
    return n, 2


def native_expected(bound):
    result = []
    def add(kind, *v):
        result.append((kind, '-', tuple(v)+(0,)*(8-len(v))))
    for kind, name, v in expected_rows():
        if kind not in NATIVE_KINDS:
            continue
        values = list(v)
        if kind in {'init', 'init-ready', 'advance-enter', 'advance-clock', 'advance-exit'} or kind.startswith('vt-'):
            values[5] = 0  # C++ deliberately does not own apploader PC.
        result.append((kind, name, tuple(values)))
        if kind == 'dispatch' and name == 'FinishExecutingCommand':
            add('dtk-transfer-zero', 0, 0)
            add('dtk-zero-sample-request', 0, 0, 0)
            if not bound:
                add('stop')
                result[-1] = ('stop', 'Mixer.cpp:253 live-dtk-log', result[-1][2])
                return result
            add('dtk-no-wave-write', 0)
            add('dtk-pending-blocks', 6)
        if kind == 'dispatch' and name == 'GPUSleeper':
            add('gpu-allow-sleep-request', 1)
            add('gpu-allow-sleep-delivered', 1)
        if kind == 'dispatch' and name == 'DSPCallback':
            add('dsp-rom-update', 486000-20000, 0x8071feed)
        if kind == 'vt-update':
            add('movie-frame-update', 1, 1, 0)
            add('new-field-no-step', 0)
            add('achievement-return', 0, 0)
        if kind == 'vt-advanced':
            add('pi-vi-update', 0x10000, 0, 0)
        if kind == 'advance-exit':
            add('external-exception-return', 0)
    return result


def compare_native(dump, bound):
    require(dump['schema'] == 'initial-boot-events-42-v1' and dump['scope'] == 'research-only',
            'native scope/schema differs')
    require(dump['control'] == ('declared-bound-control' if bound else 'unbound-reference'),
            'native control provenance differs')
    rows = [(r['kind'], r['name'], tuple(int(v, 16) for v in r['v'])) for r in dump['journal']]
    require(rows == native_expected(bound), 'native source-derived ordered journal differs')
    queue = []
    for kind, name, values in native_expected(bound):
        event = {'name': name, 'deadline': values[0], 'fifo': values[1],
                 'userdata': f'{values[2]:016x}'}
        if kind == 'enqueue':
            queue.append(event)
            queue.sort(key=lambda e: (e['deadline'], e['fifo']))
        if kind == 'dispatch':
            require(queue and queue.pop(0) == event, 'independent queue prediction is inconsistent')
    require(dump['queue'] == queue, 'native pending queue differs')
    require(dump['pi'] == {'cause': 0x10000 if bound else 0x10100, 'mask': 0, 'exceptions': 0},
            'native PI state differs')
    require(dump['dvd'] == {'dimar': 0, 'dilength': 0, 'stream': 0,
                           'pending_blocks': 6 if bound else 0, 'decoded_blocks': 0,
                           'push_calls': 1, 'streaming_frames': 0}, 'native DTK partial effects differ')
    require(dump['scheduler'] == {'global': 20000, 'slice': 10888 if bound else 20000,
                                  'downcount': 10888 if bound else 0,
                                  'sane': 0 if bound else 1, 'next_fifo': 10 if bound else 6,
                                  'inverse_bits': 0x3f800000}, 'native scheduler partial phase differs')
    require(dump['movie'] == {'frame': 1 if bound else None, 'lag': 1 if bound else None,
                              'polled': 0 if bound else None, 'total_frames': 0 if bound else None,
                              'total_lag': 0 if bound else None}, 'native unknown Movie state differs')
    require(dump['ai'] == {'playing': 0, 'ais_divisor': 2248, 'aid_divisor': 3372},
            'native AI source initialization differs')
    require(dump['dsp'] == {'hle_rom': 1, 'dma_enabled': 0, 'mail_halted': 1,
                            'rom_mail': 0x8071feed, 'slice': 0,
                            'update_calls': 1 if bound else 0,
                            'last_update_cycles': 466000 if bound else 0},
            'native retained ROM state differs')
    require(dump['vi'] == {'half_line': 1 if bound else 0, 'next_si_poll': 15,
                           'last_line_start': 0, 'odd_first': 520, 'odd_last': 519,
                           'even_first': 1045, 'even_last': 1044}, 'native VI state differs')
    require(dump['effects'] == {'gpu_sleep_delivered': int(bound),
                                'gpu_allow_sleep_calls': int(bound),
                                'new_field_calls': int(bound),
                                'achievement_return_calls': int(bound),
                                'guest_ram_write_bytes': 0}, 'native ordered callback effects differ')
    if bound:
        require(dump['stop'] == 'first-advance-complete' and dump['active_callback'] is None,
                'native bound control did not finish')
    else:
        require(dump['stop'] == 'Mixer.cpp:253 live-dtk-log' and
                dump['active_callback'] == {'name': 'FinishExecutingCommand', 'deadline': 0,
                                            'fifo': 0, 'userdata': '0000000300000001'},
                'native unknown read consumed later state')
    return len(rows)


def audit_native(executable, falsify):
    result = []
    for bound, option in [(False, '--dump'), (True, '--dump-bound-control')]:
        run = subprocess.run([str(executable.resolve()), option], check=True, text=True, capture_output=True)
        dump = json.loads(run.stdout)
        count = compare_native(dump, bound)
        rejected = 0
        if falsify:
            for i, row in enumerate(dump['journal']):
                for col in range(8):
                    changed = copy.deepcopy(dump)
                    value = int(changed['journal'][i]['v'][col], 16)
                    changed['journal'][i]['v'][col] = f'{value ^ 1:016x}'
                    try:
                        compare_native(changed, bound)
                    except ValueError:
                        rejected += 1
                    else:
                        raise AssertionError('native journal mutation admitted')
            for group in ['pi', 'dvd', 'scheduler', 'movie', 'ai', 'dsp', 'vi', 'effects']:
                for key, value in dump[group].items():
                    changed = copy.deepcopy(dump)
                    changed[group][key] = 0 if value is None else int(value) ^ 1
                    try:
                        compare_native(changed, bound)
                    except ValueError:
                        rejected += 1
                    else:
                        raise AssertionError('native partial state mutation admitted')
            for i, event in enumerate(dump['queue']):
                for key in ['deadline', 'fifo', 'userdata']:
                    changed = copy.deepcopy(dump)
                    value = changed['queue'][i][key]
                    changed['queue'][i][key] = f'{int(value,16)^1:016x}' if key == 'userdata' else value ^ 1
                    try:
                        compare_native(changed, bound)
                    except ValueError:
                        rejected += 1
                    else:
                        raise AssertionError('native queue mutation admitted')
        result.append({'executable_sha256': hashlib.sha256(executable.read_bytes()).hexdigest(),
                       'declared_bound_control': bound, 'predicted_journal_rows': count,
                       'mutation_rejections': rejected})
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, required=True)
    parser.add_argument('--event-log', type=Path, action='append', required=True)
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('--native-executable', type=Path)
    args = parser.parse_args()
    gates = source_gate(args.source_root)
    results = []
    for path in args.event_log:
        rows, boundaries = load_journal(path)
        checked = compare(rows)
        mutations, ambiguities = mutation_tests(rows) if args.self_test else (0, 0)
        incomplete = completeness_mutations(path) if args.self_test else 0
        results.append({'path': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                        'first_advance_rows': len(rows), 'fields_checked': checked,
                        'mutation_rejections': mutations, 'unobserved_movie_counterexamples': ambiguities,
                        'incomplete_evidence_rejections': incomplete,
                        'later_pi_cause_observations': len(boundaries)})
    native = audit_native(args.native_executable, args.self_test) if args.native_executable else None
    print(json.dumps({'source_file_gates': gates, 'profiles': results, 'native_research_controls': native,
                     'status': 'SOURCE_QUEUE_VI_SUBSET_PARITY_ONLY',
                     'full_callback_state_parity': False,
                     'native_production_admission': False,
                     'earliest_unbound_effect_read': 'Mixer.cpp:253 m_log_dtk_audio',
                     'unobserved': ['DTK mixer/log state', 'GPU worker atomic effect',
                                    'DSP ROM mailbox/slice state', 'Movie counters/play mode',
                                    'frame-step state/ingress', 'achievement client/DLL state',
                                    'PI cause/mask/exceptions immediately around first callback']}, indent=2))


if __name__ == '__main__':
    main()
