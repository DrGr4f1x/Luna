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

#include "ResourceManager12.h"

using namespace std;


namespace Luna::DX12
{

ResourceManager* g_resourceManager{ nullptr };


ResourceManager::ResourceManager(ID3D12Device* device, D3D12MA::Allocator* allocator)
	: m_device{ device }
	, m_allocator{ allocator }
{
	assert(g_resourceManager == nullptr);

	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		// Resource data
		m_resourceFreeList.push(i);
		m_resourceIndices[i] = InvalidAllocation;
		m_resourceData[i] = ResourceData{};

		// Initialize caches
		m_colorBufferCache.Reset(i);
		m_depthBufferCache.Reset(i);
		m_gpuBufferCache.Reset(i);
	}

	g_resourceManager = this;
}


ResourceManager::~ResourceManager()
{
	g_resourceManager = nullptr;
}


ResourceHandle ResourceManager::CreateColorBuffer(const ColorBufferDesc& colorBufferDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_resourceFreeList.empty());

	// Get a resource index allocation
	uint32_t resourceIndex = m_resourceFreeList.front();
	m_resourceFreeList.pop();

	// Get a color buffer index allocation
	uint32_t colorBufferIndex = m_colorBufferCache.freeList.front();
	m_colorBufferCache.freeList.pop();

	// Make sure we don't already have a color buffer allocation
	assert(m_resourceIndices[resourceIndex] == InvalidAllocation);

	// Store the color buffer allocation index
	m_resourceIndices[resourceIndex] = colorBufferIndex;

	// Allocate the color buffer and resource
	auto [resourceData, colorBufferData] = CreateColorBuffer_Internal(colorBufferDesc);

	m_colorBufferCache.AddData(colorBufferIndex, colorBufferDesc, colorBufferData);
	m_resourceData[resourceIndex] = resourceData;

	return Create<ResourceHandleType>(resourceIndex, ManagedColorBuffer, this);
}


ResourceHandle ResourceManager::CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_resourceFreeList.empty());

	// Get a resource index allocation
	uint32_t resourceIndex = m_resourceFreeList.front();
	m_resourceFreeList.pop();

	// Get a depth buffer index allocation
	uint32_t depthBufferIndex = m_depthBufferCache.freeList.front();
	m_depthBufferCache.freeList.pop();

	// Make sure we don't already have a depth buffer allocation
	assert(m_resourceIndices[resourceIndex] == InvalidAllocation);

	// Store the depth buffer allocation index
	m_resourceIndices[resourceIndex] = depthBufferIndex;

	// Allocate the depth buffer and resource
	auto [resourceData, depthBufferData] = CreateDepthBuffer_Internal(depthBufferDesc);

	m_depthBufferCache.AddData(depthBufferIndex, depthBufferDesc, depthBufferData);
	m_resourceData[resourceIndex] = resourceData;

	return Create<ResourceHandleType>(resourceIndex, ManagedDepthBuffer, this);
}


ResourceHandle ResourceManager::CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_resourceFreeList.empty());

	// Get a resource index allocation
	uint32_t resourceIndex = m_resourceFreeList.front();
	m_resourceFreeList.pop();

	// Get a gpu buffer index allocation
	uint32_t gpuBufferIndex = m_gpuBufferCache.freeList.front();
	m_gpuBufferCache.freeList.pop();

	// Make sure we don't already have a gpu buffer allocation
	assert(m_resourceIndices[resourceIndex] == InvalidAllocation);

	// Store the gpu buffer allocation index
	m_resourceIndices[resourceIndex] = gpuBufferIndex;

	// Allocate the gpu buffer and resource
	GpuBufferDesc gpuBufferDesc2{ gpuBufferDesc };
	if (gpuBufferDesc2.resourceType == ResourceType::ConstantBuffer)
	{
		gpuBufferDesc2.elementSize = Math::AlignUp(gpuBufferDesc2.elementSize, 256);
	}
	auto [resourceData, gpuBufferData] = CreateGpuBuffer_Internal(gpuBufferDesc2);

	m_gpuBufferCache.AddData(gpuBufferIndex, gpuBufferDesc2, gpuBufferData);
	m_resourceData[resourceIndex] = resourceData;

	return Create<ResourceHandleType>(resourceIndex, ManagedGpuBuffer, this);
}


