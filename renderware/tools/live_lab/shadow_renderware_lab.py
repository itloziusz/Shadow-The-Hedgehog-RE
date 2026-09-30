from __future__ import annotations

import ctypes
from ctypes import wintypes
import csv
import json
import math
import os
import struct
import sys
import threading
import time
import tkinter as tk
from tkinter import ttk, messagebox, filedialog
from dataclasses import dataclass
from typing import Optional, Iterable

GAME_ID = b"GUPP8P"
GC_BASE = 0x80000000
MEM1_SIZE = 0x01800000

# Verified Shadow (GUPP8P) addresses from the current reverse-engineering pass.
CAMERA_MANAGER = 0x8056FF1C        # singleton object; +0 is current RwCamera*
TOBJLAND = 0x80572560               # static TObjLand singleton
ASPECT_SCALAR = 0x805EBA30          # original 1.3195877; 16:9 = 1.7777778
FAR_DEFAULTS = (0x805F5C4C, 0x805F5E68, 0x805F8F20)
STREAM_MARGIN = 0x8003A6C0          # Engine Lab hook live float
LAB_SIGNATURE = 0x8003A6C4          # b'SELB'
LAB_VERSION = 0x8003A6C8
LAB_FLAGS = 0x8003A6CC
LAB_NOOP_CALLBACK = 0x8003A6D0  # v2: single BLR, preserves r3 for RpAtomic/RpWorldSector callback suppression

# v0.8 resident editor runtime. The DOL consumes this mailbox once per present.
EDITOR_MAILBOX = 0x805FD000
EDITOR_MAGIC = b"SE08"
EDITOR_VERSION = 0x00080000
EDITOR_CMD_POSITION = 1 << 0
EDITOR_CMD_BASIS = 1 << 1
EDITOR_CMD_ATOMIC_CB = 1 << 2
RW_VIEW_MATRIX = 0x805E428C
EDITOR_PICK_RADIUS_PX = 30.0

# RenderWare / driver globals from RENDERER_RE research bundle.
RW_GLOBALS_PTR = 0x805F265C
RW_GLOBALS_FALLBACK = 0x8060E340
RW_STATE_CACHE = 0x805E42D0
RW_BOUND_TEXTURES = 0x8056FCDC
RW_DRIVER_CURRENT_CAMERA = 0x805F26F8
RW_IM_TEXTURE_PTR = 0x805F270C
RW_WHITE_RASTER_PTR = 0x805F2708
RW_RASTER_PLUGIN_OFFSET = 0x805F2700
RW_TEXTURE_PLUGIN_OFFSET = 0x805F2718
EFFECT_MANAGER_PTR = 0x805EF2D8
DEFAULT_SECTOR_RENDER_CB = 0x80460B60
DEFAULT_ATOMIC_RENDER_CB = 0x80456F04
KNOWN_ATOMIC_RENDER_CBS = {
    0x80456F04: "RpAtomic default",
    0x801645D0: "game atomic wrapper A",
    0x8003851C: "game atomic wrapper B",
    0x800386EC: "game atomic wrapper C",
    0x80449010: "RpPTank atomic",
    LAB_NOOP_CALLBACK: "Engine Lab no-op (hidden)",
}
EFFECT_VTABLES = {
    0x8053D9BC: "EffectParticlePJS",
    0x8053DAB0: "EffectParticle3DPJS",
    0x8053D8D4: "EffectRayPJS",
    0x8053D860: "EffectLocusPJS",
    0x8053D728: "EffectShimmerPJS",
    0x8053DA3C: "EffectObjectPJS",
}
BUCKET_NAMES = {0:"Opaque",1:"Punch",2:"Transparent",3:"Additive",4:"Subtractive",5:"Glare",6:"Shimmer/PostGlare"}

# Runtime display-name resolver. Exact semantic names are only emitted when an asset/texture token supports them.
OBJECT_NAME_PATTERNS = [
    ("gunsoldier", "GUN Soldier"), ("gunbeetle", "GUN Beetle"), ("gunbigfoot", "GUN Big Foot"),
    ("gunrobot", "GUN Robot"), ("bksoldier", "Black Arms Soldier"), ("bklarva", "Black Arms Larva"),
    ("bkworm", "Black Arms Worm"), ("bkwing", "Black Arms Wing"), ("bkgiant", "Black Arms Giant"),
    ("bkninja", "Black Arms Ninja"), ("bkchaos", "Black Arms Chaos"), ("eggpawn", "Egg Pawn"),
    ("eggpierrot", "Egg Pierrot"), ("eggshadow", "Shadow Android"), ("shadow", "Shadow / player-related model"),
    ("springpanel", "Spring Panel"), ("longspring", "Long Spring"), ("spring", "Spring"),
    ("dashring", "Dash Ring"), ("goalring", "Goal Ring"), ("hintring", "Hint Ring"),
    ("firering", "Fire Ring"), ("itembox", "Item Box"), ("secretdoor", "Secret Door"),
    ("morphdoor", "Morph Door"), ("basedoor", "Base Door"), ("concretedoor", "Concrete Door"),
    ("targetswitch", "Target Switch"), ("railswitch", "Rail Switch"), ("gravitychange", "Gravity Switch"),
    ("airsaucer", "Air Saucer"), ("walker", "Walker"), ("gunlift", "GUN Lift"),
    ("bombserver", "Bomb Server"), ("bombingwall", "Bombing Wall"), ("arkcannon", "ARK Cannon"),
]
ENGINE_DOL_BASENAME = "SHADOW_GUPP8P_INGAME_EDITOR_V08_16x9.dol"
ENGINE_HELPER_START = 0x8003A578
ENGINE_HELPER_END = 0x8003A6DC  # exclusive


# TObjLand fields / slot layout
LAND_ALT_WORLD_FLAG = TOBJLAND + 0x1808
LAND_RENDER_STATE_BYTE = TOBJLAND + 0x1809
LAND_FREEZE_LOADER = TOBJLAND + 0x1810
LAND_SLOT_COUNT = 32
LAND_SLOT_SIZE = 0xC0
LAND_SLOT_STATE = 0x00
LAND_SLOT_WORLD = 0xB8
LAND_SLOT_BLOCK_ID = 0xBC

# RwCamera fields verified in _RwCameraSetViewWindow / _RwCameraSetFarClipPlane.
CAM_VIEW_X = 0x68
CAM_VIEW_Y = 0x6C
CAM_RECIP_X = 0x70
CAM_RECIP_Y = 0x74
CAM_NEAR = 0x80
CAM_FAR = 0x84
CAM_Z_SCALE = 0x8C
CAM_Z_SHIFT = 0x90

# Lab flags reserved for future baked hooks.
LAB_FLAG_FORCE_ALL_LOADED = 1 << 0
LAB_FLAG_EFFECTS_OFF = 1 << 1
LAB_FLAG_COLLISION_DRAW = 1 << 2

PROCESS_QUERY_INFORMATION = 0x0400
PROCESS_VM_READ = 0x0010
PROCESS_VM_WRITE = 0x0020
PROCESS_VM_OPERATION = 0x0008
PROCESS_ACCESS = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION
TH32CS_SNAPPROCESS = 0x00000002
INVALID_HANDLE_VALUE = ctypes.c_void_p(-1).value
MEM_COMMIT = 0x1000
PAGE_NOACCESS = 0x01
PAGE_GUARD = 0x100

DOLPHIN_NAMES = {
    "dolphin.exe",
    "dolphinqt.exe",
    "dolphinqt2.exe",
    "dolphin-emu.exe",
}


class PROCESSENTRY32W(ctypes.Structure):
    _fields_ = [
        ("dwSize", wintypes.DWORD),
        ("cntUsage", wintypes.DWORD),
        ("th32ProcessID", wintypes.DWORD),
        ("th32DefaultHeapID", ctypes.c_size_t),
        ("th32ModuleID", wintypes.DWORD),
        ("cntThreads", wintypes.DWORD),
        ("th32ParentProcessID", wintypes.DWORD),
        ("pcPriClassBase", wintypes.LONG),
        ("dwFlags", wintypes.DWORD),
        ("szExeFile", wintypes.WCHAR * 260),
    ]


class MEMORY_BASIC_INFORMATION64(ctypes.Structure):
    _fields_ = [
        ("BaseAddress", ctypes.c_ulonglong),
        ("AllocationBase", ctypes.c_ulonglong),
        ("AllocationProtect", wintypes.DWORD),
        ("__alignment1", wintypes.DWORD),
        ("RegionSize", ctypes.c_ulonglong),
        ("State", wintypes.DWORD),
        ("Protect", wintypes.DWORD),
        ("Type", wintypes.DWORD),
        ("__alignment2", wintypes.DWORD),
    ]


@dataclass
class ProcessInfo:
    pid: int
    name: str


@dataclass
class LandSlot:
    index: int
    state: int
    world: int
    block_id: int


class WinProcessMemory:
    def __init__(self, pid: int):
        if os.name != "nt":
            raise RuntimeError("This controller is Windows-only.")
        self.kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        self.handle = self.kernel32.OpenProcess(PROCESS_ACCESS, False, pid)
        if not self.handle:
            raise ctypes.WinError(ctypes.get_last_error())
        self.pid = pid

        self.kernel32.ReadProcessMemory.argtypes = [
            wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)
        ]
        self.kernel32.ReadProcessMemory.restype = wintypes.BOOL
        self.kernel32.WriteProcessMemory.argtypes = [
            wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)
        ]
        self.kernel32.WriteProcessMemory.restype = wintypes.BOOL
        self.kernel32.VirtualQueryEx.argtypes = [
            wintypes.HANDLE, ctypes.c_void_p, ctypes.POINTER(MEMORY_BASIC_INFORMATION64), ctypes.c_size_t
        ]
        self.kernel32.VirtualQueryEx.restype = ctypes.c_size_t

    def close(self):
        if getattr(self, "handle", None):
            self.kernel32.CloseHandle(self.handle)
            self.handle = None

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass

    def read(self, address: int, size: int) -> bytes:
        buf = ctypes.create_string_buffer(size)
        read = ctypes.c_size_t()
        ok = self.kernel32.ReadProcessMemory(
            self.handle, ctypes.c_void_p(address), buf, size, ctypes.byref(read)
        )
        if not ok or read.value != size:
            raise OSError(f"ReadProcessMemory failed at 0x{address:X} ({read.value}/{size})")
        return buf.raw

    def try_read(self, address: int, size: int) -> Optional[bytes]:
        try:
            return self.read(address, size)
        except Exception:
            return None

    def write(self, address: int, data: bytes):
        buf = ctypes.create_string_buffer(data)
        written = ctypes.c_size_t()
        ok = self.kernel32.WriteProcessMemory(
            self.handle, ctypes.c_void_p(address), buf, len(data), ctypes.byref(written)
        )
        if not ok or written.value != len(data):
            raise OSError(f"WriteProcessMemory failed at 0x{address:X} ({written.value}/{len(data)})")

    def regions(self) -> Iterable[MEMORY_BASIC_INFORMATION64]:
        addr = 0
        mbi = MEMORY_BASIC_INFORMATION64()
        max_addr = 0x00007FFFFFFFFFFF
        while addr < max_addr:
            got = self.kernel32.VirtualQueryEx(
                self.handle, ctypes.c_void_p(addr), ctypes.byref(mbi), ctypes.sizeof(mbi)
            )
            if not got:
                break
            # copy because VirtualQueryEx reuses the same object
            cur = MEMORY_BASIC_INFORMATION64()
            ctypes.memmove(ctypes.byref(cur), ctypes.byref(mbi), ctypes.sizeof(mbi))
            yield cur
            step = int(cur.RegionSize) if cur.RegionSize else 0x1000
            nxt = int(cur.BaseAddress) + step
            if nxt <= addr:
                break
            addr = nxt


