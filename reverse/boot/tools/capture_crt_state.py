"""Full-entry handler/CRT experiments using the unchanged L2 capture prefix.

Only labelled pre-entry controls are written. Repeated PCs retain their
occurrence order. Completed fills are read in full; gaps remain untouched.
"""
import argparse
from pathlib import Path

from capture_l2_state import run
from boot_state_diff import hex_field

ZERO_RANGES = [(0x8056FE00, 0x74700), (0x805EF020, 0x375C), (0x805FC540, 0xAC)]
HANDLER_PLAN = [0x80373378, 0x8037611C, 0x80376124, 0x803733A0,
                0x803733B4, 0x803733C0, 0x803733C4, 0x80373564,
                0x80376144, 0x8037615C, 0x8037356C, 0x80372908,
                0x80370C8C, 0x80370CB4, 0x80370CD4, 0x80372914,
                0x8000341C, 0x8000315C, 0x8000316C, 0x80003340]


class CrtExperiment:
    def __init__(self, handler, msr=None, poison=False):
        if not isinstance(handler,int):raise ValueError("explicit handler word required")
        if any(value is not None and (not isinstance(value,int) or not 0<=value<=0xFFFFFFFF)
               for value in (handler,msr)):
            raise ValueError("pre-entry word overflow")
        self.handler, self.msr, self.poison = handler, msr, poison
        self.fills = []
        self.seeds = []
        self.canaries = [(a-4,4) for a,_ in ZERO_RANGES] + [(a+n,4) for a,n in ZERO_RANGES]

    def observe(self, rsp, state):
        state["handler_slot"] = rsp.memory(0x80586CB4,4).hex()
        state["crt_descriptors"] = rsp.memory(0x80005544,0xA4).hex()
        state["crt_canaries"] = [rsp.memory(a,n).hex() for a,n in self.canaries]
        state["lowmem_44"] = rsp.memory(0x80000044,4).hex()
        state["bi2_pointer"] = rsp.memory(0x800000F4,4).hex()
        state["preentry_seed_words"] = [rsp.memory(int(s["address"],16),4).hex() for s in self.seeds]

    def prepare(self, rsp):
        writes = [(0x80586CB4, self.handler)]
        if self.msr is not None:
            if rsp.send(f"P41={self.msr:08x}") != "OK":
                raise ValueError("pre-entry MSR experiment refused")
            if hex_field(rsp.send("p41"),8,"pre-entry MSR readback")!=self.msr:
                raise ValueError("pre-entry MSR readback differs")
        if self.poison:
            # Nonzero probes throughout each fill and on both sides. They
            # never enter native C++ as an expected post-state or branch value.
            for index,(address,size) in enumerate(ZERO_RANGES):
                for offset in (0, (size//8)*4, size-4):
                    writes.append((address+offset, 0xA1020304+index))
            writes += [(a,0xD0E0F001+i) for i,(a,_) in enumerate(self.canaries)]
        for address,value in writes:
            if rsp.send(f"M{address:x},4:{value:08x}") != "OK":
                raise ValueError("pre-entry memory experiment refused")
            readback=rsp.memory(address,4).hex()
            if int(readback,16)!=value:raise ValueError("pre-entry memory readback differs")
            self.seeds.append({"address":f"{address:08x}","value":f"{value:08x}","readback":readback})

    def advance(self, rsp, reach, states):
        for pc in HANDLER_PLAN:
            reach(pc)
        for _ in range(11):
            reach(0x8000336C)
        reach(0x800033AC)
        for address,size in ZERO_RANGES:
            for pc in (0x800033C0, 0x8000540C, 0x8000543C, 0x80005424):
                reach(pc)
            raw = rsp.memory(address,size)
            if len(raw) != size:
                raise ValueError("incomplete fill observation")
            self.fills.append({"address":f"{address:08x}","size":f"{size:08x}","bytes":raw.hex()})
            reach(0x800033DC)
        for pc in (0x800033C0, 0x800033E4, 0x80003170, 0x80003188):
            reach(pc)

    def metadata(self):
        return {"name":"handler_crt_39", "controlled_initial_handler":f"{self.handler:08x}",
                "controlled_initial_msr":None if self.msr is None else f"{self.msr:08x}",
                "preentry_memory_writes":self.seeds, "filled_ranges":self.fills,
                "canary_ranges":[[f"{a:08x}",n] for a,n in self.canaries],
                "midchain_writes":False, "stop":"80003188"}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("dolphin","disc","user_dir","out"):
        parser.add_argument(name, type=lambda p:Path(p).resolve())
    parser.add_argument("--port",type=int,required=True)
    parser.add_argument("--l2cr",type=lambda x:int(x,16),required=True)
    parser.add_argument("--handler",type=lambda x:int(x,16),required=True)
    parser.add_argument("--msr",type=lambda x:int(x,16))
    parser.add_argument("--source")
    parser.add_argument("--poison",action="store_true")
    parser.add_argument("--stock",action="store_true")
    args=parser.parse_args()
    for value in (args.handler,args.msr):
        if value is not None and not 0 <= value <= 0xFFFFFFFF:
            raise ValueError("pre-entry word overflow")
    args.second_poll_busy=False
    run(args,CrtExperiment(args.handler,args.msr,args.poison))
