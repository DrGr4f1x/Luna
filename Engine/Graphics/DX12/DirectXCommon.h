//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#pragma once

#define LUNA_API_DX12 1

#define FORCE_D3D12_VALIDATION 0
#define ENABLE_D3D12_VALIDATION (ENABLE_VALIDATION || FORCE_D3D12_VALIDATION)

#define FORCE_D3D12_DEBUG_MARKERS 0
#define ENABLE_D3D12_DEBUG_MARKERS (ENABLE_DEBUG_MARKERS || FORCE_D3D12_DEBUG_MARKERS)

#define USE_AGILITY_SDK 1

// DirectX 12 headers
#include <d3d12.h>
#if USE_AGILITY_SDK
#include <d3dx12\d3dx12.h>
#endif
#include <dxgi1_4.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <dxgiformat.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

#define D3D12MA_D3D12_HEADERS_ALREADY_INCLUDED 1
#include "D3D12MemAlloc.h"

#include "Graphics\GraphicsCommon.h"
#include "Graphics\DX12\Enums12.h"
#include "Graphics\DX12\Formats12.h"
#include "Graphics\DX12\Strings12.h"


namespace Luna::DX12
{

// Forward declarations
class GraphicsDevice;


void SetDebugName(IDXGIObject* object, const std::string& name);
void SetDebugName(ID3D12Object* object, const std::string& name);

D3D12_RESOURCE_FLAGS CombineResourceFlags(uint32_t fragmentCount);
wil::com_ptr<D3D12MA::Allocation> CreateStagingBuffer(D3D12MA::Allocator* allocator, const void* initialData, size_t numBytes);
D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count = 1);
uint8_t GetFormatPlaneCount(DXGI_FORMAT format);
uint32_t GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type);

// DirectX 12 related log categories
inline LogCategory LogDirectX{ "LogDirectX" };

} // namespace Luna::DX12