class ShadowMemory:
    def __init__(self, proc: WinProcessMemory, mem1_base: int):
        self.proc = proc
        self.mem1_base = mem1_base

    def host(self, gc_addr: int) -> int:
        off = gc_addr - GC_BASE
        if off < 0 or off >= MEM1_SIZE:
            raise ValueError(f"GC address outside MEM1: 0x{gc_addr:08X}")
        return self.mem1_base + off

    def read(self, gc_addr: int, n: int) -> bytes:
        return self.proc.read(self.host(gc_addr), n)

    def write(self, gc_addr: int, data: bytes):
        self.proc.write(self.host(gc_addr), data)

    def u8(self, gc_addr: int) -> int:
        return self.read(gc_addr, 1)[0]

    def set_u8(self, gc_addr: int, v: int):
        self.write(gc_addr, bytes([v & 0xFF]))

    def u32(self, gc_addr: int) -> int:
        return struct.unpack(">I", self.read(gc_addr, 4))[0]

    def set_u32(self, gc_addr: int, v: int):
        self.write(gc_addr, struct.pack(">I", v & 0xFFFFFFFF))

    def s32(self, gc_addr: int) -> int:
        return struct.unpack(">i", self.read(gc_addr, 4))[0]

    def u16(self, gc_addr: int) -> int:
        return struct.unpack(">H", self.read(gc_addr, 2))[0]

    def s16(self, gc_addr: int) -> int:
        return struct.unpack(">h", self.read(gc_addr, 2))[0]

    def is_ptr(self, p: int) -> bool:
        return GC_BASE <= int(p) < GC_BASE + MEM1_SIZE

    def cstring(self, gc_addr: int, max_len: int = 128) -> str:
        if not self.is_ptr(gc_addr):
            return ""
        try:
            b = self.read(gc_addr, max_len)
            b = b.split(b"\0", 1)[0]
            return b.decode("ascii", "replace")
        except Exception:
            return ""

    def f32(self, gc_addr: int) -> float:
        return struct.unpack(">f", self.read(gc_addr, 4))[0]

    def set_f32(self, gc_addr: int, v: float):
        self.write(gc_addr, struct.pack(">f", float(v)))

    # ---- v0.8 resident editor mailbox --------------------------------------
    def editor_runtime_present(self) -> bool:
        try:
            return self.read(EDITOR_MAILBOX, 4) == EDITOR_MAGIC and self.u32(EDITOR_MAILBOX + 4) >= EDITOR_VERSION
        except Exception:
            return False

    def editor_select(self, ptr: int, frame: int):
        if not self.editor_runtime_present():
            raise RuntimeError("In-game editor runtime not present. Boot the supplied v0.8 DOL.")
        self.set_u32(EDITOR_MAILBOX + 0x18, int(ptr) if ptr else 0)
        self.set_u32(EDITOR_MAILBOX + 0x1C, int(frame) if frame else 0)

    def _editor_command(self, bits: int):
        seq=(self.u32(EDITOR_MAILBOX+0x10)+1)&0xFFFFFFFF
        self.set_u32(EDITOR_MAILBOX+0x10,seq)
        self.set_u32(EDITOR_MAILBOX+0x0C,bits)
        return seq

    def editor_apply_position(self, frame:int, pos):
        self.editor_select(0,frame)
        for i,v in enumerate(pos): self.set_f32(EDITOR_MAILBOX+0x20+i*4,float(v))
        return self._editor_command(EDITOR_CMD_POSITION)

    def editor_apply_basis(self, frame:int, right, up, at):
        self.editor_select(0,frame)
        vals=tuple(right)+tuple(up)+tuple(at)
        for i,v in enumerate(vals): self.set_f32(EDITOR_MAILBOX+0x30+i*4,float(v))
        return self._editor_command(EDITOR_CMD_BASIS)

    def editor_set_atomic_callback(self, atomic:int, cb:int):
        self.editor_select(atomic,0)
        self.set_u32(EDITOR_MAILBOX+0x58,cb)
        return self._editor_command(EDITOR_CMD_ATOMIC_CB)

    def rw_matrix_at(self, base:int) -> dict:
        vals=[]
        for off in (0x00,0x10,0x20,0x30):
            vals.append(tuple(self.f32(base+off+i*4) for i in range(3)))
        return {"right":vals[0],"up":vals[1],"at":vals[2],"pos":vals[3]}

    def camera_position(self):
        cam=self.camera_ptr()
        if not cam:return None
        try:
            frame=self.u32(cam+0x04)
            if self.valid_frame_matrix(frame): return self.frame_matrix(frame)["pos"]
        except Exception:pass
        return None

    def project_world_point(self, pos, width:int, height:int):
        if width<=1 or height<=1:return None
        try:
            V=self.rw_matrix_at(RW_VIEW_MATRIX)
            x,y,z=map(float,pos)
            ex=x*V['right'][0]+y*V['up'][0]+z*V['at'][0]+V['pos'][0]
            ey=x*V['right'][1]+y*V['up'][1]+z*V['at'][1]+V['pos'][1]
            ez=x*V['right'][2]+y*V['up'][2]+z*V['at'][2]+V['pos'][2]
            cam=self.camera_ptr(); near=self.f32(cam+CAM_NEAR) if cam else 0.1
            if not math.isfinite(ez) or ez<=max(near,1e-4):return None
            rx=self.f32(cam+CAM_RECIP_X); ry=self.f32(cam+CAM_RECIP_Y)
            ndcx=-ex*rx/ez; ndcy=ey*ry/ez
            sx=(ndcx*0.5+0.5)*width; sy=(0.5-ndcy*0.5)*height
            if not all(math.isfinite(v) for v in (sx,sy)):return None
            return (sx,sy,ez)
        except Exception:return None

    def geometry_texture_names(self, geom:int, limit:int=16) -> list[str]:
        out=[]
        if not self.is_ptr(geom):return out
        try:
            mats=self.u32(geom+0x20); count=self.s32(geom+0x24)
            if not self.is_ptr(mats) or not (0<=count<=256):return out
            for i in range(min(count,limit)):
                mat=self.u32(mats+i*4)
                if not self.is_ptr(mat):continue
                tex=self.u32(mat+0x00)
                if not self.is_ptr(tex):continue
                name=self.cstring(tex+0x10,32).strip()
                if name and name not in out:out.append(name)
        except Exception:pass
        return out

    def identify_atomic(self, a:dict) -> dict:
        tex=self.geometry_texture_names(a.get('geometry',0))
        hay=' '.join(tex).lower()
        for token,name in OBJECT_NAME_PATTERNS:
            if token in hay:
                return {'name':name,'confidence':'high','evidence':'texture/material: '+', '.join(tex[:4]),'textures':tex}
        pipe=a.get('pipeline',0); skin=bool(a.get('skin'))
        pmap={0x80786100:'MatFX atomic',0x80786000:'Skinned atomic',0x80786060:'Skinned MatFX atomic',0x807865C0:'PTank particle atomic',0x80786560:'DMorph atomic'}
        if pipe in pmap:
            base=pmap[pipe]
        elif skin: base='Skinned actor/model'
        else: base='Static/render atomic'
        if tex:
            return {'name':base+' ['+', '.join(tex[:3])+']','confidence':'medium','evidence':'RenderWare pipeline + material textures','textures':tex}
        return {'name':base,'confidence':'low','evidence':'RenderWare structure only','textures':tex}

    def camera_ptr(self) -> int:
        p = self.u32(CAMERA_MANAGER)
        if GC_BASE <= p < GC_BASE + MEM1_SIZE:
            return p
        return 0

    def lab_hook_present(self) -> bool:
        try:
            return self.read(LAB_SIGNATURE, 4) == b"SELB" and self.u32(LAB_VERSION) >= 1
        except Exception:
            return False

    def detect_build(self) -> str:
        try:
            if self.lab_hook_present():
                return ('In-Game Editor v0.8 + Engine Lab v%d' % self.u32(LAB_VERSION)) if self.editor_runtime_present() else f"Engine Lab v{self.u32(LAB_VERSION)}"
            pro = self.read(0x8003A578, 4)
            sig = self.read(0x8003A5C0, 8)
            if pro == bytes.fromhex("9421fff0"):
                return "stock / unpatched DOL"
            if sig == bytes.fromhex("3ce03efe60e7b852"):
                return "READY-gated 0.5% streaming test DOL"
            if sig == bytes.fromhex("3ce03ee660e76666"):
                return "READY-gated 10% streaming test DOL"
            if pro == bytes.fromhex("9421ffe0"):
                return "older runtime-only streaming test DOL"
            if pro == bytes.fromhex("9421ffc0"):
                return "older modified streaming DOL"
            return f"unknown GUPP8P DOL ({pro.hex()})"
        except Exception:
            return "unknown GUPP8P DOL"

    def get_stream_percent(self) -> Optional[float]:
        if not self.lab_hook_present():
            return None
        margin = self.f32(STREAM_MARGIN)
        extent = 1.0 - 2.0 * margin
        return max(0.0, min(100.0, extent * 100.0))

    def set_stream_percent(self, pct: float):
        if not self.lab_hook_present():
            raise RuntimeError("Engine Lab streaming hook not present. Boot the supplied ENGINE_LAB DOL first.")
        pct = max(0.01, min(100.0, float(pct)))
        extent = pct / 100.0
        margin = (1.0 - extent) * 0.5
        self.set_f32(STREAM_MARGIN, margin)

    def get_draw_percent(self) -> float:
        cam = self.camera_ptr()
        if cam:
            try:
                return self.f32(cam + CAM_FAR) / 30000.0 * 100.0
            except Exception:
                pass
        return self.f32(FAR_DEFAULTS[0]) / 30000.0 * 100.0

    def set_draw_percent(self, pct: float):
        pct = max(0.01, min(400.0, float(pct)))
        far = 30000.0 * pct / 100.0
        for a in FAR_DEFAULTS:
            self.set_f32(a, far)
        cam = self.camera_ptr()
        if cam:
            # The engine's camera update path recalculates the frustum/projection around these fields.
            # We update the live field as well as all proven defaults.
            self.set_f32(cam + CAM_FAR, far)

    def set_aspect(self, aspect: float, vertical_view_window: Optional[float] = None):
        aspect = max(0.5, min(4.0, float(aspect)))
        self.set_f32(ASPECT_SCALAR, aspect)
        cam = self.camera_ptr()
        if not cam:
            return
        if vertical_view_window is None:
            try:
                vertical_view_window = self.f32(cam + CAM_VIEW_Y)
            except Exception:
                vertical_view_window = 0.5
        if not math.isfinite(vertical_view_window) or vertical_view_window <= 0.0001:
            vertical_view_window = 0.5
        x = vertical_view_window * aspect
        self.set_f32(cam + CAM_VIEW_X, x)
        self.set_f32(cam + CAM_VIEW_Y, vertical_view_window)
        self.set_f32(cam + CAM_RECIP_X, 1.0 / x)
        self.set_f32(cam + CAM_RECIP_Y, 1.0 / vertical_view_window)

    def set_alt_world_view(self, enabled: bool):
        # +0x1808 is NOT a safe collision debug renderer. Runtime validation in Dolphin
        # proved a CP/XF vertex-spec mismatch (CP 0 normals vs XF 1 normal) when enabled.
        # Always allow returning to the stock path, but refuse activation from the tool.
        if enabled:
            raise RuntimeError(
                "Unsafe alternate land-world path blocked: Dolphin detected a GX CP/XF vertex descriptor mismatch. "
                "Collision visualization requires the dedicated Im3D debug pipeline."
            )
        self.set_u8(LAND_ALT_WORLD_FLAG, 0)

    def set_freeze_loader(self, enabled: bool):
        self.set_u8(LAND_FREEZE_LOADER, 1 if enabled else 0)

    def land_slots(self) -> list[LandSlot]:
        out = []
        for i in range(LAND_SLOT_COUNT):
            a = TOBJLAND + i * LAND_SLOT_SIZE
            try:
                state = self.u32(a + LAND_SLOT_STATE)
                world = self.u32(a + LAND_SLOT_WORLD)
                block_id = self.u32(a + LAND_SLOT_BLOCK_ID)
            except Exception:
                state, world, block_id = -1, 0, 0
            out.append(LandSlot(i, state, world, block_id))
        return out

    def rw_globals_ptr(self) -> int:
        try:
            p = self.u32(RW_GLOBALS_PTR)
            if self.is_ptr(p):
                return p
        except Exception:
            pass
        return RW_GLOBALS_FALLBACK

    def rw_globals_snapshot(self) -> dict:
        g = self.rw_globals_ptr()
        out = {"ptr": g}
        try:
            out.update({
                "curCamera": self.u32(g + 0x000),
                "curWorld": self.u32(g + 0x004),
                "renderFrame": self.u16(g + 0x008),
                "lightFrame": self.u16(g + 0x00A),
                "gamma": self.f32(g + 0x010),
                "fpSystem": self.u32(g + 0x014),
                "fpRenderStateSet": self.u32(g + 0x020),
                "fpRenderStateGet": self.u32(g + 0x024),
                "fpIm2DPrimitive": self.u32(g + 0x030),
                "engineStatus": self.u32(g + 0x124),
                "resArenaInitSize": self.u32(g + 0x128),
            })
        except Exception:
            pass
        return out

    def render_state_snapshot(self) -> list[tuple[str, str, str]]:
        S = RW_STATE_CACHE
        spec = [
            ("ZWRITEENABLE",0x00,"u32"),("ZTESTENABLE",0x04,"u32"),("GX_ZFUNC",0x08,"u32"),
            ("CULLMODE",0x0C,"u32"),("FOGENABLE",0x10,"u32"),("FOGTYPE",0x14,"u32"),
            ("FOGCOLOR",0x18,"u32"),("FOG_START",0x24,"f32"),("FOG_END",0x28,"f32"),
            ("FOG_NEAR",0x2C,"f32"),("FOG_FAR",0x30,"f32"),("SRCBLEND",0x34,"u32"),
            ("DESTBLEND",0x38,"u32"),("ALPHA_TEST_MODE",0x3C,"u32"),("ALPHA_COMP0",0x40,"u32"),
            ("ALPHA_COMP1",0x44,"u32"),("ALPHA_OP",0x48,"u32"),("ALPHA_REF0",0x4C,"u8"),
            ("ALPHA_REF1",0x4D,"u8"),("ALPHA_FULL_SET",0x4E,"u8"),
        ]
        rows=[]
        for name,off,typ in spec:
            try:
                if typ=="u32": v=self.u32(S+off)
                elif typ=="u8": v=self.u8(S+off)
                else: v=self.f32(S+off)
                rows.append((name,typ,str(v)))
            except Exception as e:
                rows.append((name,typ,f"ERR {e}"))
        return rows

    def parse_raster(self, r: int) -> dict:
        if not self.is_ptr(r):
            return {}
        try:
            return {
                "ptr":r,"parent":self.u32(r+0x00),"pixels":self.u32(r+0x04),
                "width":self.s32(r+0x0C),"height":self.s32(r+0x10),"depth":self.s32(r+0x14),
                "stride":self.s32(r+0x18),"offx":self.s16(r+0x1C),"offy":self.s16(r+0x1E),
                "type":self.u8(r+0x20),"flags":self.u8(r+0x21),"format":self.u8(r+0x23),
                "gx_fmt":self.u32(r+0x40),"has_alpha":self.u32(r+0x48),
                "pixel_data":self.u32(r+0x50),"mips_minus1":self.u8(r+0x66),
            }
        except Exception:
            return {"ptr":r}

    def parse_texture(self, t: int) -> dict:
        if not self.is_ptr(t):
            return {}
        try:
            name=self.read(t+0x10,32).split(b"\0",1)[0].decode("ascii","replace")
            mask=self.read(t+0x30,32).split(b"\0",1)[0].decode("ascii","replace")
            raster=self.u32(t+0x00)
            return {
                "ptr":t,"raster":raster,"name":name,"mask":mask,
                "filter_addressing":self.u32(t+0x50),"ref_count":self.s32(t+0x54),
                "raster_info":self.parse_raster(raster),
            }
        except Exception:
            return {"ptr":t}

    def bound_textures(self) -> list[dict]:
        out=[]
        for i in range(8):
            try: t=self.u32(RW_BOUND_TEXTURES+4*i)
            except Exception: t=0
            d=self.parse_texture(t) if t else {}
            d["map"]=i; d["ptr"]=t
            out.append(d)
        return out

    def world_info(self, world: int) -> dict:
        if not self.is_ptr(world):
            return {}
        try:
            # RpMaterialList is embedded at +0x10; the three fields below follow the RW 3.7 layout.
            return {
                "ptr":world,"flags":self.u32(world+0x08),
                "materials_ptr":self.u32(world+0x10),"material_count":self.s32(world+0x14),"material_space":self.s32(world+0x18),
                "root_sector":self.u32(world+0x1C),"num_texcoord_sets":self.s32(world+0x20),
                "sector_render_cb":self.u32(world+0x68),"pipeline":self.u32(world+0x6C),"vtxfmt":self.u32(world+0x70),
            }
        except Exception:
            return {"ptr":world}

    def set_world_sector_callback(self, world: int, cb: int):
        if not self.is_ptr(world):
            raise ValueError("Invalid RpWorld pointer")
        self.set_u32(world+0x68, cb)

    def effect_manager_ptr(self) -> int:
        try:
            p=self.u32(EFFECT_MANAGER_PTR)
            return p if self.is_ptr(p) else 0
        except Exception:
            return 0

    def effect_elements(self, max_items: int = 128) -> list[dict]:
        mgr=self.effect_manager_ptr()
        if not mgr:
            return []
        try: cur=self.u32(mgr+0x14)
        except Exception: return []
        out=[]; seen=set()
        for _ in range(max_items):
            if not self.is_ptr(cur) or cur in seen: break
            seen.add(cur)
            try:
                vt=self.u32(cur+0x00); flags=self.u32(cur+0x18); bucket=self.u32(cur+0x30)
                attr=self.u32(cur+0x44); cap=self.u32(cur+0x10C); live=self.u32(cur+0x110)
                nextp=self.u32(cur+0x3C); mode=self.u32(cur+0x108)
                texname=""
                if self.is_ptr(attr):
                    np=self.u32(attr+0x58)
                    texname=self.cstring(np,64)
                out.append({
                    "ptr":cur,"vtable":vt,"class":EFFECT_VTABLES.get(vt,f"vtable_{vt:08X}"),
                    "flags":flags,"hidden":bool(flags&8),"alive":bool(flags&1),
                    "bucket":bucket,"bucket_name":BUCKET_NAMES.get(bucket,str(bucket)),
                    "capacity":cap,"live":live,"mode":mode,"texture":texname,
                    "skip_draw":self.u32(cur+0x144) if vt==0x8053D9BC else 0,
                })
                cur=nextp
            except Exception:
                break
        return out

    def set_effects_hide_all(self, enabled: bool):
        mgr=self.effect_manager_ptr()
        if not mgr: raise RuntimeError("EffectManagerPJS not available")
        self.set_u8(mgr+0x24, 1 if enabled else 0)

    def effects_hide_all(self) -> bool:
        mgr=self.effect_manager_ptr()
        return bool(mgr and self.u8(mgr+0x24))

    def set_effect_hidden(self, elem: int, enabled: bool):
        f=self.u32(elem+0x18)
        f=(f|8) if enabled else (f&~8)
        self.set_u32(elem+0x18,f)

    def set_effect_skip_draw(self, elem: int, enabled: bool):
        self.set_u32(elem+0x144,1 if enabled else 0)

    def lens_flare_ptr(self) -> int:
        mgr=self.effect_manager_ptr()
        if not mgr: return 0
        try:
            p=self.u32(mgr+0x50)
            return p if self.is_ptr(p) else 0
        except Exception:return 0

    def set_lens_flare_enabled(self, enabled: bool):
        p=self.lens_flare_ptr()
        if not p: raise RuntimeError("Lens flare object is not available")
        self.set_u8(p+0x04,1 if enabled else 0)

    def scan_atomics(self) -> list[dict]:
        # Efficient scan: search MEM1 for known render-callback pointer values and validate the RpAtomic/RpGeometry fields.
        blob=self.proc.read(self.mem1_base, MEM1_SIZE)
        candidates=set()
        for cb in KNOWN_ATOMIC_RENDER_CBS:
            pat=struct.pack(">I",cb); start=0
            while True:
                j=blob.find(pat,start)
                if j<0: break
                if j>=0x48:
                    candidates.add(GC_BASE+j-0x48)
                start=j+4
        rows=[]
        for a in sorted(candidates):
            try:
                off=a-GC_BASE
                geom=struct.unpack_from(">I",blob,off+0x18)[0]
                frame=struct.unpack_from(">I",blob,off+0x04)[0]
                cb=struct.unpack_from(">I",blob,off+0x48)[0]
                pipe=struct.unpack_from(">I",blob,off+0x6C)[0]
                if not self.is_ptr(geom): continue
                go=geom-GC_BASE
                if go<0 or go+0x6C>len(blob): continue
                verts=struct.unpack_from(">i",blob,go+0x14)[0]
                morph=struct.unpack_from(">i",blob,go+0x18)[0]
                gflags=struct.unpack_from(">I",blob,go+0x08)[0]
                skin=struct.unpack_from(">I",blob,go+0x68)[0]
                if not (0<=verts<=1000000 and 0<=morph<=128): continue
                if frame and not self.is_ptr(frame): continue
                rows.append({
                    "atomic":a,"frame":frame,"geometry":geom,"vertices":verts,"morph_targets":morph,
                    "geom_flags":gflags,"render_cb":cb,"render_name":KNOWN_ATOMIC_RENDER_CBS.get(cb,f"0x{cb:08X}"),
                    "pipeline":pipe,"skin":skin,
                })
            except Exception:
                pass
        # Resolve RpClump ownership from the intrusive atomic list. RpClumpRender
        # walks the list at clump+0x08; each atomic contributes an RwLLLink at
        # atomic+0x40. The sentinel address therefore resolves back to clump = sentinel-8.
        link_to_atomic={r["atomic"]+0x40:r["atomic"] for r in rows}
        by_atomic={r["atomic"]:r for r in rows}
        for r in rows:
            try:
                start=r["atomic"]+0x40; node=self.u32(start); seen={start}
                members=[]
                while node in link_to_atomic and node not in seen and len(seen)<256:
                    seen.add(node); members.append(link_to_atomic[node]); node=self.u32(node)
                clump=node-0x08 if self.is_ptr(node-0x08) else 0
                if clump:
                    # Validate sentinel/list and clump root frame conservatively.
                    head_next=self.u32(clump+0x08); head_prev=self.u32(clump+0x0C); root=self.u32(clump+0x04)
                    if not (self.is_ptr(head_next) and self.is_ptr(head_prev)):
                        clump=0; root=0
                    elif root and not self.is_ptr(root):
                        root=0
                else:
                    root=0
                r["clump"]=clump
                r["clump_frame"]=root
            except Exception:
                r["clump"]=0; r["clump_frame"]=0
        for r in rows:
            try:r["identity"]=self.identify_atomic(r)
            except Exception:r["identity"]={"name":self.classify_atomic(r),"confidence":"low","evidence":"classification fallback","textures":[]}
        return rows

    def clump_atomics(self, clump: int, atomics: Optional[list[dict]]=None) -> list[int]:
        if not self.is_ptr(clump): return []
        if atomics is not None:
            return [a['atomic'] for a in atomics if a.get('clump')==clump]
        out=[]; sentinel=clump+0x08
        try: node=self.u32(sentinel)
        except Exception:return out
        seen=set()
        while self.is_ptr(node) and node!=sentinel and node not in seen and len(out)<256:
            seen.add(node); atomic=node-0x40
            if not self.is_ptr(atomic):break
            out.append(atomic)
            try:node=self.u32(node)
            except Exception:break
        return out

    def valid_frame_matrix(self, frame:int) -> bool:
        if not self.is_ptr(frame): return False
        try:
            m=self.frame_matrix(frame)
            vals=[v for key in ('right','up','at','pos') for v in m[key]]
            return all(math.isfinite(v) and abs(v)<1.0e9 for v in vals)
        except Exception:
            return False

    def resolve_transform_owner(self, kind:str, ptr:int, direct_frame:int=0, clump:int=0, clump_frame:int=0) -> dict:
        # Never invent a transform for world/sector objects: RpWorld has no object-level RwFrame.
        if kind=='World':
            return {'frame':0,'owner':ptr,'owner_kind':'RpWorld','source':'none','reason':'RpWorld has no object-level RwFrame'}
        if direct_frame and self.valid_frame_matrix(direct_frame):
            return {'frame':direct_frame,'owner':ptr,'owner_kind':kind,'source':'direct RwFrame','reason':''}
        if clump_frame and self.valid_frame_matrix(clump_frame):
            return {'frame':clump_frame,'owner':clump,'owner_kind':'RpClump','source':'clump root RwFrame','reason':''}
        if clump and self.is_ptr(clump):
            try:
                root=self.u32(clump+0x04)
                if self.valid_frame_matrix(root):
                    return {'frame':root,'owner':clump,'owner_kind':'RpClump','source':'clump root RwFrame','reason':''}
            except Exception:pass
        return {'frame':0,'owner':ptr,'owner_kind':kind,'source':'none','reason':'No validated editable RwFrame found'}

    def set_atomic_callback(self, atomic: int, cb: int):
        if not self.is_ptr(atomic): raise ValueError("Invalid RpAtomic pointer")
        self.set_u32(atomic+0x48,cb)

    # ---- RwFrame / scene transform helpers ---------------------------------
    # RenderWare 3.x RwFrame layout used by this build:
    # frame+0x10 modelling RwMatrix; matrix vectors are right/up/at/pos at
    # +0x00/+0x10/+0x20/+0x30.  Position therefore lives at frame+0x40.
    def frame_matrix(self, frame: int) -> dict:
        if not self.is_ptr(frame):
            raise ValueError("Invalid RwFrame pointer")
        base=frame+0x10
        vals=[]
        for off in (0x00,0x10,0x20,0x30):
            vals.append(tuple(self.f32(base+off+i*4) for i in range(3)))
        return {"right":vals[0],"up":vals[1],"at":vals[2],"pos":vals[3]}

    def set_frame_position(self, frame: int, x: float, y: float, z: float):
        if not self.is_ptr(frame): raise ValueError("Invalid RwFrame pointer")
        base=frame+0x40
        self.set_f32(base+0,float(x)); self.set_f32(base+4,float(y)); self.set_f32(base+8,float(z))

    def nudge_frame(self, frame: int, dx: float=0.0, dy: float=0.0, dz: float=0.0):
        m=self.frame_matrix(frame); x,y,z=m["pos"]
        self.set_frame_position(frame,x+dx,y+dy,z+dz)

    def set_frame_basis(self, frame: int, right, up, at):
        if not self.is_ptr(frame): raise ValueError("Invalid RwFrame pointer")
        base=frame+0x10
        for off,vec in ((0x00,right),(0x10,up),(0x20,at)):
            for i,v in enumerate(vec): self.set_f32(base+off+i*4,float(v))

    def scale_frame(self, frame: int, sx: float, sy: float, sz: float):
        m=self.frame_matrix(frame)
        r=tuple(v*sx for v in m['right']); u=tuple(v*sy for v in m['up']); a=tuple(v*sz for v in m['at'])
        self.set_frame_basis(frame,r,u,a)

    def rotate_frame_local(self, frame: int, axis: str, degrees: float):
        # Rotate the basis vectors in-place. This is intentionally local/model-space
        # and preserves translation. It is suitable for live scene editing without
        # calling guest code from the host process.
        m=self.frame_matrix(frame); r=list(m['right']); u=list(m['up']); a=list(m['at'])
        import math as _math
        c=_math.cos(_math.radians(degrees)); q=_math.sin(_math.radians(degrees))
        def mix(v,w,sign=1.0): return [c*v[i]+sign*q*w[i] for i in range(3)]
        if axis.lower()=='x':
            old_u,old_a=u[:],a[:]; u=mix(old_u,old_a,1); a=mix(old_a,old_u,-1)
        elif axis.lower()=='y':
            old_r,old_a=r[:],a[:]; r=mix(old_r,old_a,-1); a=mix(old_a,old_r,1)
        elif axis.lower()=='z':
            old_r,old_u=r[:],u[:]; r=mix(old_r,old_u,1); u=mix(old_u,old_r,-1)
        else: raise ValueError('axis must be x/y/z')
        self.set_frame_basis(frame,r,u,a)

    def classify_atomic(self, a: dict) -> str:
        # Conservative renderer-level classification. Gameplay identity is not
        # claimed until task ownership/factory mapping is proven.
        if a.get('skin'):
            return 'Skinned actor candidate'
        if a.get('pipeline')==0x80449010 or a.get('render_cb')==0x80449010:
            return 'Particle/PTank'
        if a.get('vertices',0) >= 5000:
            return 'Large prop/geometry'
        return 'Prop / actor atomic'

    def snapshot(self) -> dict:
        cam = self.camera_ptr()
        d = {
            "game_id": self.read(GC_BASE, 6).decode("ascii", "replace"),
            "mem1_host_base": f"0x{self.mem1_base:X}",
            "engine_lab_hook": self.lab_hook_present(),
            "stream_percent": self.get_stream_percent(),
            "land_alt_world_flag": self.u8(LAND_ALT_WORLD_FLAG),
            "land_render_state_byte": self.u8(LAND_RENDER_STATE_BYTE),
            "land_freeze_loader": self.u8(LAND_FREEZE_LOADER),
            "camera_ptr": f"0x{cam:08X}" if cam else None,
        }
        if cam:
            for key, off in [
                ("camera_view_x", CAM_VIEW_X), ("camera_view_y", CAM_VIEW_Y),
                ("camera_near", CAM_NEAR), ("camera_far", CAM_FAR),
                ("camera_z_scale", CAM_Z_SCALE), ("camera_z_shift", CAM_Z_SHIFT),
            ]:
                try:
                    d[key] = self.f32(cam + off)
                except Exception:
                    d[key] = None
        slots = self.land_slots()
        d["land_slots_state5"] = sum(s.state == 5 for s in slots)
        d["land_slots_world_nonzero"] = sum(s.world != 0 for s in slots)
        d["land_slots"] = [s.__dict__ | {"world": f"0x{s.world:08X}"} for s in slots]
        return d


