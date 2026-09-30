# Boot process reconstruction

## 1. Physical disc/system layout — CONFIRMED

| Region | Start | End | Size |
|---|---:|---:|---:|
| `boot.bin` | `0x00000000` | `0x00000440` | 1088 |
| `bi2.bin` | `0x00000440` | `0x00002440` | 8192 |
| `apploader.img` | `0x00002440` | `0x00020298` | 122456 |
| padding | `0x00020298` | `0x00020300` | 104 |
| `main.dol` | `0x00020300` | `0x005A19E0` | 5773024 |
| padding | `0x005A19E0` | `0x005A1A00` | 32 |
| `fst.bin` | `0x005A1A00` | `0x005BA519` | 101145 |
| padding to user data | `0x005BA519` | `0x005C0000` | 23271 |
| first game file (`advertise.txd`) | `0x005C0000` | `0x006400B8` | 524472 |

### Note on header offsets 0x430..0x438

Older GameCube notes label these last three words ambiguously. For this extracted image, `0x430 = 0x803E74E0` is plainly a RAM address, while `0x434 = 0x005C0000` exactly matches the first FST user-file disc offset. The modern GCM layout documentation therefore matches the bytes: `0x430` = FST memory address, `0x434` = user position, `0x438` = user length.

The boot header itself says DOL=`0x00020300`, FST=`0x005A1A00`, FST size=`0x00018B19`, FST RAM address=`0x803E74E0`, first user-data position=`0x005C0000`.

## 2. Console boot chain vs. what the recompiler needs

```text
GameCube IPL/BS1/BS2
  -> read disc header (boot.bin)
  -> load BI2
  -> load apploader.img to 0x81200000
  -> call apploader Entry
       -> returns Init/Main/Close callbacks
  -> Init(report callback)
  -> repeatedly call Main(&dst,&len,&disc_offset)
       -> DVDRead(dst,len,disc_offset)
  -> Close()
       -> returns DOL entry point
  -> jump to main.dol entry 0x80003154
       -> CRT init
       -> game main / engine boot
```

For a native static recompiler the first six stages are **build-time work**. They should not survive as emulated boot code inside the Windows executable.

## 3. Exact supplied-disc identity — CONFIRMED

- Game ID: `GUPP8P`
- Maker: `8P`
- Revision: 0
- Audio streaming flag: 1
- GameCube magic: `0xC2339F3D`
- Title: `SHADOW THE HEDGEHOG`
- `bi2.bin` simulated memory size: `0x01800000` = 24 MiB
- BI2 country code: 2
- BI2 total disc count: 1
- BI2 long-filename support: 1

## 4. What must be preserved

The native executable does **not** need the original apploader algorithm, DVD-sector requests, GameCube IPL handoff, or the loader's 128 KiB request chunker. What must survive is the state those mechanisms establish before the game's own code observes it:

1. DOL text/data contents at their expected guest addresses (or a faithful translated-address abstraction).
2. Zero-initialized regions described by the startup zero table/BSS semantics.
3. Low-memory boot fields that are actually read by translated game/SDK code.
4. Correct SDA/SDA2 bases, stack/ABI assumptions where translated code still depends on them.
5. FST semantics for DVD path lookup if the game still uses GameCube `DVD*` APIs.
6. Region/game-ID behavior where the game branches on it.

Everything else is a candidate for deletion after proving no guest-visible behavior depends on it.

## 5. Modernized replacement

```text
Build tool
  parse boot.bin + bi2.bin
  parse apploader only as validation evidence
  parse DOL sections + CRT tables
  parse FST -> normalized asset manifest
  build PPC function/data graph
  resolve direct + conservative indirect roots
  translate reachable PPC code
  generate guest-memory/low-memory initialization table
  generate asset lookup table
  host-link + LTO + linker GC
        |
        v
single native EXE
  NativeEntry()
    -> initialize minimal compatibility state
    -> install translated globals/data
    -> call translated game entry/main path
```

The goal is not to reproduce the optical-disc loader faster; it is to **delete it while retaining its observable result**.
