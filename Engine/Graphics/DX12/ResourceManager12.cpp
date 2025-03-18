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

#include "FileSystem.h"
#include "DescriptorSetManager12.h"
#include "Shader12.h"

using namespace std;


namespace Luna::DX12
{

ResourceManager* g_resourceManager{ nullptr };


pair<string, bool> GetShaderFilenameWithExtension(const string& shaderFilename)
{
	auto fileSystem = GetFileSystem();

	string shaderFileWithExtension = shaderFilename;
	bool exists = false;

	// See if the filename already has an extension
	string extension = fileSystem->GetFileExtension(shaderFilename);
	if (!extension.empty())
	{
		exists = fileSystem->Exists(shaderFileWithExtension);
	}
	else
	{
		// Try .dxil extension first
		shaderFileWithExtension = shaderFilename + ".dxil";
		exists = fileSystem->Exists(shaderFileWithExtension);
		if (!exists)
		{
			// Try .dxbc next
			shaderFileWithExtension = shaderFilename + ".dxbc";
			exists = fileSystem->Exists(shaderFileWithExtension);
		}
	}

	return make_pair(shaderFileWithExtension, exists);
}


Shader* LoadShader(ShaderType type, const ShaderNameAndEntry& shaderNameAndEntry)
{
	auto [shaderFilenameWithExtension, exists] = GetShaderFilenameWithExtension(shaderNameAndEntry.shaderFile);

	if (!exists)
	{
		return nullptr;
	}

	ShaderDesc shaderDesc{
		.filenameWithExtension = shaderFilenameWithExtension,
		.entry = shaderNameAndEntry.entry,
		.type = type
	};

	return Shader::Load(shaderDesc);
}


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
		m_graphicsPipelineCache.Reset(i);
		m_rootSignatureCache.Reset(i);
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


ResourceHandle ResourceManager::CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc)
{
	std::lock_guard guard(m_allocationMutex);

	// Get a graphics pipeline index allocation
	assert(!m_graphicsPipelineCache.freeList.empty());
	uint32_t graphicsPipelineIndex = m_graphicsPipelineCache.freeList.front();
	m_graphicsPipelineCache.freeList.pop();

	// Allocate the graphics pipeline 
	auto graphicsPipeline = CreateGraphicsPipeline_Internal(pipelineDesc);

	m_graphicsPipelineCache.AddData(graphicsPipelineIndex, pipelineDesc, graphicsPipeline);

	return Create<ResourceHandleType>(graphicsPipelineIndex, ManagedGraphicsPipeline, this);
}


ResourceHandle ResourceManager::CreateRootSignature(const RootSignatureDesc& rootSignatureDesc)
{
	std::lock_guard guard(m_allocationMutex);

	// Get a root signature index allocation
	uint32_t rootSignatureIndex = m_rootSignatureCache.freeList.front();
	m_rootSignatureCache.freeList.pop();

	// Allocate the root signature data
	auto rootSignatureData = CreateRootSignature_Internal(rootSignatureDesc);

	m_rootSignatureCache.AddData(rootSignatureIndex, rootSignatureDesc, rootSignatureData);

	return Create<ResourceHandleType>(rootSignatureIndex, ManagedRootSignature, this);
}


void ResourceManager::DestroyHandle(ResourceHandleType* handle)
{
	std::lock_guard guard(m_allocationMutex);

	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	bool isResourceHandle = false;

	switch (type)
	{
	case ManagedColorBuffer:
		m_colorBufferCache.Reset(resourceIndex);
		isResourceHandle = true;
		break;

	case ManagedDepthBuffer:
		m_depthBufferCache.Reset(resourceIndex);
		isResourceHandle = true;
		break;

	case ManagedGpuBuffer:
		m_gpuBufferCache.Reset(resourceIndex);
		isResourceHandle = true;
		break;

	case ManagedGraphicsPipeline:
		m_graphicsPipelineCache.Reset(resourceIndex);
		break;

	case ManagedRootSignature:
		m_rootSignatureCache.Reset(resourceIndex);
		break;

	default:
		LogError(LogDirectX) << "ResourceManager: Attempting to destroy handle of unknown type " << type << endl;
		break;
	}

	// Finally, mark the resource index as unallocated
	if (isResourceHandle)
	{
		m_resourceData[index] = ResourceData{};
		m_resourceIndices[index] = InvalidAllocation;
		m_resourceFreeList.push(index);
	}
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


const GraphicsPipelineDesc& ResourceManager::GetGraphicsPipelineDesc(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedGraphicsPipeline);

	return m_graphicsPipelineCache.descArray[resourceIndex];
}


