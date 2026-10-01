"""Strict reusable machine-state shapes and ordered first-divergence reports.

Parsers never make unknown values concrete. Repeated PCs are occurrences,
not dictionary keys. Byte validity is supplied by an independent store ledger.
"""
import re

FIELD_NAMES = ["pc","msr","lr","cr","xer","fpscr"] + [f"r{n}" for n in range(32)]
FIELD_NAMES += [f"PS0.f{n}" for n in range(32)] + [f"PS1.f{n}" for n in range(32)]
FIELD_NAMES += ["ctr","hid0","hid2"] + [f"GQR{n}" for n in range(8)]


def hex_field(value, digits, name):
    if not isinstance(value,str) or not re.fullmatch(rf"[0-9a-fA-F]{{{digits}}}",value):
        raise ValueError(f"incomplete or symbolic raw field {name}")
    return int(value,16)


def parse_native_state(line):
    words=line.split()
    if len(words) not in (115,118,120):
        raise ValueError("native raw-state field count differs")
    for n,value in enumerate(words[:113]):
        hex_field(value,16 if 38 <= n < 102 else 8,FIELD_NAMES[n])
    hex_field(words[113],32,"paired bytes")
    if not re.fullmatch(r"[01]{16}",words[114]):
        raise ValueError("invalid paired byte validity")
    if len(words)>=118:
        hex_field(words[115],8,"L2CR")
        if words[116:118] != ["-","-"]:
            hex_field(words[116],288,"owned stack bytes")
            if not re.fullmatch(r"[01]{144}",words[117]):
                raise ValueError("invalid owned stack byte validity")
    if len(words)==120:
        for n,name in ((118,"handler slot"),(119,"lowmem44")):
            if words[n]!="-": hex_field(words[n],8,name)
    return words


def reference_fields(state):
    result=[state[k] for k in ("pc","msr","lr","cr","architectural_xer","fpscr")]
    hex_field(state["gpr"],256,"reference GPR array")
    result += [state["gpr"][n:n+8] for n in range(0,256,8)]
    for key,count,width in (("ps0",32,16),("ps1",32,16),("gqr",8,8)):
        if len(state[key])!=count: raise ValueError(f"incomplete reference {key} array")
        for value in state[key]:hex_field(value,width,key)
    result += state["ps0"]+state["ps1"]+[state[k] for k in ("ctr","hid0","hid2")]+state["gqr"]
    for n,value in enumerate(result):hex_field(value,16 if 38<=n<102 else 8,FIELD_NAMES[n])
    return result


def compare_fields(actual, expected, names, occurrence):
    if len(actual)!=len(expected) or len(names)!=len(actual):
        raise ValueError("state comparison shape differs")
    for name,a,b in zip(names,actual,expected):
        width=16 if name.startswith(("PS0.","PS1.")) else 8
        if hex_field(a,width,name)!=hex_field(b,width,name):
            raise ValueError(f"first divergence {occurrence}: {name} native={a} reference={b}")
    return len(actual)


def compare_memory(actual, validity, observed, expected_validity, occurrence, base):
    if validity != expected_validity:
        raise ValueError(f"byte validity/store timeline differs {occurrence}")
    hex_field(actual,2*len(validity),"native memory bytes")
    hex_field(observed,2*len(validity),"reference memory bytes")
    raw=bytes.fromhex(actual);ref=bytes.fromhex(observed)
    if len(raw)!=len(ref) or len(raw)!=len(validity) or any(x not in "01" for x in validity):
        raise ValueError("memory comparison shape differs")
    count=0
    for n,known in enumerate(validity):
        if known=="1":
            if raw[n]!=ref[n]:
                raise ValueError(f"first memory divergence {occurrence} address={base+n:08X}")
            count+=1
    return count


def word_validity(base,size,addresses):
    known={address+b for address in addresses for b in range(4)}
    if any(a<base or a>=base+size for a in known):raise ValueError("store outside validity window")
    return "".join("1" if base+n in known else "0" for n in range(size))


class OrderedTrace:
    def __init__(self, states):
        self.states=list(states);self.index=0
    def take(self, pc):
        if self.index>=len(self.states) or hex_field(self.states[self.index]["pc"],8,"PC")!=pc:
            raise ValueError(f"reference CFG order differs at occurrence {self.index} expected={pc:08X}")
        value=self.states[self.index];self.index+=1;return value
    def finish(self):
        if self.index!=len(self.states):raise ValueError("unconsumed reference checkpoints")
