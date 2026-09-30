"""Add explicitly labelled PS0-only observations to the pinned HLE capture.

The stock Dolphin GDB register map exposes PS0 in FPR ids 32..63, not PS1.
This wrapper cannot establish paired-lane parity; it records the exact gap.
All output paths and inputs are supplied to capture_dolphin_rsp.py by caller.
"""

import capture_dolphin_rsp as capture


_snapshot = capture.RSP.snapshot


def snapshot_with_ps0(self):
    state = _snapshot(self)
    state["fpr_ps0_only"] = {
        "f0": self.send("p20"),
        "f1": self.send("p21"),
        "f31": self.send("p3f"),
    }
    # The stock stub returns E01 for this unallocated id. This is evidence
    # about the debugger interface, not a PS1 value.
    state["unallocated_register_143_reply"] = self.send("p8f")
    return state


capture.RSP.snapshot = snapshot_with_ps0

if __name__ == "__main__":
    capture.main()
