"""Passive live DTK logging receipt; a copied Mixer TU, never native admission."""
import argparse
import copy
import hashlib
import inspect
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

import agent_clock_source as clock
import agent_timing_source_42 as timing
import build_readonly_boot_oracle as builder
import capture_timing_prefix as base
from agent_si_device_falsifier_42 import select_explicit_port, native_parity
from capture_dolphin_rsp import sha256

BASE_CAPTURE_SHA = "d477a14b913c1b9d3b07be5a0412233121f75aa1d368a7a57b3940df388f9f81"
BASE_ORACLE_SHA = "84be4d74d3a566b92e46c416d5e9c4ff87f313612fb2f4e529852cb74b0cfe78"
AUDIO_LIB_SHA = "6274ea941d8545b3367bbd35cf1cebda8a88cb4a6ed1555e61c18cef9aeae8b2"
COMMON_LIB_SHA = "e230a2f6d17d6588dd1bae6ee72dc4b4331bb29ad79fbb531ea420a536341532"
MIXER_REL = "Source/Core/AudioCommon/Mixer.cpp"
LAYER_REL = "Source/Core/Common/Config/Layer.cpp"
WINDOW = "..timing-prefix-end@80373ac4"
WATCH = "0c005000:0c005100;0c002000:0c002100;0c003000:0c003008;0c006c00:0c006c20"
DTK_PINS = {
    MIXER_REL: "24f91152ffe824019053f89acc77a974992fa9ed19e68b0c94194521987f90a3",
    "Source/Core/AudioCommon/Mixer.h": "612025371230d433dcf62e027d1abd2da8be5c3533a8ed2947a0e3b6f9480bd5",
    "Source/Core/AudioCommon/AudioCommon.cpp": "de119dfd09a5f10309bbdd0b2de83f5666a8c2f413bf87143ab3322d7b527430",
    "Source/Core/AudioCommon/WaveFile.cpp": "bd9020f3769c3a4df30932cae3fa7efeb5dddd6f5251ce59221d59edba9d4961",
    "Source/Core/AudioCommon/SoundStream.h": "6d78d46f205f66a6804e59975df1d2344f4474021f59b1a85bf7716af3374a43",
    "Source/Core/Core/OracleEventAudit.h": "781b5f3e344877582d2e7b4506d9fc0f7c12e1ada672fe75fc9c7b9f432a036d",
    "Source/Core/Core/OracleEventAudit.cpp": "bf4665f53049fd4621aea6fa9502cf765902c357bf60062de6ba6ba14662b1c7",
    "Source/Core/Core/Config/MainSettings.cpp": "31349025a4a3d4ed7a5b32e9aa6b4d6aa52c522b2bc85b0e909876f685c78c71",
    LAYER_REL: "af5702de9c65d59902989b362da716acbc8bdcfb9a8c9fbf23ab67f389ad2c8f",
    "Source/Core/Common/Config/Layer.h": "66fed0ac7d91e93e1f6168e837d1c8efa837df8d106993060eb6bc8cde9b2470",
    "Source/Core/Common/Config/Config.cpp": "7cc14cab8048767c336c5219234b1527a434576cc9db09db13ccc716bfcacb05",
}
# These exact, reversible replacements cache each existing condition once.
# No clock/config/sample/opcode reads are added in the observer arguments.
REPLACEMENTS = [
    ('#include "AudioCommon/Mixer.h"', '#include "AudioCommon/Mixer.h"\n#include "Core/OracleEventAudit.h"'),
    ('{\n  m_config_changed_callback_id = Config::AddConfigChangedCallback([this] { RefreshConfig(); });',
     '{\n  OracleEventAudit::Record("dtk-owner-ctor", "-", {reinterpret_cast<uintptr_t>(this), m_log_dtk_audio, BackendSampleRate});\n  m_config_changed_callback_id = Config::AddConfigChangedCallback([this] { RefreshConfig(); });'),
    ('Mixer::~Mixer()\n{\n',
     'Mixer::~Mixer()\n{\n  OracleEventAudit::Record("dtk-owner-dtor", "-", {reinterpret_cast<uintptr_t>(this)});\n'),
    ('void Mixer::PushStreamingSamples(const s16* samples, std::size_t num_samples)\n{\n  if (IsOutputSampleRateValid())',
     'void Mixer::PushStreamingSamples(const s16* samples, std::size_t num_samples)\n{\n  const bool dtk_owner_valid_rate = IsOutputSampleRateValid();\n  OracleEventAudit::Record("dtk-owner-push", "-", {reinterpret_cast<uintptr_t>(this), num_samples, dtk_owner_valid_rate});\n  if (dtk_owner_valid_rate)'),
    ('  if (m_log_dtk_audio)\n  {\n    const s32 sample_rate_divisor',
     '  const bool dtk_owner_log_read = m_log_dtk_audio;\n  OracleEventAudit::Record("dtk-owner-read", "-", {reinterpret_cast<uintptr_t>(this), dtk_owner_log_read, num_samples});\n  if (dtk_owner_log_read)\n  {\n    const s32 sample_rate_divisor'),
    ('void Mixer::StartLogDTKAudio(const std::string& filename)\n{\n  if (!m_log_dtk_audio)',
     'void Mixer::StartLogDTKAudio(const std::string& filename)\n{\n  const bool dtk_owner_start_read = m_log_dtk_audio;\n  OracleEventAudit::Record("dtk-owner-start", "-", {reinterpret_cast<uintptr_t>(this), dtk_owner_start_read});\n  if (!dtk_owner_start_read)'),
    ('      m_log_dtk_audio = true;\n',
     '      m_log_dtk_audio = true;\n      OracleEventAudit::Record("dtk-owner-write", "start", {reinterpret_cast<uintptr_t>(this), 1});\n'),
    ('      NOTICE_LOG_FMT(AUDIO, "Starting DTK Audio logging");',
     '      NOTICE_LOG_FMT(AUDIO, "Starting DTK Audio logging");\n      OracleEventAudit::Record("dtk-owner-start-exit", "started", {reinterpret_cast<uintptr_t>(this), 1});'),
    ('      NOTICE_LOG_FMT(AUDIO, "Unable to start DTK Audio logging");',
     '      NOTICE_LOG_FMT(AUDIO, "Unable to start DTK Audio logging");\n      OracleEventAudit::Record("dtk-owner-start-exit", "failed", {reinterpret_cast<uintptr_t>(this), 0});'),
    ('    WARN_LOG_FMT(AUDIO, "DTK Audio logging has already been started");',
     '    WARN_LOG_FMT(AUDIO, "DTK Audio logging has already been started");\n    OracleEventAudit::Record("dtk-owner-start-exit", "already-started", {reinterpret_cast<uintptr_t>(this), 1});'),
    ('void Mixer::StopLogDTKAudio()\n{\n  if (m_log_dtk_audio)',
     'void Mixer::StopLogDTKAudio()\n{\n  const bool dtk_owner_stop_read = m_log_dtk_audio;\n  OracleEventAudit::Record("dtk-owner-stop", "-", {reinterpret_cast<uintptr_t>(this), dtk_owner_stop_read});\n  if (dtk_owner_stop_read)'),
    ('    m_log_dtk_audio = false;\n',
     '    m_log_dtk_audio = false;\n    OracleEventAudit::Record("dtk-owner-write", "stop", {reinterpret_cast<uintptr_t>(this), 0});\n'),
    ('    NOTICE_LOG_FMT(AUDIO, "Stopping DTK Audio logging");',
     '    NOTICE_LOG_FMT(AUDIO, "Stopping DTK Audio logging");\n    OracleEventAudit::Record("dtk-owner-stop-exit", "stopped", {reinterpret_cast<uintptr_t>(this), 0});'),
    ('    WARN_LOG_FMT(AUDIO, "DTK Audio logging has already been stopped");',
     '    WARN_LOG_FMT(AUDIO, "DTK Audio logging has already been stopped");\n    OracleEventAudit::Record("dtk-owner-stop-exit", "already-stopped", {reinterpret_cast<uintptr_t>(this), 0});'),
]
LAYER_REPLACEMENTS = [
    ('#include "Common/Config/Layer.h"', '#include "Common/Config/Layer.h"\n#include "Core/OracleEventAudit.h"'),
    ('  if (!OracleViWatched(location)) return;',
     '  if (location.system == System::Main && location.section == "DSP" && location.key == "DumpAudio")\n  {\n    const u64 code = value == "<absent>" ? 0 : (value == "False" || value == "false" || value == "0") ? 1 : (value == "True" || value == "true" || value == "1") ? 2 : value == "lease" ? 3 : 4;\n    OracleEventAudit::Record("dtk-config", kind, {static_cast<u64>(layer), code, a, b});\n  }\n  if (!OracleViWatched(location)) return;'),
    ('Location{System::Main, "Display", "RenderToMain"}})',
     'Location{System::Main, "Display", "RenderToMain"}, Location{System::Main, "DSP", "DumpAudio"}})'),
    ('Section Layer::GetSection(System system, const std::string& section)\n{',
     'Section Layer::GetSection(System system, const std::string& section)\n{\n  OracleViAudit("mutable-section", static_cast<int>(m_layer), {system, section, "DumpAudio"}, "lease", reinterpret_cast<uintptr_t>(this));'),
]

