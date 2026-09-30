# Shadow the Hedgehog — Resource / Streaming RE v1.2

Confidence: [H] direct DOL/RTTI/instruction evidence, [M] semantic reconstruction, [PC] modern target design.

## Target
64-bit native PC engine with a platform-independent core and two graphics backends: Direct3D 12 and Vulkan. Original gameplay/resource semantics are reconstructed from the DOL; RenderWare/GameCube/Dolphin-style runtime dependencies are not carried into the final runtime.

## Directly identified original engine types

- [H] `ResourceManagerTask` — RTTI string `0x804AB638`, descriptor refs around `0x805E45A8/0x805E45B4`, virtual functions include `0x80012360`, `0x800122A0`, `0x80012890`.
- [H] `ResourceOneFile` — RTTI string `0x804AB690`, vtable region `0x8051D7D0`, methods include `0x800145A4..0x80014BE8`.
- [H] `ResourceOneFileReq` — RTTI string `0x804AF9F8`, vtable region `0x80520B50`, methods include `0x8006A050..0x8006B300`.
- [H] `TFileControlAsycManager` — RTTI string `0x804AC8D0`, vtable/descriptor region `0x8051E2E4`; file-control methods around `0x800452C8..0x80045FC4`.
- [H] `TBGReadManager` — RTTI string `0x804AC8E8`.
- [H] `SonicteamUSA::System::Thread` — RTTI string `0x804AC920`.
- [H] `Sonicteam::System::IHeap` — RTTI string `0x804AC940` and another engine copy near `0x8051B3E0`.
- [H] `Sonicteam::System::Heap` — RTTI string `0x8051B3C8`, vtable region `0x8056AF20`, functions around `0x8040BC90..0x8040BFA8`.
- [H] `LandALoadManager` — RTTI string `0x804ACAE0`.
- [H] `TOneFileAsync<void>` — `0x804ACCC0`.
- [H] `TOneFileAsync<RwTexDictionary>` — `0x804ACCD4`.
- [H] `TOneFileAsync<RpClump>` — `0x804ACCF4`.
- [H] `TOneFileAsync<RtDict>` — `0x804ACD0C`.
- [H] `TOneFileAsync<RpWorld>` — `0x804ACD24`.
- [H] `TOneFileAsync<RpDMorphAnimation>` — `0x804ACD3C`.
- [H] `TOneFileAsyncManager` — `0x804ACD70`.
- [H] `EffectDeleteResourceTask` and `TextureUVAnimManagerTask` also exist as explicit resource-lifecycle tasks.

## ResourceOneFile structural findings

`ResourceOneFile` uses a fixed-stride entry table.

- [H] `this + 0x14` is a pointer to an entry array in multiple accessors.
- [H] `this + 0x18` is the number of entries.
- [H] entry stride is `0x3C` (60 bytes), proven by `mulli ..., 0x3C` at `0x800145C4` and `0x80014660`.
- [H] `0x800145A4` scans the entry table backwards, compares the caller key/name against the entry start and returns the pointer stored at entry offset `+0x38` when it matches.
- [M] This is effectively a name -> payload/resource lookup table inside a loaded ONE archive.

Equivalent readable intent:

```cpp
void* ResourceOneFile::FindResourceByName(std::string_view resource_name) const {
    for (int i = entry_count - 1; i >= 0; --i) {
        const OneResourceEntry& entry = entries[i]; // stride 0x3C
        if (StringCompare(entry.name, resource_name) == 0)
            return entry.resource_payload;          // original +0x38
    }
    return nullptr;
}
```

## ResourceOneFileReq state machine

The request object contains an explicit integer state at offset `+0x04` and async-owned objects around `+0x20/+0x24`.

At `0x800149F4` the code dispatches on state values and allocates/frees async read objects. Observed states include 0,1,2,3,4. [M] Current names:

- 0 `IdleOrBegin`
- 1 `AsyncSubmitted`
- 2 `AsyncCompletionPending`
- 3 `Ready`
- 4 `Failed`

Names remain [M] until every transition/callback is traced.

## Async I/O scheduler findings

`TFileControlAsycManager` is a real background I/O subsystem, not a synchronous DVD read wrapper.

At `0x800453E0`:

- [H] scans **3 global worker/request slots** at `0x805742A0`.
- [H] if all 3 are occupied, the manager changes its submission/active state rather than starting another worker.
- [H] request data size is rounded up to a **32-byte boundary** before allocation (`+31`, then mask low 5 bits at `0x800454A4..0x800454AC`).
- [H] the aligned buffer is allocated through an engine allocator call at `0x80049BB8`.
- [H] a `0x390` (912-byte) worker/control object is constructed for active background work.
- [H] the class is related through RTTI to `TBGReadManager`, `Thread`, and `IHeap`.

