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

#include "ColorBuffer12.h"
#include "DepthBuffer12.h"
#include "DescriptorAllocator12.h"
#include "DeviceCaps12.h"
#include "Formats12.h"
#include "GpuBuffer12.h"
#include "Queue12.h"

using namespace std;
using namespace Microsoft::WRL;


extern Luna::IGraphicsDevice* g_graphicsDevice;


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


GraphicsDevice::GraphicsDevice(const GraphicsDeviceDesc& desc) noexcept
	: m_desc{ desc }
	, m_deviceRLDOHelper{ desc.dx12Device, desc.enableValidation }
{
	LogInfo(LogDirectX) << "Creating DirectX 12 device." << endl;

	m_dxgiFactory = m_desc.dxgiFactory;
	m_dxDevice = m_desc.dx12Device;

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

	g_d3d12GraphicsDevice = nullptr;
	g_graphicsDevice = nullptr;
}


wil::com_ptr<IPlatformData> GraphicsDevice::CreateColorBufferData(ColorBufferDesc& desc, ResourceState& initialState)
{
	// Create resource
	auto numMips = desc.numMips == 0 ? ComputeNumMips(desc.width, desc.height) : desc.numMips;
	auto flags = CombineResourceFlags(desc.numSamples);

	D3D12_RESOURCE_DESC resourceDesc{
		.Dimension = GetResourceDimension(desc.resourceType),
		.Alignment = 0,
		.Width = (UINT64)desc.width,
		.Height = (UINT)desc.height,
		.DepthOrArraySize = (UINT16)desc.arraySizeOrDepth,
		.MipLevels = (UINT16)numMips,
		.Format = FormatToDxgi(desc.format).resourceFormat,
		.SampleDesc = {.Count = desc.numSamples, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags = (D3D12_RESOURCE_FLAGS)flags
	};

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = FormatToDxgi(desc.format).rtvFormat;
	clearValue.Color[0] = desc.clearColor.R();
	clearValue.Color[1] = desc.clearColor.G();
	clearValue.Color[2] = desc.clearColor.B();
	clearValue.Color[3] = desc.clearColor.A();

	ID3D12Resource* resource{ nullptr };
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	assert_succeeded(m_dxDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&resource)));

	SetDebugName(resource, desc.name);

	// Create descriptors and derived views
	assert_msg(desc.arraySizeOrDepth == 1 || numMips == 1, "We don't support auto-mips on texture arrays");

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};

	DXGI_FORMAT dxgiFormat = FormatToDxgi(desc.format).resourceFormat;
	rtvDesc.Format = dxgiFormat;
	uavDesc.Format = GetUAVFormat(dxgiFormat);
	srvDesc.Format = dxgiFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (desc.arraySizeOrDepth > 1)
	{
		if (desc.resourceType == ResourceType::Texture3D)
		{
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
			rtvDesc.Texture3D.MipSlice = 0;
			rtvDesc.Texture3D.FirstWSlice = 0;
			rtvDesc.Texture3D.WSize = (UINT)desc.arraySizeOrDepth;

			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			uavDesc.Texture3D.MipSlice = 0;
			uavDesc.Texture3D.FirstWSlice = 0;
			uavDesc.Texture3D.WSize = (UINT)desc.arraySizeOrDepth;

			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MipLevels = numMips;
			srvDesc.Texture3D.MostDetailedMip = 0;
		}
		else
		{
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Texture2DArray.MipSlice = 0;
			rtvDesc.Texture2DArray.FirstArraySlice = 0;
			rtvDesc.Texture2DArray.ArraySize = (UINT)desc.arraySizeOrDepth;

			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			uavDesc.Texture2DArray.MipSlice = 0;
			uavDesc.Texture2DArray.FirstArraySlice = 0;
			uavDesc.Texture2DArray.ArraySize = (UINT)desc.arraySizeOrDepth;

			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.MipLevels = numMips;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.ArraySize = (UINT)desc.arraySizeOrDepth;
		}
	}
	else if (desc.numFragments > 1)
	{
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}
	else
	{
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;

		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = numMips;
		srvDesc.Texture2D.MostDetailedMip = 0;
	}

	auto rtvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Create the render target view
	m_dxDevice->CreateRenderTargetView(resource, &rtvDesc, rtvHandle);

	// Create the shader resource view
	m_dxDevice->CreateShaderResourceView(resource, &srvDesc, srvHandle);

	// Create the UAVs for each mip level (RWTexture2D)
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 12> uavHandles;
	if (desc.numFragments == 1)
	{
		for (uint32_t i = 0; i < (uint32_t)uavHandles.size(); ++i)
		{
			uavHandles[i].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}

		for (uint32_t i = 0; i < numMips; ++i)
		{
			uavHandles[i] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			m_dxDevice->CreateUnorderedAccessView(resource, nullptr, &uavDesc, uavHandles[i]);

			uavDesc.Texture2D.MipSlice++;
		}
	}

	const uint8_t planeCount = GetFormatPlaneCount(FormatToDxgi(desc.format).resourceFormat);
	desc.planeCount = planeCount;
	initialState = ResourceState::Common;

	auto descExt = ColorBufferDescExt{}
		.SetResource(resource)
		.SetUsageState(ResourceState::Common)
		.SetPlaneCount(planeCount)
		.SetRtvHandle(rtvHandle)
		.SetSrvHandle(srvHandle)
		.SetUavHandles(uavHandles);

	auto colorBufferData = Make<ColorBufferData>(descExt);
	return colorBufferData;
}

