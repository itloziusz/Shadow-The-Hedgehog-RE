"""Connected BI2/OS prefix experiments; all controls precede DOL entry.

The complete loaded BI2 is observed, not inferred from the disc. Stops before
the first time-base read; no clock, offset or mid-chain state is injected.
"""
import argparse
from pathlib import Path
from capture_l2_state import run
from capture_crt_state import CrtExperiment
from boot_state_diff import hex_field

SDA = (0x805F1F18, 0x805F1F1C, 0x805F1F40, 0x805F1FF0)


def gpr(state, register):
    raw = state['gpr']
    if not isinstance(raw,str) or len(raw)!=256 or raw.strip()!=raw:
        raise ValueError('incomplete packed GPR bank')
    hex_field(raw,256,'packed GPR bank')
    if not 0 <= register < 32:
        raise ValueError('GPR index outside bank')
    return hex_field(raw[8*register:8*register+8],8,f'r{register}')


class Bi2Experiment(CrtExperiment):
    def __init__(self, handler, msr=None, poison=False, debug=None, offset=None, array=None):
        super().__init__(handler, msr, poison)
        self.debug, self.offset, self.array = debug, offset, array
        self.bi2_writes = []
        self.pointer = None

    def observe(self, rsp, state):
        super().observe(rsp, state)
        pointer = hex_field(state['bi2_pointer'], 8, 'live BI2 pointer')
        if not pointer or pointer & 3 or not 0x80000000 <= pointer <= 0x817FE000:
            raise ValueError('live BI2 mapping unavailable')
        if self.pointer is not None and self.pointer != pointer:
            raise ValueError('BI2 pointer changed without an explained writer')
        self.pointer = pointer
        state['bi2_blob'] = rsp.memory(pointer, 0x2000).hex()
        state['bi2_globals'] = [rsp.memory(a, 4).hex() for a in SDA]
        state['lowmem_34'] = rsp.memory(0x80000034, 4).hex()
        state['lowmem_48'] = rsp.memory(0x80000048, 4).hex()

    def prepare(self, rsp):
        super().prepare(rsp)
        writes = []
        if self.debug is not None:
            writes.append((self.pointer + 12, self.debug))
        if self.offset is not None:
            writes.append((self.pointer + 8, self.offset))
        if self.array is not None:
            if self.offset is None or self.offset & 3 or not 16 <= self.offset <= 0x1FFC:
                raise ValueError('relocation experiment offset outside owned BI2')
            if self.offset + 4 + len(self.array)*4 > 0x2000:
                raise ValueError('relocation experiment incomplete')
            writes.append((self.pointer + self.offset, len(self.array)))
            writes += [(self.pointer+self.offset+4+4*n, value) for n,value in enumerate(self.array)]
        for address,value in writes:
            if not 0 <= value <= 0xFFFFFFFF:
                raise ValueError('BI2 experiment word overflow')
            if rsp.send(f'M{address:x},4:{value:08x}') != 'OK':
                raise ValueError('pre-entry BI2 write refused')
            readback = rsp.memory(address,4).hex()
            if hex_field(readback,8,'BI2 control readback') != value:
                raise ValueError('pre-entry BI2 write readback mismatch')
            self.bi2_writes.append(dict(address=f'{address:08x}', value=f'{value:08x}', readback=readback))

    def advance(self, rsp, reach, states):
        super().advance(rsp, reach, states)
        for pc in (0x80003194, 0x800031BC, 0x800031C4):
            reach(pc)
        debug = gpr(states[-1],7)
        if debug in (2,3):
            if debug == 3:
                reach(0x800031D0)
            reach(0x800031E8)
            reach(0x800031F4)
            return
        for pc in (0x800031D0, 0x800031D8):
            reach(pc)
        if debug == 4:
            for pc in (0x80003140,0x80003148,0x800031E4):
                reach(pc)
        for pc in (0x800031F8,0x80003200,0x8000320C,0x80003214):
            reach(pc)
        offset = gpr(states[-1],6)
        if offset:
            for pc in (0x8000321C,0x80003224):
                reach(pc)
            count = gpr(states[-1],14)
            if count > 8:
                raise ValueError('experiment count not bounded')
            if count:
                reach(0x80003230)
                for index in range(count):
                    for pc in (0x80003234,0x8000323C,0x80003240):
                        reach(pc)
                    if index+1 < count:
                        reach(0x80003230)
                reach(0x80003250)
            else:
                reach(0x80003258)
        else:
            reach(0x80003258)
        for pc in (0x80003260,0x80370BF0,0x80370BFC,0x80370C08,
                   0x80370C10,0x80003264,0x80370E68,0x80370E80,
                   0x80370E98,0x80370EA0,0x80379648,0x80379660,
                   0x8037611C,0x80376124,0x80379664,0x80379628):
            reach(pc)

    def metadata(self):
        result = super().metadata()
        result.update(name='bi2_os_40', bi2_pointer=f'{self.pointer:08x}',
                      bi2_preentry_writes=self.bi2_writes,
                      stop='800031f4' if self.debug in (2,3) else '80379628')
        return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('dolphin','disc','user_dir','out'):
        parser.add_argument(name,type=lambda p:Path(p).resolve())
    for name in ('port',):
        parser.add_argument('--'+name,type=int,required=True)
    for name in ('l2cr','handler'):
        parser.add_argument('--'+name,type=lambda v:int(v,16),required=True)
    for name in ('msr','debug','offset'):
        parser.add_argument('--'+name,type=lambda v:int(v,16))
    parser.add_argument('--array',help='comma-separated hexadecimal words; count derived from length')
    parser.add_argument('--source')
    parser.add_argument('--poison',action='store_true')
    parser.add_argument('--stock',action='store_true')
    args=parser.parse_args()
    args.second_poll_busy=False
    array=None if args.array is None else [int(v,16) for v in args.array.split(',') if v]
    run(args,Bi2Experiment(args.handler,args.msr,args.poison,args.debug,args.offset,array))
