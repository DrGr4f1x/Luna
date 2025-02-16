//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Stdafx.h"

#include "Device12.h"

#include "FileSystem.h"

#include "Graphics\CommandContext.h"

#include "DescriptorAllocator12.h"
#include "DeviceCaps12.h"
#include "Formats12.h"
#include "Queue12.h"
#include "Shader12.h"

using namespace std;
using namespace Microsoft::WRL;


extern Luna::IGraphicsDevice* g_graphicsDevice;

static constexpr uint32_t c_MaxVolatileConstantBuffersPerLayout = 6;


namespace Luna::DX12
{

GraphicsDevice* g_d3d12GraphicsDevice{ nullptr };


void DebugMessageCallback(
	D3D12_MESSAGE_CATEGORY category,
	D3D12_MESSAGE_SEVERITY severity,
	D3D12_MESSAGE_ID id,
	LPCSTR pDescription,
	void* pContext)
{
	string debugMessage = format("[{}] Code {} : {}", category, (uint32_t)id, pDescription);

	switch (severity)
	{
	case D3D12_MESSAGE_SEVERITY_CORRUPTION:
		LogFatal(LogDirectX) << debugMessage << endl;
		break;
	case D3D12_MESSAGE_SEVERITY_ERROR:
		LogError(LogDirectX) << debugMessage << endl;
		break;
	case D3D12_MESSAGE_SEVERITY_WARNING:
		LogWarning(LogDirectX) << debugMessage << endl;
		break;
	case D3D12_MESSAGE_SEVERITY_INFO:
	case D3D12_MESSAGE_SEVERITY_MESSAGE:
		LogInfo(LogDirectX) << debugMessage << endl;
		break;
	}
}


D3D12_RESOURCE_FLAGS CombineResourceFlags(uint32_t fragmentCount)
{
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

	if (flags == D3D12_RESOURCE_FLAG_NONE && fragmentCount == 1)
	{
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | flags;
}


DescriptorType GetNormalizedDescriptorType(DescriptorType descriptorType)
{
	using enum DescriptorType;

	switch (descriptorType)
	{
	case StructuredBufferUAV:
	case RawBufferUAV:
		return TypedBufferUAV;
		break;

	case StructuredBufferSRV:
	case RawBufferSRV:
		return TypedBufferSRV;

	default:
		return descriptorType;
		break;
	}
}


bool AreDescriptorTypesCompatible(DescriptorType a, DescriptorType b)
{
	using enum DescriptorType;

	if (a == b)
	{
		return true;
	}

	a = GetNormalizedDescriptorType(a);
	b = GetNormalizedDescriptorType(b);

	if ((a == TypedBufferSRV && b == TextureSRV) ||
		(b == TypedBufferSRV && a == TextureSRV) ||
		(a == TypedBufferSRV && b == RayTracingAccelStruct) ||
		(a == TextureSRV && b == RayTracingAccelStruct) ||
		(b == TypedBufferSRV && a == RayTracingAccelStruct) ||
		(b == TextureSRV && a == RayTracingAccelStruct))
	{
		return true;
	}

	if ((a == TypedBufferUAV && b == TextureUAV) ||
		(b == TypedBufferUAV && a == TextureUAV))
	{
		return true;
	}

	return false;
}


DeviceRLDOHelper::~DeviceRLDOHelper()
{
	if (device && doReport)
	{
		wil::com_ptr<ID3D12DebugDevice> debugInterface;
		if (SUCCEEDED(device->QueryInterface(debugInterface.addressof())))
		{
			debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		}
	}
}


// TODO: Look at initialization in this constructor.  Can't we assign the dxDevice and d3d12Allocator in the initialization list?
GraphicsDevice::GraphicsDevice(const GraphicsDeviceDesc& desc) noexcept
	: m_desc{ desc }
	, m_deviceRLDOHelper{ desc.dx12Device, desc.enableValidation }
	, m_colorBufferPool{ m_desc.dx12Device, m_desc.d3d12maAllocator }
	, m_depthBufferPool{ m_desc.dx12Device, m_desc.d3d12maAllocator }
	, m_descriptorSetPool{ m_desc.dx12Device }
	, m_gpuBufferPool{ m_desc.dx12Device, m_desc.d3d12maAllocator }
	, m_pipelinePool{ m_desc.dx12Device }
	, m_rootSignaturePool{ m_desc.dx12Device }
{
	LogInfo(LogDirectX) << "Creating DirectX 12 device." << endl;

	m_dxgiFactory = m_desc.dxgiFactory;
	m_dxDevice = m_desc.dx12Device;
	m_d3d12maAllocator = m_desc.d3d12maAllocator;

	SetDebugName(m_dxDevice.get(), "DX12 Device");

	g_graphicsDevice = this;
	g_d3d12GraphicsDevice = this;
}


GraphicsDevice::~GraphicsDevice()
{
	LogInfo(LogDirectX) << "Destroying DirectX 12 device." << endl;

	if (m_dxInfoQueue)
	{
		m_dxInfoQueue->UnregisterMessageCallback(m_callbackCookie);
		m_dxInfoQueue.reset();
	}

	Shader::DestroyAll();
	g_userDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Destroy();
	g_userDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER].Destroy();

	g_d3d12GraphicsDevice = nullptr;
	g_graphicsDevice = nullptr;
}


void GraphicsDevice::CreateResources()
{
	if (m_desc.enableValidation)
	{
		InstallDebugCallback();
	}

	// Create descriptor allocators
	m_descriptorAllocators[0] = make_unique<DescriptorAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_descriptorAllocators[1] = make_unique<DescriptorAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	m_descriptorAllocators[2] = make_unique<DescriptorAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_descriptorAllocators[3] = make_unique<DescriptorAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	g_userDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Create("User Descriptor Heap, CBV_SRV_UAV");
	g_userDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER].Create("User Descriptor Heap, SAMPLER");