wil::com_ptr<IPlatformData> GraphicsDevice::CreateDepthBufferData(DepthBufferDesc& desc, ResourceState& initialState)
{
	// Create resource
	D3D12_RESOURCE_DESC resourceDesc{
		.Dimension			= GetResourceDimension(desc.resourceType),
		.Alignment			= 0,
		.Width				= (UINT64)desc.width,
		.Height				= (UINT)desc.height,
		.DepthOrArraySize	= (UINT16)desc.arraySizeOrDepth,
		.MipLevels			= (UINT16)desc.numMips,
		.Format				= FormatToDxgi(desc.format).resourceFormat,
		.SampleDesc			= {.Count = desc.numSamples, .Quality = 0 },
		.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags				= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	};

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = FormatToDxgi(desc.format).rtvFormat;
	clearValue.DepthStencil.Depth = desc.clearDepth;
	clearValue.DepthStencil.Stencil = desc.clearStencil;

	ID3D12Resource* resource{ nullptr };
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	assert_succeeded(m_dxDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&resource)));

	SetDebugName(resource, desc.name);

	// Create descriptors and derived views
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = GetDSVFormat(FormatToDxgi(desc.format).resourceFormat);

	if (resource->GetDesc().SampleDesc.Count == 1)
	{
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
	}
	else
	{
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	}

	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 4> dsvHandles{};
	for (auto& handle : dsvHandles)
	{
		handle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	dsvHandles[0] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	dsvHandles[1] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	m_dxDevice->CreateDepthStencilView(resource, &dsvDesc, dsvHandles[0]);

	dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	m_dxDevice->CreateDepthStencilView(resource, &dsvDesc, dsvHandles[1]);

	auto stencilReadFormat = GetStencilFormat(FormatToDxgi(desc.format).resourceFormat);
	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		dsvHandles[2] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		dsvHandles[3] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		m_dxDevice->CreateDepthStencilView(resource, &dsvDesc, dsvHandles[2]);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		m_dxDevice->CreateDepthStencilView(resource, &dsvDesc, dsvHandles[3]);
	}
	else
	{
		dsvHandles[2] = dsvHandles[0];
		dsvHandles[3] = dsvHandles[1];
	}


	auto depthSrvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE stencilSrvHandle{};
	stencilSrvHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;

	// Create the shader resource view
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = GetDepthFormat(FormatToDxgi(desc.format).resourceFormat);
	if (dsvDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D)
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
	}
	else
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	m_dxDevice->CreateShaderResourceView(resource, &srvDesc, depthSrvHandle);

	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		stencilSrvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		srvDesc.Format = stencilReadFormat;
		srvDesc.Texture2D.PlaneSlice = (srvDesc.Format == DXGI_FORMAT_X32_TYPELESS_G8X24_UINT) ? 1 : 0;
		m_dxDevice->CreateShaderResourceView(resource, &srvDesc, stencilSrvHandle);
	}

	const uint8_t planeCount = GetFormatPlaneCount(FormatToDxgi(desc.format).resourceFormat);

	initialState = ResourceState::Common;

	auto descExt = DepthBufferDescExt{}
		.SetResource(resource)
		.SetUsageState(ResourceState::DepthRead | ResourceState::DepthWrite)
		.SetPlaneCount(planeCount)
		.SetDsvHandles(dsvHandles)
		.SetDepthSrvHandle(depthSrvHandle)
		.SetStencilSrvHandle(stencilSrvHandle);

	return Make<DepthBufferData>(descExt);
}