def list_processes() -> list[ProcessInfo]:
    if os.name != "nt":
        return []
    k32 = ctypes.WinDLL("kernel32", use_last_error=True)
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    if snap == INVALID_HANDLE_VALUE:
        return []
    out = []
    try:
        pe = PROCESSENTRY32W()
        pe.dwSize = ctypes.sizeof(pe)
        if not k32.Process32FirstW(snap, ctypes.byref(pe)):
            return out
        while True:
            out.append(ProcessInfo(int(pe.th32ProcessID), pe.szExeFile))
            if not k32.Process32NextW(snap, ctypes.byref(pe)):
                break
    finally:
        k32.CloseHandle(snap)
    return out


def readable_region(mbi: MEMORY_BASIC_INFORMATION64) -> bool:
    return (
        mbi.State == MEM_COMMIT
        and not (mbi.Protect & PAGE_NOACCESS)
        and not (mbi.Protect & PAGE_GUARD)
        and mbi.RegionSize > 0
    )


def validate_mem1(proc: WinProcessMemory, candidate: int) -> bool:
    try:
        if proc.read(candidate, 6) != GAME_ID:
            return False
        # Validate against a known GUPP8P function prologue at 0x8003A578.
        prologue = proc.read(candidate + (0x8003A578 - GC_BASE), 4)
        if prologue not in (bytes.fromhex("9421fff0"), bytes.fromhex("9421ffc0")):
            return False
        # Validate camera default data region as a plausible float.
        f = struct.unpack(">f", proc.read(candidate + (FAR_DEFAULTS[0] - GC_BASE), 4))[0]
        if not math.isfinite(f) or not (1.0 <= f <= 200000.0):
            return False
        return True
    except Exception:
        return False


