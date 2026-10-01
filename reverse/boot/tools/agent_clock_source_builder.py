"""Verify the copied clock experiment gate without executing an oracle.

The generated harness compiles the original command parser and exact inserted
writer body against observable fake state. It tests packet rejection and
producer/cache writes; actual rollover remains a separate reference capture.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess

GDB_PIN = "c2e3ae4869d41eb212bf9fb08940958f17939a4a2f19d5354ec4e386d74ba35e"
TIMING_LF_PIN = "50961d834f730e5dd837692c66be8cd87f402422dab6cea01cee947231cbc18b"
PRELUDE = r'''#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
using u8 = std::uint8_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
struct Timing
{
  u64 origin = 0x123456789abcdef0ULL;
  u64 origin_cycles = 0x99;
  u64 ticks = 0x11223344;
  unsigned reads = 0, writes = 0;
  u64 GetFakeTBStartValue() const { return origin; }
  void SetFakeTBStartValue(u64 v) { origin = v; ++writes; }
  void SetFakeTBStartTicks(u64 v) { origin_cycles = v; ++writes; }
  u64 GetTicks() { ++reads; return ticks; }
};
struct PPC { u32 pc = 0x80003154; };
struct Power
{
  u64 cached = 0x8877665544332211ULL;
  unsigned writes = 0;
  void WriteFullTimeBaseValue(u64 v) { cached = v; ++writes; }
};
namespace Core {
struct System
{
  Timing timing;
  PPC ppc;
  Power power;
  static System& GetInstance() { static System instance; return instance; }
  Timing& GetCoreTiming() { return timing; }
  PPC& GetPPCState() { return ppc; }
  Power& GetPowerPC() { return power; }
};
}
static u8 s_cmd_bfr[64];
static u32 s_cmd_len;
static std::string reply;
static u8 Hex2char(u8 c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return 0;
}
static u32 re32hex(u8* p)
{
  u32 result = 0;
  for (unsigned i = 0; i < 8; ++i) result = (result << 4) | Hex2char(p[i]);
  return result;
}
static void SendReply(const char* s) { reply = s; }
'''
TESTS = r'''
static void require(bool value, const char* message)
{
  if (!value) { std::cerr << message << "\n"; std::exit(1); }
}
static void packet(const std::string& command)
{
  std::memset(s_cmd_bfr, 0, sizeof(s_cmd_bfr));
  std::memcpy(s_cmd_bfr, command.data(), command.size());
  s_cmd_len = static_cast<u32>(command.size());
  reply.clear();
  WriteRegister();
}
static void declined(const std::string& command, u32 pc = 0x80003154)
{
  auto& system = Core::System::GetInstance();
  system = Core::System{};
  system.ppc.pc = pc;
  const u64 origin = system.timing.origin;
  const u64 epoch = system.timing.origin_cycles;
  const u64 cached = system.power.cached;
  packet(command);
  require(reply == "E00", "invalid control did not decline");
  require(system.timing.origin == origin && system.timing.origin_cycles == epoch &&
          system.power.cached == cached && system.timing.reads == 0 &&
          system.timing.writes == 0 && system.power.writes == 0,
          "declined control changed clock source or sampled it");
}
int main()
{
  auto& system = Core::System::GetInstance();
  packet("Pfa=ffffffff");
  require(reply == "OK" && system.timing.origin == 0x12345678ffffffffULL,
          "low write did not preserve high");
  require(system.timing.origin_cycles == 0x99 && system.timing.reads == 0 &&
          system.power.writes == 0, "low write committed or sampled");
  packet("Pfb=FFFFFFFF");
  require(reply == "OK" && system.timing.origin == 0xffffffffffffffffULL,
          "high write did not preserve low");
  packet("Pfc=00000001");
  require(reply == "OK" && system.timing.origin_cycles == system.timing.ticks &&
          system.power.cached == 0xffffffffffffffffULL && system.timing.reads == 1 &&
          system.power.writes == 1, "commit did not preserve source contract");
  unsigned declines = 0;
  for (const auto* id : {"fa", "fb", "fc"})
  {
    const std::string command = std::string("P") + id + "=00000001";
    for (u32 pc : {0u, 0x80003158u, 0x80379628u})
    { declined(command, pc); ++declines; }
    declined(std::string("P") + id + "=0000001"); ++declines;
    declined(command + "0"); ++declines;
    declined(std::string("P") + id + ":00000001"); ++declines;
    for (unsigned pos = 4; pos < 12; ++pos)
    {
      std::string broken = command;
      broken[pos] = 'z';
      declined(broken); ++declines;
    }
  }
  declined("Pfc=00000000"); ++declines;
  declined("Pfc=00000002"); ++declines;
  std::cout << "exact generated writer passed: 3 valid packets, " << declines
            << " rejected packets with zero source/cached-state mutation\n";
}
'''

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

def function_block(text, name, next_name):
    return text[text.index("static void " + name):text.index("static void " + next_name)]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--msvc", type=Path, required=True)
    parser.add_argument("--sdk-root", type=Path, required=True)
    parser.add_argument("--sdk-version", required=True)
    args = parser.parse_args()
    source = args.source_root / "Source/Core/Core/PowerPC/GDBStub.cpp"
    if sha(source.read_bytes()) != GDB_PIN:
        raise ValueError("original GDB source identity declined")
    spec = importlib.util.spec_from_file_location(
        "clock_builder", Path(__file__).with_name("build_readonly_boot_oracle.py"))
    builder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(builder)
    text = source.read_text(encoding="utf-8")
    basic = builder.instrument_source(text)
    timing = builder.instrument_source(text, timing=True)
    experiment = builder.instrument_source(text, timing=True, clock_perturbations=True)
    if sha(timing.encode()) != TIMING_LF_PIN:
        raise ValueError("default timing export changed")
    original_writer = function_block(text, "WriteRegister()", "ReadMemory(")
    for generated in (basic, timing):
        if function_block(generated, "WriteRegister()", "ReadMemory(") != original_writer:
            raise ValueError("ordinary register writer changed")
    if function_block(timing, "ReadRegister()", "ReadRegisters()") != function_block(
            experiment, "ReadRegister()", "ReadRegisters()"):
        raise ValueError("experiment changed pure timing getters")
    if experiment.replace(builder.CLOCK_PERTURBATION_WRITER, "", 1) != timing:
        raise ValueError("experiment changed code outside the optional writer")
    for altered, use_timing, use_experiment in [
            (text, False, True),
            (text.replace("  else if (id >= 71 && id < 87)", "", 1), True, False),
            (text.replace('#include "Core/Core.h"', "", 1), True, False),
            (text.replace("  if (id < 32)", "  if (id < 31)"), True, True)]:
        try:
            builder.instrument_source(altered, use_timing, use_experiment)
        except ValueError:
            continue
        raise ValueError("malformed source/mode was accepted")
    output = args.output.resolve()
    build_root = Path(__file__).resolve().parents[3] / "build"
    if not output.is_relative_to(build_root.resolve()):
        raise ValueError("harness outputs must stay under repository build/")
    output.mkdir(parents=True, exist_ok=True)
    writer = function_block(experiment, "WriteRegister()", "ReadMemory(")
    prefix = writer[:writer.index("  if (id < 32)")]
    harness = output / "gate.cpp"
    harness.write_text(PRELUDE + prefix + '  SendReply("E01");\n}\n' + TESTS,
                       encoding="utf-8")
    tools = args.msvc / "bin/Hostx64/x64"
    env = dict(os.environ)
    env["PATH"] = str(tools) + ";" + env.get("PATH", "")
    env["INCLUDE"] = ";".join(str(p) for p in [args.msvc / "include"] +
        [args.sdk_root / "Include" / args.sdk_version / part for part in ("ucrt", "shared", "um")])
    env["LIB"] = ";".join(str(p) for p in [args.msvc / "lib/x64"] +
        [args.sdk_root / "Lib" / args.sdk_version / part / "x64" for part in ("ucrt", "um")])
    executable = output / "gate.exe"
    args_cl = [str(tools / "cl.exe"), "/nologo", "/EHsc", "/std:c++20",
               "/Fo" + str(output / "gate.obj"), "/Fe" + str(executable), str(harness)]
    result = subprocess.run(args_cl, cwd=output, env=env, capture_output=True, text=True)
    (output / "compile.log").write_text(result.stdout + result.stderr, encoding="utf-8")
    if result.returncode:
        raise RuntimeError("gate compilation failed; inspect compile.log")
    result = subprocess.run([str(executable)], cwd=output, env=env,
                            capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    report = {"default_timing_lf_sha256": sha(timing.encode()),
              "default_writer_unchanged": True, "pure_getters_unchanged": True,
              "optional_writer_only": True, "malformed_source_declines": 4,
              "harness_sha256": sha(harness.read_bytes()),
              "harness_executable_sha256": sha(executable.read_bytes()),
              "guard_result": result.stdout.strip()}
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))

if __name__ == "__main__":
    main()