const RootSignatureDesc& ResourceManager::GetRootSignatureDesc(const ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedRootSignature);

	return m_rootSignatureCache.descArray[resourceIndex];
}


uint32_t ResourceManager::GetNumRootParameters(const ResourceHandleType* handle) const
{
	return (uint32_t)GetRootSignatureDesc(handle).rootParameters.size();
}


DescriptorSetHandle ResourceManager::CreateDescriptorSet(ResourceHandleType* handle, uint32_t rootParamIndex) const
{
	const auto& rootSignatureDesc = GetRootSignatureDesc(handle);

	assert(rootParamIndex < rootSignatureDesc.rootParameters.size());

	const auto& rootParam = rootSignatureDesc.rootParameters[rootParamIndex];

	const bool isRootBuffer = rootParam.parameterType == RootParameterType::RootCBV ||
		rootParam.parameterType == RootParameterType::RootSRV ||
		rootParam.parameterType == RootParameterType::RootUAV;

	const bool isSamplerTable = rootParam.IsSamplerTable();

	const D3D12_DESCRIPTOR_HEAP_TYPE heapType = isSamplerTable
		? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
		: D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	DescriptorSetDesc descriptorSetDesc{
		.descriptorHandle = isRootBuffer ? DescriptorHandle{} : AllocateUserDescriptor(heapType),
		.numDescriptors = rootParam.GetNumDescriptors(),
		.isSamplerTable = isSamplerTable,
		.isRootBuffer = isRootBuffer
	};

	return GetD3D12DescriptorSetManager()->CreateDescriptorSet(descriptorSetDesc);
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


ID3D12PipelineState* ResourceManager::GetGraphicsPipelineState(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedGraphicsPipeline);

	return m_graphicsPipelineCache.dataArray[resourceIndex].get();
}


ID3D12RootSignature* ResourceManager::GetRootSignature(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedRootSignature);

	return m_rootSignatureCache.dataArray[resourceIndex].rootSignature.get();
}


uint32_t ResourceManager::GetDescriptorTableBitmap(const ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedRootSignature);

	return m_rootSignatureCache.dataArray[resourceIndex].descriptorTableBitmap;
}


uint32_t ResourceManager::GetSamplerTableBitmap(const ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedRootSignature);

	return m_rootSignatureCache.dataArray[resourceIndex].samplerTableBitmap;
}


const vector<uint32_t>& ResourceManager::GetDescriptorTableSize(const ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedRootSignature);

	return m_rootSignatureCache.dataArray[resourceIndex].descriptorTableSizes;
}


std::tuple<uint32_t, uint32_t, uint32_t> ResourceManager::UnpackHandle(const ResourceHandleType* handle) const 
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