void ResourceManager::DestroyHandle(ResourceHandleType* handle)
{
	std::lock_guard guard(m_allocationMutex);

	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	switch (type)
	{
	case ManagedColorBuffer:
		m_colorBufferCache.Reset(resourceIndex);
		break;

	case ManagedDepthBuffer:
		m_depthBufferCache.Reset(resourceIndex);
		break;

	case ManagedGpuBuffer:
		m_gpuBufferCache.Reset(resourceIndex);
		break;

	default:
		LogError(LogDirectX) << "ResourceManager: Attempting to destroy handle of unknown type " << type << endl;
		break;
	}

	// Finally, mark the resource index as unallocated
	m_resourceIndices[index] = InvalidAllocation;
	m_resourceFreeList.push(index);
}


std::optional<ResourceType> ResourceManager::GetResourceType(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	switch (type)
	{
	case ManagedColorBuffer: return make_optional(m_colorBufferCache.descArray[resourceIndex].resourceType);
	case ManagedDepthBuffer: return make_optional(m_depthBufferCache.descArray[resourceIndex].resourceType);
	case ManagedGpuBuffer: return make_optional(m_gpuBufferCache.descArray[resourceIndex].resourceType);

	default: 
		assert(false);
		LogError(LogGraphics) << "ResourceManager: unknown resource type " << type << endl;
		return std::nullopt;
	}
}


std::optional<ResourceState> ResourceManager::GetUsageState(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	return make_optional(m_resourceData[index].usageState);
}


void ResourceManager::SetUsageState(ResourceHandleType* handle, ResourceState newState)
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	m_resourceData[index].usageState = newState;
}


std::optional<Format> ResourceManager::GetFormat(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	switch (type)
	{
	case ManagedColorBuffer: return make_optional(m_colorBufferCache.descArray[resourceIndex].format);
	case ManagedDepthBuffer: return make_optional(m_depthBufferCache.descArray[resourceIndex].format);
	case ManagedGpuBuffer: return make_optional(m_gpuBufferCache.descArray[resourceIndex].format);

	default:
		assert(false);
		LogError(LogGraphics) << "ResourceManager: unknown resource type " << type << endl;
		return std::nullopt;
	}
}


std::optional<uint64_t> ResourceManager::GetWidth(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	switch (type)
	{
	case ManagedColorBuffer: return make_optional(m_colorBufferCache.descArray[resourceIndex].width);
	case ManagedDepthBuffer: return make_optional(m_depthBufferCache.descArray[resourceIndex].width);
	default:
		assert(false);
		LogError(LogGraphics) << "ResourceManager: unknown resource type " << type << endl;
		return std::nullopt;
	}
}


std::optional<uint32_t> ResourceManager::GetHeight(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	switch (type)
	{
	case ManagedColorBuffer: return make_optional(m_colorBufferCache.descArray[resourceIndex].height);
	case ManagedDepthBuffer: return make_optional(m_depthBufferCache.descArray[resourceIndex].height);
	default:
		assert(false);
		LogError(LogGraphics) << "ResourceManager: unknown resource type " << type << endl;
		return std::nullopt;
	}
}


std::optional<uint32_t> ResourceManager::GetDepthOrArraySize(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	switch (type)
	{
	case ManagedColorBuffer: return make_optional(m_colorBufferCache.descArray[resourceIndex].arraySizeOrDepth);
	case ManagedDepthBuffer: return make_optional(m_depthBufferCache.descArray[resourceIndex].arraySizeOrDepth);
	default:
		assert(false);
		LogError(LogGraphics) << "ResourceManager: unknown resource type " << type << endl;
		return std::nullopt;
	}
}