def require(ok, message):
    if not ok:
        raise ValueError(message)

def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")

def pins(args):
    combined = {**clock.SOURCE_PINS, **timing.EXTRA_PINS, **DTK_PINS}
    for relative, expected in combined.items():
        require(sha256(args.source_root / relative) == expected, "source pin differs: " + relative)
    require(sha256(args.library) == clock.LIB_SHA, "core library differs")
    require(sha256(args.audio_library) == AUDIO_LIB_SHA, "audio library differs")
    require(sha256(args.common_library) == COMMON_LIB_SHA, "common library differs")
    require(sha256(Path(base.__file__)) == BASE_CAPTURE_SHA, "base capture tool differs")
    return combined

def observe_source(original, replacements=REPLACEMENTS):
    copied = original
    for old, new in replacements:
        require(copied.count(old) == 1, "copied Mixer transformation site differs: " + old[:60])
        copied = copied.replace(old, new)
    restored = copied
    for old, new in reversed(replacements):
        require(restored.count(new) == 1, "transformation inverse ambiguous")
        restored = restored.replace(new, old)
    require(restored == original, "copied Mixer does not invert to exact original text")
    if replacements is REPLACEMENTS:
        require(copied.count("m_log_dtk_audio = true;") == 1 and
                copied.count("m_log_dtk_audio = false;") == 1, "flag writer coverage differs")
    return copied

def build_oracle(args):
    pins(args)
    require(args.msvc.name=="14.51.36231" and args.sdk_version=="10.0.26100.0", "matching compiler/SDK required")
    base_dir = args.base_oracle
    manifest = json.loads((base_dir / "manifest.json").read_text(encoding="utf-8"))
    require(sha256(base_dir / "Dolphin.exe") == manifest["instrumented_executable_sha256"] == BASE_ORACLE_SHA,
            "base passive oracle differs")
    require(sha256(base_dir / "Interpreter.cpp") == manifest["passive_interpreter"]["copied_sha256"] and
            sha256(base_dir / "Interpreter.obj") == manifest["passive_interpreter"]["object_sha256"], "base interpreter differs")
    require(sha256(base_dir / "GDBStub.cpp") == manifest["instrumented_gdb_source_sha256"], "base getter source differs")
    for name, expected in manifest["link_input_sha256"].items():
        require(sha256(Path(name)) == expected, "read-only base link input differs")
    output = args.oracle_output
    require(not output.exists() and output.is_relative_to(args.build_root), "fresh build-contained oracle required")
    output.mkdir(parents=True)
    (output / "tmp").mkdir()
    original = (args.source_root / MIXER_REL).read_text(encoding="utf-8")
    source = output / "Mixer.cpp"
    source.write_text(observe_source(original), encoding="utf-8")
    env = dict(os.environ)
    tool_bin = args.msvc / "bin/Hostx64/x64"
    env["TMP"] = env["TEMP"] = str(output / "tmp")
    env["PATH"] = str(tool_bin) + ";" + str(args.sdk_root / "bin" / args.sdk_version / "x64") + ";" + env.get("PATH", "")
    env["INCLUDE"] = ";".join(str(p) for p in [args.msvc / "include"] +
        [args.sdk_root / "Include" / args.sdk_version / x for x in ("ucrt","shared","um","winrt")])
    env["LIB"] = ";".join(str(p) for p in [args.msvc / "lib/x64"] +
        [args.sdk_root / "Lib" / args.sdk_version / x / "x64" for x in ("ucrt","um")])
    tlog = args.original_build / "Source/Core/AudioCommon/audiocommon.dir/Release/audiocommon.tlog/CL.command.1.tlog"
    compile_args = [a for a in builder.tlog_command(tlog, "MIXER.CPP")
                    if not a.lower().startswith(("/fo","/fd")) and not a.lower().endswith("mixer.cpp")]
    compile_args += ["/Fo" + str(output / "Mixer.obj"), "/Fd" + str(output / "Mixer.pdb"), str(source)]
    builder.response_run("cl", compile_args, output, env, tool_bin)
    for suffix in ("rsp","log"):
        shutil.copy2(output/("cl."+suffix),output/("Mixer.cl."+suffix))
    layer_source=output/"Layer.cpp"
    layer_source.write_text(observe_source((args.source_root/LAYER_REL).read_text(encoding="utf-8"),LAYER_REPLACEMENTS),encoding="utf-8")
    layer_tlog=args.original_build/"Source/Core/Common/common.dir/Release/common.tlog/CL.command.1.tlog"
    layer_args=[a for a in builder.tlog_command(layer_tlog,"LAYER.CPP")
                if not a.lower().startswith(("/fo","/fd")) and not a.lower().endswith("layer.cpp")]
    layer_args += ["/Fo"+str(output/"Layer.obj"),"/Fd"+str(output/"Layer.pdb"),str(layer_source)]
    builder.response_run("cl",layer_args,output,env,tool_bin)
    for suffix in ("rsp","log"):
        shutil.copy2(output/("cl."+suffix),output/("Layer.cl."+suffix))
    arguments = builder.command_args((base_dir / "link.rsp").read_text(encoding="utf-16"))
    rewritten = [str(output / "Mixer.obj"),str(output/"Layer.obj")] + [a for a in arguments if not a.lower().startswith(("/out:","/pdb:","/implib:"))]
    rewritten += ["/OUT:" + str(output / "Dolphin.exe"), "/PDB:" + str(output / "Dolphin.pdb"),
                  "/IMPLIB:" + str(output / "Dolphin.lib"), "/MAP:" + str(output / "Dolphin.map")]
    builder.response_run("link", rewritten, output, env, tool_bin)
    require("LNK4006" not in (output / "link.log").read_text(encoding="utf-8"), "duplicate definitions")
    link_map = (output / "Dolphin.map").read_text(encoding="utf-8")
    for symbol,obj in [(s,"Mixer.obj") for s in ("PushStreamingSamples@Mixer","StartLogDTKAudio@Mixer","StopLogDTKAudio@Mixer")] + [
            (s,"Layer.obj") for s in ("OracleViAudit@Config","GetSection@Layer","??0Layer@","Load@Layer","Save@Layer")]:
        lines = [line for line in link_map.splitlines() if symbol in line]
        require(lines and all(obj in line and "common:" not in line for line in lines),
                "copied TU does not own linked observer/consumer/writer: " + symbol)
    for dll in base_dir.glob("*.dll"):
        shutil.copy2(dll, output / dll.name)
    shutil.copy2(base_dir / "qt.conf", output / "qt.conf")
    for directory in ("QtPlugins", "Sys"):
        shutil.copytree(base_dir / directory, output / directory)
    updated = copy.deepcopy(manifest)
    updated["purpose"] = "passive live DTK flag owner receipt; no native admission"
    shutil.copy2(Path(__file__),output/"observer_helper.py")
    updated["instrumented_executable_sha256"] = sha256(output / "Dolphin.exe")
    updated["dtk_owner"] = dict(helper_sha256=sha256(Path(__file__)),base_manifest_sha256=sha256(base_dir/"manifest.json"),
        base_oracle_sha256=BASE_ORACLE_SHA,original_mixer_sha256=DTK_PINS[MIXER_REL],
        copied_mixer_sha256=sha256(source),copied_mixer_object_sha256=sha256(output/"Mixer.obj"),copied_mixer_path=str(source),
        transformation_inverse_exact=True,original_flag_writer_count=2,
        changes="observer records and single-load original-condition local caches only",timing_reads_added=0,
        opcode_fetches_added=0,guest_writes_added=0,original_sources_and_libraries_modified=False,
        gdb_object_sha256=sha256(base_dir/"GDBStub.obj"),interpreter_object_sha256=sha256(base_dir/"Interpreter.obj"),
        audio_library_sha256=AUDIO_LIB_SHA,core_library_sha256=clock.LIB_SHA,compile_tlog_sha256=sha256(tlog),
        link_map_sha256=sha256(output/"Dolphin.map"),base_link_response_sha256=sha256(base_dir/"link.rsp"),
        compiler_sha256=sha256(tool_bin/"cl.exe"),linker_sha256=sha256(tool_bin/"link.exe"),msvc=str(args.msvc),sdk_version=args.sdk_version,
        original_layer_sha256=DTK_PINS[LAYER_REL],copied_layer_sha256=sha256(layer_source),copied_layer_path=str(layer_source),
        copied_layer_object_sha256=sha256(output/"Layer.obj"),layer_compile_tlog_sha256=sha256(layer_tlog),common_library_sha256=COMMON_LIB_SHA,
        compile_recipes={"Mixer":compile_args,"Layer":layer_args},link_recipe=rewritten,
        build_artifact_sha256={name:sha256(output/name) for name in
            ("Mixer.cpp","Mixer.obj","Mixer.cl.rsp","Mixer.cl.log","Layer.cpp","Layer.obj","Layer.cl.rsp","Layer.cl.log",
             "link.rsp","link.log","Dolphin.map","observer_helper.py")})
    write_json(output / "manifest.json", updated)
    pins(args)
    print("PASS fresh passive Mixer oracle", updated["instrumented_executable_sha256"])

