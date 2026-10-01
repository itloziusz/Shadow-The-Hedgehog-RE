"""Fresh SI-device NONE falsifier of the bounded source timing contract.

Uses pinned original capture logic with two explicit changes: initial
Core.SIDevice0=0 and additional SI/DVD MMIO watches. Original files stay read
only. Reports native produced-work parity as research, never admission.
"""
import argparse
import hashlib
import inspect
import json
from pathlib import Path
import socket
import subprocess
import sys

import agent_clock_source as clock
import agent_timing_source_42 as timing
import capture_timing_prefix as base
import validate_produced_clock as produced
from capture_dolphin_rsp import sha256

BASE_CAPTURE_SHA = "d477a14b913c1b9d3b07be5a0412233121f75aa1d368a7a57b3940df388f9f81"
BASE_WATCH = "0c005000:0c005100;0c002000:0c002100;0c003000:0c003008;0c006c00:0c006c20"
ADDED_WATCH = ["0c006400:0c006500", "0c006000:0c006100"]
EXTRA_PINS = {
    "Source/Core/Core/Config/MainSettings.cpp": "31349025a4a3d4ed7a5b32e9aa6b4d6aa52c522b2bc85b0e909876f685c78c71",
    "Source/Core/Core/HW/SI/SI_Device.cpp": "0439866a62fe3aafec979a01e68345e7785eed8fda49e4567ac149af3d158dfc",
    "Source/Core/Core/HW/SI/SI_DeviceNull.cpp": "169d6c8c6cbe880d679b241e557912e03237653503283b02945d62dc75bd1cff",
    "Source/Core/Core/HW/SI/SI_Device.h": "d4732e4d5f054414f8657f8bbe633e6a033c162191c37887da48a5e063a37ac8",
    "Source/Core/Core/HW/SI/SI.h": "00c23198d5541f76e010eb3119442bb4a37cc8c2d8db2ebff143956860783205",
    "Source/Core/Core/Core.cpp": "8bf9e90fc56b5b4d7a1e42410ea2e2291f44b5d86fb4e982b3aa68912edf5944",
}

def require(ok, why):
    if not ok:
        raise ValueError(why)

def select_explicit_port(requested=None, socket_factory=socket.socket):
    """Probe an explicit bind; a Windows bind(0) port may be excluded later."""
    require(requested is None or (type(requested) is int and 0 < requested < 65536),
            "invalid explicit GDB port")
    candidates = range(30000,49152) if requested is None else (requested,)
    for candidate in candidates:
        with socket_factory() as sock:
            try:
                sock.bind(("127.0.0.1",candidate))
            except OSError:
                continue
        return candidate
    raise ValueError("no explicitly bindable loopback GDB port")

def adapted_capture():
    require(sha256(Path(base.__file__)) == BASE_CAPTURE_SHA, "base capture source changed")
    source = inspect.getsource(base.capture)
    old_watch = "mmio_watch='" + BASE_WATCH + "'"
    new_watch = "mmio_watch='" + BASE_WATCH + ";" + ";".join(ADDED_WATCH) + "'"
    old_command = "'-C','Dolphin.Interface.DebugModeEnabled=True','-e',str(args.disc)"
    new_command = "'-C','Dolphin.Interface.DebugModeEnabled=True','-C','Dolphin.Core.SIDevice0=0','-e',str(args.disc)"
    require(source.count(old_watch) == 1 and source.count(old_command) == 1,
            "capture adaptation sites changed")
    adapted = source.replace(old_watch, new_watch).replace(old_command, new_command)
    namespace = dict(base.__dict__)
    namespace["__file__"] = str(Path(__file__).resolve())
    exec(compile(adapted, str(Path(__file__).resolve()), "exec"), namespace)
    return namespace["capture"], hashlib.sha256(adapted.encode("utf-8")).hexdigest()

def selected_si(rows):
    kinds = {"sc-device-init", "sc-device-check", "vt-si-init", "vt-poll",
             "vt-devices-enter", "vt-device-data", "vt-devices-exit", "vt-poll-next"}
    return [{"kind": r["kind"], "v": [f"{x:016x}" for x in r["v"]]}
            for r in rows if r["kind"] in kinds]