wil::com_ptr<ID3D12PipelineState> ResourceManager::CreateGraphicsPipeline_Internal(const GraphicsPipelineDesc& pipelineDesc)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12PipelineDesc{};
	d3d12PipelineDesc.NodeMask = 1;
	d3d12PipelineDesc.SampleMask = pipelineDesc.sampleMask;
	d3d12PipelineDesc.InputLayout.NumElements = 0;

	// Blend state
	d3d12PipelineDesc.BlendState.AlphaToCoverageEnable = pipelineDesc.blendState.alphaToCoverageEnable ? TRUE : FALSE;
	d3d12PipelineDesc.BlendState.IndependentBlendEnable = pipelineDesc.blendState.independentBlendEnable ? TRUE : FALSE;

	for (uint32_t i = 0; i < 8; ++i)
	{
		auto& rtDesc = d3d12PipelineDesc.BlendState.RenderTarget[i];
		const auto& renderTargetBlend = pipelineDesc.blendState.renderTargetBlend[i];

		rtDesc.BlendEnable = renderTargetBlend.blendEnable ? TRUE : FALSE;
		rtDesc.LogicOpEnable = renderTargetBlend.logicOpEnable ? TRUE : FALSE;
		rtDesc.SrcBlend = BlendToDX12(renderTargetBlend.srcBlend);
		rtDesc.DestBlend = BlendToDX12(renderTargetBlend.dstBlend);
		rtDesc.BlendOp = BlendOpToDX12(renderTargetBlend.blendOp);
		rtDesc.SrcBlendAlpha = BlendToDX12(renderTargetBlend.srcBlendAlpha);
		rtDesc.DestBlendAlpha = BlendToDX12(renderTargetBlend.dstBlendAlpha);
		rtDesc.BlendOpAlpha = BlendOpToDX12(renderTargetBlend.blendOpAlpha);
		rtDesc.LogicOp = LogicOpToDX12(renderTargetBlend.logicOp);
		rtDesc.RenderTargetWriteMask = ColorWriteToDX12(renderTargetBlend.writeMask);
	}

	// Rasterizer state
	const auto& rasterizerState = pipelineDesc.rasterizerState;
	d3d12PipelineDesc.RasterizerState.FillMode = FillModeToDX12(rasterizerState.fillMode);
	d3d12PipelineDesc.RasterizerState.CullMode = CullModeToDX12(rasterizerState.cullMode);
	d3d12PipelineDesc.RasterizerState.FrontCounterClockwise = rasterizerState.frontCounterClockwise ? TRUE : FALSE;
	d3d12PipelineDesc.RasterizerState.DepthBias = rasterizerState.depthBias;
	d3d12PipelineDesc.RasterizerState.DepthBiasClamp = rasterizerState.depthBiasClamp;
	d3d12PipelineDesc.RasterizerState.SlopeScaledDepthBias = rasterizerState.slopeScaledDepthBias;
	d3d12PipelineDesc.RasterizerState.DepthClipEnable = rasterizerState.depthClipEnable ? TRUE : FALSE;
	d3d12PipelineDesc.RasterizerState.MultisampleEnable = rasterizerState.multisampleEnable ? TRUE : FALSE;
	d3d12PipelineDesc.RasterizerState.AntialiasedLineEnable = rasterizerState.antialiasedLineEnable ? TRUE : FALSE;
	d3d12PipelineDesc.RasterizerState.ForcedSampleCount = rasterizerState.forcedSampleCount;
	d3d12PipelineDesc.RasterizerState.ConservativeRaster =
		rasterizerState.conservativeRasterizationEnable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// Depth-stencil state
	const auto& depthStencilState = pipelineDesc.depthStencilState;
	d3d12PipelineDesc.DepthStencilState.DepthEnable = depthStencilState.depthEnable ? TRUE : FALSE;
	d3d12PipelineDesc.DepthStencilState.DepthWriteMask = DepthWriteToDX12(depthStencilState.depthWriteMask);
	d3d12PipelineDesc.DepthStencilState.DepthFunc = ComparisonFuncToDX12(depthStencilState.depthFunc);
	d3d12PipelineDesc.DepthStencilState.StencilEnable = depthStencilState.stencilEnable ? TRUE : FALSE;
	d3d12PipelineDesc.DepthStencilState.StencilReadMask = depthStencilState.stencilReadMask;
	d3d12PipelineDesc.DepthStencilState.StencilWriteMask = depthStencilState.stencilWriteMask;
	d3d12PipelineDesc.DepthStencilState.FrontFace.StencilFailOp = StencilOpToDX12(depthStencilState.frontFace.stencilFailOp);
	d3d12PipelineDesc.DepthStencilState.FrontFace.StencilDepthFailOp = StencilOpToDX12(depthStencilState.frontFace.stencilDepthFailOp);
	d3d12PipelineDesc.DepthStencilState.FrontFace.StencilPassOp = StencilOpToDX12(depthStencilState.frontFace.stencilPassOp);
	d3d12PipelineDesc.DepthStencilState.FrontFace.StencilFunc = ComparisonFuncToDX12(depthStencilState.frontFace.stencilFunc);
	d3d12PipelineDesc.DepthStencilState.BackFace.StencilFailOp = StencilOpToDX12(depthStencilState.backFace.stencilFailOp);
	d3d12PipelineDesc.DepthStencilState.BackFace.StencilDepthFailOp = StencilOpToDX12(depthStencilState.backFace.stencilDepthFailOp);
	d3d12PipelineDesc.DepthStencilState.BackFace.StencilPassOp = StencilOpToDX12(depthStencilState.backFace.stencilPassOp);
	d3d12PipelineDesc.DepthStencilState.BackFace.StencilFunc = ComparisonFuncToDX12(depthStencilState.backFace.stencilFunc);

	// Primitive topology & primitive restart
	d3d12PipelineDesc.PrimitiveTopologyType = PrimitiveTopologyToPrimitiveTopologyTypeDX12(pipelineDesc.topology);
	d3d12PipelineDesc.IBStripCutValue = IndexBufferStripCutValueToDX12(pipelineDesc.indexBufferStripCut);

	// Render target formats
	const uint32_t numRtvs = (uint32_t)pipelineDesc.rtvFormats.size();
	const uint32_t maxRenderTargets = 8;
	for (uint32_t i = 0; i < numRtvs; ++i)
	{
		d3d12PipelineDesc.RTVFormats[i] = FormatToDxgi(pipelineDesc.rtvFormats[i]).rtvFormat;
	}
	for (uint32_t i = numRtvs; i < maxRenderTargets; ++i)
	{
		d3d12PipelineDesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	d3d12PipelineDesc.NumRenderTargets = numRtvs;
	d3d12PipelineDesc.DSVFormat = GetDSVFormat(FormatToDxgi(pipelineDesc.dsvFormat).resourceFormat);
	d3d12PipelineDesc.SampleDesc.Count = pipelineDesc.msaaCount;
	d3d12PipelineDesc.SampleDesc.Quality = 0; // TODO Rework this to enable quality levels in DX12

	// Input layout
	d3d12PipelineDesc.InputLayout.NumElements = (UINT)pipelineDesc.vertexElements.size();
	unique_ptr<const D3D12_INPUT_ELEMENT_DESC> d3dElements;

	if (d3d12PipelineDesc.InputLayout.NumElements > 0)
	{
		D3D12_INPUT_ELEMENT_DESC* newD3DElements = (D3D12_INPUT_ELEMENT_DESC*)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * d3d12PipelineDesc.InputLayout.NumElements);

		const auto& vertexElements = pipelineDesc.vertexElements;

		for (uint32_t i = 0; i < d3d12PipelineDesc.InputLayout.NumElements; ++i)
		{
			newD3DElements[i].AlignedByteOffset = vertexElements[i].alignedByteOffset;
			newD3DElements[i].Format = FormatToDxgi(vertexElements[i].format).srvFormat;
			newD3DElements[i].InputSlot = vertexElements[i].inputSlot;
			newD3DElements[i].InputSlotClass = InputClassificationToDX12(vertexElements[i].inputClassification);
			newD3DElements[i].InstanceDataStepRate = vertexElements[i].instanceDataStepRate;
			newD3DElements[i].SemanticIndex = vertexElements[i].semanticIndex;
			newD3DElements[i].SemanticName = vertexElements[i].semanticName;
		}

		d3dElements.reset((const D3D12_INPUT_ELEMENT_DESC*)newD3DElements);
	}

	// Shaders
	if (pipelineDesc.vertexShader)
	{
		Shader* vertexShader = LoadShader(ShaderType::Vertex, pipelineDesc.vertexShader);
		assert(vertexShader);
		d3d12PipelineDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader->GetByteCode(), vertexShader->GetByteCodeSize());
	}

	if (pipelineDesc.pixelShader)
	{
		Shader* pixelShader = LoadShader(ShaderType::Pixel, pipelineDesc.pixelShader);
		assert(pixelShader);
		d3d12PipelineDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader->GetByteCode(), pixelShader->GetByteCodeSize());
	}

	if (pipelineDesc.geometryShader)
	{
		Shader* geometryShader = LoadShader(ShaderType::Geometry, pipelineDesc.geometryShader);
		assert(geometryShader);
		d3d12PipelineDesc.GS = CD3DX12_SHADER_BYTECODE(geometryShader->GetByteCode(), geometryShader->GetByteCodeSize());
	}

	if (pipelineDesc.hullShader)
	{
		Shader* hullShader = LoadShader(ShaderType::Hull, pipelineDesc.hullShader);
		assert(hullShader);
		d3d12PipelineDesc.HS = CD3DX12_SHADER_BYTECODE(hullShader->GetByteCode(), hullShader->GetByteCodeSize());
	}

	if (pipelineDesc.domainShader)
	{
		Shader* domainShader = LoadShader(ShaderType::Domain, pipelineDesc.domainShader);
		assert(domainShader);
		d3d12PipelineDesc.DS = CD3DX12_SHADER_BYTECODE(domainShader->GetByteCode(), domainShader->GetByteCodeSize());
	}

	// Make sure the root signature is finalized first
	d3d12PipelineDesc.pRootSignature = GetRootSignature(pipelineDesc.rootSignature.get());
	assert(d3d12PipelineDesc.pRootSignature != nullptr);

	d3d12PipelineDesc.InputLayout.pInputElementDescs = nullptr;

	size_t hashCode = Utility::HashState(&d3d12PipelineDesc);
	hashCode = Utility::HashState(d3dElements.get(), d3d12PipelineDesc.InputLayout.NumElements, hashCode);

	d3d12PipelineDesc.InputLayout.pInputElementDescs = d3dElements.get();

	ID3D12PipelineState** ppPipelineState = nullptr;
	ID3D12PipelineState* pPipelineState;
	bool firstCompile = false;
	{
		lock_guard<mutex> CS(m_pipelineStateMutex);

		auto iter = m_pipelineStateMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_pipelineStateMap.end())
		{
			firstCompile = true;
			ppPipelineState = m_pipelineStateMap[hashCode].addressof();
		}
		else
		{
			ppPipelineState = iter->second.addressof();
		}
	}

	if (firstCompile)
	{
		HRESULT res = m_device->CreateGraphicsPipelineState(&d3d12PipelineDesc, IID_PPV_ARGS(&pPipelineState));
		ThrowIfFailed(res);

		SetDebugName(pPipelineState, pipelineDesc.name);

		m_pipelineStateMap[hashCode].attach(pPipelineState);
	}
	else
	{
		while (*ppPipelineState == nullptr)
		{
			this_thread::yield();
		}
		pPipelineState = *ppPipelineState;
	}

	return pPipelineState;
}