def adapted_capture(control):
    require(sha256(Path(base.__file__)) == BASE_CAPTURE_SHA, "base capture changed")
    source = inspect.getsource(base.capture)
    old_window = "DOLPHIN_EVENT_WINDOW='init..timing-prefix-end@80373ac4'"
    require(source.count(old_window) == 1, "capture window transformation ambiguous")
    source = source.replace(old_window, "DOLPHIN_EVENT_WINDOW='" + WINDOW + "'")
    source = source.replace("'SHADOW_TIMING_'))", "'SHADOW_TIMING_','DOLPHIN_VI_'))")
    if control:
        old = "'-C','Dolphin.Interface.DebugModeEnabled=True','-e',str(args.disc)"
        new = "'-C','Dolphin.Interface.DebugModeEnabled=True','-C','Dolphin.DSP.DumpAudio=True','-e',str(args.disc)"
        require(source.count(old) == 1, "capture initial-option site ambiguous")
        source = source.replace(old, new)
    namespace = dict(base.__dict__)
    namespace["__file__"] = str(Path(__file__).resolve())
    exec(compile(source, str(Path(__file__).resolve()), "exec"), namespace)
    return namespace["capture"], hashlib.sha256(source.encode("utf-8")).hexdigest()

def read_rows(path):
    raw = path.read_bytes()
    require(raw.endswith(b"\n"), "truncated owner event stream")
    rows, headers = [], []
    previous = 0
    for number, line in enumerate(raw.decode("utf-8").splitlines(), 1):
        fields = line.split("\t")
        require(line != "LIMIT" and len(fields) == 16, "limited/malformed owner row")
        require(fields[0].isdigit() and fields[1].isdigit() and fields[13].isdigit(), "invalid event ordinal/thread/source line")
        require(all(re.fullmatch(r"[0-9a-fA-F]{16}", v) for v in fields[4:12]), "invalid event values")
        row = dict(seq=int(fields[0]),thread=int(fields[1]),kind=fields[2],name=fields[3],
                   v=tuple(int(v,16) for v in fields[4:12]),source=fields[12],line=int(fields[13]),function=fields[14])
        if row["kind"] == "event-filter":
            require(number == 1 and row["seq"] == 0 and row["name"] == "kinds=;window=" + WINDOW + ";limit=3000000"
                    and row["v"] == (3000000,0,0,0,1,0,0,0), "startup-open header differs")
            headers.append(row["name"])
        else:
            require(row["seq"] == previous + 1, "complete startup event sequence has gaps")
            previous = row["seq"]
            rows.append(row)
    require(len(headers) == 1 and rows and rows[-1]["kind"] == "timing-prefix-end" and
            rows[-1]["v"][0] == 0x80373ac4, "owner stream lacks explicit endpoint")
    return rows