	m_caps = make_unique<DeviceCaps>();
	ReadCaps();
	m_caps->LogCaps();
}


void GraphicsDevice::InstallDebugCallback()
{
	if (SUCCEEDED(m_dxDevice->QueryInterface(IID_PPV_ARGS(&m_dxInfoQueue))))
	{
		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID denyIds[] =
		{
			// This occurs when there are uninitialized descriptors in a descriptor table, even when a
			// shader does not access the missing descriptors.  I find this is common when switching
			// shader permutations and not wanting to change much code to reorder resources.
			D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,

			// Triggered when a shader does not export all color components of a render target, such as
			// when only writing RGB to an R10G10B10A2 buffer, ignoring alpha.
			D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_PS_OUTPUT_RT_OUTPUT_MISMATCH,

			// This occurs when a descriptor table is unbound even when a shader does not access the missing
			// descriptors.  This is common with a root signature shared between disparate shaders that
			// don't all need the same types of resources.
			D3D12_MESSAGE_ID_COMMAND_LIST_DESCRIPTOR_TABLE_NOT_SET,

			D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES,

			// Silence complaints about shaders not being signed by DXIL.dll.  We don't care about this.
			D3D12_MESSAGE_ID_NON_RETAIL_SHADER_MODEL_WONT_VALIDATE,

			// RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS
			(D3D12_MESSAGE_ID)1008,
		};

		D3D12_INFO_QUEUE_FILTER newFilter = {};
		//newFilter.DenyList.NumCategories = _countof(Categories);
		//newFilter.DenyList.pCategoryList = Categories;
		newFilter.DenyList.NumSeverities = _countof(severities);
		newFilter.DenyList.pSeverityList = severities;
		newFilter.DenyList.NumIDs = _countof(denyIds);
		newFilter.DenyList.pIDList = denyIds;

#ifdef _DEBUG
		m_dxInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		m_dxInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
		m_dxInfoQueue->PushStorageFilter(&newFilter);
		m_dxInfoQueue->RegisterMessageCallback(DebugMessageCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &m_callbackCookie);
	}
}