def validate_capture(capture, args):
    environment = {
        "event_window": "init..timing-prefix-end@80373ac4",
        "event_kinds": "", "event_limit": "3000000",
        "mmio_watch": BASE_WATCH + ";" + ";".join(ADDED_WATCH),
        "mmio_watch_phase0": True,
    }
    require(capture["observer_environment"] == environment and capture["capture_complete"],
            "extended observer environment is not bound")
    require(capture["capture_tool_sha256"] == sha256(Path(__file__)),
            "own capture helper source changed")
    require(not capture["controlled_midchain_writes"], "midchain writer")
    pins = dict(clock.SOURCE_PINS, **timing.EXTRA_PINS, **EXTRA_PINS)
    for relative, expected in pins.items():
        require(sha256(args.source_root / relative) == expected, "source pin changed: " + relative)
    require(sha256(args.library) == clock.LIB_SHA, "core library changed")
    paths = {kind: args.out.with_name(capture[key])
             for kind, key in [("events", "event_log"), ("instructions", "instruction_trace"), ("mmio", "mmio_log")]}
    for kind, path in paths.items():
        require(sha256(path) == capture[{"events":"event_log_sha256", "instructions":"instruction_trace_sha256",
                                       "mmio":"mmio_log_sha256"}[kind]], "capture file binding changed")
    rows, inputs, headers = timing.read_logs([paths["events"]])
    audit = timing.Audit().run(rows)
    audit["callback_projection"] = timing.callback_projection(rows)
    audit["source_locations"] = timing.source_locations(rows, args.source_root, pins, capture)
    audit["instruction_cost_audit"] = timing.instruction_audit(
        paths["instructions"], args.source_root, audit["elapsed_entry_to_clock"])
    audit["mmio_access_audit"] = timing.mmio_audit(paths["mmio"])
    audit["mmio_access_audit"]["environment_bound"] = True
    audit["mmio_access_audit"]["added_watch_ranges"] = ADDED_WATCH
    audit["source_pin_count"] = len(pins)
    audit["logs"], audit["filters"] = inputs, headers
    require(rows[-1]["kind"] == "timing-prefix-end" and rows[-1]["v"][0] == 0x80373ac4,
            "explicit event endpoint missing")
    configured = [r for r in rows if r["kind"] == "sc-device-init"]
    require(len(configured) == 4 and all(r["v"][:3] == (n,0,0) for n,r in enumerate(configured)),
            "initial NONE configuration did not reach SI device producer")
    checks = [r for r in rows if r["kind"] == "sc-device-check"]
    require(len(checks) == 4 and all(r["v"][:4] == (n,0,0,0) for n,r in enumerate(checks)),
            "SI configuration changed during startup poll")
    require(audit["mmio_access_audit"]["watched_cpu_mmio_writes"] == 0,
            "unexpected watched device writer")
    return audit, rows, paths

