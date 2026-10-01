"""Read-only raw/source/late-capture audit of the first three apploader words.

This is a finite evidence checker, not an interpreter or native state owner.
The late DOL snapshot is reported separately from derived Entry effects.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

INPUT_SHA = {
    'apploader.img': '8a1fe9487b029b2518a6633b1e0eb9ec0922f5f5eb5afdd7eba1267a85f096fe',
    'boot.bin': '7d387ce9162342a93a3becac1d2df6db1cccf6a2439f3fc97b29b2f7047d8e88',
    'bi2.bin': '8bbadffc2a7f79eda5982aa2fe30831969c2702aa9206f621b930aae0b70af6b',
}
SOURCE_SHA = {
    'Source/Core/Core/Boot/Boot.cpp': '2245b1ab41fa4ab092d26416016c05d52b5591c9035f2ed4b1e3ffe8d256daaa',
    'Source/Core/Core/Boot/Boot_BS2Emu.cpp': '8c204a36bd7fe7171d6c8e03e7f07739c11297e483dd6180c6ff7391973ec06d',
    'Source/Core/Core/ConfigManager.cpp': '3418e4660cfd4baf1d8d570e6dd7bc18779499c7310ad45684da1914413b1912',
    'Source/Core/Core/PowerPC/Interpreter/Interpreter_LoadStore.cpp': 'dbfc463df0e83aa989e7728bec097d3d95f05ec2e0fa2a409291e3b985234553',
    'Source/Core/Core/PowerPC/Interpreter/Interpreter_SystemRegisters.cpp': '698f153ad3a9e457c7d51f9a84358b86140c29b879f35dc39fe70412f23f8556',
    'Source/Core/Core/PowerPC/MMU.cpp': 'ffc04c07b33b2ff77f1d72f9d6e09dca65c042d94a970e0e570755103aef9d78',
    'Source/Core/Core/PowerPC/PowerPC.cpp': 'cd0de27b7b7359674723a18796fa1032ffd7728a1b177a77d6fefd9376463341',
    'Source/Core/Core/PowerPC/PowerPC.h': '74beb71dd8f8246ace89a463914eb2290faa6b309deb8f22059b8ed92b5fd675',
    'Source/Core/Core/Config/MainSettings.cpp': '31349025a4a3d4ed7a5b32e9aa6b4d6aa52c522b2bc85b0e909876f685c78c71',
    'Source/Core/Core/HW/Memmap.cpp': 'c8b5b5ea180c73f54736b6835e20d8027d0203cda7641349254fbce99bb2747f',
    'Source/Core/DiscIO/VolumeGC.cpp': '6e0308c9a5206ff291e4dca442f476e0c76bc924d4e8cdb68fead2075ad95799',
    'Source/Core/DiscIO/VolumeDisc.cpp': '62d2cd35541c07e42518c2e7b611619d6d0bcf18b6d514baad8aff306fadd5e6',
    'Source/Core/DiscIO/Enums.cpp': '67fa886e5bc14f2cd2df424cff5c5b453a1704e64524384aee4566944bbb6ba4',
    'Source/Core/Core/HW/DVD/DVDInterface.cpp': 'b017c9ce65bd788be55c3310d2a0f4cb8687f2a3a7f0aa674131548e45997da1',
}
WORDS = {0x81200258: 0x7c0802a6, 0x8120025c: 0x9421fff8,
         0x81200260: 0x9001000c}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def checked(data, expected, what):
    actual = digest(data)
    if actual != expected:
        raise ValueError(f'{what} identity differs: {actual}')
    return actual


def verify_words(app):
    if len(app) < 0x284:
        raise ValueError('first-three raw input is truncated')
    raw_words = {f'{pc:08x}': f'{struct.unpack_from(">I", app, 0x20 + pc - 0x81200000)[0]:08x}'
                 for pc in WORDS}
    for pc, word in WORDS.items():
        if int(raw_words[f'{pc:08x}'], 16) != word:
            raise ValueError('first-three word gate differs')
    return raw_words


def falsify(blobs, source):
    def rejects(operation):
        try:
            operation()
        except ValueError:
            return
        raise AssertionError('negative gate accepted a mutation')
    count = 0
    for pc in WORDS:
        mutated = bytearray(blobs['apploader.img'])
        mutated[0x20 + pc - 0x81200000 + 3] ^= 1
        rejects(lambda: verify_words(mutated))
        count += 1
    rejects(lambda: verify_words(blobs['apploader.img'][:0x283]))
    count += 1
    for name, blob in blobs.items():
        mutated = bytes([blob[0] ^ 1]) + blob[1:]
        rejects(lambda: checked(mutated, INPUT_SHA[name], name))
        count += 1
    name = 'Source/Core/Core/Boot/Boot_BS2Emu.cpp'
    blob = (source / name).read_bytes()
    rejects(lambda: checked(bytes([blob[0] ^ 1]) + blob[1:], SOURCE_SHA[name], name))
    return count + 1


def audit(inputs, source, capture):
    blobs = {n: (inputs / n).read_bytes() for n in INPUT_SHA}
    hashes = {n: checked(b, INPUT_SHA[n], n) for n, b in blobs.items()}
    pins = {n: checked((source / n).read_bytes(), h, n) for n, h in SOURCE_SHA.items()}
    app = blobs['apploader.img']
    entry, code_size, trailer = struct.unpack_from('>III', app, 0x10)
    if (entry, code_size, trailer) != (0x81200258, 0x1a98, 0x1c3a0):
        raise ValueError('apploader load header differs')
    raw_words = verify_words(app)
    negative_gates = falsify(blobs, source)
    region = struct.unpack_from('>I', blobs['bi2.bin'], 0x18)[0]
    if region != 2:
        raise ValueError('original GC volume region is not PAL')
    data = capture.read_bytes()
    j = json.loads(data)
    late = j['original_unmodified_entry']
    if late['pc'].lower() != '80003154':
        raise ValueError('late capture is not the original DOL entry')
    stack_base = int(late['stack_window_address'], 16)
    stack = bytes.fromhex(late['stack_window'])
    late_stack = {f'{stack_base + n:08x}': f'{struct.unpack_from(">I", stack, n)[0]:08x}'
                  for n in range(0, len(stack), 4)}
    gprs = bytes.fromhex(late['gpr'])
    r1 = struct.unpack_from('>I', gprs, 4)[0]
    # Explicit consequences of three gated statements. No opcode dispatcher.
    initial_sp = 0x815edca8
    initial_lr = 0
    frame_sp = (initial_sp - 8) & 0xffffffff
    derived = [
        dict(pc='81200258', statement='r0 = incoming LR', r0=f'{initial_lr:08x}', stores=[]),
        dict(pc='8120025c', statement='store old r1; update r1 only without DSI',
             r1=f'{frame_sp:08x}', stores=[dict(logical=f'{frame_sp:08x}',
             physical=f'{frame_sp & 0x3fffffff:08x}', bytes=f'{initial_sp:08x}')]),
        dict(pc='81200260', statement='store r0 at new r1 + 12',
             stores=[dict(logical=f'{frame_sp + 12:08x}', physical=f'{(frame_sp + 12) & 0x3fffffff:08x}',
                          bytes=f'{initial_lr:08x}')]),
    ]
    # The 42d observer logs only the post-apploader DOL state; do not admit it
    # as a receipt of initial Entry registers or an ordered store journal.
    direct_entry_capture = any(int(s['pc'], 16) in (entry, entry + 4, entry + 8, entry + 12)
                               for s in [late] + j['checkpoints'])
    return dict(scope='source-conditioned first-three-word evidence; no native state owner',
        input_sha256=hashes, source_sha256=pins, capture_sha256=digest(data),
        raw_words=raw_words, pal_region_bi2_18=region,
        apploader_load=dict(physical='01200000', length=code_size + trailer,
                           end=f'{0x01200000 + code_size + trailer:08x}'),
        source_initial_registers=dict(pc=f'{entry:08x}', lr='00000000',
                                      r1=f'{initial_sp:08x}', r3='80003100',
                                      r4='80003104', r5='80003108'),
        derived_no_dsi_effects=derived,
        late_dol_observation=dict(pc=late['pc'], r1=f'{r1:08x}', lr=late['lr'], stack=late_stack),
        directly_observed_first_three=direct_entry_capture, evidence_negative_gates=negative_gates,
        admission='UNKNOWN: require first-three original register/store observations and native producers')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--inputs', type=Path, required=True)
    p.add_argument('--source-root', type=Path, required=True)
    p.add_argument('--capture', type=Path, required=True)
    p.add_argument('--report', type=Path)
    a = p.parse_args()
    result = audit(a.inputs, a.source_root, a.capture)
    if a.report:
        build = Path(__file__).resolve().parents[3] / 'build'
        if not a.report.resolve().is_relative_to(build.resolve()):
            raise ValueError('report must stay inside repository build')
        a.report.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('PASS raw first-three gates; 14 source pins; PAL SP/LR source derivation')
    print('Evidence-checker negative gates:', result['evidence_negative_gates'])
    print('Late DOL stack:', result['late_dol_observation']['stack'])
    print('First-three direct capture:', result['directly_observed_first_three'])
    print(result['admission'])


if __name__ == '__main__':
    main()
