# v1.4 target

1. GPU upload scheduler with frame byte budget and timeline/fence tracking.
2. DX12 upload-ring + copy queue + fences + texture creation.
3. Vulkan staging-ring + transfer/graphics queue ownership + timeline semaphore path.
4. NativeTexture/NativeMesh/NativeMaterial typed factories.
5. CPU/GPU residency split in ResourceManager.
6. Mip streaming implementation and sparse-resource capability routing.
7. LandALoadManager deeper DOL reconstruction -> real stage-cell policy.
8. TOneFileAsync<T> method-by-method ASM/C++ reconstruction.
9. Heap/IHeap vtable method naming from PPC call sites.
10. Cooked registry generation from the full Shadow asset tree.
