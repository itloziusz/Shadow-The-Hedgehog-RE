"""Clock-frontier evidence from untouched instructions and pure producer getters.

Every ordinary capture starts at DOL entry and retains the whole checkpoint40
prefix. Offset controls (if requested) occur before entry only. No tick result,
branch result, register, instruction or mid-chain state is forced.
"""
import argparse
from pathlib import Path
import time
from capture_l2_state import run
from capture_bi2_state import Bi2Experiment, gpr
from boot_state_diff import hex_field

CLOCK_GLOBALS=(0x805F1F50,0x805F1F54)
LOW_WORDS=(0x800030D8,0x800030DC,0x800000F8,0x800000FC,0x800030F0)
GETTERS=((123,'exceptions',8),(250,'cycles',16),(251,'epoch_cycles',16),(252,'epoch_value',16),
         (253,'cpu_hz',8),(254,'rtc_offset',16),(255,'cached_tb',16))

class ClockExperiment(Bi2Experiment):
    def __init__(self, handler, msr=None, poison=False, debug=None, offset=None, array=None, clock_offset=None, continuous=False, timebase_epoch=None):
        super().__init__(handler,msr,poison,debug,offset,array)
        self.continuous=continuous
        self.timebase_epoch=timebase_epoch
        self.epoch_controls=[]
        self.clock_offset=clock_offset
        self.clock_writes=[]
        self.pause_samples=[]
        self.clock_plan=[]
    def observe(self,rsp,state):
        super().observe(rsp,state)
        clock={}
        for register,name,width in GETTERS:
            value=rsp.send(f'p{register:x}')
            hex_field(value,width,'clock getter '+name)
            clock[name]=value
        state['clock_producer']=clock
        state['clock_low_words']=[rsp.memory(a,4).hex() for a in LOW_WORDS]
        state['clock_globals']=[rsp.memory(a,4).hex() for a in CLOCK_GLOBALS]
    def prepare(self,rsp):
        super().prepare(rsp)
        if self.timebase_epoch is not None:
            # Source perturbation occurs at DOL entry, never at a clock read.
            # These private commands are absent from the ordinary observer.
            for register,value in ((250,self.timebase_epoch&0xffffffff),(251,self.timebase_epoch>>32),(252,1)):
                packet=f'P{register:x}={value:08x}'
                if rsp.send(packet)!='OK':raise ValueError('pre-entry epoch control refused')
                readback={name:rsp.send(f'p{reg:x}') for reg,name,_ in GETTERS}
                for _,name,width in GETTERS:hex_field(readback[name],width,'epoch control '+name)
                self.epoch_controls.append(dict(packet=packet,pc='80003154',readback=readback))
            committed=self.epoch_controls[-1]['readback']
            if int(committed['epoch_value'],16)!=self.timebase_epoch or committed['epoch_cycles']!=committed['cycles'] or committed['cached_tb']!=committed['epoch_value']:
                raise ValueError('pre-entry epoch producer readback differs')
        if self.clock_offset is not None:
            for address,value in zip(LOW_WORDS[:2],(self.clock_offset>>32,self.clock_offset&0xffffffff)):
                if rsp.send(f'M{address:x},4:{value:08x}')!='OK':raise ValueError('pre-entry offset control refused')
                readback=rsp.memory(address,4).hex()
                if hex_field(readback,8,'offset readback')!=value:raise ValueError('offset readback differs')
                self.clock_writes.append(dict(address=f'{address:08x}',value=f'{value:08x}',readback=readback))
    def advance(self,rsp,reach,states):
        super().advance(rsp,reach,states)
        if int(states[-1]['pc'],16)!=0x80379628:return
        first={k:rsp.send(f'p{reg:x}') for reg,k,_ in GETTERS}
        time.sleep(.25)
        second={k:rsp.send(f'p{reg:x}') for reg,k,_ in GETTERS}
        self.pause_samples=[first,second]
        def step(expected):
            reach(expected,True)
            self.clock_plan.append(expected)
        if self.continuous:
            reach(0x8037963C)
            self.clock_plan.append(0x8037963C)
        else:
            for attempt in range(16):
                for pc in (0x8037962C,0x80379630,0x80379634,0x80379638):step(pc)
                a,b=gpr(states[-1],3),gpr(states[-1],5)
                step(0x80379628 if a!=b else 0x8037963C)
                if a==b:break
            else:raise ValueError('unbounded stable-clock retry; fail closed')
        step(0x8037966C)
        for pc in range(0x80379670,0x80379688,4):step(pc)
        # Direct BL at684 enters restore leaf. Saved prior EE is still r3.
        step(0x80376144)
        for pc in (0x80376148,0x8037614C):step(pc)
        saved=gpr(states[-1],3)
        if saved:
            for pc in (0x80376150,0x80376154,0x8037615C):step(pc)
        else:
            for pc in (0x80376158,0x8037615C):step(pc)
        for pc in (0x80376160,0x80376164,0x80379688):step(pc)
        for pc in range(0x8037968C,0x803796AC,4):step(pc)
        for pc in (0x80370EA8,0x80370EAC,0x80370EB0,0x8037611C,
                   0x80376120,0x80376124,0x80376128,0x8037612C,
                   0x80370EB4,0x80370EB8,0x80373AB4,0x80373AB8,
                   0x80373ABC,0x80373AC0,0x80373AC4):step(pc)
    def metadata(self):
        result=super().metadata()
        result.update(name='clock_research_41',clock_preentry_writes=self.clock_writes,
                      clock_plan=[f'{pc:08x}' for pc in self.clock_plan],pause_samples=self.pause_samples,
                      sampler_mode='continuous' if self.continuous else 'single_step',
                      producer_mode='preentry_rebased' if self.timebase_epoch is not None else 'rtc_initial',
                      epoch_controls=self.epoch_controls,
                      requested_epoch=None if self.timebase_epoch is None else f'{self.timebase_epoch:016x}',
                      stop='80373ac4' if self.clock_plan else result['stop'],
                      timing_status='research capture; not a native clock provider')
        return result

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('dolphin','disc','user_dir','out'):p.add_argument(name,type=lambda s:Path(s).resolve())
    p.add_argument('--port',type=int,required=True)
    for name in ('l2cr','handler'):p.add_argument('--'+name,type=lambda s:int(s,16),required=True)
    for name in ('msr','debug','offset'):p.add_argument('--'+name,type=lambda s:int(s,16))
    p.add_argument('--array')
    p.add_argument('--clock-offset',type=lambda s:int(s,16))
    p.add_argument('--continuous-sampler',action='store_true')
    p.add_argument('--timebase-epoch',type=lambda s:int(s,16),help='controlled epoch input before boot entry only; requires separate experiment oracle')
    p.add_argument('--source');p.add_argument('--poison',action='store_true')
    args=p.parse_args();args.stock=False;args.second_poll_busy=False
    if args.clock_offset is not None and not 0<=args.clock_offset<=0xffffffffffffffff:p.error('offset overflow')
    if args.timebase_epoch is not None and not 0<=args.timebase_epoch<=0xffffffffffffffff:p.error('epoch overflow')
    array=None if args.array is None else [int(v,16) for v in args.array.split(',') if v]
    run(args,ClockExperiment(args.handler,args.msr,args.poison,args.debug,args.offset,array,args.clock_offset,args.continuous_sampler,args.timebase_epoch))
