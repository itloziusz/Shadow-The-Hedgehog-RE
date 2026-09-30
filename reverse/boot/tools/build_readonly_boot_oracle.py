"""Build a separate read-only GDB export using an existing MSVC Dolphin build.

Supply the matching x64 MSVC and Windows SDK directories. Original source, objects,
libraries and executable are read-only. Only one copied GDB translation unit
is changed; outputs and compiler temporaries stay under repository build/.
The native Shadow reconstruction never links or launches this oracle.
"""

import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess


EXPORT = '''  else if (id >= 143 && id < 175)
  {
    wbe64hex(reply, ppc_state.ps[id - 143].PS1AsU64());
  }
  else if (id >= 175 && id < 183)
  {
    wbe32hex(reply, ppc_state.spr[SPR_GQR0 + id - 175]);
  }
  else if (id == 183)
  {
    wbe32hex(reply, ppc_state.spr[SPR_HID2]);
  }
  else if (id >= 184 && id < 248)
  {
    const auto& bytes = id < 216 ? ppc_state.iCache.valid : ppc_state.iCache.plru;
    const u32 offset = ((id < 216 ? id - 184 : id - 216) * 4);
    wbe32hex(reply, (u32(bytes[offset]) << 24) | (u32(bytes[offset + 1]) << 16) |
                   (u32(bytes[offset + 2]) << 8) | u32(bytes[offset + 3]));
  }
  else if (id == 248)
  {
    wbe32hex(reply, ppc_state.iCache.m_disable_icache ? 1 : 0);
  }
  else if (id == 249)
  {
    wbe32hex(reply, ppc_state.GetXER().Hex);
  }
'''


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def command_args(command):
    # MSVC tlogs contain Windows command lines, not shell programs.
    shell = ctypes.WinDLL("shell32")
    shell.CommandLineToArgvW.argtypes = [ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_int)]
    shell.CommandLineToArgvW.restype = ctypes.POINTER(ctypes.c_wchar_p)
    count = ctypes.c_int()
    argv = shell.CommandLineToArgvW("tool " + command, ctypes.byref(count))
    try:
        return [argv[i] for i in range(1, count.value)]
    finally:
        kernel = ctypes.WinDLL("kernel32")
        kernel.LocalFree.argtypes = [ctypes.c_void_p]
        kernel.LocalFree(argv)


def tlog_command(path, source=None):
    lines = path.read_text(encoding="utf-16").splitlines()
    for i, line in enumerate(lines[:-1]):
        if line.startswith("^") and (source is None or line.upper().endswith(source.upper())):
            return command_args(lines[i + 1])
    raise ValueError(f"no matching command in {path.name}")


def response_run(tool, args, output, env, tool_bin):
    response = output / (tool + ".rsp")
    response.write_text(subprocess.list2cmdline(args), encoding="utf-16")
    completed = subprocess.run([str(tool_bin / (tool + ".exe")), "@" + str(response)], cwd=output,
                               env=env, capture_output=True, text=True)
    (output / (tool + ".log")).write_text(completed.stdout + completed.stderr,
                                          encoding="utf-8")
    if completed.returncode:
        raise RuntimeError(f"{tool} failed; see {output / (tool + '.log')}")


