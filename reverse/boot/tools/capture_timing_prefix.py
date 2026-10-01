"""Passive continuous-entry timing capture; no internal prefix breakpoints.

Keep original inputs read-only and output/oracle profiles inside repository
build/. Pre-entry experiments retain immediate readbacks. Event logs preserve
startup ancestry; a single stop occurs before the next unprovided OS pointer.
"""
import argparse,json,os,socket,subprocess,time
from pathlib import Path
from capture_dolphin_rsp import RSP,sha256
from capture_boot_machine_state import DISC_SHA,snapshot
from capture_clock_state import ClockExperiment


def capture(args):
    root=Path(__file__).resolve().parents[3]/'build'
    if any(not p.is_relative_to(root.resolve()) for p in (args.user_dir,args.out)):raise ValueError('outputs outside repository build')
    if args.user_dir.exists() or args.out.exists():raise ValueError('fresh profile/output required')
    if sha256(args.disc)!=DISC_SHA:raise ValueError('startup disc identity differs')
    manifest=json.loads((args.dolphin.parent/'manifest.json').read_text(encoding='utf-8'))
    if sha256(args.dolphin)!=manifest['instrumented_executable_sha256']:raise ValueError('oracle identity differs')
    if args.pause_entry<0 or args.pause_entry>60:raise ValueError('pause must be0..60seconds')
    args.user_dir.mkdir(parents=True);(args.user_dir/'Temp').mkdir()
    event=args.out.with_suffix('.events.tsv');trace=args.out.with_suffix('.instructions.tsv')
    env=dict(os.environ,TMP=str(args.user_dir/'Temp'),TEMP=str(args.user_dir/'Temp'))
    # Explicit all-kind stream; no filter gaps may hide queue/state changes.
    for key in list(env):
        if key.startswith(('DOLPHIN_EVENT_','DOLPHIN_SI_','DOLPHIN_OSINIT_','DOLPHIN_COMPOSITION_','SHADOW_TIMING_')):env.pop(key)
    mmio=args.out.with_suffix('.mmio.tsv')
    mmio_watch='0c005000:0c005100;0c002000:0c002100;0c003000:0c003008;0c006c00:0c006c20'
    env.update(DOLPHIN_OSINIT_LOG=str(mmio),DOLPHIN_OSINIT_WATCH=mmio_watch,DOLPHIN_OSINIT_WATCH_PHASE0='1')
    env.update(DOLPHIN_SI_EVENT_LOG=str(event),DOLPHIN_EVENT_WINDOW='init..timing-prefix-end@80373ac4',DOLPHIN_EVENT_LIMIT='3000000')
    if 'passive_interpreter' in manifest:env['SHADOW_TIMING_PREFIX_TRACE']=str(trace)
    startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=subprocess.SW_HIDE
    command=[str(args.dolphin),'-b','-u',str(args.user_dir),'-C',f'Dolphin.General.GDBPort={args.port}',
             '-C','Dolphin.Core.CPUCore=0','-C','Dolphin.Interface.DebugModeEnabled=True','-e',str(args.disc)]
    proc=subprocess.Popen(command,env=env,cwd=args.dolphin.parent,startupinfo=startup,creationflags=subprocess.CREATE_NO_WINDOW)
    exp=ClockExperiment(args.handler,debug=args.debug,continuous=True)
    states=[]
    try:
        deadline=time.monotonic()+35;sock=None
        while time.monotonic()<deadline:
            if proc.poll() is not None:raise RuntimeError('oracle exited')
            try:sock=socket.create_connection(('127.0.0.1',args.port),timeout=1);break
            except OSError:time.sleep(.1)
        if sock is None:raise TimeoutError('GDB unavailable')
        with sock:
            rsp=RSP(sock)
            def observe():
                s=snapshot(rsp,True);s['l2_stack_address']='8060c570';s['l2_stack_bytes']=rsp.memory(0x8060c570,0x90).hex();exp.observe(rsp,s)
                s['os_context_pointer']=rsp.memory(0x800030f0,4).hex();s['os_boot_copy']=rsp.memory(0x80586c90,28).hex()
                return s
            original=observe()
            if original['pc'].lower()!='80003154':raise ValueError('wrong original entry')
            for packet in ('P77=0011c064',f'P8a={args.l2cr:08x}'):
                if rsp.send(packet)!='OK':raise ValueError('entry control refused')
            exp.prepare(rsp);states.append(observe())
            time.sleep(args.pause_entry)
            after_pause=observe()
            if states[-1]['clock_producer']!=after_pause['clock_producer']:raise ValueError('paused source changed')
            if rsp.send('Z0,80373ac4,4')!='OK':raise ValueError('sole endpoint breakpoint refused')
            rsp.send('c',reply=False);stop=rsp.packet();s=observe()
            if s['pc'].lower()!='80373ac4':raise ValueError('unexpected continuous stop '+s['pc']+':'+stop)
            if rsp.send('z0,80373ac4,4')!='OK':raise ValueError('stop removal refused')
            states.append(s)
            result=dict(name='passive_timing_prefix_42',scope='original continuous reference; not native clock proof',
                capture_complete=False,
                capture_tool_sha256=sha256(Path(__file__)),
                observer_environment=dict(event_window=env['DOLPHIN_EVENT_WINDOW'],event_kinds='',
                    event_limit=env['DOLPHIN_EVENT_LIMIT'],mmio_watch=mmio_watch,mmio_watch_phase0=True),
                dolphin_sha256=sha256(args.dolphin),disc_sha256=DISC_SHA,instrumentation_manifest=manifest,
                original_unmodified_entry=original,controlled_initial_hid0='0011c064',controlled_initial_l2cr=f'{args.l2cr:08x}',
                controlled_midchain_writes=[],checkpoints=states,region_extension=exp.metadata(),
                sole_internal_stop='80373ac4',event_log=event.name,instruction_trace=trace.name if trace.exists() else None,mmio_log=mmio.name,
                pause_entry_seconds=args.pause_entry,pause_source_readbacks=[states[0]['clock_producer'],after_pause['clock_producer']])
            result['region_extension']['stop']='80373ac4'
            args.out.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    finally:
        proc.terminate()
        try:proc.wait(timeout=5)
        except subprocess.TimeoutExpired:proc.kill();proc.wait(timeout=5)
    result['event_log_sha256']=sha256(event)
    validate_event_tail(event)
    if 'passive_interpreter' in manifest and not trace.exists():
        raise ValueError('mandatory passive instruction trace missing')
    if trace.exists():
        validate_instruction_tail(trace)
        result['instruction_trace_sha256']=sha256(trace)
    result['mmio_log_sha256']=sha256(mmio)
    result['capture_complete']=True
    args.out.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('PASS captured continuous entry ->80373AC4; complete passive endpoint')
    print('eventSHA',result['event_log_sha256'],'traceSHA',result.get('instruction_trace_sha256','not instrumented'))

