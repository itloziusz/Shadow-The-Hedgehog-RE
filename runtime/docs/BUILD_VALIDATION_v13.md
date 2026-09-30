# ShadowPC Engine v1.3 — build validation

Date: 2026-09-18

## Host validation
- GCC: g++ (Debian 14.2.0-19) 14.2.0
- Clang: clang version 17.0.0 (https://github.com/swiftlang/llvm-project.git 10999b6d034fe318f3d56c83bddb6572593a8bb0)
- CMake: cmake version 3.31.6
- Configuration tested: 64-bit native core + Null RHI; DX12/Vulkan platform SDKs disabled on this Linux validation host.

## GCC result
`cmake --build build`: PASS
`ctest --test-dir build`: **1/1 PASS**
Warnings: **0**

## Clang result
`cmake --build build-clang`: PASS
`ctest --test-dir build-clang`: **1/1 PASS**
Warnings: **0**

## Runtime tests covered
- canonical path -> stable AssetId;
- 64-bit generation handle packing;
- stale handle invalidation after eviction/reuse;
- LinearArena alignment;
- SlabPool lifecycle;
- PRS literal-stream decode;
- AssetRegistry dependency graph;
- binary AssetRegistry save/load round-trip;
- priority threaded async file I/O;
- dependency-aware ResourceManager readiness;
- reference release -> cache transition;
- CPU cache eviction;
- Null RHI buffer lifecycle.

## Backend status
- DX12: device/factory/graphics+copy queue bootstrap and committed-buffer creation are implemented behind the Windows build option. GPU upload/fence and texture allocation remain explicit v1.4 work; no false parity claim.
- Vulkan: Vulkan 1.3 instance/device/graphics queue bootstrap is implemented behind the Vulkan SDK build option. GPU memory allocation/image/upload path remains explicit v1.4 work.
