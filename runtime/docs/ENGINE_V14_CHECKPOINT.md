# ShadowPC Engine v1.4 — Resource/GPU/Streaming checkpoint

## Goal
64-bit native PC engine preserving reconstructed Shadow behavior while replacing GameCube/RenderWare platform constraints with a backend-independent modern runtime. Direct3D 12 and Vulkan consume the same cooked native assets through `IGraphicsBackend`.

## Implemented in v1.4

### CPU memory
- `VirtualArena`: reserve-large / commit-on-demand virtual address arena.
- Windows path: `VirtualAlloc(MEM_RESERVE/MEM_COMMIT)`.
- POSIX path: `mmap(PROT_NONE)` + incremental `mprotect` commit.
- Existing `SystemAllocator`, `LinearArena`, `SlabPool<T>` retained.

### Native asset layer
- `NativeTexture`, `NativeMesh`, `NativeMaterial`, `NativeSkeleton`, `NativeAnimation`, `NativeWorldChunk`.
- `AssetDecoderRegistry` maps `AssetType` to a typed offline/cooked decoder.
- RenderWare types are deliberately absent from the runtime native asset API.

### CPU -> GPU pipeline
`AssetPipeline` joins:
1. `AssetRegistry`
2. dependency load plan
3. `ResourceManager` async I/O/decompression/cache
4. typed decoder
5. `UploadScheduler`
6. `GpuResidencyManager`

A root model request therefore gives its dependencies independent CPU and GPU lifecycles. Release walks the same dependency graph and drops GPU residency when the final pipeline reference goes away.

### GPU upload/residency
- `UploadScheduler` handles mesh vertex/index buffers and texture mip uploads.
- fence-backed completion state: Queued -> Submitted -> Ready/Failed.
- `GpuResidencyManager` accounts bytes, touches LRU timestamps and evicts oldest assets to a target VRAM budget.
- NullRHI validates this deterministically on the host.

### Streaming priority
`StreamingPriorityEngine` scores:
- distance / object radius,
- camera forward direction,
- player velocity toward asset,
- current visibility flag,
- mission-critical override.

`WorldStreamer` uses predictive preload radius based on player speed and keeps mission-critical cells resident.

### DX12 backend source
v1.4 source now contains:
- hardware adapter + D3D12 device,
- graphics and copy queues,
- device/upload/readback buffers,
- 2D/3D texture creation,
- committed upload staging resources,
- buffer copies,
- mip texture copies with copyable footprints,
- COPY_DEST -> shader-resource subresource transition,
- copy-queue fence values and pending upload lifetime tracking.

### Vulkan backend source
v1.4 source now contains:
- Vulkan 1.3 instance/device/graphics queue,
- buffer + device memory allocation,
- image + device memory allocation,
- host-visible staging buffers,
- buffer copies,
- per-mip image layout transitions and buffer-to-image copies,
- VkFence-backed upload completion and pending staging lifetime tracking.

The current Linux container does not contain the Vulkan development headers/library package, and does not have Windows/D3D12 SDK headers, so the two native backend translation units are not claimed as host-validated here. Core/RHI-independent code is host-validated with GCC and Clang.

## Original-engine reverse engineering added

### TOneFileAsync<T>
Specializations confirmed:
- `RpDMorphAnimation` state machine `0x8004AE60`
- `RpWorld` `0x8004B3A0`
- `RtDict` `0x8004B7BC`
- `RpClump` `0x8004BBD4`
- `RwTexDictionary` `0x8004BFF0`
- `void` `0x8004C408`

All share state at `this+0x28`, resource index at `+0x34`, 32-byte aligned work buffer at `+0x38`, typed result at `+0x48`, pending byte at `+0x50`, and a support object at `+0x54`. Focused original PPC and named C++ reconstruction are in `re/`.

### LandALoadManager
Embedded paths and state machine are now tied to concrete code:
- `%s/%s_TEX%02d.one` used at `0x80046874`
- `%s/%s_%02d.one` used in constructor at `0x80046B28`
- `stg%04d/stg%04d_light.bin` used at `0x8004874C`
- update state machine `0x800466B4` dispatches 13 states (0..12)
- `this+0xB4` is the state field
- `this+0xB8` is an iterated resource index [M]
- `this+0xBC` stores a parsed two-decimal-digit resource suffix [M]
- 15 bytes starting at `this+0xC4` are explicitly cleared/scanned child-pending flags [H]

`[H]` means directly evidenced. `[M]` means semantic name inferred from use but not yet proven by a symbol.

## Host validation
- GCC 14.2: build PASS, 0 warnings, CTest 1/1 PASS.
- Clang 17: build PASS, 0 warnings, CTest 1/1 PASS.
- Tests cover stable IDs, generation handles, stale handle rejection, slab/linear/virtual allocators, PRS, dependency ordering, registry round-trip, threaded I/O, cache eviction, priority scoring, predictive world streaming, NullRHI uploads, GPU residency, and dependency-aware CPU->GPU asset pipeline.

## Next deepening
- read actual SME2/STEX/SMAT/SSKL/SANM headers directly into typed decoders;
- connect renderer draw packets to `NativeMaterial` feature flags;
- descriptor/bindless tables for DX12 and Vulkan;
- dedicated copy/transfer queue selection in Vulkan;
- persistent staging ring instead of one staging allocation per upload;
- per-mip texture residency and sparse-resource path;
- continue exact `LandALoadManager` state naming and identify helpers `0x8004C878`, `0x8004B318`, `0x8004CC94`, `0x8004CF90`;
- reconstruct original heap/pool implementation and ResourceManagerTask tick semantics from PPC.
