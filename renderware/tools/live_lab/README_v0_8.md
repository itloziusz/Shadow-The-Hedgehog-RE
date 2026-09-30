# Shadow RenderWare Lab v0.8 — In-Game Editor Runtime

This build adds a resident PowerPC editor command consumer to the GUPP8P DOL and a Windows keyboard/mouse bridge.

## Working in v0.8
- DOL-side transform application once per present via MEM1 mailbox `0x805FD000` (`SE08`).
- W/E/R editor modes while Dolphin is focused: move / rotate / scale.
- Left-click screen-space picking from currently scanned scene rows.
- X/Y/Z drag constraint.
- Transparent projected XYZ gizmo overlay over the Dolphin client.
- Right-click context menu over the game.
- Runtime object naming from RenderWare material/texture evidence, with confidence labels.
- Exact effect class names and land block/world names.
- Player-proximity mode with manual player reference and conservative auto-player selection.
- Existing world/atomic/effect show/hide, draw distance, streaming, FOV, RenderWare inspection.

## Spawn menu status
The right-click Spawn submenu exposes the reverse-engineered type catalog, but actual enemy/task factory calls remain disabled until the gameplay constructor/task ownership chain is proven. This is deliberate: copying an RpAtomic/RpClump is not a valid enemy spawn and would corrupt AI/animation/destruction ownership.

## Runtime hook
`RwGameCubeRasterShowRaster @ 0x80495CBC` branches to the appended PPC editor runtime at `0x805FC600`, then resumes at `0x80495CC8`. The appended mailbox data section is at `0x805FD000`. These addresses are immediately above the original DOL BSS end (`0x805FC5EC`) and below the observed OS arena start.
