# Resource Core v1.3 — original behavior to modern PC mapping

## Proven original pieces
- `ResourceManagerTask`: central task/service type.
- `ResourceOneFile`: fixed-stride archive resource table; entry array at +0x14, count at +0x18, 0x3C-byte entry stride, payload pointer at +0x38.
- `ResourceOneFileReq`: explicit request state machine.
- `TFileControlAsycManager`: background I/O with three original active slots and 32-byte-aligned read buffers.
- `TOneFileAsync<T>` specializations: typed post-I/O construction for textures, clumps, worlds, dictionaries and delta-morph animations.
- `Sonicteam::System::IHeap/Heap`: engine-owned allocator abstraction.
- `LandALoadManager`: level/world loading manager.
- explicit lifecycle tasks such as `EffectDeleteResourceTask`.

The accompanying `re/Original_Resource_Streaming_PPC_v13.s` contains direct PowerPC disassembly for the currently mapped regions.

## Modernization mapping
| Original | PC v1.3 |
|---|---|
| ONE entry pointer | `AssetId` / generation `ResourceHandle` |
| 3 fixed async read slots | priority work queue, configurable 1–32 workers |
| 32-byte DVD/read buffer alignment | allocator/RHI-specific alignment, no public ABI dependency |
| RenderWare typed object construction | cooker/native asset decoder |
| `IHeap` | `IAllocator`, `LinearArena`, `SlabPool`, later GPU upload arenas |
| `LandALoadManager` | `WorldStreamer` + cooked cell dependency lists |
| explicit delete task | reference count + cached state + budgeted eviction |
| console texture residency | backend-independent mip residency interface |

## Resource lifecycle
`Unloaded -> Requested -> Reading -> Decompressing -> Creating -> Uploading -> Ready -> Cached -> Evicted`

v1.3 implements the CPU path through Ready/Cached/Evicted. `Creating/Uploading` are reserved state points for native typed decoders and the v1.4 GPU upload scheduler.

## Backend contract
Gameplay, AI, animation and world code include no D3D12/Vulkan headers. Backend handles do not leak into serialized game state.

DX12 and Vulkan are compilation targets over the same native cooked formats. The Null backend is only a test backend.