void GraphicsDevice::ReadCaps()
{
	const D3D_FEATURE_LEVEL minFeatureLevel{ D3D_FEATURE_LEVEL_12_0 };
	const D3D_SHADER_MODEL maxShaderModel{ D3D_SHADER_MODEL_6_8 };

	m_caps->ReadFullCaps(m_dxDevice.get(), minFeatureLevel, maxShaderModel);

	// TODO
	//if (g_graphicsDeviceOptions.logDeviceFeatures)
	if (false)
	{
		m_caps->LogCaps();
	}
}


D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDevice::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
{
	return m_descriptorAllocators[type]->Allocate(m_dxDevice.get(), count);
}


wil::com_ptr<D3D12MA::Allocation> GraphicsDevice::CreateGpuBuffer(const GpuBufferDesc& desc, ResourceState& initialState)
{ 
	initialState = ResourceState::GenericRead;

	const UINT64 bufferSize = desc.elementSize * desc.elementCount;

	D3D12_HEAP_TYPE heapType = GetHeapType(desc.memoryAccess);
	D3D12_RESOURCE_FLAGS flags = (desc.bAllowUnorderedAccess || IsUnorderedAccessType(desc.resourceType)) 
		? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS 
		: D3D12_RESOURCE_FLAG_NONE;


	D3D12_RESOURCE_DESC resourceDesc{
		.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment			= 0,
		.Width				= bufferSize,
		.Height				= 1,
		.DepthOrArraySize	= 1,
		.MipLevels			= 1,
		.Format				= FormatToDxgi(desc.format).resourceFormat,
		.SampleDesc			= { .Count = 1, .Quality = 0 },
		.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags				= flags
	};
	
	auto allocationDesc = D3D12MA::ALLOCATION_DESC{
		.HeapType = heapType
	};

	wil::com_ptr<D3D12MA::Allocation> allocation;
	HRESULT hr = m_d3d12maAllocator->CreateResource(
		&allocationDesc,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		NULL,
		&allocation,
		IID_NULL, NULL);

	return allocation;
}


wil::com_ptr<D3D12MA::Allocation> GraphicsDevice::CreateStagingBuffer(const void* initialData, size_t numBytes) const
{
	// Create an upload buffer
	auto resourceDesc = D3D12_RESOURCE_DESC{
		.Dimension	= D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment			= 0,
		.Width				= numBytes,
		.Height				= 1,
		.DepthOrArraySize	= 1,
		.MipLevels			= 1,
		.Format				= DXGI_FORMAT_UNKNOWN,
		.SampleDesc			= {.Count = 1, .Quality = 0 },
		.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags				= D3D12_RESOURCE_FLAG_NONE
	};

	auto allocationDesc = D3D12MA::ALLOCATION_DESC{	.HeapType = D3D12_HEAP_TYPE_UPLOAD };

	wil::com_ptr<D3D12MA::Allocation> allocation;
	HRESULT hr = GetD3D12GraphicsDevice()->GetAllocator()->CreateResource(
		&allocationDesc,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		&allocation,
		IID_NULL, nullptr);

	SetDebugName(allocation->GetResource(), "Staging Buffer");

	auto resource = allocation->GetResource();
	void* mappedPtr{ nullptr };
	assert_succeeded(resource->Map(0, nullptr, &mappedPtr));

	memcpy(mappedPtr, initialData, numBytes);

	resource->Unmap(0, nullptr);

	return allocation;
}


uint8_t GraphicsDevice::GetFormatPlaneCount(DXGI_FORMAT format)
{
	uint8_t& planeCount = m_dxgiFormatPlaneCounts[format];
	if (planeCount == 0)
	{
		D3D12_FEATURE_DATA_FORMAT_INFO formatInfo{ format, 1 };
		if (FAILED(m_dxDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &formatInfo, sizeof(formatInfo))))
		{
			// Format not supported, store a special value in the cache to avoid querying later
			planeCount = 255;
		}
		else
		{
			// Format supported - store the plane count in the cache
			planeCount = formatInfo.PlaneCount;
		}
	}

	if (planeCount == 255)
	{
		return 0;
	}

	return planeCount;
}


GraphicsDevice* GetD3D12GraphicsDevice()
{
	return g_d3d12GraphicsDevice;
}

} // namespace Luna::DX12