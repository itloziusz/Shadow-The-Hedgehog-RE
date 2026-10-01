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


TIMING_EXPORT = '''  else if (id == 123)
  {
    wbe32hex(reply, ppc_state.Exceptions);
  }
  else if (id == 250)
  {
    wbe64hex(reply, system.GetCoreTiming().GetTicks());
  }
  else if (id == 251)
  {
    wbe64hex(reply, system.GetCoreTiming().GetFakeTBStartTicks());
  }
  else if (id == 252)
  {
    wbe64hex(reply, system.GetCoreTiming().GetFakeTBStartValue());
  }
  else if (id == 253)
  {
    wbe32hex(reply, system.GetSystemTimers().GetTicksPerSecond());
  }
  else if (id == 254)
  {
    wbe64hex(reply, static_cast<u64>(system.GetSystemTimers().GetLocalTimeRTCOffset()));
  }
  else if (id == 255)
  {
    wbe64hex(reply, (u64(ppc_state.spr[SPR_TU]) << 32) | ppc_state.spr[SPR_TL]);
  }
'''


CLOCK_PERTURBATION_WRITER = '''  if (id >= 250 && id <= 252)
  {
    // Controlled clock-source falsification. This gate exists only in a
    // separately labelled experiment oracle, before the first DOL instruction.
    if (ppc_state.pc != 0x80003154 || s_cmd_len != 12 || s_cmd_bfr[3] != '=')
      return SendReply("E00");
    for (u32 i = 0; i < 8; ++i)
    {
      const u8 c = bufptr[i];
      if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F')))
        return SendReply("E00");
    }
    const u32 value = re32hex(bufptr);
    auto& timing = system.GetCoreTiming();
    if (id == 250)
      timing.SetFakeTBStartValue((timing.GetFakeTBStartValue() & 0xffffffff00000000ULL) |
                                u64(value));
    else if (id == 251)
      timing.SetFakeTBStartValue((u64(value) << 32) |
                                (timing.GetFakeTBStartValue() & 0xffffffffULL));
    else
    {
      if (value != 1)
        return SendReply("E00");
      timing.SetFakeTBStartTicks(timing.GetTicks());
      system.GetPowerPC().WriteFullTimeBaseValue(timing.GetFakeTBStartValue());
    }
    return SendReply("OK");
  }

'''


def instrument_source(text, timing=False, clock_perturbations=False):
    if clock_perturbations and not timing:
        raise ValueError("clock perturbations require timing exports")
    read_start = text.index("static void ReadRegister()")
    read_end = text.index("static void ReadRegisters()")
    block = text[read_start:read_end]
    anchor = "  else if (id >= 71 && id < 87)"
    if block.count(anchor) != 1 or "id >= 143" in block:
        raise ValueError("unexpected upstream GDB read-register map")
    block = block.replace(anchor, EXPORT + (TIMING_EXPORT if timing else "") + anchor)
    modified_text = text[:read_start] + block + text[read_end:]
    if timing:
        include = '#include "Core/Core.h"'
        if modified_text.count(include) != 1:
            raise ValueError("unexpected upstream core include")
        modified_text = modified_text.replace(
            include, include + '\n#include "Core/CoreTiming.h"\n#include "Core/HW/SystemTimers.h"')
    if clock_perturbations:
        write_start = modified_text.index("static void WriteRegister()")
        write_end = modified_text.index("static void ReadMemory(", write_start)
        block = modified_text[write_start:write_end]
        anchor = "  if (id < 32)"
        if block.count(anchor) != 1 or "id >= 250" in block:
            raise ValueError("unexpected upstream GDB write-register map")
        block = block.replace(anchor, CLOCK_PERTURBATION_WRITER + anchor)
        modified_text = modified_text[:write_start] + block + modified_text[write_end:]
    return modified_text


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


def build(source, original_build, output, msvc, sdk_root, sdk_version, timing=False, clock_perturbations=False):
    if clock_perturbations and not timing:
        raise ValueError("clock perturbations require timing exports")
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
    # Default export modes leave the original register writer unchanged.
    modified = output / "GDBStub.cpp"
    modified_text = instrument_source(text, timing, clock_perturbations)
    modified.write_text(modified_text, encoding="utf-8")
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
    if timing:
        manifest["register_map"].update({"123": "pending architectural exception flags (read only)", "250": "CoreTiming elapsed CPU-domain cycles (u64)",
            "251": "TB producer epoch cycles (u64)", "252": "TB producer epoch value (u64)",
            "253": "CPU-domain cycles per second (u32)", "254": "RTC local-time offset raw s64",
            "255": "last architecturally sampled TU/TL pair, not a fresh clock read"})
        manifest["timing_export"] = "pure getters only; no GetFakeTimeBase call or register writer"
    if clock_perturbations:
        manifest["purpose"] = "controlled pre-entry clock-source experiment oracle; no native dependency"
        manifest["clock_perturbations"] = {
            "enabled": True, "allowed_pc": "80003154", "exact_packet_value_width": 8,
            "write_register_map": {
                "250": "set TB epoch low u32, preserving high; Pfa=<8 hex>",
                "251": "set TB epoch high u32, preserving low; Pfb=<8 hex>",
                "252": "commit only value00000001: epochcycles=GetTicks; mirror cachedTU/TL; Pfc=00000001"},
            "restrictions": "all controls decline outside PC80003154; no guest instruction or midchain patch",
            "evidence_scope": "rollover/carry falsification only; cannot advance native frontier",
            "default_timing_export_unchanged": True}
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print("clock experiment oracle" if clock_perturbations else "read-only oracle",
          manifest["instrumented_executable_sha256"])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("original_build", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--msvc", required=True, type=Path,
                        help="MSVC Tools/MSVC/<version> matching the original build")
    parser.add_argument("--sdk-root", required=True, type=Path)
    parser.add_argument("--sdk-version", required=True)
    parser.add_argument("--timing", action="store_true", help="add pure TB-producer getter exports")
    parser.add_argument("--clock-perturbations", action="store_true",
                        help="controlled pre-entry TB epoch writes; requires --timing")
    args = parser.parse_args()
    if args.clock_perturbations and not args.timing:
        parser.error("--clock-perturbations requires --timing")
    build(args.source, args.original_build, args.output, args.msvc,
          args.sdk_root, args.sdk_version, args.timing, args.clock_perturbations)