def producer_receipt(rows, control=False):
    states, histories, pending = {}, {}, {}
    seen=set();pending_threads={};active=None;push_pending=None;finished=0
    consumers, writers, calls = [], [], []
    first_init = next(r for r in rows if r["kind"] == "init")
    dispatches = [r for r in rows if r["kind"] == "dispatch" and
                  r["name"] == "FinishExecutingCommand" and r["v"][2] == 0x300000001]
    require(dispatches, "DTK dispatch missing")
    for r in rows:
        kind, v, name = r["kind"], r["v"], r["name"]
        if kind=="dispatch" and name=="FinishExecutingCommand" and v[2]==0x300000001:
            require(active is None and push_pending is None,"nested/unfinished DTK callback")
            active=dict(thread=r["thread"],reads=0)
        elif kind=="callback-return" and name=="FinishExecutingCommand" and active is not None:
            require(active["thread"]==r["thread"] and active["reads"]==1 and push_pending is None,
                    "DTK callback lacks exactly one complete same-thread sample/read pair")
            active=None;finished+=1
        if not kind.startswith("dtk-owner-"):
            continue
        key = v[0]
        require(key != 0, "null Mixer identity")
        if kind == "dtk-owner-ctor":
            require(key not in seen and v[1] == 0, "ambiguous/reused/nondefault actual constructor flag")
            seen.add(key)
            states[key],histories[key],pending[key] = 0,[r],None
            continue
        require(key in states, "flag record without actual constructor")
        history = histories[key]
        history.append(r)
        if kind == "dtk-owner-dtor":
            require(pending[key] is None, "unfinished logging call at Mixer destruction")
            del states[key]
        elif kind in ("dtk-owner-start", "dtk-owner-stop"):
            require(pending[key] is None and v[1] == states[key], "racing/unexplained logging ingress")
            pending[key] = (kind, v[1], False)
            pending_threads[key]=r["thread"]
            calls.append(r)
        elif kind == "dtk-owner-write":
            expected = ("dtk-owner-start",0,1) if name == "start" else ("dtk-owner-stop",1,0)
            require(name in ("start","stop") and pending[key] == (expected[0],expected[1],False) and
                    v[1] == expected[2] and r["thread"]==pending_threads[key], "flag write lacks matching live branch producer")
            states[key] = v[1]
            pending[key] = (expected[0],expected[1],True)
            writers.append(r)
        elif kind in ("dtk-owner-start-exit","dtk-owner-stop-exit"):
            expected = {"started":("dtk-owner-start",0,True,1),"failed":("dtk-owner-start",0,False,0),
                "already-started":("dtk-owner-start",1,False,1),"stopped":("dtk-owner-stop",1,True,0),
                "already-stopped":("dtk-owner-stop",0,False,0)}.get(name)
            require(expected is not None and pending[key] == expected[:3] and v[1] == expected[3] == states[key],
                    "logging exit lacks completed producer")
            require(r["thread"]==pending_threads[key],"logging call changed thread")
            require(kind == expected[0] + "-exit", "logging exit kind differs")
            pending[key] = None
        elif kind == "dtk-owner-push":
            require(active is not None and active["thread"]==r["thread"] and push_pending is None and
                    pending[key] is None and v[2] in (0,1), "unbound/misnested/concurrent push ingress/rate")
            push_pending=(key,v[1],r["thread"])
        elif kind == "dtk-owner-read":
            require(active is not None and active["thread"]==r["thread"] and
                    push_pending==(key,v[2],r["thread"]),"read not paired with exact same-thread DTK push")
            push_pending=None;active["reads"]+=1
            require(pending[key] is None and v[1] == states[key], "actual consumer lacks complete flag ancestry")
            require(len(history) >= 2 and history[-2]["kind"] == "dtk-owner-push" and
                    history[-2]["v"][1] == v[2], "consumer lacks matching sample request")
            if not consumers:
                require(history[0]["seq"] < first_init["seq"] < dispatches[0]["seq"] < r["seq"],
                        "constructor does not predate initialization/first DTK dispatch")
                require(dispatches[0]["thread"] == r["thread"] and v[2] == 0, "first consumer is not zero-sample CPU DTK")
                relevant = [e for e in history if e["kind"] in ("dtk-owner-start","dtk-owner-stop","dtk-owner-write")]
                if control:
                    require(v[1] == 1 and [e["kind"] for e in relevant] == ["dtk-owner-start","dtk-owner-write"],
                            "initial DumpAudio control did not produce actual true flag")
                else:
                    require(v[1] == 0 and not relevant, "prior effective DTK log ingress exists")
            consumers.append(r)
        else:
            raise ValueError("unknown DTK observer kind")
    require(consumers and all(value is None for value in pending.values()), "missing/unfinished consumer lifecycle")
    require(active is None and push_pending is None and len(consumers)==len(dispatches)==finished,
            "DTK dispatch/consumer/callback cardinality differs")
    def brief(row):
        return dict(seq=row["seq"],thread=row["thread"],kind=row["kind"],name=row["name"],
                    values=[f"{x:016x}" for x in row["v"]],source_line=row["line"])
    return dict(scope="actual copied-source instance receipt only; historical original live flag remains unobserved",
        first_consumer=brief(consumers[0]),constructor=brief(next(r for r in rows if r["kind"]=="dtk-owner-ctor" and
            r["v"][0]==consumers[0]["v"][0])),consumer_count=len(consumers),effective_writer_count=len(writers),
        logging_entry_count=len(calls),first_flag_value=consumers[0]["v"][1],first_sample_count=consumers[0]["v"][2],
        writers=[brief(r) for r in writers],logging_entries=[brief(r) for r in calls],
        first_read_prior_effective_ingress="observed initial StartAudioDump producer" if control else "none",
        no_prior_effective_ingress_proved=not control,raw_host_config_toggle_absence_proved=False,
        arbitrary_racing_host_ingress_equivalence_proved=False,true_branch_wavefile_consequences_owned=False,
        next_native_dependency="GPU BlockingLoop atomic AllowSleep delivery/worker ingress")

def malformed_receipt_tests(rows):
    mutations = []
    first = next(i for i,r in enumerate(rows) if r["kind"]=="dtk-owner-read")
    ctor = next(i for i,r in enumerate(rows) if r["kind"]=="dtk-owner-ctor" and r["v"][0]==rows[first]["v"][0])
    for index,field,value in [(first,1,1),(first,0,0),(first,2,1),(ctor,1,1)]:
        altered = list(rows);altered[index]=dict(rows[index])
        v=list(altered[index]["v"]);v[field]=value;altered[index]["v"]=tuple(v)
        mutations.append(altered)
    mutations += [rows[:ctor]+rows[ctor+1:],rows[:first]+rows[first+1:]]
    altered=list(rows);altered[first]=dict(rows[first],thread=rows[first]["thread"]+1);mutations.append(altered)
    altered=list(rows)
    altered.insert(first,dict(rows[first-1],kind="dtk-owner-start",name="-",v=(rows[first]["v"][0],0,0,0,0,0,0,0)))
    mutations.append(altered)
    altered=list(rows);altered[first]=dict(rows[first],kind="dtk-owner-unproved");mutations.append(altered)
    push=next(i for i,r in enumerate(rows) if r["kind"]=="dtk-owner-push")
    altered=list(rows);altered.insert(push,dict(rows[push]));mutations.append(altered)
    later=next(i for i,r in enumerate(rows[first+1:],first+1) if r["kind"]=="dtk-owner-read")
    altered=list(rows);altered[later]=dict(rows[later],thread=rows[later]["thread"]+1);mutations.append(altered)
    end=next(i for i,r in enumerate(rows[first+1:],first+1) if r["kind"]=="callback-return" and r["name"]=="FinishExecutingCommand")
    mutations.append(rows[:end]+rows[end+1:])
    for altered in mutations:
        try:
            producer_receipt(altered)
        except (ValueError,StopIteration):
            continue
        raise ValueError("unexplained flag ancestry mutation accepted")
    return len(mutations)

