"""Compare finite native first-Advance outputs with original source-bound records.

No captured queue/deadline/counter is supplied to C++. Unbound reference stops
at the first unprovided consumer; complete-control parity remains conditional.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys

import agent_clock_source as clock
import agent_timing_source_42 as timing
import validate_produced_clock as produced

ORACLE_SHA = "84be4d74d3a566b92e46c416d5e9c4ff87f313612fb2f4e529852cb74b0cfe78"
COPIED_INTERPRETER_SHA = "355c7217ae52cb5a6b117b5b5d2a565078e9dd713833a23ad2366348f071b85f"
DISC_SHA = "a8a526ef7006a2d2a8d41f536ef30b208ff197c185c05e9b2276e6e93cb41a8e"
BASE_CAPTURE_SHA = "d477a14b913c1b9d3b07be5a0412233121f75aa1d368a7a57b3940df388f9f81"
EXTRA_PINS = {
"Source/Core/Core/Core.cpp":"8bf9e90fc56b5b4d7a1e42410ea2e2291f44b5d86fb4e982b3aa68912edf5944",
"Source/Core/AudioCommon/Mixer.cpp":"24f91152ffe824019053f89acc77a974992fa9ed19e68b0c94194521987f90a3",
"Source/Core/AudioCommon/WaveFile.cpp":"bd9020f3769c3a4df30932cae3fa7efeb5dddd6f5251ce59221d59edba9d4961",
"Source/Core/Common/BlockingLoop.h":"410f0fc12388580664481862d8cbc975539b7bfb56ecdef935d07c255adb3043",
"Source/Core/Core/Movie.cpp":"c6dc7711039e9c3c271412311bb5b342f31c39dd95300e35f970d15836b0ba39",
"Source/Core/Core/Movie.h":"6f799ca407acc14f8d0ceab919aad63220ae7f9ce4c8e6a52f9afc52a9ce0fb7",
"Source/Core/Core/AchievementManager.cpp":"687394d3f9601102b200f22048f52ac2bd6dd2eae976ac07eb3f83c6a435f9d2",
"Source/Core/Core/AchievementManager.h":"689252000d9b1d9b11f1207e9f3bd3f5ba3ba21b13538fe054d52dcee44b37e5",
"Source/Core/Core/Config/AchievementSettings.cpp":"eaa04213954068ee4e4168259d4a005b0bd9d59d9c60f5b80e15ab6ca9533f79",
"Externals/rcheevos/rcheevos/src/rc_client.c":"ec6c68ffa4769b786d3ab3f45065d8872ad8b70afc8e15ac1436ee3910935029",
"Source/Core/Core/PowerPC/Gekko.h":"fad9f1cbb274f63955b44680627bfab542e1940b24b6f6625ee1653436a6020c",
"Source/Core/Core/HW/DSPHLE/MailHandler.cpp":"950b42a3ad61480cc54bf26163d7802f471305c8ce52f30a4099585696af789f",
"Source/Core/Core/HW/DSPHLE/MailHandler.h":"35a94fd4d1fc7dc7302f8208adeade9a79db01d78fa8b0c0e0fe53e0dee41cc5",
"Source/Core/Core/HW/DSPHLE/DSPHLE.h":"4ad76991b0f777779e72bacf2d8050032b21bf2354a6a904503413992987e6e6",
}
OBSERVED = {"init","init-ready","enqueue","advance-enter","advance-clock",
            "dispatch","callback-return","vt-update","vt-rebase","vt-advanced","advance-exit"}
CLOCK_KINDS = {"init","init-ready","advance-enter","advance-clock","advance-exit"}

def require(ok, why):
    if not ok:
        raise ValueError(why)

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def signed(value):
    return value - (1 << 64) if value >= 1 << 63 else value

def validate_capture_contract(capture):
    require(isinstance(capture, dict), "capture contract is not an object")
    require(capture.get("capture_complete") is True and
            capture.get("capture_tool_sha256") == BASE_CAPTURE_SHA,
            "incomplete/unbound original capture tool")
    expected_environment = {
        "event_window":"init..timing-prefix-end@80373ac4","event_kinds":"",
        "event_limit":"3000000",
        "mmio_watch":"0c005000:0c005100;0c002000:0c002100;0c003000:0c003008;0c006c00:0c006c20",
        "mmio_watch_phase0":True,
    }
    require(capture.get("observer_environment") == expected_environment,
            "unbound observer environment")
    manifest = capture.get("instrumentation_manifest",{})
    require(isinstance(manifest, dict), "instrumentation manifest is not an object")
    private = manifest.get("passive_interpreter",{})
    require(isinstance(private, dict), "passive Interpreter binding is not an object")
    require(capture.get("dolphin_sha256") == ORACLE_SHA and
            manifest.get("instrumented_executable_sha256") == ORACLE_SHA,
            "unrelated reference oracle")
    require(private.get("copied_sha256") == COPIED_INTERPRETER_SHA and
            private.get("original_sha256") ==
            clock.SOURCE_PINS["Source/Core/Core/PowerPC/Interpreter/Interpreter.cpp"],
            "unbound original/copied Interpreter")
    require(capture.get("disc_sha256") == DISC_SHA and
            capture.get("controlled_initial_hid0") == "0011c064" and
            capture.get("controlled_initial_l2cr") in ("00000000","c0480000"),
            "unsupported controlled source profile")
    # This validates entry/readback/epoch/BI2/debugger controls only. None of
    # its resulting observed values are passed to the initial-event owner.
    produced.checked_inputs(capture)

def capture_binding_self_test(capture):
    mutators = [
        lambda c:c.update(capture_complete=False),
        lambda c:c.update(capture_tool_sha256="0"*64),
        lambda c:c["observer_environment"].update(mmio_watch_phase0=False),
        lambda c:c["observer_environment"].update(event_kinds="enqueue"),
        lambda c:c["observer_environment"].update(mmio_watch=""),
        lambda c:c.update(dolphin_sha256="0"*64),
        lambda c:c["instrumentation_manifest"].update(instrumented_executable_sha256="0"*64),
        lambda c:c["instrumentation_manifest"]["passive_interpreter"].update(copied_sha256="0"*64),
        lambda c:c["instrumentation_manifest"]["passive_interpreter"].update(original_sha256="0"*64),
        lambda c:c.update(controlled_initial_l2cr="00000001"),
        lambda c:c.update(controlled_initial_hid0="00000000"),
        lambda c:c.update(controlled_midchain_writes=["unprovided"]),
        lambda c:c["region_extension"].update(requested_epoch="unprovided"),
        lambda c:c.update(disc_sha256="0"*64),
    ]
    declined = 0
    for mutate in mutators:
        changed = copy.deepcopy(capture)
        mutate(changed)
        try:
            validate_capture_contract(changed)
        except (ValueError,KeyError):
            declined += 1
        else:
            raise ValueError("malformed source binding accepted")
    return declined

def complete_source_audit(args, path, capture):
    tool = Path(__file__).with_name("agent_timing_source_42.py")
    command = [sys.executable,"-B",str(tool),"--source-root",str(args.source_root),
        "--library",str(args.library),"--capture",str(path),
        "--logs",str(path.with_name(capture["event_log"])),
        "--instructions",str(path.with_name(capture["instruction_trace"])),
        "--mmio",str(path.with_name(capture["mmio_log"]))]
    result = json.loads(subprocess.run(command,capture_output=True,text=True,
        encoding="utf-8",check=True).stdout)
    require(result["source_pin_count"] == 37 and
            result["core_library_sha256"] == clock.LIB_SHA and
            result["capture"]["sha256"] == sha(path) and
            result["mmio_access_audit"]["environment_bound"] and
            result["source_locations"] and
            all(item["observed_stop"] for item in result["window_closure"]),
            "complete source audit did not close")
    return result

def first_advance(path):
    rows = []
    started = False
    previous = None
    with path.open(encoding="utf-8") as stream:
        for line in stream:
            fields = line.rstrip("\n").split("\t")
            require(len(fields) == 16, "malformed source event row")
            sequence = int(fields[0])
            if sequence == 0:
                require("kinds=;window=init..timing-prefix-end@80373ac4;" in fields[3],
                        "incomplete first-Advance source window")
                continue
            require(previous is None or sequence > previous, "nonmonotone event stream")
            previous = sequence
            kind = fields[2]
            require(kind != "LIMIT", "limited source evidence")
            if kind == "init":
                require(not started, "repeated initialization")
                started = True
            if not started:
                continue
            require(kind not in {"ts-publish","ts-commit","ts-move","cancel-item","adjust-item"},
                    "unowned first-Advance ingress or queue mutation")
            if kind in OBSERVED:
                rows.append({"kind":kind,"name":fields[3],"v":fields[4:12]})
            if kind == "advance-exit":
                break
    require(rows and rows[0]["kind"] == "init" and rows[-1]["kind"] == "advance-exit",
            "missing closed first-Advance reference")
    return rows

def comparable(row):
    values = list(row["v"])
    if row["kind"] in CLOCK_KINDS or row["kind"].startswith("vt-"):
        values[5] = None  # apploader PC belongs to a different owner
    return {"kind":row["kind"],"name":row["name"],"v":values}

def projected_queue(rows):
    queue = []
    for row in rows:
        values = [int(x,16) for x in row["v"]]
        event = {"name":row["name"],"deadline":signed(values[0]),"fifo":values[1],
                 "userdata":row["v"][2]}
        if row["kind"] == "enqueue":
            require(all(item["fifo"] != event["fifo"] for item in queue), "duplicate FIFO")
            queue.append(event)
            queue.sort(key=lambda item:(item["deadline"],item["fifo"]))
        if row["kind"] == "dispatch":
            require(queue and queue[0] == event, "source violated signed/FIFO dispatch")
            queue.pop(0)
    return queue

def compare(dump, reference, bound):
    require(dump["schema"] == "initial-boot-events-42-v1" and dump["scope"] == "research-only",
            "native schema/scope changed")
    selected = [row for row in dump["journal"] if row["kind"] in OBSERVED]
    require(bool(selected), "empty native journal")
    require(len(selected) == len(reference) if bound else len(selected) < len(reference),
            "native journal completion differs")
    require([comparable(row) for row in selected] ==
            [comparable(row) for row in reference[:len(selected)]],
            "native first-Advance ordered effect differs")
    require(dump["queue"] == projected_queue(reference[:len(selected)]),
            "native pending queue differs")
    if bound:
        require(dump["stop"] == "first-advance-complete" and dump["active_callback"] is None,
                "bound control did not finish")
        require(dump["scheduler"] == {"global":20000,"slice":10888,"downcount":10888,
                "sane":0,"next_fifo":10,"inverse_bits":0x3f800000}, "bound scheduler differs")
    else:
        require(dump["stop"] == "Mixer.cpp:253 live-dtk-log", "unbound consumer was silently skipped")
        require(dump["active_callback"]["name"] == "FinishExecutingCommand" and
                dump["dvd"]["pending_blocks"] == 0 and dump["scheduler"]["next_fifo"] == 6,
                "unbound DTK included later mutations")
        require(dump["movie"]["frame"] is None and dump["movie"]["lag"] is None,
                "unobserved movie value manufactured")
    count = sum(7 if row["kind"] in CLOCK_KINDS or row["kind"].startswith("vt-") else 8
                for row in selected)
    return {"matched_records":len(selected),"matched_observed_fields":count,
            "excluded_apploader_pc_fields":sum(
                row["kind"] in CLOCK_KINDS or row["kind"].startswith("vt-") for row in selected)}

def falsify(bound, reference):
    declined = 0
    mutations = []
    # Numerically equal total clocks cannot hide changed initial deadline/FIFO,
    # relative successor phase or removed VI effects.
    for row_index, row in enumerate(bound["journal"]):
        if row["kind"] == "enqueue":
            for column in (0,1,2,3):
                changed = copy.deepcopy(bound)
                original = changed["journal"][row_index]["v"][column]
                changed["journal"][row_index]["v"][column] = f"{int(original,16)^1:016x}"
                mutations.append(changed)
        if row["kind"] in {"dispatch","callback-return","vt-rebase","vt-advanced","advance-exit"}:
            changed = copy.deepcopy(bound)
            changed["journal"][row_index]["v"][0] = f"{int(row['v'][0],16)^1:016x}"
            mutations.append(changed)
    changed = copy.deepcopy(bound)
    first = [n for n,row in enumerate(changed["journal"]) if row["kind"] == "dispatch"][:2]
    changed["journal"][first[0]],changed["journal"][first[1]] = (
        changed["journal"][first[1]],changed["journal"][first[0]])
    mutations.append(changed)
    for changed in mutations:
        try:
            compare(changed,reference,True)
        except ValueError:
            declined += 1
        else:
            raise ValueError("corrupted event consequence accepted")
    return declined

def run(args):
    build = Path(__file__).resolve().parents[3] / "build"
    require(args.output.is_relative_to(build), "output outside repository build")
    require(sha(args.program) == args.program_sha256, "native executable changed")
    pins = dict(clock.SOURCE_PINS, **timing.EXTRA_PINS, **EXTRA_PINS)
    for relative, expected in pins.items():
        require(sha(args.source_root/relative) == expected, "source changed: " + relative)
    require(sha(args.library) == clock.LIB_SHA, "source library changed")
    outputs = {}
    for name, flag in [("unbound","--dump"),("bound_control","--dump-bound-control")]:
        outputs[name] = json.loads(subprocess.run([str(args.program),flag],
            capture_output=True,text=True,encoding="utf-8",check=True).stdout)
    references = []
    for path in args.capture:
        capture = json.loads(path.read_text(encoding="utf-8"))
        validate_capture_contract(capture)
        full_audit = complete_source_audit(args,path,capture)
        malformed_declines = capture_binding_self_test(capture)
        events = path.with_name(capture["event_log"])
        require(sha(events) == capture["event_log_sha256"], "source stream binding changed")
        base_path = Path(__file__).with_name("capture_timing_prefix.py")
        require(sha(base_path) == BASE_CAPTURE_SHA, "original capture source changed")
        rows = first_advance(events)
        references.append({"capture":path.name,"capture_sha256":sha(path),
            "events_sha256":sha(events),
            "complete_source_audit":full_audit,
            "malformed_binding_declines":malformed_declines,
            "unbound_prefix_parity":compare(outputs["unbound"],rows,False),
            "declared_bound_control_parity":compare(outputs["bound_control"],rows,True),
            "output_mutation_declines":falsify(outputs["bound_control"],rows)})
    report = {"schema":"initial-event-owner-audit-42-v1",
        "scope":"finite native research owner; baseline stopped before unbound DTK logging",
        "native_program_sha256":args.program_sha256,"audit_tool_sha256":sha(Path(__file__)),
        "source_pin_count":len(pins),"core_library_sha256":clock.LIB_SHA,
        "references":references,"native_outputs":outputs,
        "directly_observed":"ordered init/clock/queue/VI rows and signed pending heap",
        "source_derived_conditional_fields":["DVD pending blocks/zero transfer",
            "DSP ROM/mail/DMA fields","PI mask/cause/exceptions at first Advance",
            "Movie counters","achievement client/DLL controls","GPU external effect delivery"],
        "unresolved":["live DTK log flag and WAV lifecycle","GPU worker ownership",
            "live Movie lifecycle and host frame-step ingress","achievement client/DLL ingress",
            "apploader PC/RAM ownership","RTC/physical elapsed equivalence"],
        "native_ownership_admitted":False}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    summary = {k:v for k,v in report.items() if k not in ("native_outputs","references")}
    summary["references"] = [{k:v for k,v in item.items() if k != "complete_source_audit"}
                             for item in report["references"]]
    summary["complete_source_audit"] = "PASS all complete37-pin lifecycle/trace/environment/source-location gates"
    print(json.dumps(summary,indent=2))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("program","source-root","library","output"):
        parser.add_argument("--"+name,type=lambda value:Path(value).resolve(),required=True)
    parser.add_argument("--program-sha256",required=True)
    parser.add_argument("--capture",type=lambda value:Path(value).resolve(),action="append",required=True)
    run(parser.parse_args())