std::optional<uint32_t> ResourceManager::GetNumMips(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	switch (type)
	{
	case ManagedColorBuffer: return make_optional(m_colorBufferCache.descArray[resourceIndex].numMips);
	case ManagedDepthBuffer: return make_optional(m_depthBufferCache.descArray[resourceIndex].numMips);
	default:
		assert(false);
		LogError(LogGraphics) << "ResourceManager: unknown resource type " << type << endl;
		return std::nullopt;
	}
}


std::optional<uint32_t> ResourceManager::GetNumSamples(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	switch (type)
	{
	case ManagedColorBuffer: return make_optional(m_colorBufferCache.descArray[resourceIndex].numSamples);
	case ManagedDepthBuffer: return make_optional(m_depthBufferCache.descArray[resourceIndex].numSamples);
	default:
		assert(false);
		LogError(LogGraphics) << "ResourceManager: unknown resource type " << type << endl;
		return std::nullopt;
	}
}


std::optional<uint32_t> ResourceManager::GetPlaneCount(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	switch (type)
	{
	case ManagedColorBuffer: return make_optional(m_colorBufferCache.dataArray[resourceIndex].planeCount);
	case ManagedDepthBuffer: return make_optional(m_depthBufferCache.dataArray[resourceIndex].planeCount);
	default:
		assert(false);
		LogError(LogGraphics) << "ResourceManager: unknown resource type " << type << endl;
		return std::nullopt;
	}
}


std::optional<Color> ResourceManager::GetClearColor(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	switch (type)
	{
	case ManagedColorBuffer: return make_optional(m_colorBufferCache.descArray[resourceIndex].clearColor);
	default:
		assert(false);
		LogError(LogGraphics) << "ResourceManager: unknown resource type " << type << endl;
		return std::nullopt;
	}
}


std::optional<float> ResourceManager::GetClearDepth(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	switch (type)
	{
	case ManagedDepthBuffer: return make_optional(m_depthBufferCache.descArray[resourceIndex].clearDepth);
	default:
		assert(false);
		LogError(LogGraphics) << "ResourceManager: unknown resource type " << type << endl;
		return std::nullopt;
	}
}


std::optional<uint8_t> ResourceManager::GetClearStencil(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	switch (type)
	{
	case ManagedDepthBuffer: return make_optional(m_depthBufferCache.descArray[resourceIndex].clearStencil);
	default:
		assert(false);
		LogError(LogGraphics) << "ResourceManager: unknown resource type " << type << endl;
		return std::nullopt;
	}
}


std::optional<size_t> ResourceManager::GetSize(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	switch (type)
	{
	case ManagedGpuBuffer: return make_optional(m_gpuBufferCache.descArray[resourceIndex].elementCount * m_gpuBufferCache.descArray[resourceIndex].elementSize);
	default:
		assert(false);
		LogError(LogGraphics) << "ResourceManager: unknown resource type " << type << endl;
		return std::nullopt;
	}
}


std::optional<size_t> ResourceManager::GetElementCount(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	switch (type)
	{
	case ManagedGpuBuffer: return make_optional(m_gpuBufferCache.descArray[resourceIndex].elementCount);
	default:
		assert(false);
		LogError(LogGraphics) << "ResourceManager: unknown resource type " << type << endl;
		return std::nullopt;
	}
}


std::optional<size_t> ResourceManager::GetElementSize(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	switch (type)
	{
	case ManagedGpuBuffer: return make_optional(m_gpuBufferCache.descArray[resourceIndex].elementSize);
	default:
		assert(false);
		LogError(LogGraphics) << "ResourceManager: unknown resource type " << type << endl;
		return std::nullopt;
	}
}


