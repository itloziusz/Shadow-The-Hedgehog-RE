"""GameCube DOL loader for the Shadow the Hedgehog gameplay RE toolkit.

Provides virtual-address reads over main.dol, section classification and
helpers for reading big-endian values / C strings.  Every other tool builds
on this module.  Fails closed: reading an address that is not backed by a
DOL section (or is BSS) raises / returns None instead of guessing.
"""
import os
import struct

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))
DOL_PATH = os.path.join(ROOT, 'sys', 'main.dol')
DATA_DIR = os.path.normpath(os.path.join(HERE, '..', 'data'))

# Section roles established from the DOL header + CodeWarrior layout
# conventions (verified in GAMEPLAY_DOL_MAP.md):
SECTION_NAMES = {
    'T0': '.init', 'T1': '.text',
    'D0': 'extab', 'D1': 'extabindex', 'D2': '.ctors', 'D3': '.dtors',
    'D4': '.rodata', 'D5': '.data', 'D6': '.sdata', 'D7': '.sdata2',
}


class Section:
    __slots__ = ('key', 'name', 'file_off', 'addr', 'size', 'is_text')

    def __init__(self, key, file_off, addr, size, is_text):
        self.key = key
        self.name = SECTION_NAMES.get(key, key)
        self.file_off = file_off
        self.addr = addr
        self.size = size
        self.is_text = is_text

    @property
    def end(self):
        return self.addr + self.size

    def __repr__(self):
        return '<%s %s %08X-%08X>' % (self.key, self.name, self.addr, self.end)


class Dol:
    def __init__(self, path=DOL_PATH):
        with open(path, 'rb') as f:
            self.raw = f.read()
        d = self.raw
        offs = struct.unpack('>18I', d[0:0x48])
        addrs = struct.unpack('>18I', d[0x48:0x90])
        sizes = struct.unpack('>18I', d[0x90:0xD8])
        self.bss_addr, self.bss_size, self.entry = struct.unpack('>3I', d[0xD8:0xE4])
        self.sections = []
        for i in range(18):
            if sizes[i] == 0:
                continue
            key = ('T%d' % i) if i < 7 else ('D%d' % (i - 7))
            self.sections.append(Section(key, offs[i], addrs[i], sizes[i], i < 7))
        self.sections.sort(key=lambda s: s.addr)
        self.text = [s for s in self.sections if s.is_text]
        self._by_name = {s.name: s for s in self.sections}

    def section(self, name):
        return self._by_name[name]

    def find(self, addr):
        for s in self.sections:
            if s.addr <= addr < s.end:
                return s
        return None

    def in_bss(self, addr):
        # bss region includes .sbss/.sbss2 which CodeWarrior interleaves
        # between .sdata/.sdata2; anything in [bss, bss+size) not backed
        # by a section is zero-initialised storage.
        return (self.bss_addr <= addr < self.bss_addr + self.bss_size
                and self.find(addr) is None)

    def is_code(self, addr):
        s = self.find(addr)
        return s is not None and s.is_text

    def region(self, addr):
        s = self.find(addr)
        if s is not None:
            return s.name
        if self.in_bss(addr):
            if addr >= 0x805E4500:
                return '.sbss'
            return '.bss'
        return None

    def read(self, addr, n):
        s = self.find(addr)
        if s is None or addr + n > s.end:
            return None
        o = s.file_off + (addr - s.addr)
        return self.raw[o:o + n]

    def u32(self, addr):
        b = self.read(addr, 4)
        return None if b is None else struct.unpack('>I', b)[0]

    def s32(self, addr):
        b = self.read(addr, 4)
        return None if b is None else struct.unpack('>i', b)[0]

    def u16(self, addr):
        b = self.read(addr, 2)
        return None if b is None else struct.unpack('>H', b)[0]

    def u8(self, addr):
        b = self.read(addr, 1)
        return None if b is None else b[0]

    def f32(self, addr):
        b = self.read(addr, 4)
        return None if b is None else struct.unpack('>f', b)[0]

    def f64(self, addr):
        b = self.read(addr, 8)
        return None if b is None else struct.unpack('>d', b)[0]

    def cstr(self, addr, maxlen=512):
        s = self.find(addr)
        if s is None:
            return None
        o = s.file_off + (addr - s.addr)
        end = self.raw.find(b'\0', o, min(o + maxlen, s.file_off + s.size))
        if end < 0:
            return None
        return self.raw[o:end]

    def printable_str(self, addr, minlen=3):
        """Return decoded string if addr points at a plausible C string."""
        b = self.cstr(addr)
        if b is None or len(b) < minlen:
            return None
        ok = sum(1 for c in b if 0x20 <= c < 0x7F or c in (9, 10, 13))
        if ok != len(b):
            # allow Shift-JIS text (Japanese debug strings)
            try:
                t = b.decode('shift_jis')
            except UnicodeDecodeError:
                return None
            if any(ord(ch) < 0x20 and ch not in '\t\r\n' for ch in t):
                return None
            return t
        return b.decode('ascii')


_DOL = None


def get_dol():
    global _DOL
    if _DOL is None:
        _DOL = Dol()
    return _DOL


if __name__ == '__main__':
    d = get_dol()
    print('entry %08X  bss %08X+%X' % (d.entry, d.bss_addr, d.bss_size))
    for s in d.sections:
        print('  %-3s %-11s file %07X  %08X-%08X  size %X' % (
            s.key, s.name, s.file_off, s.addr, s.end, s.size))
