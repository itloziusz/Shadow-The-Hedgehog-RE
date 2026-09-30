# GX / Gekko source map

This is a curated research index, not a redistribution of proprietary SDK material. Prefer public/open implementations and hardware documentation as working references. When an archival proprietary/confidential manual is consulted, use it only as a research cross-check and do not vendor it into the project.

## A. GX command stream / hardware

1. WiiBrew — Hardware/GX: https://www.wiibrew.org/wiki/Hardware/GX
   - WGPIPE `0xCC008000`; FIFO overview; BP/CP/XF command framing.
2. WiiBrew — GX Command Processor: https://www.wiibrew.org/wiki/Hardware/GX/Command_Processor
   - memory-mapped CP FIFO registers/status/control.
3. WiiBrew — GX Blitting Processor: https://www.wiibrew.org/wiki/Hardware/GX/Blitting_Processor
   - BP registers.
4. WiiBrew — GX Transform Unit: https://www.wiibrew.org/wiki/Hardware/GX/Transform_Unit
   - XF registers/memory.
5. YAGCD index: https://hitmen.c02.at/files/yagcd/yagcd/index.html
6. YAGCD chapter 5: https://hitmen.c02.at/files/yagcd/yagcd/chap5.html
   - hardware register map and GX FIFO/display-list packet description.
7. YAGCD chapter 8: https://hitmen.c02.at/files/yagcd/yagcd/chap8.html
   - 3D graphics processing and FIFO command examples.
8. YAGCD chapter 4: https://hitmen.c02.at/files/yagcd/yagcd/chap4.html
   - GameCube memory map.

## B. Public GX API / inline write semantics

9. libogc generated docs: https://libogc.devkitpro.org/files.html
10. libogc gx.h (public homebrew API): https://github.com/devkitPro/libogc/blob/master/gc/ogc/gx.h
11. Historical libogc gx.h with explicit inline FIFO helpers: https://github.com/comex/libogc/blob/master/gc/ogc/gx.h
12. devkitPro GameCube examples: https://github.com/devkitPro/gamecube-examples
13. devkitPro Wii GX examples: https://github.com/devkitPro/wii-examples

Key use: establish which `GX_Position*`, `GX_Normal*`, `GX_Color*`, `GX_TexCoord*`, and matrix-index helpers compile to which FIFO widths/order.

## C. Recovered Dolphin SDK library implementations

14. doldecomp/dolsdk2001: https://github.com/doldecomp/dolsdk2001
15. GXAttr.c: https://github.com/doldecomp/dolsdk2001/blob/main/src/gx/GXAttr.c
16. GXGeometry.c: https://github.com/doldecomp/dolsdk2001/blob/main/src/gx/GXGeometry.c
17. GXTev.c: https://github.com/doldecomp/dolsdk2001/blob/main/src/gx/GXTev.c
18. GXPixel.c: https://github.com/doldecomp/dolsdk2001/blob/main/src/gx/GXPixel.c
19. GXTransform.c: https://github.com/doldecomp/dolsdk2001/blob/main/src/gx/GXTransform.c
20. GXFifo.c: https://github.com/doldecomp/dolsdk2001/blob/main/src/gx/GXFifo.c
21. GXDisplayList.c: https://github.com/doldecomp/dolsdk2001/blob/main/src/gx/GXDisplayList.c
22. doldecomp/dolsdk2004: https://github.com/doldecomp/dolsdk2004
23. Melee decomp generated context (many SDK headers/inlines): https://doldecomp.github.io/melee/ctx.html

These are especially valuable for SDK-function fingerprint generation and for mapping API setters to CP/BP/XF writes and SDK shadow-state updates.

## D. Dolphin VideoCommon — operational GX reference

24. Dolphin source: https://github.com/dolphin-emu/dolphin
25. OpcodeDecoding.h: https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/OpcodeDecoding.h
26. OpcodeDecoding.cpp: https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/OpcodeDecoding.cpp
27. CPMemory.h: https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/CPMemory.h
28. CPMemory.cpp: https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/CPMemory.cpp
29. BPMemory.h: https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/BPMemory.h
30. XFMemory.h: https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/XFMemory.h
31. VertexLoaderManager.cpp: https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/VertexLoaderManager.cpp
32. VertexLoaderBase.cpp/.h: https://github.com/dolphin-emu/dolphin/tree/master/Source/Core/VideoCommon
33. VertexManagerBase.cpp: https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/VertexManagerBase.cpp
34. PixelShaderGen.cpp: https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/PixelShaderGen.cpp
35. VertexShaderGen.cpp: https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/VertexShaderGen.cpp
36. TextureDecoder.cpp/.h: https://github.com/dolphin-emu/dolphin/tree/master/Source/Core/VideoCommon
37. FIFO Player guide: https://dolphin-emu.org/docs/guides/