def config_receipt(rows, first_consumer, control=False):
    selected=[r for r in rows if r["kind"]=="dtk-config" and r["seq"]<first_consumer["seq"]]
    require(selected, "no configuration lifecycle receipt before first DTK read")
    for r in selected:
        require(r["v"][1] in (0,1,2), "unknown configuration value or mutable DSP lease")
        require(r["name"] in ("construct","construct-loader","load-save-end","set","delete","delete-all","add-live","resolve-result"),
                "unknown configuration ingress")
    if control:
        require(any(r["name"]=="set" and r["v"][1]==2 for r in selected),
                "initial DumpAudio control not seen at actual configuration writer")
    else:
        require(all(r["v"][1] in (0,1) for r in selected), "true logging configuration ingress exists")
    return dict(records=len(selected),changed_sets=sum(r["name"]=="set" for r in selected),
        initialized_or_loaded_layers=sorted({r["v"][0] for r in selected if r["name"].startswith("construct")}),
        true_values=sum(r["v"][1]==2 for r in selected),mutable_dsp_leases=0,
        changed_config_ingress_excluded=not control,
        unchanged_noop_set_attempts_observed=False,
        scope="every changed Set/delete plus layer/load snapshots and mutable DSP lease exclusion; no-op request attempts do not change semantics")

def validate_owner_artifacts(args, capture):
    owner=capture["instrumentation_manifest"]["dtk_owner"]
    output=args.oracle_output
    expected_files={"Mixer.cpp","Mixer.obj","Mixer.cl.rsp","Mixer.cl.log","Layer.cpp","Layer.obj","Layer.cl.rsp","Layer.cl.log",
                    "link.rsp","link.log","Dolphin.map","observer_helper.py"}
    require(set(owner["build_artifact_sha256"])==expected_files,"incomplete private build provenance")
    for name,expected in owner["build_artifact_sha256"].items():
        require(sha256(output/name)==expected,"private build artifact differs: "+name)
    require(owner["audio_library_sha256"]==AUDIO_LIB_SHA and owner["core_library_sha256"]==clock.LIB_SHA and
            owner["common_library_sha256"]==COMMON_LIB_SHA,"producer libraries differ")
    for tu in ("Mixer","Layer"):
        require(builder.command_args((output/(tu+".cl.rsp")).read_text(encoding="utf-16"))==owner["compile_recipes"][tu],
                "private compile recipe differs")
        require(sha256(output/(tu+".obj"))==owner["copied_"+tu.lower()+"_object_sha256"],"compiled TU pin differs")
    require(builder.command_args((output/"link.rsp").read_text(encoding="utf-16"))==owner["link_recipe"],"private link recipe differs")
    require(sha256(args.base_oracle/"link.rsp")==owner["base_link_response_sha256"] and
            sha256(args.base_oracle/"manifest.json")==owner["base_manifest_sha256"],"base recipe/manifest differs")
    require(sha256(output/"Dolphin.map")==owner["link_map_sha256"],"linked symbol map differs")
    for tool,key in (("cl","compiler_sha256"),("link","linker_sha256")):
        require(sha256(args.msvc/"bin/Hostx64/x64"/(tool+".exe"))==owner[key],"compiler/linker differs")
    require(sha256(args.base_oracle/"GDBStub.obj")==owner["gdb_object_sha256"] and
            sha256(args.base_oracle/"Interpreter.obj")==owner["interpreter_object_sha256"],"retained observer objects differ")
    for name,expected in capture["instrumentation_manifest"]["link_input_sha256"].items():
        require(sha256(Path(name))==expected,"original link input differs")

def build_binding_falsification(args, capture):
    cases=[]
    for field in ("audio_library_sha256","core_library_sha256","common_library_sha256","base_manifest_sha256",
                  "base_link_response_sha256","compiler_sha256","linker_sha256","gdb_object_sha256",
                  "interpreter_object_sha256","copied_mixer_object_sha256","copied_layer_object_sha256","link_map_sha256"):
        altered=copy.deepcopy(capture);altered["instrumentation_manifest"]["dtk_owner"][field]="0"*64;cases.append(altered)
    for name in ("Mixer.cpp","Mixer.obj","Mixer.cl.rsp","Layer.cpp","Layer.obj","Layer.cl.rsp","Dolphin.map","link.rsp","observer_helper.py"):
        altered=copy.deepcopy(capture);altered["instrumentation_manifest"]["dtk_owner"]["build_artifact_sha256"][name]="0"*64;cases.append(altered)
    altered=copy.deepcopy(capture);del altered["instrumentation_manifest"]["dtk_owner"]["build_artifact_sha256"]["Mixer.cl.rsp"];cases.append(altered)
    for altered in cases:
        try:
            validate_owner_artifacts(args,altered)
        except ValueError:
            continue
        raise ValueError("altered build binding accepted")
    return len(cases)

