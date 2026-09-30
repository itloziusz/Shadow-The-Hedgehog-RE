# Windows build targets

## Core + Direct3D 12
```powershell
cmake -S . -B build-dx12 -G Ninja -DSHADOWPC_ENABLE_DX12=ON -DSHADOWPC_ENABLE_VULKAN=OFF
cmake --build build-dx12
ctest --test-dir build-dx12 --output-on-failure
```
Requires Windows SDK with D3D12/DXGI headers and libraries.

## Core + Vulkan
Install a Vulkan SDK and expose it to CMake, then:
```powershell
cmake -S . -B build-vk -G Ninja -DSHADOWPC_ENABLE_DX12=OFF -DSHADOWPC_ENABLE_VULKAN=ON
cmake --build build-vk
ctest --test-dir build-vk --output-on-failure
```

## Dual-backend build
```powershell
cmake -S . -B build-pc -G Ninja -DSHADOWPC_ENABLE_DX12=ON -DSHADOWPC_ENABLE_VULKAN=ON
cmake --build build-pc
```
Game code links against `shadowpc_core`; backend selection belongs to the platform/bootstrap layer.