def find_mem1_base(proc: WinProcessMemory, status_cb=None) -> int:
    regions = [r for r in proc.regions() if readable_region(r)]
    # Fast pass: GameCube MEM1 normally starts at the beginning (or near the beginning) of a mapped region.
    for idx, r in enumerate(regions):
        if status_cb and idx % 200 == 0:
            status_cb(f"Scanning Dolphin memory… {idx}/{len(regions)}")
        n = min(int(r.RegionSize), 0x10000)
        chunk = proc.try_read(int(r.BaseAddress), n)
        if not chunk:
            continue
        pos = 0
        while True:
            j = chunk.find(GAME_ID, pos)
            if j < 0:
                break
            candidate = int(r.BaseAddress) + j
            if validate_mem1(proc, candidate):
                return candidate
            pos = j + 1

    # Slow fallback: scan moderate-size readable regions in chunks.
    for idx, r in enumerate(regions):
        size = int(r.RegionSize)
        if size <= 0 or size > 0x20000000:
            continue
        base = int(r.BaseAddress)
        step = 0x100000
        overlap = len(GAME_ID) - 1
        off = 0
        prev = b""
        while off < size:
            n = min(step, size - off)
            chunk = proc.try_read(base + off, n)
            if not chunk:
                break
            data = prev + chunk
            j = data.find(GAME_ID)
            if j >= 0:
                candidate = base + off - len(prev) + j
                if validate_mem1(proc, candidate):
                    return candidate
            prev = data[-overlap:] if overlap else b""
            off += n
    raise RuntimeError("GUPP8P MEM1 mapping not found. Make sure Shadow is actually running in Dolphin.")


def _dol_sections(blob: bytes):
    if len(blob) < 0x100:
        raise ValueError("File is too small to be a DOL.")
    text_off = struct.unpack(">7I", blob[0x00:0x1C])
    data_off = struct.unpack(">11I", blob[0x1C:0x48])
    text_addr = struct.unpack(">7I", blob[0x48:0x64])
    data_addr = struct.unpack(">11I", blob[0x64:0x90])
    text_size = struct.unpack(">7I", blob[0x90:0xAC])
    data_size = struct.unpack(">11I", blob[0xAC:0xD8])
    out = []
    for o, a, n in zip(text_off, text_addr, text_size):
        if n:
            out.append((o, a, n))
    for o, a, n in zip(data_off, data_addr, data_size):
        if n:
            out.append((o, a, n))
    return out


def _dol_offset(blob: bytes, va: int, size: int = 1) -> int:
    for off, addr, n in _dol_sections(blob):
        if addr <= va and va + size <= addr + n:
            return off + (va - addr)
    raise ValueError(f"Address 0x{va:08X} is not mapped by this DOL.")


def install_engine_lab_dol(target_path: str) -> tuple[str, str]:
    """Install the verified v0.8 DOL.

    v0.8 appends new DOL text/data sections for the resident editor, so it cannot
    be installed as an in-place same-size patch. We verify the selected file is
    the known GUPP8P family, back it up once, then replace it with the bundled build.
    """
    app_dir = os.path.dirname(os.path.abspath(__file__))
    template_path = os.path.join(app_dir, ENGINE_DOL_BASENAME)
    if not os.path.isfile(template_path):
        raise FileNotFoundError(f"Bundled v0.8 editor DOL not found: {template_path}")
    with open(target_path, "rb") as f: original=f.read()
    # Verify known GUPP8P code family before replacement.
    try:
        tpro=_dol_offset(original,ENGINE_HELPER_START,4); pro=original[tpro:tpro+4]
    except Exception as e:
        raise ValueError(f"Selected file is not the expected GUPP8P DOL layout: {e}")
    known={bytes.fromhex("9421fff0"),bytes.fromhex("9421ffc0"),bytes.fromhex("9421ffe0")}
    if pro not in known:
        raise ValueError(f"Unexpected code at 0x{ENGINE_HELPER_START:08X}: {pro.hex()}; refusing replacement.")
    with open(template_path,"rb") as f: template=f.read()
    backup=target_path+".pre_ingame_editor_v08.bak"
    if not os.path.exists(backup):
        with open(backup,"wb") as f:f.write(original)
    with open(target_path,"wb") as f:f.write(template)
    import hashlib
    return backup, hashlib.sha256(template).hexdigest()