def audit_capture(args, capture, path, baseline, baseline_audit, control):
    source_pins=pins(args)
    validate_owner_artifacts(args,capture)
    manifest=capture["instrumentation_manifest"];owner=manifest["dtk_owner"]
    require(capture["capture_complete"] and capture["capture_tool_sha256"] == sha256(Path(__file__)) ==
            owner["helper_sha256"], "new helper/capture identity differs")
    require(capture["dolphin_sha256"] == manifest["instrumented_executable_sha256"] ==
            sha256(args.oracle_output/"Dolphin.exe"), "new executable binding differs")
    require(capture["controlled_midchain_writes"] == [] and capture["sole_internal_stop"] == "80373ac4",
            "unexpected capture mutation/schedule")
    require(capture["observer_environment"] == dict(event_window=WINDOW,event_kinds="",event_limit="3000000",
            mmio_watch=WATCH,mmio_watch_phase0=True), "unbound startup observer environment")
    copied_path=Path(owner["copied_mixer_path"])
    require(sha256(copied_path) == owner["copied_mixer_sha256"] and copied_path.read_text(encoding="utf-8") ==
            observe_source((args.source_root/MIXER_REL).read_text(encoding="utf-8")), "copied flag observer differs")
    layer_path=Path(owner["copied_layer_path"])
    require(sha256(layer_path)==owner["copied_layer_sha256"] and layer_path.read_text(encoding="utf-8") ==
            observe_source((args.source_root/LAYER_REL).read_text(encoding="utf-8"),LAYER_REPLACEMENTS),
            "copied configuration observer differs")
    paths={k:path.with_name(capture[n]) for k,n in
           (("events","event_log"),("instructions","instruction_trace"),("mmio","mmio_log"))}
    for key,p in paths.items():
        field={"events":"event_log_sha256","instructions":"instruction_trace_sha256","mmio":"mmio_log_sha256"}[key]
        require(sha256(p)==capture[field], "capture stream binding differs")
    rows=read_rows(paths["events"])
    original_rows=[r for r in rows if not r["kind"].startswith("dtk-owner-") and r["kind"]!="dtk-config"]
    lifecycle=timing.Audit().run(original_rows)
    lifecycle["callback_projection"]=timing.callback_projection(original_rows)
    lifecycle["source_locations"]=timing.source_locations(original_rows,args.source_root,source_pins,capture)
    lifecycle["instruction_cost_audit"]=timing.instruction_audit(paths["instructions"],args.source_root,
        lifecycle["elapsed_entry_to_clock"])
    lifecycle["mmio_access_audit"]=timing.mmio_audit(paths["mmio"])
    require(lifecycle["mmio_access_audit"]["watched_cpu_mmio_writes"] ==
            lifecycle["mmio_access_audit"]["watched_cpu_mmio_reads"] == 0, "new watched device ingress")
    lines=copied_path.read_text(encoding="utf-8").splitlines()
    layer_lines=layer_path.read_text(encoding="utf-8").splitlines()
    for r in rows:
        if r["kind"].startswith("dtk-owner-"):
            require(Path(r["source"]).resolve()==copied_path.resolve(), "unbound flag observer source")
            require(1 <= r["line"] <= len(lines) and '"'+r["kind"]+'"' in lines[r["line"]-1],
                    "flag observer source-location mismatch")
        elif r["kind"]=="dtk-config":
            require(Path(r["source"]).resolve()==layer_path.resolve() and
                    1 <= r["line"] <= len(layer_lines) and '"dtk-config"' in layer_lines[r["line"]-1],
                    "configuration observer source-location mismatch")
    require(sha256(args.baseline)==baseline_audit["capture"]["sha256"], "baseline audit identity differs")
    for key in ("entry","first_clock","elapsed_entry_to_clock","pending_at_first_clock","interval_dispatch_counts"):
        require(lifecycle[key]==baseline_audit[key], "new observer changed "+key)
    require(lifecycle["callback_projection"]["boot_unit_steps"] ==
            baseline_audit["callback_projection"]["boot_unit_steps"], "new observer changed pre-entry work")
    require(capture["instruction_trace_sha256"]==baseline["instruction_trace_sha256"],
            "new observer changed instruction timing trace")
    baseline_rows,_,_=timing.read_logs([args.baseline.with_name(baseline["event_log"])])
    # Exact semantic scheduler/device/CPU projection. Other rows contain Qt
    # handles/pointers and live host RTC input, so raw cross-process equality
    # of those rows is not an equivalence test. Validate the RTC chain below.
    projection_kinds={"init","config","init-ready","register","enqueue","dispatch","callback-return",
        "cancel","cancel-item","cancel-all","clear","adjust","adjust-item","ts-publish","ts-commit","ts-move",
        "boundary","timing-prefix-end"}
    def admitted(r):
        return r["kind"] in projection_kinds or r["kind"].startswith("vt-")
    def stable(r):
        return (r["kind"],r["name"],r["v"])
    init_index=next(i for i,r in enumerate(original_rows) if r["kind"]=="init")
    original_projection=[stable(r) for r in original_rows[init_index:] if admitted(r)]
    baseline_projection=[stable(r) for r in baseline_rows if admitted(r)]
    require(original_projection==baseline_projection, "new observer changed semantic queue/device projection")
    rtc_local=[r for r in rows if r["kind"]=="rtc-local"]
    rtc_result=[r for r in rows if r["kind"]=="rtc-result"]
    require(len(rtc_local)==len(rtc_result)==2 and
            all(a["v"][:3]==(b["v"][0],0,b["v"][1]) and b["v"][1]==0x386d4380 and
                b["v"][2]==(b["v"][0]-b["v"][1])&0xffffffff for a,b in zip(rtc_local,rtc_result)),
            "fresh RTC producer equation differs")
    import validate_produced_clock as produced
    _,_,seconds,epoch,_=produced.checked_inputs(capture)
    require(seconds==rtc_result[0]["v"][2] and epoch==seconds*40500000, "RTC-to-TB epoch production differs")
    receipt=producer_receipt(rows,control)
    first_row=next(r for r in rows if r["kind"]=="dtk-owner-read")
    receipt["configuration_lifecycle"]=config_receipt(rows,first_row,control)
    receipt.update(source_pin_count=len(source_pins),source_pins=source_pins,lifecycle=lifecycle,
        original_projection_record_count=len(original_projection),
        original_projection_kinds=sorted({r[0] for r in original_projection}),
        rtc_producer=dict(local_calls=2,epoch_seconds=seconds,epoch_value=f"{epoch:016x}",equations_checked=True),
        capture_sha256=sha256(path),event_sha256=capture["event_log_sha256"],
        instruction_sha256=capture["instruction_trace_sha256"],mmio_sha256=capture["mmio_log_sha256"],
        observer_sha256=owner["copied_mixer_sha256"],oracle_sha256=capture["dolphin_sha256"])
    if not control:
        receipt["malformed_ancestry_declines"]=malformed_receipt_tests(rows)
        receipt["build_binding_declines"]=build_binding_falsification(args,capture)
    native_args=copy.copy(args);native_args.out=path
    receipt["native_research_checkpoint_parity"]=native_parity(capture,native_args,paths)
    return receipt

def capture_one(args, path, profile, control):
    capture,adapted_sha=adapted_capture(control)
    options=argparse.Namespace(dolphin=args.oracle_output/"Dolphin.exe",disc=args.disc,user_dir=profile,out=path,
        port=select_explicit_port(args.port),l2cr=0,handler=0,pause_entry=0,debug=None)
    capture(options)
    result=json.loads(path.read_text(encoding="utf-8"))
    result["name"]="passive_live_dtk_log_owner_42"+("_dump_control" if control else "")
    result["scope"]="copied-source actual flag receipt; no historical or production admission"
    result["capture_adaptation"]=dict(base_capture_sha256=BASE_CAPTURE_SHA,adapted_function_sha256=adapted_sha,
        startup_open_window=True,initial_dump_audio_control=control,original_capture_modified=False)
    result["initial_configuration"]={"Dolphin.DSP.DumpAudio":True} if control else {}
    write_json(path,result)
    require(sha256(Path(base.__file__))==BASE_CAPTURE_SHA, "base capture tool changed")
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ("source-root","original-build","base-oracle","oracle-output","msvc","sdk-root","library",
                 "audio-library","common-library","disc","baseline","baseline-audit","user-dir","out",
                 "program","prefix-program","dol"):
        parser.add_argument("--"+name,type=lambda x:Path(x).resolve(),required=True)
    parser.add_argument("--sdk-version",required=True)
    parser.add_argument("--port",type=int)
    parser.add_argument("--program-sha256",required=True)
    parser.add_argument("--prefix-program-sha256",required=True)
    parser.add_argument("--dump-control",action="store_true",
                        help="also capture a fresh initial DumpAudio=True source producer control")
    args=parser.parse_args()
    require(all(re.fullmatch("[0-9a-f]{64}",value) for value in (args.program_sha256,args.prefix_program_sha256)),
            "explicit native SHA256 required")
    args.build_root=Path(__file__).resolve().parents[3]/"build"
    require(all(p.is_relative_to(args.build_root) for p in (args.oracle_output,args.user_dir,args.out)),
            "all outputs must be under canonical repository build")
    require(not args.user_dir.exists() and not args.out.exists(), "fresh receipt/profile required")
    baseline=json.loads(args.baseline.read_text(encoding="utf-8"))
    baseline_audit=json.loads(args.baseline_audit.read_text(encoding="utf-8"))
    require(baseline["capture_complete"] and baseline["controlled_initial_l2cr"]=="00000000" and
            baseline["pause_entry_seconds"]==0 and baseline["dolphin_sha256"]==BASE_ORACLE_SHA,
            "baseline reference profile differs")
    build_oracle(args)
    captured=capture_one(args,args.out,args.user_dir,False)
    report=audit_capture(args,captured,args.out,baseline,baseline_audit,False)
    report["helper_sha256"]=sha256(Path(__file__))
    if args.dump_control:
        control_path=args.out.with_name(args.out.stem+"-dump-control.json")
        control_profile=args.user_dir.with_name(args.user_dir.name+"-dump-control")
        controlled=capture_one(args,control_path,control_profile,True)
        report["positive_source_control"]=audit_capture(args,controlled,control_path,baseline,baseline_audit,True)
    write_json(args.out.with_suffix(".receipt.json"),report)
    print("PASS actual DTK logging read and bounded producer ancestry")
    print(json.dumps({k:report[k] for k in ("first_flag_value","first_sample_count","consumer_count",
        "effective_writer_count","malformed_ancestry_declines","oracle_sha256","helper_sha256")},indent=2))

