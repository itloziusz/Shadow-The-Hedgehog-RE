# Backend status v1.4

| Capability | NullRHI | Direct3D 12 source | Vulkan source |
|---|---:|---:|---:|
| Buffer create/destroy | tested | implemented | implemented |
| Texture create/destroy | tested | implemented | implemented |
| Buffer staged upload | tested | implemented | implemented |
| Texture mip staged upload | tested | implemented | implemented |
| Fence completion | tested | implemented | implemented |
| GPU byte residency accounting | tested | backend-independent | backend-independent |
| Sparse/virtual texture binding | no | future | future |
| Bindless descriptors | no | future | future |
| RenderGraph execution | no | future | future |

`implemented` for DX12/Vulkan means source implementation is present. It is not marked `tested` until compiled/run against the relevant SDK/runtime. The current container lacks both Windows D3D12 SDK and Vulkan development package.
