"""Reproduce the archived startup-only PAL oracle disc from read-only SYS inputs.

The output contains no ordinary game-file payload and belongs under ignored
build/. It is reference input, never a dependency of the native executable.
"""

import argparse
import ctypes
import hashlib
import os
from pathlib import Path


DISC_SIZE = 0x57058000
DISC_SHA256 = "a8a526ef7006a2d2a8d41f536ef30b208ff197c185c05e9b2276e6e93cb41a8e"
INPUTS = (
    (0, "boot.bin", "7d387ce9162342a93a3becac1d2df6db1cccf6a2439f3fc97b29b2f7047d8e88"),
    (0x440, "bi2.bin", "8bbadffc2a7f79eda5982aa2fe30831969c2702aa9206f621b930aae0b70af6b"),
    (0x2440, "apploader.img", "8a1fe9487b029b2518a6633b1e0eb9ec0922f5f5eb5afdd7eba1267a85f096fe"),
    (0x20300, "main.dol", "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af"),
    (0x5A1A00, "fst.bin", "0ceb019b93db37a0b359f0918cd97c262069638fcab62726684d268c09a64db0"),
)


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def make_disc(sys_dir, output):
    build = Path(__file__).resolve().parents[3] / "build"
    output = output.resolve()
    if not output.is_relative_to(build.resolve()):
        raise ValueError("oracle output must stay under repository build/")
    chunks = []
    for offset, name, expected in INPUTS:
        content = (sys_dir / name).read_bytes()
        if hashlib.sha256(content).hexdigest() != expected:
            raise ValueError(f"read-only {name} SHA-256 mismatch")
        chunks.append((offset, content))
    if output.exists():
        if output.stat().st_size != DISC_SIZE or digest(output) != DISC_SHA256:
            raise ValueError("existing oracle disc differs; refuse to overwrite")
        return
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as target:
        if os.name == "nt":
            import msvcrt
            kernel = ctypes.WinDLL("kernel32", use_last_error=True)
            kernel.DeviceIoControl.argtypes = [ctypes.c_void_p, ctypes.c_uint32,
                ctypes.c_void_p, ctypes.c_uint32, ctypes.c_void_p, ctypes.c_uint32,
                ctypes.POINTER(ctypes.c_uint32), ctypes.c_void_p]
            returned = ctypes.c_uint32()
            if not kernel.DeviceIoControl(msvcrt.get_osfhandle(target.fileno()),
                    0x900C4, None, 0, None, 0, ctypes.byref(returned), None):
                raise ctypes.WinError(ctypes.get_last_error())
        target.truncate(DISC_SIZE)
        for offset, content in chunks:
            target.seek(offset)
            target.write(content)
    if digest(output) != DISC_SHA256:
        raise ValueError("reproduced disc SHA-256 mismatch")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sys_dir", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    make_disc(args.sys_dir, args.output)
    print("startup-only synthetic oracle", DISC_SHA256)