void ResourceManager::Update(ResourceHandleType* handle, size_t sizeInBytes, size_t offset, const void* data) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert((sizeInBytes + offset) <= (m_gpuBufferCache.descArray[resourceIndex].elementSize * m_gpuBufferCache.descArray[resourceIndex].elementCount));
	assert(HasFlag(m_gpuBufferCache.descArray[resourceIndex].memoryAccess, MemoryAccess::CpuWrite));

	CD3DX12_RANGE readRange(0, 0);

	ID3D12Resource* resource = m_resourceData[index].resource.get();

	// Map uniform buffer and update it
	uint8_t* pData = nullptr;
	ThrowIfFailed(resource->Map(0, &readRange, reinterpret_cast<void**>(&pData)));
	memcpy((void*)(pData + offset), data, sizeInBytes);
	// Unmap after data has been copied
	// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
	resource->Unmap(0, nullptr);
}


ResourceHandle ResourceManager::CreateColorBufferFromSwapChain(IDXGISwapChain* swapChain, uint32_t imageIndex)
{
	wil::com_ptr<ID3D12Resource> displayPlane;
	assert_succeeded(swapChain->GetBuffer(imageIndex, IID_PPV_ARGS(&displayPlane)));

	const string name = format("Primary SwapChain Image {}", imageIndex);
	SetDebugName(displayPlane.get(), name);

	D3D12_RESOURCE_DESC resourceDesc = displayPlane->GetDesc();

	ColorBufferDesc colorBufferDesc{
		.name				= name,
		.resourceType		= ResourceType::Texture2D,
		.width				= resourceDesc.Width,
		.height				= resourceDesc.Height,
		.arraySizeOrDepth	= resourceDesc.DepthOrArraySize,
		.numSamples			= resourceDesc.SampleDesc.Count,
		.format				= DxgiToFormat(resourceDesc.Format)
	};

	auto rtvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_device->CreateRenderTargetView(displayPlane.get(), nullptr, rtvHandle);

	auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateShaderResourceView(displayPlane.get(), nullptr, srvHandle);

	const uint8_t planeCount = GetFormatPlaneCount(resourceDesc.Format);
	colorBufferDesc.planeCount = planeCount;

	ResourceData resourceData{
		.resource		= displayPlane.get(),
		.usageState		= ResourceState::Present
	};

	ColorBufferData colorBufferData{
		.srvHandle = srvHandle,
		.rtvHandle = rtvHandle,
		.planeCount = planeCount
	};

	std::lock_guard guard(m_allocationMutex);

	assert(!m_resourceFreeList.empty());

	uint32_t index = m_resourceFreeList.front();
	m_resourceFreeList.pop();

	// Get a color buffer index allocation
	uint32_t colorBufferIndex = m_colorBufferCache.freeList.front();
	m_colorBufferCache.freeList.pop();

	// Make sure we don't already have a color buffer allocation
	assert(m_resourceIndices[index] == InvalidAllocation);

	// Store the color buffer allocation index
	m_resourceIndices[index] = colorBufferIndex;

	m_colorBufferCache.descArray[colorBufferIndex] = colorBufferDesc;
	m_colorBufferCache.dataArray[colorBufferIndex] = colorBufferData;
	m_resourceData[index] = resourceData;

	return Create<ResourceHandleType>(index, ManagedColorBuffer, this);
}


ID3D12Resource* ResourceManager::GetResource(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	return m_resourceData[index].resource.get();
}


D3D12_CPU_DESCRIPTOR_HANDLE ResourceManager::GetSRV(ResourceHandleType* handle, bool depthSrv) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	switch (type)
	{
	case ManagedColorBuffer: return m_colorBufferCache.dataArray[resourceIndex].srvHandle;
	case ManagedDepthBuffer: return depthSrv ? m_depthBufferCache.dataArray[resourceIndex].depthSrvHandle : m_depthBufferCache.dataArray[resourceIndex].stencilSrvHandle;
	case ManagedGpuBuffer: return m_gpuBufferCache.dataArray[resourceIndex].srvHandle;
	default: return D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = 0 };
	}
}