RootSignatureData ResourceManager::CreateRootSignature_Internal(const RootSignatureDesc& rootSignatureDesc)
{
	std::vector<D3D12_ROOT_PARAMETER1> d3d12RootParameters;

	auto exitGuard = wil::scope_exit([&]()
		{
			for (auto& param : d3d12RootParameters)
			{
				if (param.DescriptorTable.NumDescriptorRanges > 0)
				{
					delete[] param.DescriptorTable.pDescriptorRanges;
				}
			}
		});

	// Validate RootSignatureDesc
	if (!rootSignatureDesc.Validate())
	{
		LogError(LogDirectX) << "RootSignature is not valid!" << endl;
		return RootSignatureData{};
	}

	// Build DX12 root parameter descriptions
	for (const auto& rootParameter : rootSignatureDesc.rootParameters)
	{
		if (rootParameter.parameterType == RootParameterType::RootConstants)
		{
			D3D12_ROOT_PARAMETER1& param = d3d12RootParameters.emplace_back();
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			param.ShaderVisibility = ShaderStageToDX12(rootParameter.shaderVisibility);
			param.Constants.Num32BitValues = rootParameter.num32BitConstants;
			param.Constants.RegisterSpace = rootParameter.registerSpace;
			param.Constants.ShaderRegister = rootParameter.startRegister;
		}
		else if (rootParameter.parameterType == RootParameterType::RootCBV ||
			rootParameter.parameterType == RootParameterType::RootSRV ||
			rootParameter.parameterType == RootParameterType::RootUAV)
		{
			D3D12_ROOT_PARAMETER1& param = d3d12RootParameters.emplace_back();
			param.ParameterType = RootParameterTypeToDX12(rootParameter.parameterType);
			param.ShaderVisibility = ShaderStageToDX12(rootParameter.shaderVisibility);
			param.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
			param.Descriptor.RegisterSpace = rootParameter.registerSpace;
			param.Descriptor.ShaderRegister = rootParameter.startRegister;
		}
		else if (rootParameter.parameterType == RootParameterType::Table)
		{
			D3D12_ROOT_PARAMETER1& param = d3d12RootParameters.emplace_back();
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			param.ShaderVisibility = ShaderStageToDX12(rootParameter.shaderVisibility);

			const uint32_t numRanges = (uint32_t)rootParameter.table.size();
			param.DescriptorTable.NumDescriptorRanges = numRanges;
			D3D12_DESCRIPTOR_RANGE1* pRanges = new D3D12_DESCRIPTOR_RANGE1[numRanges];
			for (uint32_t i = 0; i < numRanges; ++i)
			{
				D3D12_DESCRIPTOR_RANGE1& d3d12Range = pRanges[i];
				const DescriptorRange& range = rootParameter.table[i];
				d3d12Range.RangeType = DescriptorTypeToDX12(range.descriptorType);
				d3d12Range.NumDescriptors = range.numDescriptors;
				d3d12Range.BaseShaderRegister = range.startRegister;
				d3d12Range.RegisterSpace = rootParameter.registerSpace;
				d3d12Range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			}
			param.DescriptorTable.pDescriptorRanges = pRanges;
		}
	}

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC d3d12RootSignatureDesc{
		.Version	= D3D_ROOT_SIGNATURE_VERSION_1_1,
		.Desc_1_1	= {
			.NumParameters		= (uint32_t)d3d12RootParameters.size(),
			.pParameters		= d3d12RootParameters.data(),
			.NumStaticSamplers	= 0,
			.pStaticSamplers	= nullptr,
			.Flags				= RootSignatureFlagsToDX12(rootSignatureDesc.flags)
		}
	};

	uint32_t descriptorTableBitmap{ 0 };
	uint32_t samplerTableBitmap{ 0 };
	std::vector<uint32_t> descriptorTableSize;
	descriptorTableSize.reserve(16);

	// Calculate hash
	size_t hashCode = Utility::HashState(&d3d12RootSignatureDesc.Version);
	hashCode = Utility::HashState(&d3d12RootSignatureDesc.Desc_1_1.Flags, 1, hashCode);

	for (uint32_t param = 0; param < d3d12RootSignatureDesc.Desc_1_1.NumParameters; ++param)
	{
		const D3D12_ROOT_PARAMETER1& rootParam = d3d12RootSignatureDesc.Desc_1_1.pParameters[param];
		descriptorTableSize.push_back(0);

		if (rootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			assert(rootParam.DescriptorTable.pDescriptorRanges != nullptr);

			hashCode = Utility::HashState(rootParam.DescriptorTable.pDescriptorRanges,
				rootParam.DescriptorTable.NumDescriptorRanges, hashCode);

			// We keep track of sampler descriptor tables separately from CBV_SRV_UAV descriptor tables
			if (rootParam.DescriptorTable.pDescriptorRanges->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
			{
				samplerTableBitmap |= (1 << param);
			}
			else
			{
				descriptorTableBitmap |= (1 << param);
			}

			for (uint32_t tableRange = 0; tableRange < rootParam.DescriptorTable.NumDescriptorRanges; ++tableRange)
			{
				descriptorTableSize[param] += rootParam.DescriptorTable.pDescriptorRanges[tableRange].NumDescriptors;
			}
		}
		else
		{
			hashCode = Utility::HashState(&rootParam, 1, hashCode);
		}
	}

	ID3D12RootSignature** ppRootSignature{ nullptr };
	ID3D12RootSignature* pRootSignature{ nullptr };
	bool firstCompile = false;
	{
		lock_guard<mutex> CS(m_rootSignatureMutex);
		auto iter = m_rootSignatureHashMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_rootSignatureHashMap.end())
		{
			ppRootSignature = m_rootSignatureHashMap[hashCode].addressof();
			firstCompile = true;
		}
		else
		{
			ppRootSignature = iter->second.addressof();
		}
	}

	if (firstCompile)
	{
		wil::com_ptr<ID3DBlob> pOutBlob, pErrorBlob;

		assert_succeeded(D3D12SerializeVersionedRootSignature(&d3d12RootSignatureDesc, &pOutBlob, &pErrorBlob));

		assert_succeeded(m_device->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(),
			IID_PPV_ARGS(&pRootSignature)));

		SetDebugName(pRootSignature, rootSignatureDesc.name);

		m_rootSignatureHashMap[hashCode].attach(pRootSignature);
		assert(*ppRootSignature == pRootSignature);
	}
	else
	{
		while (*ppRootSignature == nullptr)
		{
			this_thread::yield();
		}
		pRootSignature = *ppRootSignature;
	}

	RootSignatureData rootSignatureData{
		.rootSignature			= pRootSignature,
		.descriptorTableBitmap	= descriptorTableBitmap,
		.samplerTableBitmap		= samplerTableBitmap,
		.descriptorTableSizes	= descriptorTableSize
	};

	return rootSignatureData;
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