Use Dolphin as a behavior/reference implementation, while respecting its GPL license if code is copied/derived. Facts and independently reimplemented behavior can be used with appropriate project/legal review.

## E. Gekko CPU / ABI

38. NXP PowerPC EABI: https://www.nxp.com/docs/en/application-note/PPCEABI.pdf
39. Sourceware mirror of PowerPC EABI: https://sourceware.org/pub/binutils/ppc-docs/ppc-eabi-1995-01.pdf
40. NXP MPC750 family documentation landing page: https://www.nxp.com/products/MPC755
41. WiiBrew Paired single: https://www.wiibrew.org/wiki/Paired_single
42. IBM Gekko User Manual archival scan (provenance/confidentiality caution; do not vendor): search for `Gekko_User_Manual_200005.pdf` in archival hardware-document collections.
43. IBM/Motorola PowerPC Programming Environments Manual — use for base PowerPC instruction semantics.

Critical decompiler concerns: GPR/FPR calling convention, `r2` SDA2, `r13` SDA, LR/CTR, big-endian memory, paired singles, `psq_l/psq_st`, and GQR quantization.

## F. Decompilation / analysis tooling

44. decomp-toolkit (DTK): https://github.com/encounter/decomp-toolkit
45. Ghidra GameCube Loader: https://github.com/Cuyler36/Ghidra-GameCube-Loader
46. Gekko/Broadway Ghidra language (historical; integrated into loader): https://github.com/aldelaro5/ghidra-gekko-broadway-lang
47. m2c PowerPC decompiler: https://github.com/matt-kempster/m2c
48. DolRecomp: https://github.com/ExpansionPak/DolRecomp
49. Decomp Academy GameCube course: https://decomp-academy.dev/courses/gamecube-c/
50. decomp.me: https://decomp.me/

## G. Independent static-recomp / GX projects worth cross-checking

51. gcrecomp: https://github.com/sp00nznet/gcrecomp
52. Wind Waker static-recomp experiment: https://github.com/sp00nznet/ww
53. Ikaruga static-recomp experiment: https://github.com/sp00nznet/ikaruga
54. GXRuntime: https://github.com/aharonahdoot/GXRuntime
55. RingOut: https://github.com/jackpoison-prog/RingOut

Treat young projects as secondary references. Validate claims against hardware docs, libogc, recovered SDK code, and independent traces.

## H. TEV / texture research

56. Dolphin BPMemory/PixelShaderGen (above) — strongest open executable semantics source.
57. `tevsl` — TEV-oriented shader compiler/research: search GitHub for `tevsl GameCube Wii TEV`.
58. libogc texture conversion/examples and `gxtexconv`-style tools — useful for tiled texture formats and palettes.

## I. Archival Nintendo GX manual

An archival copy of Nintendo's “Graphics Library (GX)” documentation is visible on public mirrors and is very comprehensive (vertices/primitives, matrices/viewing, lighting, texgen, textures/TLUT/cache, TEV, indirect texturing, fog/Z/blending, EFB/XFB/video copy, FIFO, metrics, limitations). It is proprietary/confidential historical material. Do not redistribute it or copy large passages/code into this project; use public/open sources above as the implementation basis and the manual, where legally appropriate, only as a cross-check.

## J. Additional architecture / TEV cross-checks

59. `tevsl` shader compiler: https://github.com/crtc-demos/tevsl
   - High-value independent model of TEV stage composition and a readable C-oriented representation.
60. GameCube Architecture — Graphics (Rodrigo Copetti): https://www.copetti.org/writings/consoles/gamecube/
   - Architectural overview; useful orientation, not a register-level authority.
61. OldMachines — GameCube Graphics course: https://oldmachines.io/gamecube/graphics/
   - Modern educational walkthrough of Flipper, FIFO, vertex pipeline, textures and TEV; secondary reference.
62. Nintendo GameCube technical details (Nintendo regional history pages).
   - First-party high-level hardware capabilities/specifications; not enough for decoding but useful sanity checks.
63. 21C3 / GameCube Hacking presentation (2004), mirrored by CCC-related archives.
   - Historical hardware/reverse-engineering context.
64. Google Patents / Nintendo patent-family documents that describe the example graphics pipeline (command processor, transform, texture, TEV, pixel engine).
   - Useful for conceptual block diagrams and hardware intent; not a substitute for exact register semantics.

## Source priority for implementation

Use this order when sources disagree:

1. **Observed original FIFO/PPC traces from the target game** plus reproducible hardware behavior.
2. **Recovered Dolphin SDK implementation** for source-level SDK semantics/version fingerprints.
3. **Dolphin VideoCommon** for independently exercised packet/register behavior.
4. **YAGCD + WiiBrew** for public hardware/register documentation.
5. **libogc** for public API names, constants, inline FIFO helpers and real-world usage.
6. Secondary recompilers, tutorials, architecture articles, talks and patents.
7. Archival proprietary manuals only as a cross-check, never as the sole implementation source.