D3D12_CPU_DESCRIPTOR_HANDLE ResourceManager::GetRTV(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);
	assert(type == ManagedColorBuffer);

	return m_colorBufferCache.dataArray[resourceIndex].rtvHandle;
}


D3D12_CPU_DESCRIPTOR_HANDLE ResourceManager::GetDSV(ResourceHandleType* handle, DepthStencilAspect depthStencilAspect) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);
	assert(type == ManagedDepthBuffer);

	switch (depthStencilAspect)
	{
	case DepthStencilAspect::ReadWrite:		return m_depthBufferCache.dataArray[resourceIndex].dsvHandles[0];
	case DepthStencilAspect::ReadOnly:		return m_depthBufferCache.dataArray[resourceIndex].dsvHandles[1];
	case DepthStencilAspect::DepthReadOnly:	return m_depthBufferCache.dataArray[resourceIndex].dsvHandles[2];
	default:								return m_depthBufferCache.dataArray[resourceIndex].dsvHandles[3];
	}
}


D3D12_CPU_DESCRIPTOR_HANDLE ResourceManager::GetUAV(ResourceHandleType* handle, uint32_t uavIndex) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);
	assert(type == ManagedColorBuffer || type == ManagedGpuBuffer);

	switch (type)
	{
	case ManagedColorBuffer: return m_colorBufferCache.dataArray[resourceIndex].uavHandles[uavIndex];
	case ManagedGpuBuffer: return m_gpuBufferCache.dataArray[resourceIndex].uavHandle;
	default:
		return D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = 0 };
	}
}


D3D12_CPU_DESCRIPTOR_HANDLE ResourceManager::GetCBV(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedGpuBuffer);

	return m_gpuBufferCache.dataArray[resourceIndex].cbvHandle;
}


uint64_t ResourceManager::GetGpuAddress(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);
	return m_resourceData[index].resource->GetGPUVirtualAddress();
}


std::tuple<uint32_t, uint32_t, uint32_t> ResourceManager::UnpackHandle(ResourceHandleType* handle) const 
{
	assert(handle != nullptr);

	const auto index = handle->GetIndex();
	const auto type = handle->GetType();

	// Ensure that we have an allocated resource
	const auto resourceIndex = m_resourceIndices[index];
	assert(resourceIndex != InvalidAllocation);

	return make_tuple(index, type, resourceIndex);
}