wil::com_ptr<IPlatformData> GraphicsDevice::CreateGpuBufferData(GpuBufferDesc& desc, ResourceState& initialState)
{
	assert(desc.elementSize == 2 || desc.elementSize == 4);

	D3D12MA::Allocation* pAllocation = CreateGpuBuffer(desc, initialState);
	ID3D12Resource* pResource = pAllocation->GetResource();

	uint64_t gpuAddress = pResource->GetGPUVirtualAddress();

	GpuBufferDescExt descExt{
		.resource		= pResource,
		.allocation		= pAllocation,
		.gpuAddress		= gpuAddress
	};

	const size_t bufferSize = desc.elementCount * desc.elementSize;

	if (desc.resourceType == ResourceType::ByteAddressBuffer || desc.resourceType == ResourceType::IndirectArgsBuffer)
	{
		auto srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{
			.Format						= DXGI_FORMAT_R32_TYPELESS,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer	= {
				.NumElements	= (uint32_t)bufferSize / 4,
				.Flags			= D3D12_BUFFER_SRV_FLAG_RAW
			}
		};

		auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		auto uavDesc = D3D12_UNORDERED_ACCESS_VIEW_DESC{
			.Format			= DXGI_FORMAT_R32_TYPELESS,
			.ViewDimension	= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer	= {
				.NumElements	= (uint32_t)bufferSize / 4,
				.Flags			= D3D12_BUFFER_UAV_FLAG_RAW
			}
		};
		
		auto uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);

		descExt.SetSrvHandle(srvHandle);
		descExt.SetUavHandle(uavHandle);
	}

	if (desc.resourceType == ResourceType::StructuredBuffer)
	{
		auto srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{
			.Format						= DXGI_FORMAT_UNKNOWN,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer	= {
				.NumElements			= (uint32_t)desc.elementCount,
				.StructureByteStride	= (uint32_t)desc.elementSize,
				.Flags					= D3D12_BUFFER_SRV_FLAG_NONE
			}
		};

		auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		auto uavDesc = D3D12_UNORDERED_ACCESS_VIEW_DESC{
			.Format			= DXGI_FORMAT_UNKNOWN,
			.ViewDimension	= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements			= (uint32_t)desc.elementCount,
				.StructureByteStride	= (uint32_t)desc.elementSize,
				.CounterOffsetInBytes	= 0,
				.Flags					= D3D12_BUFFER_UAV_FLAG_NONE
			}
		};

		auto uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);

		descExt.SetSrvHandle(srvHandle);
		descExt.SetUavHandle(uavHandle);
	}

	if (desc.resourceType == ResourceType::TypedBuffer)
	{
		auto srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{
			.Format						= FormatToDxgi(desc.format).resourceFormat,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer = {
				.NumElements	= (uint32_t)desc.elementCount,
				.Flags			= D3D12_BUFFER_SRV_FLAG_NONE
			}
		};

		auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		auto uavDesc = D3D12_UNORDERED_ACCESS_VIEW_DESC{
			.Format			= FormatToDxgi(desc.format).resourceFormat,
			.ViewDimension	= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements	= (uint32_t)desc.elementCount,
				.Flags			= D3D12_BUFFER_UAV_FLAG_NONE
			}
		};

		auto uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);

		descExt.SetSrvHandle(srvHandle);
		descExt.SetUavHandle(uavHandle);
	}

	return Make<GpuBufferData>(desc, descExt);
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


D3D12MA::Allocation* GraphicsDevice::CreateGpuBuffer(GpuBufferDesc& desc, ResourceState& initialState)
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

	D3D12MA::Allocation* pAllocation{ nullptr };
	HRESULT hr = m_d3d12maAllocator->CreateResource(
		&allocationDesc,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL,
		&pAllocation,
		IID_NULL, NULL);

	return pAllocation;
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