class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Shadow RenderWare Lab v0.8 — In-Game Editor Runtime — GUPP8P")
        self.geometry("1080x720")
        self.minsize(760, 520)
        self.proc: Optional[WinProcessMemory] = None
        self.mem: Optional[ShadowMemory] = None
        self._slider_job = None
        self._world_callbacks: dict[int,int] = {}
        self._atomic_callbacks: dict[int,int] = {}
        self._atomics_cache: list[dict] = []
        self._effect_cache: list[dict] = []
        self._scene_cache: list[dict] = []
        self._scene_scan_busy=False
        self._selected_scene: Optional[dict] = None
        self._transform_clipboard: Optional[dict] = None
        self._gizmo_widgets: list[tk.Widget] = []
        self._responsive_scales: list[ttk.Scale] = []
        self._player_frame: int = 0
        self._player_source: str = "unresolved"
        self._input_bridge_enabled = False
        self._input_bridge_thread = None
        self._input_stop = threading.Event()
        self._bridge_last_lmb = False
        self._bridge_last_rmb = False
        self._bridge_last_mouse = None
        self._editor_mode = "move"
        self._overlay = None
        self._overlay_canvas = None
        self._spawn_catalog = {}
        self._load_spawn_catalog()
        self._load_parameter_db()
        self._configure_ui_style()
        self._build_ui()
        self.bind("<Configure>", self._on_window_resize, add="+")
        self.after(500, self._telemetry_tick)

    def _load_spawn_catalog(self):
        p=os.path.join(os.path.dirname(os.path.abspath(__file__)),"object_catalog_v08.json")
        try:self._spawn_catalog=json.load(open(p,"r",encoding="utf-8"))
        except Exception:self._spawn_catalog={}

    def _configure_ui_style(self):
        """Compact defaults so the lab remains usable on smaller desktop windows."""
        style = ttk.Style(self)
        style.configure("TButton", padding=(6, 2))
        style.configure("TCheckbutton", padding=(2, 1))
        style.configure("TNotebook.Tab", padding=(8, 3))
        style.configure("Treeview", rowheight=20)
        style.configure("Compact.TLabelframe", padding=4)
        style.configure("Compact.TLabelframe.Label", font=("TkDefaultFont", 9, "bold"))

    def _on_window_resize(self, _event=None):
        # Sliders are responsive, but deliberately capped so they do not consume
        # the whole tab on ultrawide windows.
        try:
            length = max(150, min(340, int(self.winfo_width() * 0.26)))
            for scale in self._responsive_scales:
                scale.configure(length=length)
        except Exception:
            pass

    def _tree_panel(self, parent, columns, widths, height=16, anchors=None):
        """Create a Treeview with both scrollbars, allowing narrow resizable windows."""
        holder = ttk.Frame(parent)
        holder.pack(fill="both", expand=True)
        tree = ttk.Treeview(holder, columns=columns, show="headings", height=height)
        ysb = ttk.Scrollbar(holder, orient="vertical", command=tree.yview)
        xsb = ttk.Scrollbar(holder, orient="horizontal", command=tree.xview)
        tree.configure(yscrollcommand=ysb.set, xscrollcommand=xsb.set)
        holder.rowconfigure(0, weight=1)
        holder.columnconfigure(0, weight=1)
        tree.grid(row=0, column=0, sticky="nsew")
        ysb.grid(row=0, column=1, sticky="ns")
        xsb.grid(row=1, column=0, sticky="ew")
        anchors = anchors or ["center"] * len(columns)
        for c, w, a in zip(columns, widths, anchors):
            tree.heading(c, text=c)
            tree.column(c, width=w, minwidth=max(40, min(w, 80)), anchor=a, stretch=False)
        return tree

    def _load_parameter_db(self):
        p=os.path.join(os.path.dirname(os.path.abspath(__file__)),"parameters.json")
        try:
            self.parameters=json.load(open(p,"r",encoding="utf-8"))
        except Exception:
            self.parameters=[]

    def _build_ui(self):
        root=ttk.Frame(self,padding=5); root.pack(fill="both",expand=True)
        top=ttk.LabelFrame(root,text="Dolphin / GUPP8P MEM1")
        top.pack(fill="x")
        self.status=tk.StringVar(value="Not attached")
        ttk.Label(top,textvariable=self.status).pack(side="left",padx=8,pady=7)
        ttk.Button(top,text="Attach",command=self.attach_async).pack(side="right",padx=4,pady=6)
        ttk.Button(top,text="Install / Repair v0.8 In-Game Editor DOL",command=self.install_dol).pack(side="right",padx=4,pady=6)

        self.nb=ttk.Notebook(root); self.nb.pack(fill="both",expand=True,pady=(5,0))
        self._tab_live(); self._tab_scene(); self._tab_land(); self._tab_effects(); self._tab_rw(); self._tab_textures(); self._tab_atomics(); self._tab_parameters(); self._tab_raw()

    def _tab_live(self):
        f=ttk.Frame(self.nb,padding=6); self.nb.add(f,text="Live controls")
        f.columnconfigure(1,weight=0)
        f.columnconfigure(3,weight=1)
        self.draw_var=tk.DoubleVar(value=100.0); self.stream_var=tk.DoubleVar(value=100.0)
        self.alt_world=tk.BooleanVar(value=False); self.freeze_loader=tk.BooleanVar(value=False)
        self.effects_hide=tk.BooleanVar(value=False); self.lens_flare=tk.BooleanVar(value=False)

        controls=ttk.LabelFrame(f,text="Camera / streaming",style="Compact.TLabelframe")
        controls.grid(row=0,column=0,columnspan=4,sticky="nw",pady=(0,5))
        ttk.Label(controls,text="Draw distance %").grid(row=0,column=0,sticky="w",padx=(4,6),pady=2)
        self.draw_scale=ttk.Scale(controls,from_=0.1,to=300.0,variable=self.draw_var,command=lambda _v:self._debounce(self.apply_draw),length=260)
        self.draw_scale.grid(row=0,column=1,sticky="w",padx=3)
        self._responsive_scales.append(self.draw_scale)
        self.draw_lbl=ttk.Label(controls,text="100.0%",width=9,anchor="e"); self.draw_lbl.grid(row=0,column=2,padx=(4,6))

        ttk.Label(controls,text="Streaming extent %").grid(row=1,column=0,sticky="w",padx=(4,6),pady=2)
        self.stream_scale=ttk.Scale(controls,from_=0.1,to=100.0,variable=self.stream_var,command=lambda _v:self._debounce(self.apply_stream),length=260)
        self.stream_scale.grid(row=1,column=1,sticky="w",padx=3)
        self._responsive_scales.append(self.stream_scale)
        self.stream_lbl=ttk.Label(controls,text="100.0%",width=9,anchor="e"); self.stream_lbl.grid(row=1,column=2,padx=(4,6))

        ttk.Label(controls,text="Aspect").grid(row=2,column=0,sticky="w",padx=(4,6),pady=2)
        af=ttk.Frame(controls); af.grid(row=2,column=1,sticky="w",padx=3)
        for text,val in [("4:3",4/3),("16:9",16/9),("21:9",21/9),("32:9",32/9)]:
            ttk.Button(af,text=text,width=5,command=lambda v=val:self.apply_aspect(v)).pack(side="left",padx=(0,3))
        self.aspect_lbl=ttk.Label(controls,text="—",width=9,anchor="e"); self.aspect_lbl.grid(row=2,column=2,padx=(4,6))

        toggles=ttk.LabelFrame(f,text="Runtime toggles",style="Compact.TLabelframe")
        toggles.grid(row=1,column=0,columnspan=4,sticky="ew",pady=(0,5))
        self.alt_world.set(False)
        ttk.Label(toggles,text="Collision render: dedicated debug pipeline required (legacy +0x1808 blocked)").grid(row=0,column=0,sticky="w",padx=4,pady=1)
        ttk.Checkbutton(toggles,text="Freeze land loader",variable=self.freeze_loader,command=self.apply_freeze).grid(row=0,column=1,sticky="w",padx=8,pady=1)
        ttk.Checkbutton(toggles,text="Hide EffectManager elements",variable=self.effects_hide,command=self.apply_effects_hide).grid(row=1,column=0,sticky="w",padx=4,pady=1)
        ttk.Checkbutton(toggles,text="Lens flare enable",variable=self.lens_flare,command=self.apply_lens_flare).grid(row=1,column=1,sticky="w",padx=8,pady=1)

        bf=ttk.Frame(f); bf.grid(row=2,column=0,columnspan=4,sticky="w",pady=(2,5))
        ttk.Button(bf,text="Safe defaults",command=self.safe_defaults).pack(side="left",padx=(0,4))
        ttk.Button(bf,text="Dump snapshot JSON",command=self.dump_snapshot).pack(side="left")

        self.telemetry=tk.StringVar(value="Attach to a running Shadow GUPP8P process.")
        tele_holder=ttk.LabelFrame(f,text="Telemetry",style="Compact.TLabelframe")
        tele_holder.grid(row=3,column=0,columnspan=4,sticky="nsew")
        f.rowconfigure(3,weight=1)
        ttk.Label(tele_holder,textvariable=self.telemetry,justify="left",anchor="nw",wraplength=900).pack(fill="both",expand=True,padx=4,pady=3)
    def _tab_scene(self):
        f=ttk.Frame(self.nb,padding=5); self.nb.add(f,text="Scene editor")
        pan=ttk.Panedwindow(f,orient="horizontal"); pan.pack(fill="both",expand=True)
        left=ttk.Frame(pan); right=ttk.Frame(pan); pan.add(left,weight=5); pan.add(right,weight=2)

        top=ttk.Frame(left); top.pack(fill="x",pady=(0,4))
        self.scene_filter=tk.StringVar(value="")
        ttk.Label(top,text="Filter").pack(side="left")
        se=ttk.Entry(top,textvariable=self.scene_filter,width=22); se.pack(side="left",padx=4); se.bind('<KeyRelease>',lambda _e:self.populate_scene())
        self.scene_kind=tk.StringVar(value="All")
        ttk.Combobox(top,textvariable=self.scene_kind,values=["All","World","Clump","Atomic","Actor candidate","Effect"],width=16,state="readonly").pack(side="left",padx=3)
        ttk.Button(top,text="Apply",command=self.populate_scene).pack(side="left")
        ttk.Button(top,text="Rescan scene",command=self.scan_scene_async).pack(side="right")

        prox=ttk.Frame(left); prox.pack(fill="x",pady=(0,4))
        self.scene_near_player=tk.BooleanVar(value=False); self.scene_radius=tk.DoubleVar(value=50.0)
        ttk.Checkbutton(prox,text="Near player",variable=self.scene_near_player,command=self.populate_scene).pack(side="left")
        ttk.Label(prox,text="radius m").pack(side="left",padx=(6,2)); ttk.Entry(prox,textvariable=self.scene_radius,width=7).pack(side="left")
        ttk.Button(prox,text="Mark selected as player",command=self.mark_selected_as_player).pack(side="left",padx=8)
        ttk.Button(prox,text="Auto player",command=self.auto_player_reference).pack(side="left")
        self.player_ref_status=tk.StringVar(value="player reference unresolved")
        ttk.Label(prox,textvariable=self.player_ref_status).pack(side="left",padx=8)
        self.bridge_var=tk.BooleanVar(value=False)
        ttk.Checkbutton(prox,text="In-game mouse/keyboard editor",variable=self.bridge_var,command=self.toggle_input_bridge).pack(side="right")

        cols=("kind","name","conf","dist","ptr","visible","owner","x","y","z","caps","extra")
        widths=(100,220,70,72,105,55,155,68,68,68,140,250)
        self.scene_tree=self._tree_panel(left,cols,widths,height=18,anchors=("w","w","center","e","w","center","w","e","e","e","w","w"))
        self.scene_tree.bind('<<TreeviewSelect>>',self.scene_select)
        b=ttk.Frame(left); b.pack(fill="x",pady=4)
        ttk.Button(b,text="Hide",command=lambda:self.scene_visibility(False)).pack(side="left")
        ttk.Button(b,text="Show",command=lambda:self.scene_visibility(True)).pack(side="left",padx=3)
        ttk.Button(b,text="Hide same type",command=lambda:self.scene_visibility_group(False)).pack(side="left",padx=(8,3))
        ttk.Button(b,text="Show same type",command=lambda:self.scene_visibility_group(True)).pack(side="left")
        ttk.Button(b,text="Blink",command=self.scene_blink).pack(side="left",padx=8)
        self.scene_status=tk.StringVar(value="Rescan to enumerate RenderWare worlds, clumps, atomics and live effects.")
        ttk.Label(left,textvariable=self.scene_status).pack(anchor="w")

        info=ttk.LabelFrame(right,text="Selection / capabilities",style="Compact.TLabelframe"); info.pack(fill="x",pady=(0,5))
        self.scene_info=tk.StringVar(value="Nothing selected")
        ttk.Label(info,textvariable=self.scene_info,justify="left",wraplength=350).pack(fill="x",padx=5,pady=4)

        giz=ttk.LabelFrame(right,text="Transform gizmo",style="Compact.TLabelframe"); giz.pack(fill="x")
        self.gizmo_state=tk.StringVar(value="Select an object with a validated transform owner.")
        ttk.Label(giz,textvariable=self.gizmo_state,wraplength=340).grid(row=0,column=0,columnspan=4,sticky="w",padx=4,pady=(2,4))
        self.gizmo_x=tk.StringVar(value="0"); self.gizmo_y=tk.StringVar(value="0"); self.gizmo_z=tk.StringVar(value="0")
        self._gizmo_widgets=[]
        for row,(lab,var) in enumerate((("X",self.gizmo_x),("Y",self.gizmo_y),("Z",self.gizmo_z)),start=1):
            ttk.Label(giz,text=lab,width=2).grid(row=row,column=0,padx=3,pady=2)
            ent=ttk.Entry(giz,textvariable=var,width=13); ent.grid(row=row,column=1,padx=2,pady=2); self._gizmo_widgets.append(ent)
        apply_btn=ttk.Button(giz,text="Apply XYZ",command=self.gizmo_apply_position); apply_btn.grid(row=1,column=2,rowspan=3,sticky="ns",padx=5,pady=2); self._gizmo_widgets.append(apply_btn)
        self.gizmo_step=tk.DoubleVar(value=1.0)
        ttk.Label(giz,text="Move step").grid(row=4,column=0,columnspan=2,sticky="w",padx=3)
        step_ent=ttk.Entry(giz,textvariable=self.gizmo_step,width=8); step_ent.grid(row=4,column=2,sticky="w",padx=3); self._gizmo_widgets.append(step_ent)
        mv=ttk.Frame(giz); mv.grid(row=5,column=0,columnspan=4,pady=3)
        for text,axis,sgn in (("−X","x",-1),("+X","x",1),("−Y","y",-1),("+Y","y",1),("−Z","z",-1),("+Z","z",1)):
            btn=ttk.Button(mv,text=text,width=4,command=lambda a=axis,s=sgn:self.gizmo_nudge(a,s)); btn.pack(side="left",padx=1); self._gizmo_widgets.append(btn)
        self.gizmo_rot=tk.DoubleVar(value=5.0)
        ttk.Label(giz,text="Rotate °").grid(row=6,column=0,sticky="w",padx=3); rot_ent=ttk.Entry(giz,textvariable=self.gizmo_rot,width=8); rot_ent.grid(row=6,column=1,sticky="w"); self._gizmo_widgets.append(rot_ent)
        rf=ttk.Frame(giz); rf.grid(row=7,column=0,columnspan=4,pady=3)
        for text,axis,sgn in (("X−","x",-1),("X+","x",1),("Y−","y",-1),("Y+","y",1),("Z−","z",-1),("Z+","z",1)):
            btn=ttk.Button(rf,text=text,width=4,command=lambda a=axis,s=sgn:self.gizmo_rotate(a,s)); btn.pack(side="left",padx=1); self._gizmo_widgets.append(btn)
        self.gizmo_scale=tk.DoubleVar(value=1.05)
        ttk.Label(giz,text="Scale factor").grid(row=8,column=0,sticky="w",padx=3); scale_ent=ttk.Entry(giz,textvariable=self.gizmo_scale,width=8); scale_ent.grid(row=8,column=1,sticky="w"); self._gizmo_widgets.append(scale_ent)
        sf=ttk.Frame(giz); sf.grid(row=9,column=0,columnspan=4,pady=3)
        for text,axis in (("Scale X","x"),("Scale Y","y"),("Scale Z","z"),("Uniform","all")):
            btn=ttk.Button(sf,text=text,command=lambda a=axis:self.gizmo_scale_axis(a)); btn.pack(side="left",padx=1); self._gizmo_widgets.append(btn)
        cf=ttk.Frame(giz); cf.grid(row=10,column=0,columnspan=4,sticky="w",pady=(4,2))
        copy_btn=ttk.Button(cf,text="Copy transform",command=self.gizmo_copy_transform); copy_btn.pack(side="left"); self._gizmo_widgets.append(copy_btn)
        paste_btn=ttk.Button(cf,text="Paste transform",command=self.gizmo_paste_transform); paste_btn.pack(side="left",padx=3); self._gizmo_widgets.append(paste_btn)
        ttk.Button(cf,text="Refresh owner",command=self.scene_select).pack(side="left",padx=3)
        self._set_gizmo_enabled(False,"No editable transform selected")
        ttk.Label(right,text="v0.8: W/E/R = move/rotate/scale. Left-click picks the nearest projected scene object; X/Y/Z constrain drag. Right-click opens the editor menu. Transform writes are consumed by resident PPC code once per present.",wraplength=350).pack(fill="x",pady=6)

    def _tab_land(self):
        f=ttk.Frame(self.nb,padding=5); self.nb.add(f,text="Land / worlds")
        cols=("slot","state","world","block","flags","root","sectorcb","pipeline")
        widths=(50,60,120,80,100,120,120,120)
        self.land_tree=self._tree_panel(f,cols,widths,height=14)
        b=ttk.Frame(f); b.pack(fill="x",pady=6)
        ttk.Button(b,text="Refresh",command=self.refresh_land).pack(side="left")
        ttk.Button(b,text="Hide selected world geometry",command=lambda:self.toggle_world(False)).pack(side="left",padx=5)
        ttk.Button(b,text="Show selected world geometry",command=lambda:self.toggle_world(True)).pack(side="left")
        ttk.Label(f,text="RpWorld+0x68 sector render callback; v2 DOL provides a BLR no-op callback.").pack(anchor="w",pady=(0,2))

    def _tab_effects(self):
        f=ttk.Frame(self.nb,padding=5); self.nb.add(f,text="Effects / particles")
        cols=("ptr","class","bucket","alive","hidden","live","cap","skip","texture")
        widths=(110,170,120,55,60,55,55,55,180)
        self.effect_tree=self._tree_panel(f,cols,widths,height=14)
        b=ttk.Frame(f); b.pack(fill="x",pady=6)
        ttk.Button(b,text="Refresh",command=self.refresh_effects).pack(side="left")
        ttk.Button(b,text="Hide selected",command=lambda:self.effect_hide_selected(True)).pack(side="left",padx=5)
        ttk.Button(b,text="Show selected",command=lambda:self.effect_hide_selected(False)).pack(side="left")
        ttk.Button(b,text="Skip draw ON",command=lambda:self.effect_skip_selected(True)).pack(side="left",padx=5)
        ttk.Button(b,text="Skip draw OFF",command=lambda:self.effect_skip_selected(False)).pack(side="left")
        ttk.Button(b,text="Hide bucket",command=self.effect_hide_bucket).pack(side="left",padx=5)

    def _tab_rw(self):
        f=ttk.Frame(self.nb,padding=5); self.nb.add(f,text="RenderWare state")
        self.rw_summary=tk.StringVar(value="—")
        ttk.Label(f,textvariable=self.rw_summary,justify="left").pack(anchor="w",pady=(0,6))
        cols=("state","type","value")
        self.rs_tree=self._tree_panel(f,cols,(220,80,260),height=15,anchors=("w","w","w"))
        ttk.Label(f,text="Render-state cache is telemetry only in v0.3. Direct cache pokes can desynchronise RW and GX; proper force-overrides will use an ASM hook in the next lab DOL.").pack(anchor="w",pady=6)

    def _tab_textures(self):
        f=ttk.Frame(self.nb,padding=5); self.nb.add(f,text="Bound textures")
        cols=("map","texture","name","raster","size","depth","gxfmt","alpha","filter","refs")
        widths=(45,110,180,110,90,55,60,55,110,55)
        self.tex_tree=self._tree_panel(f,cols,widths,height=14)
        ttk.Button(f,text="Refresh",command=self.refresh_textures).pack(anchor="w",pady=6)

    def _tab_atomics(self):
        f=ttk.Frame(self.nb,padding=5); self.nb.add(f,text="Live RpAtomics")
        cols=("atomic","geometry","verts","morph","flags","rendercb","pipeline","skin","frame")
        widths=(110,110,70,55,90,150,110,110,110)
        self.atomic_tree=self._tree_panel(f,cols,widths,height=14)
        b=ttk.Frame(f); b.pack(fill="x",pady=6)
        ttk.Button(b,text="Scan atomics",command=self.scan_atomics_async).pack(side="left")
        ttk.Button(b,text="Hide selected",command=lambda:self.toggle_atomic(False)).pack(side="left",padx=5)
        ttk.Button(b,text="Show selected",command=lambda:self.toggle_atomic(True)).pack(side="left")
        ttk.Button(b,text="Hide same geometry",command=lambda:self.toggle_atomic_same_geom(False)).pack(side="left",padx=5)
        ttk.Button(b,text="Show same geometry",command=lambda:self.toggle_atomic_same_geom(True)).pack(side="left")
        self.atomic_status=tk.StringVar(value="This is renderer-level discovery. Mapping atomics to gameplay enemy/task identities is the next layer.")
        ttk.Label(f,textvariable=self.atomic_status).pack(anchor="w")

    def _tab_parameters(self):
        f=ttk.Frame(self.nb,padding=5); self.nb.add(f,text="Parameter DB")
        top=ttk.Frame(f); top.pack(fill="x")
        ttk.Label(top,text="Filter").pack(side="left")
        self.param_filter=tk.StringVar(); ent=ttk.Entry(top,textvariable=self.param_filter,width=40); ent.pack(side="left",padx=6); ent.bind("<KeyRelease>",lambda _e:self.refresh_parameters())
        cols=("group","name","address","type","access","confidence","description")
        widths=(110,220,110,70,100,90,430)
        self.param_tree=self._tree_panel(f,cols,widths,height=15,anchors=("w","w","w","w","w","w","w")); self.refresh_parameters()

    def _tab_raw(self):
        f=ttk.Frame(self.nb,padding=6); self.nb.add(f,text="Raw memory")
        ttk.Label(f,text="Advanced direct MEM1 editor. Values are big-endian GameCube data.").grid(row=0,column=0,columnspan=3,sticky="w",pady=(0,10))
        self.raw_addr=tk.StringVar(value="0x805E42D0"); self.raw_type=tk.StringVar(value="u32"); self.raw_value=tk.StringVar()
        ttk.Label(f,text="Address").grid(row=1,column=0,sticky="w"); ttk.Entry(f,textvariable=self.raw_addr,width=18).grid(row=1,column=1,sticky="w")
        ttk.Label(f,text="Type").grid(row=2,column=0,sticky="w"); ttk.Combobox(f,textvariable=self.raw_type,values=["u8","u16","u32","s32","f32"],width=10,state="readonly").grid(row=2,column=1,sticky="w")
        ttk.Label(f,text="Value").grid(row=3,column=0,sticky="w"); ttk.Entry(f,textvariable=self.raw_value,width=28).grid(row=3,column=1,sticky="w")
        ttk.Button(f,text="Read",command=self.raw_read).grid(row=4,column=0,pady=8); ttk.Button(f,text="Write",command=self.raw_write).grid(row=4,column=1,sticky="w",pady=8)

    def _debounce(self,func):
        if self._slider_job is not None: self.after_cancel(self._slider_job)
        self._slider_job=self.after(60,func)

    def require_mem(self)->ShadowMemory:
        if not self.mem: raise RuntimeError("Attach to Dolphin first.")
        return self.mem

    def attach_async(self):
        self.status.set("Looking for Dolphin…"); threading.Thread(target=self._attach_worker,daemon=True).start()

    def _attach_worker(self):
        try:
            procs=[p for p in list_processes() if p.name.lower() in DOLPHIN_NAMES]
            if not procs: raise RuntimeError("No Dolphin process found.")
            last=None
            for p in procs:
                proc=None
                try:
                    proc=WinProcessMemory(p.pid); self.after(0,lambda p=p:self.status.set(f"Dolphin PID {p.pid}: locating GUPP8P MEM1…"))
                    base=find_mem1_base(proc,lambda s:self.after(0,lambda s=s:self.status.set(s)))
                    self.proc=proc; self.mem=ShadowMemory(proc,base); self.after(0,lambda p=p,base=base:self._attached(p,base)); return
                except Exception as e:
                    last=e
                    if proc:
                        try: proc.close()
                        except Exception: pass
            raise last or RuntimeError("Could not attach")
        except Exception as e:
            self.after(0,lambda e=e:messagebox.showerror("Attach failed",str(e))); self.after(0,lambda:self.status.set("Not attached"))

    def _attached(self,p,base):
        build=self.mem.detect_build(); hook=self.mem.lab_hook_present()
        self.stream_scale.state(["!disabled"] if hook else ["disabled"])
        self.status.set(f"Attached: {p.name} PID {p.pid} | MEM1 host 0x{base:X} | {build}")
        try:
            self.draw_var.set(self.mem.get_draw_percent()); sp=self.mem.get_stream_percent();
            if sp is not None:self.stream_var.set(sp)
            self.alt_world.set(bool(self.mem.u8(LAND_ALT_WORLD_FLAG))); self.freeze_loader.set(bool(self.mem.u8(LAND_FREEZE_LOADER)))
            self.effects_hide.set(self.mem.effects_hide_all())
            lf=self.mem.lens_flare_ptr(); self.lens_flare.set(bool(lf and self.mem.u8(lf+4)))
        except Exception: pass
        self.refresh_all(); self.scan_scene_async()

    def install_dol(self):
        path=filedialog.askopenfilename(title="Select Shadow GUPP8P main.dol",filetypes=[("DOL executable","*.dol"),("All files","*.*")])
        if not path:return
        try:
            if not messagebox.askyesno("Install RenderWare Lab DOL","Patch selected main.dol in place? A .pre_engine_lab.bak backup will be created. Restart emulation afterwards."):return
            backup,sha=install_engine_lab_dol(path); messagebox.showinfo("Installed",f"Patched:\n{path}\n\nBackup:\n{backup}\n\nSHA-256:\n{sha}")
        except Exception as e: messagebox.showerror("Install failed",str(e))

    def apply_draw(self):
        try:v=self.draw_var.get(); self.require_mem().set_draw_percent(v); self.draw_lbl.config(text=f"{v:.2f}%")
        except Exception as e:self.status.set(str(e))
    def apply_stream(self):
        try:v=self.stream_var.get(); self.require_mem().set_stream_percent(v); self.stream_lbl.config(text=f"{v:.2f}%")
        except Exception as e:self.status.set(str(e))
    def apply_aspect(self,a):
        try:self.require_mem().set_aspect(a); self.aspect_lbl.config(text=f"{a:.5f}")
        except Exception as e:self.status.set(str(e))
    def apply_alt_world(self):
        # Kept for compatibility with older UI state/configs. Never enable +0x1808.
        self.alt_world.set(False)
        try:
            self.require_mem().set_alt_world_view(False)
            self.status.set("Legacy alternate land-world path disabled. Use the dedicated collision overlay path when available.")
        except Exception as e:
            self.status.set(str(e))
    def apply_freeze(self):
        try:self.require_mem().set_freeze_loader(self.freeze_loader.get())
        except Exception as e:self.status.set(str(e))
    def apply_effects_hide(self):
        try:self.require_mem().set_effects_hide_all(self.effects_hide.get())
        except Exception as e:self.status.set(str(e))
    def apply_lens_flare(self):
        try:self.require_mem().set_lens_flare_enabled(self.lens_flare.get())
        except Exception as e:self.status.set(str(e))

    def safe_defaults(self):
        try:
            m=self.require_mem(); self.draw_var.set(100); self.stream_var.set(100); self.alt_world.set(False); self.freeze_loader.set(False); self.effects_hide.set(False)
            m.set_draw_percent(100); m.set_aspect(16/9); m.set_alt_world_view(False); m.set_freeze_loader(False); m.set_effects_hide_all(False)
            if m.lab_hook_present():m.set_stream_percent(100)
            for w,cb in list(self._world_callbacks.items()):
                if m.is_ptr(w): m.set_world_sector_callback(w,cb)
            self._world_callbacks.clear()
            for a,cb in list(self._atomic_callbacks.items()):
                if m.is_ptr(a): m.set_atomic_callback(a,cb)
            self._atomic_callbacks.clear(); self.status.set("Safe defaults restored; hidden world/atomic callbacks restored.")
        except Exception as e:messagebox.showerror("Restore failed",str(e))

    def refresh_all(self):
        self.refresh_land(); self.refresh_effects(); self.refresh_rw(); self.refresh_textures()

    def scan_scene_async(self):
        if self._scene_scan_busy:return
        self._scene_scan_busy=True; self.scene_status.set("Scanning MEM1 for RenderWare scene hierarchy…")
        threading.Thread(target=self._scene_scan_worker,daemon=True).start()

    def _row_transform(self,m:ShadowMemory,kind:str,ptr:int,direct_frame:int=0,clump:int=0,clump_frame:int=0):
        t=m.resolve_transform_owner(kind,ptr,direct_frame,clump,clump_frame)
        pos=None
        if t.get('frame'):
            try:pos=m.frame_matrix(t['frame'])['pos']
            except Exception:pass
        return t,pos

    def _scene_scan_worker(self):
        try:
            m=self.require_mem(); atomics=m.scan_atomics(); effects=m.effect_elements(); lands=m.land_slots()
            self._atomics_cache=atomics; self._effect_cache=effects
            rows=[]
            for sl in lands:
                if sl.world:
                    wi=m.world_info(sl.world); t,pos=self._row_transform(m,'World',sl.world)
                    rows.append({'kind':'World','name':f'Land block {sl.block_id} / slot {sl.index}','ptr':sl.world,'visible':m.u32(sl.world+0x68)!=LAB_NOOP_CALLBACK,
                                 'frame':0,'transform':t,'pos':pos,'caps':['visibility','sector/block'],
                                 'extra':f"state={sl.state} root=0x{wi.get('root_sector',0):08X}"})

            # One row per unique RpClump, then the member atomics. This groups multi-atomic actors/props.
            clumps={}
            for a in atomics:
                c=a.get('clump',0)
                if c:clumps.setdefault(c,[]).append(a)
            for c,members in sorted(clumps.items()):
                root=members[0].get('clump_frame',0)
                actor=any(bool(x.get('skin')) for x in members)
                t,pos=self._row_transform(m,'Clump',c,0,c,root)
                visible=any(x.get('render_cb')!=LAB_NOOP_CALLBACK for x in members)
                caps=['visibility-group']+(['move','rotate','scale'] if t.get('frame') else [])
                rows.append({'kind':'Clump','name':((next((x.get('identity',{}).get('name') for x in members if x.get('identity',{}).get('confidence')=='high'),None)) or ('Actor clump' if actor else 'Render clump'))+f' ({len(members)} atomic)',
                             'confidence':('high' if any(x.get('identity',{}).get('confidence')=='high' for x in members) else 'medium'),'ptr':c,'visible':visible,'frame':t.get('frame',0),'transform':t,'pos':pos,'caps':caps,
                             'members':[x['atomic'] for x in members],
                             'extra':f"root=0x{root:08X} atomics={len(members)} skinned={sum(1 for x in members if x.get('skin'))}"})

            for a in atomics:
                cls=m.classify_atomic(a); kind='Actor candidate' if cls=='Skinned actor candidate' else 'Atomic'
                t,pos=self._row_transform(m,kind,a['atomic'],a.get('frame',0),a.get('clump',0),a.get('clump_frame',0))
                caps=['visibility']+(['move','rotate','scale'] if t.get('frame') else [])
                ident=a.get('identity') or {'name':cls,'confidence':'low','evidence':'classification fallback','textures':[]}
                rows.append({'kind':kind,'name':ident.get('name',cls),'confidence':ident.get('confidence','low'),'ptr':a['atomic'],'visible':a['render_cb']!=LAB_NOOP_CALLBACK,
                             'frame':t.get('frame',0),'transform':t,'pos':pos,'caps':caps,'clump':a.get('clump',0),
                             'extra':f"geom=0x{a['geometry']:08X} v={a['vertices']} clump=0x{a.get('clump',0):08X} pipe=0x{a.get('pipeline',0):08X} tex={','.join(ident.get('textures',[])[:4])} evidence={ident.get('evidence','')}"})
            for e in effects:
                fr=0
                if e['class']=='EffectParticlePJS':
                    try:
                        cand=m.u32(e['ptr']+0x140)
                        if m.valid_frame_matrix(cand):fr=cand
                    except Exception:pass
                t,pos=self._row_transform(m,'Effect',e['ptr'],fr)
                caps=['visibility']+(['move','rotate','scale'] if t.get('frame') else [])
                rows.append({'kind':'Effect','name':e['class'],'confidence':'exact','ptr':e['ptr'],'visible':not e['hidden'],'frame':t.get('frame',0),
                             'transform':t,'pos':pos,'caps':caps,
                             'extra':f"{e['bucket_name']} live={e['live']}/{e['capacity']} tex={e['texture']}"})
            self._scene_cache=rows
            self.after(0,self.populate_scene)
        except Exception as e:self.after(0,lambda e=e:self.scene_status.set(f"Scene scan failed: {e}"))
        finally:self._scene_scan_busy=False

    def _player_position(self):
        if self.mem and self._player_frame and self.mem.valid_frame_matrix(self._player_frame):
            try:return self.mem.frame_matrix(self._player_frame)['pos']
            except Exception:pass
        return None

    def mark_selected_as_player(self):
        r=self._scene_selected_row() or self._selected_scene
        if not r:return
        fr=r.get('transform',{}).get('frame',0)
        if fr and self.mem and self.mem.valid_frame_matrix(fr):
            self._player_frame=fr; self._player_source=f"manual: {r.get('name','object')}"
            self.player_ref_status.set(self._player_source); self.populate_scene()

    def auto_player_reference(self):
        if not self.mem:return
        cam=self.mem.camera_position(); cands=[]
        for r in self._scene_cache:
            if r.get('kind') not in ('Actor candidate','Clump'):continue
            fr=r.get('transform',{}).get('frame',0); p=r.get('pos')
            if not fr or not p:continue
            score=0.0
            name=r.get('name','').lower()
            if 'shadow' in name:score-=10000.0
            if cam:score+=math.dist(p,cam)
            cands.append((score,r))
        if cands:
            _,r=min(cands,key=lambda q:q[0]); self._player_frame=r.get('transform',{}).get('frame',0)
            self._player_source=('auto high: Shadow asset match' if 'shadow' in r.get('name','').lower() else 'auto medium: nearest skinned actor to camera')
            self.player_ref_status.set(self._player_source); self.populate_scene()
        else:self.player_ref_status.set('auto player unresolved')

    def populate_scene(self):
        if not hasattr(self,'scene_tree'):return
        for i in self.scene_tree.get_children():self.scene_tree.delete(i)
        q=self.scene_filter.get().strip().lower(); k=self.scene_kind.get(); shown=0
        ppos=self._player_position(); radius=float(self.scene_radius.get() or 50.0) if hasattr(self,'scene_radius') else 50.0
        indexed=[]
        for idx,r in enumerate(self._scene_cache):
            p=r.get('pos'); dist=math.dist(p,ppos) if (p and ppos) else None; r['distance']=dist
            indexed.append((idx,r))
        if getattr(self,'scene_near_player',None) and self.scene_near_player.get() and ppos:
            indexed=[it for it in indexed if it[1].get('distance') is not None and it[1]['distance']<=radius]
        indexed.sort(key=lambda it:(1e30 if it[1].get('distance') is None else it[1]['distance'],it[1].get('name','')))
        for idx,r in indexed:
            if k!='All' and r['kind']!=k:continue
            t=r.get('transform',{}); caps=', '.join(r.get('caps',[])); owner=(f"{t.get('owner_kind','?')} 0x{t.get('owner',0):08X}" if t.get('owner') else '—')
            hay=(r['kind']+' '+r['name']+' '+r['extra']+' '+caps+' '+owner).lower()
            if q and q not in hay:continue
            p=r.get('pos'); xyz=(('—','—','—') if not p else tuple(f"{v:.2f}" for v in p)); dist=r.get('distance')
            iid=f"scene_{idx}"; conf=r.get('confidence','—'); ds='—' if dist is None else f"{dist:.2f}"
            self.scene_tree.insert('', 'end', iid=iid, values=(r['kind'],r['name'],conf,ds,f"0x{r['ptr']:08X}",'yes' if r['visible'] else 'no',owner,xyz[0],xyz[1],xyz[2],caps,r['extra']))
            shown+=1
        ref=self._player_source if ppos else 'no player reference'
        self.scene_status.set(f"{shown}/{len(self._scene_cache)} rows | proximity reference: {ref} | resident editor: "+('OK' if self.mem and self.mem.editor_runtime_present() else 'not loaded'))

    def _scene_selected_row(self):
        sel=self.scene_tree.selection()
        if not sel:return None
        try:return self._scene_cache[int(sel[0].split('_',1)[1])]
        except Exception:return None

    def _set_gizmo_enabled(self,enabled:bool,reason:str=''):
        state='normal' if enabled else 'disabled'
        for w in getattr(self,'_gizmo_widgets',[]):
            try:w.configure(state=state)
            except Exception:pass
        if hasattr(self,'gizmo_state'):
            self.gizmo_state.set(reason if reason else ('Editable transform resolved' if enabled else 'No editable transform'))

    def scene_select(self,_event=None):
        r=self._scene_selected_row() or self._selected_scene; self._selected_scene=r
        if not r:
            self.scene_info.set('Nothing selected'); self._set_gizmo_enabled(False,'No object selected'); return
        t=r.get('transform',{}); frame=t.get('frame',0)
        owner=(f"{t.get('owner_kind','?')} 0x{t.get('owner',0):08X}" if t.get('owner') else 'none')
        self.scene_info.set(f"{r['kind']}\n{r['name']}\nptr=0x{r['ptr']:08X}\ntransform owner={owner}\nframe="+(f"0x{frame:08X}" if frame else 'none')+f"\nsource={t.get('source','none')}\ncapabilities={', '.join(r.get('caps',[]))}\n{r['extra']}")
        if frame:
            try:
                if self.mem and self.mem.editor_runtime_present(): self.mem.editor_select(r.get('ptr',0),frame)
                pos=self.require_mem().frame_matrix(frame)['pos']; r['pos']=pos
                self.gizmo_x.set(f"{pos[0]:.5f}"); self.gizmo_y.set(f"{pos[1]:.5f}"); self.gizmo_z.set(f"{pos[2]:.5f}")
                self._set_gizmo_enabled(True,f"{t.get('source')} → RwFrame 0x{frame:08X}")
            except Exception as e:self._set_gizmo_enabled(False,f"Transform validation failed: {e}")
        else:self._set_gizmo_enabled(False,t.get('reason','No editable transform for this object type'))

    def _set_row_visibility(self,r,on:bool):
        m=self.require_mem(); kind=r['kind']
        if kind=='World':
            w=r['ptr']; cur=m.u32(w+0x68)
            if on:m.set_world_sector_callback(w,self._world_callbacks.pop(w,DEFAULT_SECTOR_RENDER_CB))
            else:
                if cur!=LAB_NOOP_CALLBACK:self._world_callbacks.setdefault(w,cur)
                m.set_world_sector_callback(w,LAB_NOOP_CALLBACK)
        elif kind in ('Atomic','Actor candidate'):
            a=r['ptr']; cur=m.u32(a+0x48)
            if on:m.set_atomic_callback(a,self._atomic_callbacks.pop(a,DEFAULT_ATOMIC_RENDER_CB))
            else:
                if cur!=LAB_NOOP_CALLBACK:self._atomic_callbacks.setdefault(a,cur)
                m.set_atomic_callback(a,LAB_NOOP_CALLBACK)
        elif kind=='Clump':
            members=r.get('members') or m.clump_atomics(r['ptr'],self._atomics_cache)
            for a in members:
                try:
                    cur=m.u32(a+0x48)
                    if on:m.set_atomic_callback(a,self._atomic_callbacks.pop(a,DEFAULT_ATOMIC_RENDER_CB))
                    else:
                        if cur!=LAB_NOOP_CALLBACK:self._atomic_callbacks.setdefault(a,cur)
                        m.set_atomic_callback(a,LAB_NOOP_CALLBACK)
                except Exception:pass
        elif kind=='Effect':m.set_effect_hidden(r['ptr'],not on)
        else:raise RuntimeError('No visibility handler for this type')
        r['visible']=on

    def scene_visibility(self,on:bool):
        try:
            r=self._scene_selected_row() or self._selected_scene
            if not r:raise RuntimeError('Select a scene object')
            self._set_row_visibility(r,on); self.populate_scene()
        except Exception as e:messagebox.showerror('Scene visibility',str(e))

    def scene_visibility_group(self,on:bool):
        r=self._scene_selected_row() or self._selected_scene
        if not r:return
        kind=r['kind']
        for row in [x for x in self._scene_cache if x['kind']==kind]:
            try:self._set_row_visibility(row,on)
            except Exception:pass
        self.populate_scene()

    def scene_blink(self):
        r=self._scene_selected_row() or self._selected_scene
        if not r:return
        self._set_row_visibility(r,False); self.populate_scene()
        self.after(250,lambda r=r:(self._set_row_visibility(r,True),self.populate_scene()))

    def _selected_frame(self):
        r=self._scene_selected_row() or self._selected_scene
        if not r:raise RuntimeError('No scene object selected')
        frame=r.get('transform',{}).get('frame',0)
        if not frame:raise RuntimeError(r.get('transform',{}).get('reason','No editable transform owner'))
        return frame

    def _refresh_selected_transform(self):
        r=self._scene_selected_row() or self._selected_scene
        if not r:return
        frame=r.get('transform',{}).get('frame',0)
        if not frame:return
        try:
            pos=self.require_mem().frame_matrix(frame)['pos']; r['pos']=pos
            self.gizmo_x.set(f"{pos[0]:.5f}"); self.gizmo_y.set(f"{pos[1]:.5f}"); self.gizmo_z.set(f"{pos[2]:.5f}"); self.populate_scene(); self.scene_select()
        except Exception:pass

    def gizmo_apply_position(self):
        try:
            m=self.require_mem(); fr=self._selected_frame(); pos=(float(self.gizmo_x.get()),float(self.gizmo_y.get()),float(self.gizmo_z.get()))
            (m.editor_apply_position(fr,pos) if m.editor_runtime_present() else m.set_frame_position(fr,*pos)); self.after(35,self._refresh_selected_transform)
        except Exception as e:self.gizmo_state.set(str(e))
    def gizmo_nudge(self,axis,sgn):
        try:
            v=float(self.gizmo_step.get())*sgn; kw={'dx':0,'dy':0,'dz':0}; kw['d'+axis]=v
            self.require_mem().nudge_frame(self._selected_frame(),**kw); self._refresh_selected_transform()
        except Exception as e:self.gizmo_state.set(str(e))
    def gizmo_rotate(self,axis,sgn):
        try:self.require_mem().rotate_frame_local(self._selected_frame(),axis,float(self.gizmo_rot.get())*sgn); self._refresh_selected_transform()
        except Exception as e:self.gizmo_state.set(str(e))
    def gizmo_scale_axis(self,axis):
        try:
            f=float(self.gizmo_scale.get()); sx=sy=sz=1.0
            if axis in ('x','all'):sx=f
            if axis in ('y','all'):sy=f
            if axis in ('z','all'):sz=f
            self.require_mem().scale_frame(self._selected_frame(),sx,sy,sz); self._refresh_selected_transform()
        except Exception as e:self.gizmo_state.set(str(e))

    def gizmo_copy_transform(self):
        try:
            m=self.require_mem().frame_matrix(self._selected_frame()); self._transform_clipboard={k:tuple(v) for k,v in m.items()}
            self.gizmo_state.set('Transform copied')
        except Exception as e:self.gizmo_state.set(str(e))

    def gizmo_paste_transform(self):
        try:
            if not self._transform_clipboard:raise RuntimeError('Transform clipboard is empty')
            fr=self._selected_frame(); m=self._transform_clipboard
            self.require_mem().set_frame_basis(fr,m['right'],m['up'],m['at']); self.require_mem().set_frame_position(fr,*m['pos']); self._refresh_selected_transform()
        except Exception as e:self.gizmo_state.set(str(e))

    # ---- v0.8 mouse/keyboard bridge + projected in-game gizmo -------------
    def toggle_input_bridge(self):
        on=bool(self.bridge_var.get())
        if on:
            try:
                m=self.require_mem()
                if not m.editor_runtime_present():raise RuntimeError('Boot the supplied v0.8 editor DOL first')
                self._input_bridge_enabled=True; self._input_stop.clear()
                if not self._input_bridge_thread or not self._input_bridge_thread.is_alive():
                    self._input_bridge_thread=threading.Thread(target=self._input_bridge_worker,daemon=True); self._input_bridge_thread.start()
                self.gizmo_state.set('In-game bridge active: W/E/R modes, LMB select/drag, X/Y/Z constraint, RMB menu')
            except Exception as e:
                self.bridge_var.set(False); self.gizmo_state.set(str(e))
        else:
            self._input_bridge_enabled=False; self._input_stop.set(); self._destroy_overlay()

    def _dolphin_hwnd(self):
        if os.name!='nt' or not self.proc:return 0
        user32=ctypes.WinDLL('user32',use_last_error=True); wins=[]
        CALLBACK=ctypes.WINFUNCTYPE(wintypes.BOOL,wintypes.HWND,wintypes.LPARAM)
        @CALLBACK
        def cb(hwnd,lparam):
            pid=wintypes.DWORD(); user32.GetWindowThreadProcessId(hwnd,ctypes.byref(pid))
            if pid.value==self.proc.pid and user32.IsWindowVisible(hwnd):
                r=wintypes.RECT();
                if user32.GetClientRect(hwnd,ctypes.byref(r)) and (r.right-r.left)>320 and (r.bottom-r.top)>200:wins.append(hwnd)
            return True
        user32.EnumWindows(cb,0)
        return wins[0] if wins else 0

    def _window_client_info(self,hwnd):
        user32=ctypes.WinDLL('user32',use_last_error=True); r=wintypes.RECT(); p=wintypes.POINT(0,0)
        if not user32.GetClientRect(hwnd,ctypes.byref(r)):return None
        user32.ClientToScreen(hwnd,ctypes.byref(p))
        return (p.x,p.y,r.right-r.left,r.bottom-r.top)

    def _pick_scene_at(self,mx,my,w,h):
        if not self.mem:return None
        best=None
        for r in self._scene_cache:
            p=r.get('pos'); fr=r.get('transform',{}).get('frame',0)
            if not p or not fr:continue
            sp=self.mem.project_world_point(p,w,h)
            if not sp:continue
            d=math.hypot(sp[0]-mx,sp[1]-my)
            if d<=EDITOR_PICK_RADIUS_PX and (best is None or d<best[0]):best=(d,r)
        return best[1] if best else None

    def _select_row_from_bridge(self,r):
        self._selected_scene=r
        try:
            fr=r.get('transform',{}).get('frame',0)
            if self.mem and self.mem.editor_runtime_present():self.mem.editor_select(r.get('ptr',0),fr)
            iid=f"scene_{self._scene_cache.index(r)}"
            if self.scene_tree.exists(iid):self.scene_tree.selection_set(iid); self.scene_tree.see(iid)
            self.scene_select(); self._draw_overlay()
        except Exception:pass

    def _show_ingame_context_menu(self,x,y):
        r=self._selected_scene
        menu=tk.Menu(self,tearoff=0)
        menu.add_command(label=(r.get('name','No selection') if r else 'No selection'),state='disabled')
        if r:
            menu.add_command(label='Hide',command=lambda:self._set_row_visibility(r,False))
            menu.add_command(label='Show',command=lambda:self._set_row_visibility(r,True))
            menu.add_command(label='Mark as player',command=self.mark_selected_as_player)
            menu.add_separator(); menu.add_command(label='Move here (same depth)',command=lambda:self.gizmo_state.set('Use W + drag; ray/surface placement will use collision overlay in next factory pass'))
        sp=tk.Menu(menu,tearoff=0)
        # Catalog is real reverse-engineered type inventory; factory calls stay disabled until constructor ownership is proven.
        for name in (self._spawn_catalog.get('enemies') or [])[:18]:sp.add_command(label=name,state='disabled')
        sp.add_separator(); sp.add_command(label='Spawn factory mapping in progress',state='disabled')
        menu.add_cascade(label='Spawn',menu=sp)
        try:menu.tk_popup(x,y)
        finally:menu.grab_release()

    def _input_bridge_worker(self):
        user32=ctypes.WinDLL('user32',use_last_error=True)
        VK_LBUTTON=0x01; VK_RBUTTON=0x02
        def down(vk):return bool(user32.GetAsyncKeyState(vk)&0x8000)
        last=None; lprev=rprev=False
        while self._input_bridge_enabled and not self._input_stop.is_set():
            try:
                hwnd=self._dolphin_hwnd(); info=self._window_client_info(hwnd) if hwnd else None
                if not info or user32.GetForegroundWindow()!=hwnd: time.sleep(.03); continue
                ox,oy,w,h=info; pt=wintypes.POINT(); user32.GetCursorPos(ctypes.byref(pt)); mx,my=pt.x-ox,pt.y-oy
                inside=0<=mx<w and 0<=my<h; l=down(VK_LBUTTON); r=down(VK_RBUTTON)
                if down(ord('W')):self._editor_mode='move'
                elif down(ord('E')):self._editor_mode='rotate'
                elif down(ord('R')):self._editor_mode='scale'
                if inside and l and not lprev:
                    row=self._pick_scene_at(mx,my,w,h)
                    if row:self.after(0,lambda rr=row:self._select_row_from_bridge(rr))
                    last=(mx,my)
                elif inside and l and lprev and last and self._selected_scene:
                    dx,dy=mx-last[0],my-last[1]; last=(mx,my)
                    fr=self._selected_scene.get('transform',{}).get('frame',0)
                    if fr and self.mem and (abs(dx)+abs(dy)>0):
                        mat=self.mem.frame_matrix(fr)
                        if self._editor_mode=='move':
                            p=list(mat['pos']); scale=max(.005,abs(mat['pos'][2])*0.0004)
                            # Explicit axis constraints; otherwise move in a camera-ish XY plane.
                            if down(ord('X')):p[0]+=dx*scale
                            elif down(ord('Y')):p[1]-=dy*scale
                            elif down(ord('Z')):p[2]+=(-dy+dx)*0.5*scale
                            else:p[0]+=dx*scale; p[1]-=dy*scale
                            self.mem.editor_apply_position(fr,p); self._selected_scene['pos']=tuple(p)
                        elif self._editor_mode=='rotate':
                            # Rotation math is host-side; write resulting basis through resident mailbox.
                            axis='x' if down(ord('X')) else ('y' if down(ord('Y')) else 'z')
                            deg=(dx-dy)*0.35
                            r0=list(mat['right']); u0=list(mat['up']); a0=list(mat['at']); c=math.cos(math.radians(deg)); q=math.sin(math.radians(deg))
                            def mix(v,w,sg=1):return [c*v[i]+sg*q*w[i] for i in range(3)]
                            if axis=='x':u,a=mix(u0,a0,1),mix(a0,u0,-1); rr=r0
                            elif axis=='y':rr,a=mix(r0,a0,-1),mix(a0,r0,1); u=u0
                            else:rr,u=mix(r0,u0,1),mix(u0,r0,-1); a=a0
                            self.mem.editor_apply_basis(fr,rr,u,a)
                        elif self._editor_mode=='scale':
                            fac=max(.1,1.0+(-dy+dx)*0.005); rr=list(mat['right']); uu=list(mat['up']); aa=list(mat['at'])
                            if down(ord('X')):rr=[v*fac for v in rr]
                            elif down(ord('Y')):uu=[v*fac for v in uu]
                            elif down(ord('Z')):aa=[v*fac for v in aa]
                            else:rr=[v*fac for v in rr];uu=[v*fac for v in uu];aa=[v*fac for v in aa]
                            self.mem.editor_apply_basis(fr,rr,uu,aa)
                        self.after(0,self._draw_overlay)
                if not l:last=None
                if inside and r and not rprev:self.after(0,lambda x=pt.x,y=pt.y:self._show_ingame_context_menu(x,y))
                lprev,rprev=l,r
                self.after(0,self._draw_overlay)
            except Exception:pass
            time.sleep(.016)

    def _ensure_overlay(self):
        if self._overlay and self._overlay.winfo_exists():return
        try:
            ov=tk.Toplevel(self); ov.overrideredirect(True); ov.attributes('-topmost',True)
            key='#010203'; ov.configure(bg=key)
            try:ov.wm_attributes('-transparentcolor',key)
            except Exception:ov.attributes('-alpha',0.75)
            cv=tk.Canvas(ov,bg=key,highlightthickness=0); cv.pack(fill='both',expand=True)
            self._overlay=ov; self._overlay_canvas=cv
            if os.name=='nt':
                ov.update_idletasks(); hwnd=ov.winfo_id(); user32=ctypes.WinDLL('user32'); GWL_EXSTYLE=-20; WS_EX_TRANSPARENT=0x20; WS_EX_TOOLWINDOW=0x80; WS_EX_LAYERED=0x80000
                style=user32.GetWindowLongW(hwnd,GWL_EXSTYLE); user32.SetWindowLongW(hwnd,GWL_EXSTYLE,style|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW|WS_EX_LAYERED)
        except Exception:self._overlay=None;self._overlay_canvas=None

    def _destroy_overlay(self):
        try:
            if self._overlay:self._overlay.destroy()
        except Exception:pass
        self._overlay=None;self._overlay_canvas=None

    def _draw_overlay(self):
        if not self._input_bridge_enabled or not self.mem or not self._selected_scene:return
        hwnd=self._dolphin_hwnd(); info=self._window_client_info(hwnd) if hwnd else None
        if not info:return
        ox,oy,w,h=info; self._ensure_overlay()
        if not self._overlay_canvas:return
        self._overlay.geometry(f'{w}x{h}+{ox}+{oy}'); cv=self._overlay_canvas; cv.delete('all')
        p=self._selected_scene.get('pos');
        if not p:return
        s0=self.mem.project_world_point(p,w,h)
        if not s0:return
        # World-axis gizmo scaled to keep a useful on-screen size.
        length=max(0.5,s0[2]*0.08)
        axes=[((p[0]+length,p[1],p[2]),'#ff4040','X'),((p[0],p[1]+length,p[2]),'#40ff60','Y'),((p[0],p[1],p[2]+length),'#4090ff','Z')]
        for end,color,label in axes:
            se=self.mem.project_world_point(end,w,h)
            if se:
                cv.create_line(s0[0],s0[1],se[0],se[1],fill=color,width=3,arrow='last');cv.create_text(se[0]+8,se[1],text=label,fill=color,font=('Arial',10,'bold'))
        name=self._selected_scene.get('name','object');cv.create_text(s0[0]+8,s0[1]-14,text=f'{name} [{self._editor_mode.upper()}]',fill='white',anchor='w',font=('Arial',10,'bold'))

    def refresh_land(self):
        if not self.mem:return
        for i in self.land_tree.get_children(): self.land_tree.delete(i)
        try:
            for s in self.mem.land_slots():
                wi=self.mem.world_info(s.world) if s.world else {}
                self.land_tree.insert("","end",iid=str(s.index),values=(s.index,s.state,f"0x{s.world:08X}",s.block_id,f"0x{wi.get('flags',0):08X}",f"0x{wi.get('root_sector',0):08X}",f"0x{wi.get('sector_render_cb',0):08X}",f"0x{wi.get('pipeline',0):08X}"))
        except Exception as e:self.status.set(f"Land refresh: {e}")

    def _selected_land_world(self):
        sel=self.land_tree.selection()
        if not sel:return 0
        vals=self.land_tree.item(sel[0],"values")
        return int(str(vals[2]),16)
    def toggle_world(self,enabled):
        try:
            m=self.require_mem()
            if m.u32(LAB_VERSION)<2: raise RuntimeError("Per-world geometry suppression requires RenderWare Lab DOL v2.")
            w=self._selected_land_world()
            if not w:raise RuntimeError("Select a land slot with an RpWorld pointer.")
            cur=m.u32(w+0x68)
            if enabled:
                cb=self._world_callbacks.pop(w,DEFAULT_SECTOR_RENDER_CB); m.set_world_sector_callback(w,cb)
            else:
                if cur!=LAB_NOOP_CALLBACK:self._world_callbacks.setdefault(w,cur)
                m.set_world_sector_callback(w,LAB_NOOP_CALLBACK)
            self.refresh_land()
        except Exception as e:messagebox.showerror("World toggle",str(e))

    def refresh_effects(self):
        if not self.mem:return
        for i in self.effect_tree.get_children(): self.effect_tree.delete(i)
        try:
            self._effect_cache=self.mem.effect_elements()
            for e in self._effect_cache:
                self.effect_tree.insert("","end",iid=f"{e['ptr']:08X}",values=(f"0x{e['ptr']:08X}",e['class'],f"{e['bucket']} {e['bucket_name']}",int(e['alive']),int(e['hidden']),e['live'],e['capacity'],e['skip_draw'],e['texture']))
        except Exception as e:self.status.set(f"Effects refresh: {e}")
    def _selected_effect(self):
        sel=self.effect_tree.selection(); return int(sel[0],16) if sel else 0
    def effect_hide_selected(self,on):
        try:self.require_mem().set_effect_hidden(self._selected_effect(),on); self.refresh_effects()
        except Exception as e:messagebox.showerror("Effect",str(e))
    def effect_skip_selected(self,on):
        try:
            p=self._selected_effect(); e=next((x for x in self._effect_cache if x['ptr']==p),None)
            if not e or e['class']!="EffectParticlePJS": raise RuntimeError("skip-draw is verified for EffectParticlePJS only.")
            self.require_mem().set_effect_skip_draw(p,on); self.refresh_effects()
        except Exception as e:messagebox.showerror("Effect",str(e))
    def effect_hide_bucket(self):
        try:
            p=self._selected_effect(); e=next((x for x in self._effect_cache if x['ptr']==p),None)
            if not e:raise RuntimeError("Select an effect element")
            for x in self._effect_cache:
                if x['bucket']==e['bucket']: self.require_mem().set_effect_hidden(x['ptr'],True)
            self.refresh_effects()
        except Exception as e:messagebox.showerror("Bucket",str(e))

    def refresh_rw(self):
        if not self.mem:return
        try:
            g=self.mem.rw_globals_snapshot(); self.rw_summary.set(f"RwGlobals 0x{g.get('ptr',0):08X} | curCamera 0x{g.get('curCamera',0):08X} | curWorld 0x{g.get('curWorld',0):08X} | renderFrame {g.get('renderFrame','?')} | lightFrame {g.get('lightFrame','?')} | engineStatus {g.get('engineStatus','?')} | RSSet 0x{g.get('fpRenderStateSet',0):08X}")
            for i in self.rs_tree.get_children():self.rs_tree.delete(i)
            for n,t,v in self.mem.render_state_snapshot():self.rs_tree.insert("","end",values=(n,t,v))
        except Exception as e:self.rw_summary.set(str(e))

    def refresh_textures(self):
        if not self.mem:return
        for i in self.tex_tree.get_children():self.tex_tree.delete(i)
        try:
            for d in self.mem.bound_textures():
                r=d.get('raster_info',{}); size=f"{r.get('width','?')}x{r.get('height','?')}" if r else "—"
                self.tex_tree.insert("","end",values=(d['map'],f"0x{d.get('ptr',0):08X}",d.get('name',''),f"0x{d.get('raster',0):08X}",size,r.get('depth',''),r.get('gx_fmt',''),r.get('has_alpha',''),f"0x{d.get('filter_addressing',0):08X}",d.get('ref_count','')))
        except Exception as e:self.status.set(f"Texture refresh: {e}")

    def scan_atomics_async(self):
        self.atomic_status.set("Scanning 24 MiB MEM1 for validated RpAtomic candidates…")
        threading.Thread(target=self._scan_atomics_worker,daemon=True).start()
    def _scan_atomics_worker(self):
        try:
            rows=self.require_mem().scan_atomics(); self._atomics_cache=rows; self.after(0,self._populate_atomics)
        except Exception as e:self.after(0,lambda e=e:self.atomic_status.set(f"Scan failed: {e}"))
    def _populate_atomics(self):
        for i in self.atomic_tree.get_children():self.atomic_tree.delete(i)
        for a in self._atomics_cache:
            self.atomic_tree.insert("","end",iid=f"{a['atomic']:08X}",values=(f"0x{a['atomic']:08X}",f"0x{a['geometry']:08X}",a['vertices'],a['morph_targets'],f"0x{a['geom_flags']:08X}",a['render_name'],f"0x{a['pipeline']:08X}",f"0x{a['skin']:08X}",f"0x{a['frame']:08X}"))
        self.atomic_status.set(f"Found {len(self._atomics_cache)} validated RpAtomic candidates. These include characters, props, effects and other clumps; gameplay identity mapping is not yet complete.")
    def _selected_atomic(self):
        sel=self.atomic_tree.selection(); return int(sel[0],16) if sel else 0
    def toggle_atomic(self,enabled):
        try:
            m=self.require_mem()
            if m.u32(LAB_VERSION)<2:raise RuntimeError("Atomic suppression requires RenderWare Lab DOL v2.")
            a=self._selected_atomic();
            if not a:raise RuntimeError("Select an RpAtomic")
            cur=m.u32(a+0x48)
            if enabled:
                cb=self._atomic_callbacks.pop(a,DEFAULT_ATOMIC_RENDER_CB); m.set_atomic_callback(a,cb)
            else:
                if cur!=LAB_NOOP_CALLBACK:self._atomic_callbacks.setdefault(a,cur)
                m.set_atomic_callback(a,LAB_NOOP_CALLBACK)
            self.scan_atomics_async()
        except Exception as e:messagebox.showerror("Atomic",str(e))
    def toggle_atomic_same_geom(self,enabled):
        try:
            a=self._selected_atomic(); row=next((x for x in self._atomics_cache if x['atomic']==a),None)
            if not row:raise RuntimeError("Select an RpAtomic")
            targets=[x for x in self._atomics_cache if x['geometry']==row['geometry']]
            m=self.require_mem()
            for x in targets:
                cur=m.u32(x['atomic']+0x48)
                if enabled:
                    cb=self._atomic_callbacks.pop(x['atomic'],DEFAULT_ATOMIC_RENDER_CB); m.set_atomic_callback(x['atomic'],cb)
                else:
                    if cur!=LAB_NOOP_CALLBACK:self._atomic_callbacks.setdefault(x['atomic'],cur)
                    m.set_atomic_callback(x['atomic'],LAB_NOOP_CALLBACK)
            self.scan_atomics_async()
        except Exception as e:messagebox.showerror("Atomic group",str(e))

    def refresh_parameters(self):
        if not hasattr(self,'param_tree'):return
        q=self.param_filter.get().lower().strip() if hasattr(self,'param_filter') else ''
        for i in self.param_tree.get_children():self.param_tree.delete(i)
        for p in self.parameters:
            hay=' '.join(str(p.get(k,'')) for k in p).lower()
            if q and q not in hay:continue
            self.param_tree.insert("","end",values=(p.get('group',''),p.get('name',''),p.get('address',''),p.get('type',''),p.get('access',''),p.get('confidence',''),p.get('description','')))

    def raw_read(self):
        try:
            m=self.require_mem(); a=int(self.raw_addr.get(),0); t=self.raw_type.get()
            v={'u8':m.u8,'u16':m.u16,'u32':m.u32,'s32':m.s32,'f32':m.f32}[t](a); self.raw_value.set(str(v))
        except Exception as e:messagebox.showerror("Raw read",str(e))
    def raw_write(self):
        try:
            if not messagebox.askyesno("Raw MEM1 write","Write directly to guest memory? This can crash the game."):return
            m=self.require_mem(); a=int(self.raw_addr.get(),0); t=self.raw_type.get(); s=self.raw_value.get().strip()
            if t=='u8':m.set_u8(a,int(s,0))
            elif t=='u16':m.write(a,struct.pack('>H',int(s,0)&0xffff))
            elif t=='u32':m.set_u32(a,int(s,0))
            elif t=='s32':m.write(a,struct.pack('>i',int(s,0)))
            elif t=='f32':m.set_f32(a,float(s))
        except Exception as e:messagebox.showerror("Raw write",str(e))

    def _telemetry_tick(self):
        try:
            if self.mem:
                snap=self.mem.snapshot(); g=self.mem.rw_globals_snapshot(); mgr=self.mem.effect_manager_ptr();
                self.telemetry.set(f"Build: {self.mem.detect_build()}\nCamera {snap.get('camera_ptr')} near={snap.get('camera_near')} far={snap.get('camera_far')} view=({snap.get('camera_view_x')},{snap.get('camera_view_y')})\nLand worlds {snap.get('land_slots_world_nonzero')}/32 | stream={snap.get('stream_percent')}% | RwGlobals=0x{g.get('ptr',0):08X} curWorld=0x{g.get('curWorld',0):08X} | EffectMgr=0x{mgr:08X}")
                self.refresh_rw(); self.refresh_textures()
        except Exception as e:self.telemetry.set(f"Telemetry read failed: {e}")
        self.after(750,self._telemetry_tick)

    def dump_snapshot(self):
        try:
            m=self.require_mem(); snap=m.snapshot(); snap['rw_globals']=m.rw_globals_snapshot(); snap['render_states']=m.render_state_snapshot(); snap['effects']=m.effect_elements(); snap['bound_textures']=m.bound_textures()
            path=filedialog.asksaveasfilename(defaultextension='.json',filetypes=[('JSON','*.json')],initialfile='shadow_renderware_snapshot.json')
            if not path:return
            json.dump(snap,open(path,'w',encoding='utf-8'),indent=2); self.status.set(f"Saved {path}")
        except Exception as e:messagebox.showerror("Dump failed",str(e))


def main():
    if os.name != "nt":
        print("Shadow RenderWare Lab is Windows-only because it attaches to Dolphin via Win32 process memory APIs.")
        return 2
    App().mainloop(); return 0

if __name__ == "__main__":
    raise SystemExit(main())