def native_parity(capture, args, paths):
    require(sha256(args.program) == args.program_sha256 and
            sha256(args.prefix_program) == args.prefix_program_sha256,
            "native candidate executable changed")
    entry, end, seconds, _, fixture_text = produced.checked_inputs(capture)
    fixture = args.out.with_suffix(".entry.txt")
    fixture.write_text(fixture_text, encoding="utf-8")
    producer = entry["clock_producer"]
    command = [str(args.program),str(args.dol),str(fixture),f"{seconds:08x}",
               producer["cpu_hz"],producer["epoch_cycles"],"-",
               "".join(entry["clock_low_words"][:2]),producer["cached_tb"],producer["exceptions"],
               "research-produced-work"]
    native = subprocess.run(command,capture_output=True,text=True,encoding="utf-8",check=True).stdout
    prefix = subprocess.run([str(args.prefix_program),str(args.dol),str(fixture)],
                            capture_output=True,text=True,encoding="utf-8",check=True).stdout
    args.out.with_suffix(".native.txt").write_text(native,encoding="utf-8")
    args.out.with_suffix(".prefix.txt").write_text(prefix,encoding="utf-8")
    events = produced.ppc_checkpoints(paths["events"])
    effects = produced.word_effects(paths["mmio"])
    phases = produced.clock_phase_states(paths["instructions"],end)
    result = produced.compare_output(native,prefix,capture,events,effects,phases)
    result["output_mutation_declines"] = produced.falsify_output(
        native,prefix,capture,events,effects,phases)
    result["executable_sha256"] = args.program_sha256
    result["prefix_executable_sha256"] = args.prefix_program_sha256
    result["scope"] = "produced-work candidate parity only; queue/input ownership unadmitted"
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("dolphin","disc","source-root","library","baseline","baseline-audit","user-dir","out",
                 "program","prefix-program","dol"):
        parser.add_argument("--"+name,type=lambda s:Path(s).resolve(),required=True)
    for name in ("program-sha256","prefix-program-sha256"):
        parser.add_argument("--"+name,required=True)
    parser.add_argument("--port",type=int)
    args = parser.parse_args()
    for pin in (args.program_sha256,args.prefix_program_sha256):
        require(len(pin)==64 and all(c in "0123456789abcdef" for c in pin),
                "candidate SHA256 must be an explicit lowercase 64-character hexadecimal pin")
    build = Path(__file__).resolve().parents[3] / "build"
    require(all(path.is_relative_to(build) for path in (args.user_dir,args.out)),
            "all outputs must remain under repository build")
    baseline = json.loads(args.baseline.read_text(encoding="utf-8"))
    require(baseline["controlled_initial_l2cr"] == "00000000" and
            baseline["pause_entry_seconds"] == 0 and baseline["capture_complete"],
            "comparison baseline profile differs")
    before_pin = sha256(Path(base.__file__))
    args.port = select_explicit_port(args.port)
    args.l2cr, args.handler, args.pause_entry, args.debug = 0, 0, 0, None
    capture, adapted_sha = adapted_capture()
    capture(args)
    result = json.loads(args.out.read_text(encoding="utf-8"))
    result["name"] = "fresh_initial_si_device_none_falsifier_42"
    result["scope"] = "initial source-configuration perturbation; no native clock promotion"
    result["initial_configuration"] = {"Dolphin.Core.SIDevice0":0}
    result["added_mmio_watch_ranges"] = ADDED_WATCH
    result["capture_transport"] = {"loopback_port":args.port,"explicit_bind_checked":True}
    result["capture_adaptation"] = {
        "base_tool_sha256": BASE_CAPTURE_SHA,
        "adapted_function_sha256": adapted_sha,
        "changes": ["initial SI device0 NONE option", "additional SI/DVD watch ranges"],
        "original_source_modified": False,
    }
    require(before_pin == sha256(Path(base.__file__)) == BASE_CAPTURE_SHA,
            "original capture tool was modified")
    args.out.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    audit, rows, paths = validate_capture(result,args)
    baseline_report_path = args.baseline_audit
    baseline_audit = json.loads(baseline_report_path.read_text(encoding="utf-8"))
    require(baseline_audit["capture"]["sha256"] == sha256(args.baseline),
            "baseline audit binding differs")
    baseline_rows, _, _ = timing.read_logs([args.baseline.with_name(baseline["event_log"])])
    for key in ("entry","first_clock","elapsed_entry_to_clock","pending_at_first_clock","interval_dispatch_counts"):
        require(audit[key] == baseline_audit[key], "perturbation changed " + key)
    require(audit["callback_projection"]["boot_unit_steps"] ==
            baseline_audit["callback_projection"]["boot_unit_steps"], "perturbation changed N")
    require(result["instruction_trace_sha256"] == baseline["instruction_trace_sha256"],
            "SI perturbation altered continuous instruction timing trace")
    before_si, after_si = selected_si(baseline_rows), selected_si(rows)
    before_channels = [r for r in before_si if r["kind"]=="vt-device-data"]
    after_channels = [r for r in after_si if r["kind"]=="vt-device-data"]
    require(len(before_channels)==len(after_channels)==4 and before_channels != after_channels,
            "SI perturbation did not change actual channel output")
    native = native_parity(result,args,paths)
    comparison = {
        "scope":"actual initial NONE-device falsifier; source timing projection only",
        "capture_sha256":sha256(args.out),"baseline_capture_sha256":sha256(args.baseline),
        "baseline_audit":{"path":args.baseline_audit.name,"sha256":sha256(args.baseline_audit)},
        "helper_sha256":sha256(Path(__file__)),"base_capture_sha256":BASE_CAPTURE_SHA,
        "unchanged":["boot N","entry/first-clock state","pending heap/FIFO order",
                     "due callbacks","continuous instruction trace"],
        "actual_si_before":before_si,"actual_si_after":after_si,
        "source_audit":audit,"native_parity":native,
        "unresolved":"native event/device lifecycle and real controller input ownership",
    }
    args.out.with_suffix(".comparison.json").write_text(json.dumps(comparison,indent=2)+"\n",encoding="utf-8")
    print(json.dumps({"result":"PASS","boot_N":audit["callback_projection"]["boot_unit_steps"],
                     "elapsed":audit["elapsed_entry_to_clock"],
                     "native_parity":native,"si_channel_output_changed":True},indent=2))

if __name__=="__main__":
    main()
