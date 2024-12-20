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

#include "Graphics\GraphicsCommon.h"
#include "Graphics\DX12\Enums12.h"
#include "Graphics\DX12\Formats12.h"
#include "Graphics\DX12\Strings12.h"


namespace Luna::DX12
{

void SetDebugName(IDXGIObject* object, const std::string& name);
void SetDebugName(ID3D12Object* object, const std::string& name);


// DirectX 12 related log categories
inline LogCategory LogDirectX{ "LogDirectX" };

} // namespace Luna::DX12