def exact_tree(actual, expected):
    require(type(actual) is type(expected), "native owned field type differs")
    if isinstance(expected,dict):
        require(set(actual)==set(expected), "native owned field set differs")
        for key,value in expected.items():
            exact_tree(actual[key],value)
    elif isinstance(expected,list):
        require(len(actual)==len(expected), "native owned list cardinality differs")
        for a,b in zip(actual,expected):
            exact_tree(a,b)
    else:
        require(actual==expected, "native owned value/order differs")

def owned_expected(enabled=False):
    cpu=486000000;half_line=(2*cpu//27000000)*429
    ais=1124*2;aid=ais*3//2;audio=cpu*aid//(108000000*4//32)
    dtk=cpu*6*28*ais//108000000
    def event(name,deadline,fifo,userdata=0):
        return dict(name=name,deadline=deadline,fifo=fifo,userdata=f"{userdata:016x}")
    initial=[event("FinishExecutingCommand",0,0,0x300000001),event("GPUSleeper",0,1),
        event("VICallback",half_line,2),event("DSPCallback",0,3),
        event("AudioDMACallback",audio,4),event("PatchEngine",half_line*525,5)]
    queue=sorted(initial[2:]+[event("FinishExecutingCommand",dtk,6,0x300000001)],
                 key=lambda x:(x["deadline"],x["fifo"]))
    journal=[]
    def record(kind,name="-",values=()):
        journal.append(dict(kind=kind,name=name,v=[f"{x:016x}" for x in tuple(values)+(0,)*(8-len(values))]))
    record("dtk-owned-mixer-construct",values=(0,48000))
    record("init",values=(0,20000,0,1,0,0,0,0))
    record("init-ready",values=(0,20000,0,1,0,0,0x3f800000,0))
    for e in initial:
        record("enqueue",e["name"],(e["deadline"],e["fifo"],int(e["userdata"],16),e["deadline"]))
    record("dtk-owned-dump-default",values=(0,))
    record("advance-enter",values=(0,20000,0,1,6,0,0x3f800000,0))
    record("advance-clock",values=(20000,20000,0,1,6,0,0x3f800000,20000))
    record("dispatch","FinishExecutingCommand",(0,0,0x300000001,20000))
    record("dtk-transfer-zero",values=(0,0))
    record("dtk-zero-sample-request",values=(0,0,0))
    record("dtk-owned-log-read",values=(0,0))
    record("dtk-no-wave-write",values=(0,))
    record("dtk-pending-blocks",values=(6,))
    record("enqueue","FinishExecutingCommand",(dtk,6,0x300000001,dtk-20000))
    record("callback-return","FinishExecutingCommand",(0,0,0x300000001))
    record("dispatch","GPUSleeper",(0,1,0,20000))
    record("gpu-allow-sleep-request",values=(1,))
    record("stop","BlockingLoop.h:233 undelivered-allow-sleep")
    result=dict(schema="initial-boot-events-42-v1",scope="research-only",control="owned-fresh-dtk-source",
        stop="BlockingLoop.h:233 undelivered-allow-sleep",
        scheduler={"global":20000,"slice":20000,"downcount":0,"sane":1,"next_fifo":7,"inverse_bits":0x3f800000},
        queue=queue,active_callback=initial[1],pi=dict(cause=0x10100,mask=0,exceptions=0),
        dvd=dict(dimar=0,dilength=0,stream=0,pending_blocks=6,decoded_blocks=0,push_calls=1,streaming_frames=0),
        ai=dict(playing=0,ais_divisor=ais,aid_divisor=aid),
        dsp=dict(hle_rom=1,dma_enabled=0,mail_halted=1,rom_mail=0x8071feed,slice=0,update_calls=0,last_update_cycles=0),
        vi=dict(half_line=0,next_si_poll=15,last_line_start=0,odd_first=520,odd_last=519,even_first=1045,even_last=1044),
        movie=dict(frame=None,lag=None,polled=None,total_frames=None,total_lag=None),
        effects=dict(gpu_sleep_delivered=0,gpu_allow_sleep_calls=0,new_field_calls=0,achievement_return_calls=0,guest_ram_write_bytes=0),
        dtk_logging_owner=dict(constructor_owned=1,configuration_owned=1,flag_read=1,enabled=0,backend_sample_rate=48000),
        journal=journal)
    if enabled:
        result["control"]="owned-initial-dump-enabled"
        result["stop"]="AudioCommon.cpp:78 unknown-audio-dump-started"
        result["scheduler"].update({"global":0,"next_fifo":6})
        result["queue"]=sorted(initial,key=lambda x:(x["deadline"],x["fifo"]))
        result["active_callback"]=None
        result["dvd"].update(pending_blocks=0,push_calls=0)
        result["dtk_logging_owner"].update(configuration_owned=0,flag_read=0)
        journal=journal[:9]
        result["journal"]=journal
        record("dtk-owned-dump-enabled",values=(1,))
        record("stop",result["stop"])
    return result

def owned_mutations(actual, expected):
    paths=[];containers=[];objects=[]
    def walk(value,path=()):
        if isinstance(value,dict):
            objects.append(path)
            for key,v in value.items():
                containers.append(path+(key,));walk(v,path+(key,))
        elif isinstance(value,list):
            for key,v in enumerate(value):
                containers.append(path+(key,));walk(v,path+(key,))
        else:
            paths.append(path)
    walk(actual)
    count=0
    for path,remove in [(p,False) for p in paths]+[(p,True) for p in containers]:
        altered=copy.deepcopy(actual);target=altered
        for key in path[:-1]:target=target[key]
        if not remove:
            value=target[path[-1]]
            target[path[-1]]=value+1 if type(value) is int else 0 if value is None else value+"X"
        else:
            del target[path[-1]]
        try:
            exact_tree(altered,expected)
        except ValueError:
            count+=1;continue
        raise ValueError("owned native omission/field/mode mutation accepted")
    for path in paths:
        original=actual
        for key in path:original=original[key]
        if type(original) is not int:
            continue
        altered=copy.deepcopy(actual);target=altered
        for key in path[:-1]:target=target[key]
        target[path[-1]]=bool(original)
        try:
            exact_tree(altered,expected)
        except ValueError:
            count+=1;continue
        raise ValueError("numeric bool was accepted as an owned count/state")
    for path in objects:
        altered=copy.deepcopy(actual);target=altered
        for key in path:target=target[key]
        target["unexplained_extra_field"]=0
        try:
            exact_tree(altered,expected)
        except ValueError:
            count+=1;continue
        raise ValueError("extra owned field accepted")
    return count

def compare_owned_main():
    parser=argparse.ArgumentParser(description="Fresh DTK owned native prefix; frozen receipt audit first")
    parser.add_argument("--compare-owned-native",action="store_true",required=True)
    for name in ("program","old-program","capture","receipt","source-root","baseline","baseline-audit",
                 "clock-program","prefix-program","dol","out"):
        parser.add_argument("--"+name,type=lambda x:Path(x).resolve(),required=True)
    for name in ("program-sha256","old-program-sha256","receipt-sha256"):
        parser.add_argument("--"+name,required=True)
    args=parser.parse_args()
    build=Path(__file__).resolve().parents[3]/"build"
    require(args.out.is_relative_to(build),"native comparison output outside build")
    require(sha256(args.program)==args.program_sha256 and sha256(args.old_program)==args.old_program_sha256,
            "native comparison executable pin differs")
    require(sha256(args.receipt)==args.receipt_sha256,"frozen receipt report differs")
    report=json.loads(args.receipt.read_text(encoding="utf-8"));capture=json.loads(args.capture.read_text(encoding="utf-8"))
    require(report["capture_sha256"]==sha256(args.capture),"wrong receipt capture binding")
    exact_tree(report["first_flag_value"],0)
    exact_tree(report["no_prior_effective_ingress_proved"],True)
    manifest=capture["instrumentation_manifest"];owner=manifest["dtk_owner"]
    oracle=Path(owner["copied_mixer_path"]).parent
    frozen_path=oracle/"observer_helper.py"
    require(sha256(frozen_path)==owner["helper_sha256"]==capture["capture_tool_sha256"]==report["helper_sha256"],
            "archived capture-helper identity differs")
    import importlib.util
    spec=importlib.util.spec_from_file_location("frozen_dtk_receipt_42",frozen_path)
    frozen=importlib.util.module_from_spec(spec);spec.loader.exec_module(frozen)
    inputs={Path(p).name.lower():Path(p) for p in manifest["link_input_sha256"]}
    parity=report["native_research_checkpoint_parity"]
    audit_args=argparse.Namespace(source_root=args.source_root,library=inputs["core.lib"],
        audio_library=inputs["audiocommon.lib"],common_library=inputs["common.lib"],oracle_output=oracle,
        base_oracle=Path(owner["compile_recipes"]["Mixer"][-1]).parent.parent/"passive-timing-oracle-42c",
        msvc=Path(owner["msvc"]),baseline=args.baseline,baseline_audit=args.baseline_audit,out=args.capture,
        program=args.clock_program,program_sha256=parity["executable_sha256"],
        prefix_program=args.prefix_program,prefix_program_sha256=parity["prefix_executable_sha256"],dol=args.dol)
    # Every original observer/source/library/build/lifecycle/trace binding is
    # rerun by the exact source that produced the receipt. No metadata is relabeled.
    baseline=json.loads(args.baseline.read_text(encoding="utf-8"))
    baseline_audit=json.loads(args.baseline_audit.read_text(encoding="utf-8"))
    repeated=frozen.audit_capture(audit_args,capture,args.capture,baseline,baseline_audit,False)
    for key in ("first_consumer","constructor","configuration_lifecycle","consumer_count","effective_writer_count",
                "logging_entry_count","original_projection_record_count","event_sha256","oracle_sha256"):
        exact_tree(repeated[key],report[key])
    control_path=args.capture.with_name(args.capture.stem+"-dump-control.json")
    control=json.loads(control_path.read_text(encoding="utf-8"))
    positive=report["positive_source_control"]
    require(sha256(control_path)==positive["capture_sha256"],"positive receipt capture binding differs")
    repeated_control=frozen.audit_capture(audit_args,control,control_path,baseline,baseline_audit,True)
    for key in ("first_consumer","constructor","configuration_lifecycle","consumer_count","effective_writer_count",
                "logging_entry_count","original_projection_record_count","event_sha256","oracle_sha256"):
        exact_tree(repeated_control[key],positive[key])
    owned=subprocess.run([str(args.program),"--dump-owned-dtk"],capture_output=True,check=True).stdout
    actual=json.loads(owned.decode("utf-8"));expected=owned_expected();exact_tree(actual,expected)
    rows=frozen.read_rows(args.capture.with_name(capture["event_log"]))
    first=next(i for i,r in enumerate(rows) if r["kind"]=="dispatch" and r["name"]=="FinishExecutingCommand" and r["v"][2]==0x300000001)
    gpu=next(i for i,r in enumerate(rows[first+1:],first+1) if r["kind"]=="dispatch" and r["name"]=="GPUSleeper")
    selected=[r for r in rows[first:gpu+1] if r["kind"] in ("dispatch","enqueue","callback-return","dtk-owner-push","dtk-owner-read")]
    require([r["kind"] for r in selected]==["dispatch","dtk-owner-push","dtk-owner-read","enqueue","callback-return","dispatch"],
            "genuine producer prefix has missing/reordered records")
    native_selected=[r for r in actual["journal"] if r["kind"] in ("dispatch","enqueue","callback-return")][-4:]
    source_selected=[r for r in selected if r["kind"] in ("dispatch","enqueue","callback-return")]
    require([(r["kind"],r["name"],tuple(int(x,16) for x in r["v"])) for r in native_selected] ==
            [(r["kind"],r["name"],r["v"]) for r in source_selected],"owned ordered prefix differs from producer receipt")
    require(tuple(int(x,16) for x in actual["journal"][0]["v"][:2])==tuple(int(x,16) for x in report["constructor"]["values"][1:3]) and
            selected[2]["v"][1:3]==(0,0) and selected[1]["v"][1:3]==(0,1),"owned constructor/read differs from actual values")
    old_modes={}
    for option in ("--dump","--dump-bound-control"):
        original=subprocess.run([str(args.old_program),option],capture_output=True,check=True).stdout
        current=subprocess.run([str(args.program),option],capture_output=True,check=True).stdout
        require(original==current,"historical default/control bytes changed")
        old_modes[option]=dict(bytes=len(current),sha256=hashlib.sha256(current).hexdigest())
    enabled_raw=subprocess.run([str(args.program),"--dump-owned-dtk-enabled"],capture_output=True,check=True).stdout
    enabled=json.loads(enabled_raw.decode("utf-8"))
    expected_enabled=owned_expected(True);exact_tree(enabled,expected_enabled)
    result=dict(scope="research-owned fresh DTK only; stop at GPU, no connected production admission",
        receipt_pipeline="complete frozen44-pin off/control audits rerun before native comparison",receipt_sha256=args.receipt_sha256,
        capture_sha256=sha256(args.capture),program_sha256=args.program_sha256,
        archived_capture_tool_sha256=owner["helper_sha256"],comparison_tool_sha256=sha256(Path(__file__)),
        old_modes_byte_identical=old_modes,owned_journal_records=len(actual["journal"]),
        native_output_sha256=hashlib.sha256(owned).hexdigest(),
        source_ordered_prefix_records=len(selected),native_field_and_omission_declines=owned_mutations(actual,expected),
        enabled_field_and_omission_declines=owned_mutations(enabled,expected_enabled),
        stop=actual["stop"],enabled_stop=enabled["stop"],next_fifo=actual["scheduler"]["next_fifo"],
        unresolved=["GPU atomic delivery/worker ownership","Movie/NewField/achievement live ingress",
                    "historical original DTK live flag","physical Gekko elapsed-time equivalence"])
    args.out.with_suffix(".native.json").write_bytes(owned)
    args.out.with_suffix(".enabled.json").write_bytes(enabled_raw)
    write_json(args.out,result);print(json.dumps(result,indent=2))

if __name__ == "__main__":
    if "--compare-owned-native" in sys.argv:
        compare_owned_main()
    else:
        main()
