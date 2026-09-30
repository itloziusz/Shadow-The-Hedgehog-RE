"""Bounded PAL CRT return-state check against raw DOL words and HLE capture.

This is an offline proof check for the reached zero-fill path. It does not
execute or claim native CRT parity, and the captured return values are test
expectations rather than inputs to the C++ boot implementation.
"""

from __future__ import annotations

import hashlib
import pathlib
import sys


DOL_SHA256 = "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af"


def word(dol: bytes, va: int) -> int:
    # The checked PAL DOL text0 mapping is VA 0x80003100 -> file 0x100.
    offset = va - 0x80003100 + 0x100
    if va & 3 or offset < 0 or offset + 4 > len(dol):
        raise ValueError(f"invalid PAL text0 word address {va:08X}")
    return int.from_bytes(dol[offset : offset + 4], "big")


def project_last_fill(dol: bytes) -> tuple[int, int, int, int]:
    # The walker passes (destination, 0, size) to this exact wrapper/leaf.
    fingerprints = {
        0x80003158: 0x480002A9,  # bl hardware wrapper; LR=0x8000315C
        0x8000340C: 0x7FE802A6,  # mflr r31, retained through wrapper
        0x8000334C: 0x93E10014,  # walker saves the incoming r31
        0x800033E8: 0x83E10014,  # walker restores that r31
        0x800033D4: 0x38800000,  # li r4,0 before each fill
        0x80005490: 0x3886FFFD,  # addi r4,r6,-3; r6 was destination-1
        0x800054B8: 0x94E40020,  # stwu r7,0x20(r4)
        0x800054CC: 0x94E40004,  # stwu r7,4(r4)
    }
    for address, expected in fingerprints.items():
        actual = word(dol, address)
        if actual != expected:
            raise AssertionError(f"word {address:08X}: {actual:08X} != {expected:08X}")

    entries: list[tuple[int, int]] = []
    for va in range(0x800055C8, 0x800055E8, 8):
        destination, size = word(dol, va), word(dol, va + 4)
        if size == 0:
            break
        if size < 32 or (destination | size) & 3:
            raise AssertionError("PAL zero descriptor no longer follows checked word path")
        entries.append((destination, size))
    if len(entries) != 3:
        raise AssertionError("PAL zero descriptor count changed")

    destination, size = entries[-1]
    # r4 starts at destination-4 and is advanced by the reached updating
    # stores. The last address stays in r4 when the tail branch returns.
    r4 = destination - 4
    r4 += (size >> 5) * 32
    r4 += ((size >> 2) & 7) * 4
    r6 = destination + size - 1
    r31 = 0x80003158 + 4  # branch link, saved/restored by the checked words
    return destination, r4, r6, r31


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_crt_return_projection.py <read-only-main.dol>")
    dol = pathlib.Path(sys.argv[1]).read_bytes()
    if hashlib.sha256(dol).hexdigest() != DOL_SHA256:
        raise AssertionError("PAL GUPP8P main.dol SHA-256 mismatch")
    actual = project_last_fill(dol)
    # Independent synthetic/HLE checkpoint at PC 0x80003170; not a retail
    # claim. Both registers were previously documented incorrectly.
    expected = (0x805FC540, 0x805FC5E8, 0x805FC5EB, 0x8000315C)
    if actual != expected:
        raise AssertionError(f"CRT return projection {actual!r} != {expected!r}")

    changed = bytearray(dol)
    descriptor_size_offset = 0x800055DC - 0x80003100 + 0x100
    changed[descriptor_size_offset : descriptor_size_offset + 4] = (0xA8).to_bytes(4, "big")
    if project_last_fill(changed) == expected:
        raise AssertionError("changed last fill length escaped the return-state gate")
    print("PASS PAL CRT return r4/r6/r31 projection and negative descriptor case")


if __name__ == "__main__":
    main()