pair<ResourceData, ColorBufferData> ResourceManager::CreateColorBuffer_Internal(const ColorBufferDesc& colorBufferDesc)
{
	// Create resource
	auto numMips = colorBufferDesc.numMips == 0 ? ComputeNumMips(colorBufferDesc.width, colorBufferDesc.height) : colorBufferDesc.numMips;
	auto flags = CombineResourceFlags(colorBufferDesc.numSamples);

	D3D12_RESOURCE_DESC resourceDesc{
		.Dimension			= GetResourceDimension(colorBufferDesc.resourceType),
		.Alignment			= 0,
		.Width				= (UINT64)colorBufferDesc.width,
		.Height				= (UINT)colorBufferDesc.height,
		.DepthOrArraySize	= (UINT16)colorBufferDesc.arraySizeOrDepth,
		.MipLevels			= (UINT16)numMips,
		.Format				= FormatToDxgi(colorBufferDesc.format).resourceFormat,
		.SampleDesc			= {.Count = colorBufferDesc.numSamples, .Quality = 0 },
		.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags				= (D3D12_RESOURCE_FLAGS)flags
	};

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = FormatToDxgi(colorBufferDesc.format).rtvFormat;
	clearValue.Color[0] = colorBufferDesc.clearColor.R();
	clearValue.Color[1] = colorBufferDesc.clearColor.G();
	clearValue.Color[2] = colorBufferDesc.clearColor.B();
	clearValue.Color[3] = colorBufferDesc.clearColor.A();

	ID3D12Resource* resource{ nullptr };
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	assert_succeeded(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&resource)));

	SetDebugName(resource, colorBufferDesc.name);

	// Create descriptors and derived views
	assert_msg(colorBufferDesc.arraySizeOrDepth == 1 || numMips == 1, "We don't support auto-mips on texture arrays");

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};

	DXGI_FORMAT dxgiFormat = FormatToDxgi(colorBufferDesc.format).resourceFormat;
	rtvDesc.Format = dxgiFormat;
	uavDesc.Format = GetUAVFormat(dxgiFormat);
	srvDesc.Format = dxgiFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (colorBufferDesc.arraySizeOrDepth > 1)
	{
		if (colorBufferDesc.resourceType == ResourceType::Texture3D)
		{
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
			rtvDesc.Texture3D.MipSlice = 0;
			rtvDesc.Texture3D.FirstWSlice = 0;
			rtvDesc.Texture3D.WSize = (UINT)colorBufferDesc.arraySizeOrDepth;

			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			uavDesc.Texture3D.MipSlice = 0;
			uavDesc.Texture3D.FirstWSlice = 0;
			uavDesc.Texture3D.WSize = (UINT)colorBufferDesc.arraySizeOrDepth;

			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MipLevels = numMips;
			srvDesc.Texture3D.MostDetailedMip = 0;
		}
		else
		{
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Texture2DArray.MipSlice = 0;
			rtvDesc.Texture2DArray.FirstArraySlice = 0;
			rtvDesc.Texture2DArray.ArraySize = (UINT)colorBufferDesc.arraySizeOrDepth;

			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			uavDesc.Texture2DArray.MipSlice = 0;
			uavDesc.Texture2DArray.FirstArraySlice = 0;
			uavDesc.Texture2DArray.ArraySize = (UINT)colorBufferDesc.arraySizeOrDepth;

			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.MipLevels = numMips;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.ArraySize = (UINT)colorBufferDesc.arraySizeOrDepth;
		}
	}
	else if (colorBufferDesc.numFragments > 1)
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
	m_device->CreateRenderTargetView(resource, &rtvDesc, rtvHandle);

	// Create the shader resource view
	m_device->CreateShaderResourceView(resource, &srvDesc, srvHandle);

	// Create the UAVs for each mip level (RWTexture2D)
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 12> uavHandles;
	if (colorBufferDesc.numFragments == 1)
	{
		for (uint32_t i = 0; i < (uint32_t)uavHandles.size(); ++i)
		{
			uavHandles[i].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}

		for (uint32_t i = 0; i < numMips; ++i)
		{
			uavHandles[i] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			m_device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, uavHandles[i]);

			uavDesc.Texture2D.MipSlice++;
		}
	}

	const uint8_t planeCount = GetFormatPlaneCount(FormatToDxgi(colorBufferDesc.format).resourceFormat);

	ResourceData resourceData{
		.resource		= resource,
		.usageState		= ResourceState::Common
	};

	ColorBufferData colorBufferData{
		.srvHandle		= srvHandle,
		.rtvHandle		= rtvHandle,
		.uavHandles		= uavHandles,
		.planeCount		= planeCount
	};

	return make_pair(resourceData, colorBufferData);
}


