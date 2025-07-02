# Luna
New rendering engine

## External Software
### D3D12MemoryAllocator
The D3D12MemoryAlloctor project on github is here: [D3D12MemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator)
To update the submodule to latest, make sure you're in the root Luna folder and run:
``git submodule update --remote --merge External/D3D12MemoryAllocator``

### KTX
The KTX project on github is here: [KTX](https://github.com/KhronosGroup/KTX-Software)
To update the KTX library in Luna, follow these steps.
- Download and install the latest KTX packaged release for Win64 from here: [KTX Releases](https://github.com/KhronosGroup/KTX-Software/releases)
- KTX should now be installed in a folder such as C:\Program Files\KTX-Software
- From the installation folder, copy the following to Luna\External\ktx
	- ktx.dll from the \bin folder
	- The entire \include folder
	- The entire \lib folder

### stb
The stb project on github is here: [stb](https://github.com/nothings/stb)
To update the submodule to latest, make sure you're in the root Luna folder and run:
``git submodule update --remote --merge External/stb``

### vk-bootstrap
The vk-bootstrap project on github is here: [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap)
To update the submodule to latest, make sure you're in the root Luna folder and run:
``git submodule update --remote --merge External/vk-bootstrap``

### volk
The volk project on github is here: [volk](https://github.com/zeux/volk)
To update the submodule to latest, make sure you're in the root Luna folder and run:
``git submodule update --remote --merge External/volk``

### VulkanMemoryAllocator
The VulkanMemoryAllocator project on github is here: [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
To update the submodule to latest, make sure you're in the root Luna folder and run:
``git submodule update --remote --merge External/VulkanMemoryAllocator``