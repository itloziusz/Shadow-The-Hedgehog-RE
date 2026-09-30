#!/usr/bin/env python3
import struct, sys

def cp(reg, value): return bytes([0x08,reg])+struct.pack('>I',value)

# VCD: POS direct, CLR0 direct. VCD high: TEX0 direct.
vcd_lo=(1<<9)|(1<<13)
vcd_hi=1
# VAT0: position XYZ/F32, color0 RGBA/RGBA8888, tex0 ST/F32, byte-dequant on.
vat_a=(1<<0)|(4<<1)|(1<<13)|(5<<14)|(1<<21)|(4<<22)|(1<<30)
out=bytearray()
out+=cp(0x50,vcd_lo); out+=cp(0x60,vcd_hi); out+=cp(0x70,vat_a); out+=cp(0x80,0); out+=cp(0x90,0)
out+=bytes([0x90])+struct.pack('>H',3)  # TRIANGLES, VAT0
verts=[
    ((0.0,0.0,0.0),(255,0,0,255),(0.0,0.0)),
    ((1.0,0.0,0.0),(0,255,0,255),(1.0,0.0)),
    ((0.0,1.0,0.0),(0,0,255,255),(0.0,1.0)),
]
for pos,col,uv in verts:
    out+=struct.pack('>3f',*pos); out+=bytes(col); out+=struct.pack('>2f',*uv)
path=sys.argv[1] if len(sys.argv)>1 else 'sample_fifo.bin'
open(path,'wb').write(out)
print(f'wrote {len(out)} bytes to {path}')