pair<ResourceData, DepthBufferData> ResourceManager::CreateDepthBuffer_Internal(const DepthBufferDesc& depthBufferDesc)
{
	// Create resource
	D3D12_RESOURCE_DESC resourceDesc{
		.Dimension			= GetResourceDimension(depthBufferDesc.resourceType),
		.Alignment			= 0,
		.Width				= (UINT64)depthBufferDesc.width,
		.Height				= (UINT)depthBufferDesc.height,
		.DepthOrArraySize	= (UINT16)depthBufferDesc.arraySizeOrDepth,
		.MipLevels			= (UINT16)depthBufferDesc.numMips,
		.Format				= FormatToDxgi(depthBufferDesc.format).resourceFormat,
		.SampleDesc			= {.Count = depthBufferDesc.numSamples, .Quality = 0 },
		.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags				= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	};

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = FormatToDxgi(depthBufferDesc.format).rtvFormat;
	clearValue.DepthStencil.Depth = depthBufferDesc.clearDepth;
	clearValue.DepthStencil.Stencil = depthBufferDesc.clearStencil;

	wil::com_ptr<ID3D12Resource> resource;
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	assert_succeeded(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&resource)));

	SetDebugName(resource.get(), depthBufferDesc.name);

	// Create descriptors and derived views
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = GetDSVFormat(FormatToDxgi(depthBufferDesc.format).resourceFormat);

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
	m_device->CreateDepthStencilView(resource.get(), &dsvDesc, dsvHandles[0]);

	dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	m_device->CreateDepthStencilView(resource.get(), &dsvDesc, dsvHandles[1]);

	auto stencilReadFormat = GetStencilFormat(FormatToDxgi(depthBufferDesc.format).resourceFormat);
	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		dsvHandles[2] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		dsvHandles[3] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		m_device->CreateDepthStencilView(resource.get(), &dsvDesc, dsvHandles[2]);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		m_device->CreateDepthStencilView(resource.get(), &dsvDesc, dsvHandles[3]);
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
	srvDesc.Format = GetDepthFormat(FormatToDxgi(depthBufferDesc.format).resourceFormat);
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
	m_device->CreateShaderResourceView(resource.get(), &srvDesc, depthSrvHandle);

	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		stencilSrvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		srvDesc.Format = stencilReadFormat;
		srvDesc.Texture2D.PlaneSlice = (srvDesc.Format == DXGI_FORMAT_X32_TYPELESS_G8X24_UINT) ? 1 : 0;
		m_device->CreateShaderResourceView(resource.get(), &srvDesc, stencilSrvHandle);
	}

	const uint8_t planeCount = GetFormatPlaneCount(FormatToDxgi(depthBufferDesc.format).resourceFormat);

	ResourceData resourceData{
		.resource	= resource,
		.usageState		= ResourceState::Common
	};

	DepthBufferData depthBufferData{
		.dsvHandles			= dsvHandles,
		.depthSrvHandle		= depthSrvHandle,
		.stencilSrvHandle	= stencilSrvHandle,
		.planeCount			= planeCount
	};

	return make_pair(resourceData, depthBufferData);
}