def build(source, original_build, output, msvc, sdk_root, sdk_version):
    build_root = Path(__file__).resolve().parents[3] / "build"
    output = output.resolve()
    if not output.is_relative_to(build_root.resolve()):
        raise ValueError("instrumented outputs must stay under repository build/")
    original_build, source = original_build.resolve(), source.resolve()
    if any(output.is_relative_to(p) or p.is_relative_to(output)
           for p in (original_build, source)):
        raise ValueError("private outputs must not overlap read-only source/build inputs")
    output.mkdir(parents=True, exist_ok=True)
    (output / "tmp").mkdir(exist_ok=True)
    tool_bin = msvc / "bin/Hostx64/x64"
    env = dict(os.environ, TMP=str(output / "tmp"), TEMP=str(output / "tmp"))
    env["PATH"] = str(tool_bin) + ";" + str(sdk_root / "bin" / sdk_version / "x64") + ";" + env.get("PATH", "")
    env["INCLUDE"] = ";".join(str(p) for p in [msvc / "include"] +
        [sdk_root / "Include" / sdk_version / part for part in ("ucrt", "shared", "um", "winrt")])
    env["LIB"] = ";".join(str(p) for p in [msvc / "lib/x64"] +
        [sdk_root / "Lib" / sdk_version / part / "x64" for part in ("ucrt", "um")])
    original = source / "Source/Core/Core/PowerPC/GDBStub.cpp"
    text = original.read_text(encoding="utf-8")
    # No emulator instruction, store, branch, register writer or Reset changes.
    read_start, read_end = text.index("static void ReadRegister()"), text.index("static void ReadRegisters()")
    block = text[read_start:read_end]
    anchor = "  else if (id >= 71 && id < 87)"
    if block.count(anchor) != 1 or "id >= 143" in block:
        raise ValueError("unexpected upstream GDB read-register map")
    block = block.replace(anchor, EXPORT + anchor)
    modified = output / "GDBStub.cpp"
    modified.write_text(text[:read_start] + block + text[read_end:], encoding="utf-8")
    compile_args = tlog_command(original_build /
        "Source/Core/Core/core.dir/Release/core.tlog/CL.command.1.tlog", "GDBSTUB.CPP")
    compile_args = [arg for arg in compile_args
                    if not arg.lower().startswith(("/fo", "/fd")) and
                    not arg.upper().endswith("GDBSTUB.CPP")]
    compile_args += ["/Fo" + str(output / "GDBStub.obj"),
                     "/Fd" + str(output / "GDBStub.pdb"), str(modified)]
    response_run("cl", compile_args, output, env, tool_bin)
    link_dir = original_build / "Source/Core/DolphinQt"
    link_tlog = link_dir / "dolphin-emu.dir/Release/dolphin-emu.tlog/link.command.1.tlog"
    link_args = tlog_command(link_tlog)
    inputs = []
    rewritten = [str(output / "GDBStub.obj")]
    for arg in link_args:
        # MSBuild uppercases the tlog, including this case-sensitive C symbol.
        # Original Common/CompatPatches.cpp and CMake use enableCompatPatches.
        if arg.upper() == "-INCLUDE:ENABLECOMPATPATCHES":
            arg = "-INCLUDE:enableCompatPatches"
        if arg.lower().startswith(("/out:", "/pdb:", "/implib:")):
            continue
        if not arg.startswith(("/", "-")) and ("\\" in arg or "/" in arg):
            path = Path(arg)
            if not path.is_absolute():
                path = link_dir / path
            path = path.resolve()
            if not path.is_file():
                raise ValueError(f"missing read-only link input {path}")
            inputs.append(path)
            arg = str(path)
        rewritten.append(arg)
    # MSBuild's linker tlog stores object inputs in the tracking marker,
    # separately from the recorded options/libraries command line.
    marker = link_tlog.read_text(encoding="utf-16").splitlines()[0]
    for name in marker.removeprefix("^").split("|"):
        path = Path(name).resolve()
        if path.suffix.lower() == ".obj":
            if not path.is_file():
                raise ValueError(f"missing read-only object {path}")
            inputs.append(path)
            rewritten.append(str(path))
    rewritten += ["/OUT:" + str(output / "Dolphin.exe"),
                  "/PDB:" + str(output / "Dolphin.pdb"),
                  "/IMPLIB:" + str(output / "Dolphin.lib")]
    response_run("link", rewritten, output, env, tool_bin)
    binaries = original_build / "Binaries"
    for dll in binaries.glob("*.dll"):
        shutil.copy2(dll, output / dll.name)
    if (binaries / "qt.conf").is_file():
        shutil.copy2(binaries / "qt.conf", output / "qt.conf")
    for directory in ("QtPlugins", "Sys"):
        if (binaries / directory).is_dir():
            shutil.copytree(binaries / directory, output / directory, dirs_exist_ok=True)
    manifest = {"purpose": "read-only HLE reference export; no native dependency",
        "base_executable_sha256": digest(binaries / "Dolphin.exe"),
        "original_gdb_source_sha256": digest(original),
        "instrumented_gdb_source_sha256": digest(modified),
        "instrumented_executable_sha256": digest(output / "Dolphin.exe"),
        "register_map": {"143:175": "PS1 raw u64", "175:183": "GQR0..7",
            "183": "HID2", "184:216": "ICache valid bytes packed BE32",
            "216:248": "ICache PLRU bytes packed BE32", "248": "ICache disabled flag",
            "249": "architectural XER composed from live carry/SO/OV fields"},
        "link_input_sha256": {str(p): digest(p) for p in inputs}}
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print("read-only oracle", manifest["instrumented_executable_sha256"])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("original_build", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--msvc", required=True, type=Path,
                        help="MSVC Tools/MSVC/<version> matching the original build")
    parser.add_argument("--sdk-root", required=True, type=Path)
    parser.add_argument("--sdk-version", required=True)
    args = parser.parse_args()
    build(args.source, args.original_build, args.output, args.msvc,
          args.sdk_root, args.sdk_version)