def validate_instruction_tail(path):
    with path.open('rb') as f:
        f.seek(0,2);size=f.tell();f.seek(max(0,size-512));data=f.read()
    if not data or not data.endswith(b'\n'):raise ValueError('truncated instruction observer')
    fields=data.splitlines()[-1].split()
    if len(fields)!=15 or fields[1]!=b'exit' or fields[2]!=b'80373ac0' or fields[6]!=b'80373ac4':
        raise ValueError('missing passive instruction endpoint')
    if not fields[0].isdigit() or int(fields[0])==0:raise ValueError('invalid instruction ordinal')
    for i in range(2,15):
        width=16 if i in (8,9,10) else 8
        if len(fields[i])!=width or any(c not in b'0123456789abcdefABCDEF' for c in fields[i]):
            raise ValueError('malformed passive instruction state')

def validate_event_tail(path):
    with path.open('rb') as f:
        f.seek(0,2);size=f.tell();f.seek(max(0,size-4096));data=f.read()
    if not data or not data.endswith(b'\n'):raise ValueError('truncated event observer')
    fields=data.splitlines()[-1].split(b'\t')
    if len(fields)!=16 or fields[2]!=b'timing-prefix-end' or fields[4]!=b'0000000080373ac4':
        raise ValueError('missing passive event endpoint')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for n in ('dolphin','disc','user_dir','out'):p.add_argument(n,type=lambda s:Path(s).resolve())
    p.add_argument('--port',type=int,required=True)
    for n in ('l2cr','handler'):p.add_argument('--'+n,type=lambda s:int(s,16),required=True)
    p.add_argument('--pause-entry',type=float,default=0)
    p.add_argument('--debug',type=lambda s:int(s,16))
    capture(p.parse_args())