pair<ResourceData, GpuBufferData> ResourceManager::CreateGpuBuffer_Internal(const GpuBufferDesc& gpuBufferDesc)
{
	ResourceState initialState = ResourceState::GenericRead;

	wil::com_ptr<D3D12MA::Allocation> allocation = AllocateBuffer(gpuBufferDesc);
	ID3D12Resource* pResource = allocation->GetResource();

	SetDebugName(pResource, gpuBufferDesc.name);

	ResourceData resourceData{
		.resource		= pResource,
		.usageState		= ResourceState::Common
	};

	GpuBufferData gpuBufferData{
		.allocation = allocation.get()
	};

	const size_t bufferSize = gpuBufferDesc.elementCount * gpuBufferDesc.elementSize;

	if (gpuBufferDesc.resourceType == ResourceType::ByteAddressBuffer || gpuBufferDesc.resourceType == ResourceType::IndirectArgsBuffer)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
			.Format						= DXGI_FORMAT_R32_TYPELESS,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer = {
				.NumElements	= (uint32_t)bufferSize / 4,
				.Flags			= D3D12_BUFFER_SRV_FLAG_RAW
			}
		};

		auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format			= DXGI_FORMAT_R32_TYPELESS,
			.ViewDimension	= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements	= (uint32_t)bufferSize / 4,
				.Flags			= D3D12_BUFFER_UAV_FLAG_RAW
			}
		};

		auto uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);

		gpuBufferData.srvHandle = srvHandle;
		gpuBufferData.uavHandle = uavHandle;
	}

	if (gpuBufferDesc.resourceType == ResourceType::StructuredBuffer)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
			.Format						= DXGI_FORMAT_UNKNOWN,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer = {
				.NumElements			= (uint32_t)gpuBufferDesc.elementCount,
				.StructureByteStride	= (uint32_t)gpuBufferDesc.elementSize,
				.Flags					= D3D12_BUFFER_SRV_FLAG_NONE
			}
		};

		auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format			= DXGI_FORMAT_UNKNOWN,
			.ViewDimension	= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements			= (uint32_t)gpuBufferDesc.elementCount,
				.StructureByteStride	= (uint32_t)gpuBufferDesc.elementSize,
				.CounterOffsetInBytes	= 0,
				.Flags					= D3D12_BUFFER_UAV_FLAG_NONE
			}
		};

		auto uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);

		gpuBufferData.srvHandle = srvHandle;
		gpuBufferData.uavHandle = uavHandle;
	}

	if (gpuBufferDesc.resourceType == ResourceType::TypedBuffer)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
			.Format						= FormatToDxgi(gpuBufferDesc.format).resourceFormat,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer = {
				.NumElements	= (uint32_t)gpuBufferDesc.elementCount,
				.Flags			= D3D12_BUFFER_SRV_FLAG_NONE
			}
		};

		auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format			= FormatToDxgi(gpuBufferDesc.format).resourceFormat,
			.ViewDimension	= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements	= (uint32_t)gpuBufferDesc.elementCount,
				.Flags			= D3D12_BUFFER_UAV_FLAG_NONE
			}
		};

		auto uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);

		gpuBufferData.srvHandle = srvHandle;
		gpuBufferData.uavHandle = uavHandle;
	}

	if (gpuBufferDesc.resourceType == ResourceType::ConstantBuffer)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{
			.BufferLocation		= pResource->GetGPUVirtualAddress(),
			.SizeInBytes		= (uint32_t)(gpuBufferDesc.elementCount * gpuBufferDesc.elementSize)
		};

		auto cbvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);

		gpuBufferData.cbvHandle = cbvHandle;
	}

	return make_pair(resourceData, gpuBufferData);
}


wil::com_ptr<D3D12MA::Allocation> ResourceManager::AllocateBuffer(const GpuBufferDesc& gpuBufferDesc) const
{
	const UINT64 bufferSize = gpuBufferDesc.elementSize * gpuBufferDesc.elementCount;

	D3D12_HEAP_TYPE heapType = GetHeapType(gpuBufferDesc.memoryAccess);
	D3D12_RESOURCE_FLAGS flags = (gpuBufferDesc.bAllowUnorderedAccess || IsUnorderedAccessType(gpuBufferDesc.resourceType))
		? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
		: D3D12_RESOURCE_FLAG_NONE;


	D3D12_RESOURCE_DESC resourceDesc{
		.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment			= 0,
		.Width				= bufferSize,
		.Height				= 1,
		.DepthOrArraySize	= 1,
		.MipLevels			= 1,
		.Format				= FormatToDxgi(gpuBufferDesc.format).resourceFormat,
		.SampleDesc			= {.Count = 1, .Quality = 0 },
		.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags				= flags
	};

	auto allocationDesc = D3D12MA::ALLOCATION_DESC{
		.HeapType = heapType
	};

	wil::com_ptr<D3D12MA::Allocation> allocation;
	HRESULT hr = m_allocator->CreateResource(
		&allocationDesc,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		NULL,
		&allocation,
		IID_NULL, NULL);

	return allocation;
}


ResourceManager* const GetD3D12ResourceManager()
{
	return g_resourceManager;
}

} // namespace Luna::DX12