This establishes an original pipeline equivalent to:

`request -> background-read slot -> aligned buffer -> worker/thread -> completion -> resource callback`.

## Original typed ONE loading

The presence of separate template instantiations proves the engine had typed conversion after archive/file I/O:

`ONE bytes -> typed decoder/constructor -> RenderWare object`

Known specializations include texture dictionary, clump/model, generic dictionary, world and delta-morph animation.

For the PC version these become native types:

- `TOneFileAsync<RwTexDictionary>` -> `AsyncAssetLoad<NativeTextureDictionary>`
- `TOneFileAsync<RpClump>` -> `AsyncAssetLoad<NativeModel>`
- `TOneFileAsync<RtDict>` -> `AsyncAssetLoad<NativeDictionary>`
- `TOneFileAsync<RpWorld>` -> `AsyncAssetLoad<NativeWorldChunk>`
- `TOneFileAsync<RpDMorphAnimation>` -> `AsyncAssetLoad<NativeMorphAnimation>`

The final PC runtime must not instantiate RenderWare objects.

## Memory system

`Sonicteam::System::Heap` and `IHeap` are explicit engine interfaces. `Heap` has a virtual table at `0x8056AF20` with allocator-related functions in the `0x8040BC90..0x8040BFA8` range.

[PC] We preserve subsystem-owned allocators but replace console ABI allocation with:

- `VirtualArena` for large reserved address spaces;
- `LinearArena` for per-frame/transient data;
- `SlabPool<T>` for fixed-size request/command objects;
- `UploadRing` for GPU staging;
- ordinary 64-bit virtual addresses internally;
- no public raw pointer identity for movable resource objects.

## Modern 64-bit resource identity

The PC runtime uses a 64-bit generation handle:

```text
bits  0..31 : slot index
bits 32..55 : generation
bits 56..63 : resource type
```

The game can keep a handle while the resource manager evicts, recreates or moves the underlying allocation. Debug builds additionally retain a stable 64-bit AssetId derived from canonical source/cooked paths.

## Dependency tracker

[H] Original ONE archives already group models, TXDs, BON/MTN/UVA and other resources; typed async loaders prove explicit type relationships.

[PC] Cooker writes a dependency graph into the cooked asset registry. Runtime loads dependencies by AssetId/ResourceHandle and never reparses ONE dependency relationships during gameplay.

## Cache / lifecycle

[H] The original engine contains explicit deletion/lifecycle tasks (`EffectDeleteResourceTask`) and shared ownership machinery elsewhere in the binary (`boost::detail::sp_counted_base*`).

[PC] Runtime lifecycle:

`Unloaded -> Requested -> IO -> Decompress -> Decode/Create -> GPUUpload -> Ready -> Cached -> Evicted`

Handles remain generation-validated throughout. CPU and GPU residency are tracked separately.

## Streaming modernization

Original semantics recovered so far:

- explicit background read manager;
- multiple concurrent slots;
- aligned read buffers;
- ONE archive async loading;
- level-specific `LandALoadManager`;
- resource-specific async conversion;
- explicit cleanup tasks.

Modern target:

- scalable priority queue rather than 3 fixed workers;
- overlapped/IOCP on Windows and native async/pread/io_uring-compatible path where appropriate;
- worker/job system for decode;
- optional DirectStorage path on DX12 without making it a gameplay dependency;
- Vulkan path uses the same CPU I/O/decompression scheduler and Vulkan transfer queues;
- upload budgets are frame-limited to avoid stutter;
- world cells and asset residency are backend independent.

## Graphics backend boundary

Core systems never include D3D12 or Vulkan headers.

```text
Gameplay / World / Animation
          |
ResourceSystem / Streaming / Cooked Assets
          |
RenderGraph + RHI
       /       \
   DX12RHI   VulkanRHI
```

Both backends consume the same `NativeMesh`, `NativeTexture`, `NativeMaterial`, `NativeSkeleton`, `NativeAnimation`, `NativeWorldChunk` data.

## Explicit non-goals

- no GameCube memory map in the final runtime;
- no emulation/interpreter dependency;
- no RenderWare runtime dependency;
- no 32-bit pointer serialization;
- no copying the original three-request limit;
- no backend-specific resource handles leaking into gameplay code.
