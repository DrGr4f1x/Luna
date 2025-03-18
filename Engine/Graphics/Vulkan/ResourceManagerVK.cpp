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

#include "ResourceManagerVK.h"

#include "FileSystem.h"
#include "DescriptorSetManagerVK.h"
#include "VulkanUtil.h"


using namespace std;


namespace Luna::VK
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
		// Try .spirv extension
		shaderFileWithExtension = shaderFilename + ".spirv";
		exists = fileSystem->Exists(shaderFileWithExtension);
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


void FillShaderStageCreateInfo(VkPipelineShaderStageCreateInfo& createInfo, VkShaderModule shaderModule, const Shader* shader)
{
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.stage = ShaderTypeToVulkan(shader->GetShaderType());
	createInfo.pName = shader->GetEntry().c_str();
	createInfo.module = shaderModule;
	createInfo.pSpecializationInfo = nullptr;
}


inline VkDescriptorType RootParameterTypeToVulkanDescriptorType(RootParameterType rootParameterType)
{
	switch (rootParameterType)
	{
	case RootParameterType::RootCBV: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	case RootParameterType::RootSRV:
	case RootParameterType::RootUAV: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	default:						 return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}


inline uint32_t GetDescriptorOffset(VulkanBindingOffsets bindingOffsets, VkDescriptorType descriptorType)
{
	switch (descriptorType)
	{
	case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
	case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
	case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
	case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:	return bindingOffsets.shaderResource;

	case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
	case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:		return bindingOffsets.unorderedAccess;

	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:		return bindingOffsets.constantBuffer;

	case VK_DESCRIPTOR_TYPE_SAMPLER:					return bindingOffsets.sampler;
	}
	return 0;
}


ResourceManager::ResourceManager(CVkDevice* device, CVmaAllocator* allocator)
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

	m_pipelineCache = CreatePipelineCache();

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
	auto [resourceData, gpuBufferData] = CreateGpuBuffer_Internal(gpuBufferDesc);

	m_gpuBufferCache.AddData(gpuBufferIndex, gpuBufferDesc, gpuBufferData);
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
	auto graphicsPipelineData = CreateGraphicsPipeline_Internal(pipelineDesc);

	m_graphicsPipelineCache.AddData(graphicsPipelineIndex, pipelineDesc, graphicsPipelineData);

	return Create<ResourceHandleType>(graphicsPipelineIndex, ManagedGraphicsPipeline, this);
}


ResourceHandle ResourceManager::CreateRootSignature(const RootSignatureDesc& rootSignatureDesc)
{
	std::lock_guard guard(m_allocationMutex);

	// Get a root signature index allocation
	assert(!m_rootSignatureCache.freeList.empty());
	uint32_t rootSignatureIndex = m_rootSignatureCache.freeList.front();
	m_rootSignatureCache.freeList.pop();

	// Allocate the root signature 
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
		LogError(LogVulkan) << "ResourceManager: Attempting to destroy handle of unknown type " << type << endl;
		break;
	}

	// Finally, mark the resource index as unallocated
	if (isResourceHandle)
	{
		m_resourceIndices[index] = InvalidAllocation;
		m_resourceFreeList.push(index);
		m_resourceData[index] = ResourceData{};
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
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedRootSignature);

	const auto& rootSignatureDesc = GetRootSignatureDesc(handle);
	const auto& data = m_rootSignatureCache.dataArray[resourceIndex];

	const auto& rootParam = rootSignatureDesc.rootParameters[rootParamIndex];

	const bool isDynamicBuffer = rootParam.parameterType == RootParameterType::RootCBV ||
		rootParam.parameterType == RootParameterType::RootSRV ||
		rootParam.parameterType == RootParameterType::RootUAV;

	DescriptorSetDesc descriptorSetDesc{
		.descriptorSetLayout	= data.descriptorSetLayouts[rootParamIndex].get(),
		.rootParameter			= rootParam,
		.bindingOffsets			= rootSignatureDesc.bindingOffsets,
		.numDescriptors			= rootParam.GetNumDescriptors(),
		.isDynamicBuffer		= isDynamicBuffer
	};

	return GetVulkanDescriptorSetManager()->CreateDescriptorSet(descriptorSetDesc);
}


void ResourceManager::Update(ResourceHandleType* handle, size_t sizeInBytes, size_t offset, const void* data) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert((sizeInBytes + offset) <= (m_gpuBufferCache.descArray[resourceIndex].elementSize * m_gpuBufferCache.descArray[resourceIndex].elementCount));
	assert(HasFlag(m_gpuBufferCache.descArray[resourceIndex].memoryAccess, MemoryAccess::CpuWrite));

	CVkBuffer* buffer = m_resourceData[index].buffer.get();

	// Map uniform buffer and update it
	uint8_t* pData = nullptr;
	ThrowIfFailed(vmaMapMemory(buffer->GetAllocator(), buffer->GetAllocation(), (void**)&pData));
	memcpy(pData + offset, data, sizeInBytes);
	// Unmap after data has been copied
	// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
	vmaUnmapMemory(buffer->GetAllocator(), buffer->GetAllocation());
}


ResourceHandle ResourceManager::CreateColorBufferFromSwapChainImage(CVkImage* swapChainImage, uint32_t width, uint32_t height, Format format, uint32_t imageIndex)
{
	const string name = std::format("Primary Swapchain Image {}", imageIndex);

	// Swapchain image
	ColorBufferDesc colorBufferDesc{
		.name				= name,
		.resourceType		= ResourceType::Texture2D,
		.width				= width,
		.height				= height,
		.arraySizeOrDepth	= 1,
		.numSamples			= 1,
		.format				= format
	};

	SetDebugName(*m_device, swapChainImage->Get(), name);

	// RTV view
	ImageViewDesc imageViewDesc{
		.image				= swapChainImage,
		.name				= std::format("Primary Swapchain {} RTV Image View", imageIndex),
		.resourceType		= ResourceType::Texture2D,
		.imageUsage			= GpuImageUsage::RenderTarget,
		.format				= format,
		.imageAspect		= ImageAspect::Color,
		.baseMipLevel		= 0,
		.mipCount			= 1,
		.baseArraySlice		= 0,
		.arraySize			= 1
	};

	auto imageViewRtv = CreateImageView(m_device.get(), imageViewDesc);

	// SRV view
	imageViewDesc
		.SetImageUsage(GpuImageUsage::ShaderResource)
		.SetName(std::format("Primary SwapChain {} SRV Image View", imageIndex));

	auto imageViewSrv = CreateImageView(m_device.get(), imageViewDesc);

	// Descriptors
	VkDescriptorImageInfo imageInfoSrv{ VK_NULL_HANDLE, *imageViewSrv, GetImageLayout(ResourceState::ShaderResource) };
	VkDescriptorImageInfo imageInfoUav{ VK_NULL_HANDLE, *imageViewSrv, GetImageLayout(ResourceState::UnorderedAccess) };

	ResourceData resourceData{
		.image			= swapChainImage,
		.buffer			= nullptr,
		.usageState		= ResourceState::Undefined
	};

	ColorBufferData colorBufferData{
		.imageViewRtv = imageViewRtv.get(),
		.imageViewSrv = imageViewSrv.get(),
		.imageInfoSrv = imageInfoSrv,
		.imageInfoUav = imageInfoUav
	};

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

	m_colorBufferCache.descArray[colorBufferIndex] = colorBufferDesc;
	m_colorBufferCache.dataArray[colorBufferIndex] = colorBufferData;
	m_resourceData[resourceIndex] = resourceData;

	return Create<ResourceHandleType>(resourceIndex, ManagedColorBuffer, this);
}


VkImage ResourceManager::GetImage(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedColorBuffer || type == ManagedDepthBuffer);

	return m_resourceData[index].image->Get();
}


VkBuffer ResourceManager::GetBuffer(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedGpuBuffer);

	return m_resourceData[index].buffer->Get();
}


VkImageView ResourceManager::GetImageViewSrv(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedColorBuffer);

	return m_colorBufferCache.dataArray[resourceIndex].imageViewSrv->Get();
}


VkImageView ResourceManager::GetImageViewRtv(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedColorBuffer);

	return m_colorBufferCache.dataArray[resourceIndex].imageViewRtv->Get();
}


VkImageView ResourceManager::GetImageViewDepth(ResourceHandleType* handle, DepthStencilAspect depthStencilAspect) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedDepthBuffer);

	switch (depthStencilAspect)
	{
	case DepthStencilAspect::DepthReadOnly:		return m_depthBufferCache.dataArray[resourceIndex].imageViewDepthOnly->Get();
	case DepthStencilAspect::StencilReadOnly:	return m_depthBufferCache.dataArray[resourceIndex].imageViewStencilOnly->Get();
	default:									return m_depthBufferCache.dataArray[resourceIndex].imageViewDepthStencil->Get();
	}
}


VkDescriptorImageInfo ResourceManager::GetImageInfoSrv(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedColorBuffer);

	return m_colorBufferCache.dataArray[resourceIndex].imageInfoSrv;
}


VkDescriptorImageInfo ResourceManager::GetImageInfoUav(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedColorBuffer);

	return m_colorBufferCache.dataArray[resourceIndex].imageInfoUav;
}


VkDescriptorImageInfo ResourceManager::GetImageInfoDepth(ResourceHandleType* handle, bool depthSrv) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedDepthBuffer);

	return depthSrv ? m_depthBufferCache.dataArray[resourceIndex].imageInfoDepth : m_depthBufferCache.dataArray[resourceIndex].imageInfoStencil;
}


VkDescriptorBufferInfo ResourceManager::GetBufferInfo(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedGpuBuffer);

	return m_gpuBufferCache.dataArray[resourceIndex].bufferInfo;
}


VkBufferView ResourceManager::GetBufferView(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedGpuBuffer);

	return m_gpuBufferCache.dataArray[resourceIndex].bufferView->Get();
}


VkPipeline ResourceManager::GetGraphicsPipeline(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedGraphicsPipeline);

	return m_graphicsPipelineCache.dataArray[resourceIndex]->Get();
}


VkPipelineLayout ResourceManager::GetPipelineLayout(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedRootSignature);

	return m_rootSignatureCache.dataArray[resourceIndex].pipelineLayout->Get();
}


CVkDescriptorSetLayout* ResourceManager::GetDescriptorSetLayout(ResourceHandleType* handle, uint32_t paramIndex) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedRootSignature);

	int descriptorSetIndex = GetDescriptorSetIndexFromRootParameterIndex(handle, paramIndex);

	if (descriptorSetIndex != -1)
	{
		return m_rootSignatureCache.dataArray[resourceIndex].descriptorSetLayouts[descriptorSetIndex].get();
	}

	return VK_NULL_HANDLE;
}


int ResourceManager::GetDescriptorSetIndexFromRootParameterIndex(ResourceHandleType* handle, uint32_t paramIndex) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedRootSignature);

	const auto& paramToSetMap = m_rootSignatureCache.dataArray[resourceIndex].rootParameterIndexToDescriptorSetMap;
	auto it = paramToSetMap.find(paramIndex);
	if (it != paramToSetMap.end())
	{
		return it->second;
	}

	return -1;
}


const std::vector<DescriptorBindingDesc>& ResourceManager::GetLayoutBindings(ResourceHandleType* handle, uint32_t paramIndex) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedRootSignature);

	auto it = m_rootSignatureCache.dataArray[resourceIndex].layoutBindingMap.find(paramIndex);
	assert(it != m_rootSignatureCache.dataArray[resourceIndex].layoutBindingMap.end());

	return it->second;
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
	// Create image
	ImageDesc imageDesc{
		.name				= colorBufferDesc.name,
		.width				= colorBufferDesc.width,
		.height				= colorBufferDesc.height,
		.arraySizeOrDepth	= colorBufferDesc.arraySizeOrDepth,
		.format				= colorBufferDesc.format,
		.numMips			= colorBufferDesc.numMips,
		.numSamples			= colorBufferDesc.numSamples,
		.resourceType		= colorBufferDesc.resourceType,
		.imageUsage			= GpuImageUsage::ColorBuffer,
		.memoryAccess		= MemoryAccess::GpuReadWrite
	};

	if (HasFlag(colorBufferDesc.resourceType, ResourceType::Texture3D))
	{
		imageDesc.SetNumMips(1);
		imageDesc.SetDepth(colorBufferDesc.arraySizeOrDepth);
	}
	else if (HasAnyFlag(colorBufferDesc.resourceType, ResourceType::Texture2D_Type))
	{
		if (HasAnyFlag(colorBufferDesc.resourceType, ResourceType::TextureArray_Type))
		{
			imageDesc.SetResourceType(colorBufferDesc.numSamples == 1 ? ResourceType::Texture2D_Array : ResourceType::Texture2DMS_Array);
			imageDesc.SetArraySize(colorBufferDesc.arraySizeOrDepth);
		}
		else
		{
			imageDesc.SetResourceType(colorBufferDesc.numSamples == 1 ? ResourceType::Texture2D : ResourceType::Texture2DMS_Array);
		}
	}

	auto image = CreateImage(m_device.get(), m_allocator.get(), imageDesc);

	// Render target view
	ImageViewDesc imageViewDesc{
		.image				= image.get(),
		.name				= colorBufferDesc.name,
		.resourceType		= ResourceType::Texture2D,
		.imageUsage			= GpuImageUsage::RenderTarget,
		.format				= colorBufferDesc.format,
		.imageAspect		= ImageAspect::Color,
		.baseMipLevel		= 0,
		.mipCount			= colorBufferDesc.numMips,
		.baseArraySlice		= 0,
		.arraySize			= colorBufferDesc.arraySizeOrDepth
	};
	auto imageViewRtv = CreateImageView(m_device.get(), imageViewDesc);

	// Shader resource view
	imageViewDesc.SetImageUsage(GpuImageUsage::ShaderResource);
	auto imageViewSrv = CreateImageView(m_device.get(), imageViewDesc);

	// Descriptors
	VkDescriptorImageInfo imageInfoSrv{
		.sampler		= VK_NULL_HANDLE,
		.imageView		= *imageViewSrv,
		.imageLayout	= GetImageLayout(ResourceState::ShaderResource)
	};
	VkDescriptorImageInfo imageInfoUav{
		.sampler		= VK_NULL_HANDLE,
		.imageView		= *imageViewSrv,
		.imageLayout	= GetImageLayout(ResourceState::UnorderedAccess)
	};

	ResourceData resourceData{
		.image			= image.get(),
		.buffer			= nullptr,
		.usageState		= ResourceState::Common
	};

	ColorBufferData colorBufferData{
		.imageViewRtv = imageViewRtv.get(),
		.imageViewSrv = imageViewSrv.get(),
		.imageInfoSrv = imageInfoSrv,
		.imageInfoUav = imageInfoUav
	};

	return make_pair(resourceData, colorBufferData);
}


pair<ResourceData, DepthBufferData> ResourceManager::CreateDepthBuffer_Internal(const DepthBufferDesc& depthBufferDesc)
{
	// Create depth image
	ImageDesc imageDesc{
		.name				= depthBufferDesc.name,
		.width				= depthBufferDesc.width,
		.height				= depthBufferDesc.height,
		.arraySizeOrDepth	= depthBufferDesc.arraySizeOrDepth,
		.format				= depthBufferDesc.format,
		.numMips			= depthBufferDesc.numMips,
		.numSamples			= depthBufferDesc.numSamples,
		.resourceType		= depthBufferDesc.resourceType,
		.imageUsage			= GpuImageUsage::DepthBuffer,
		.memoryAccess		= MemoryAccess::GpuReadWrite
	};

	auto image = CreateImage(m_device.get(), m_allocator.get(), imageDesc);

	// Create image views and descriptors
	const bool bHasStencil = IsStencilFormat(depthBufferDesc.format);

	auto imageAspect = ImageAspect::Depth;
	if (bHasStencil)
	{
		imageAspect |= ImageAspect::Stencil;
	}

	ImageViewDesc imageViewDesc{
		.image				= image.get(),
		.name				= depthBufferDesc.name,
		.resourceType		= ResourceType::Texture2D,
		.imageUsage			= GpuImageUsage::DepthStencilTarget,
		.format				= depthBufferDesc.format,
		.imageAspect		= imageAspect,
		.baseMipLevel		= 0,
		.mipCount			= depthBufferDesc.numMips,
		.baseArraySlice		= 0,
		.arraySize			= depthBufferDesc.arraySizeOrDepth
	};

	auto imageViewDepthStencil = CreateImageView(m_device.get(), imageViewDesc);
	wil::com_ptr<CVkImageView> imageViewDepthOnly;
	wil::com_ptr<CVkImageView> imageViewStencilOnly;

	if (bHasStencil)
	{
		imageViewDesc
			.SetName(format("{} Depth Image View", depthBufferDesc.name))
			.SetImageAspect(ImageAspect::Depth)
			.SetViewType(TextureSubresourceViewType::DepthOnly);

		imageViewDepthOnly = CreateImageView(m_device.get(), imageViewDesc);

		imageViewDesc
			.SetName(format("{} Stencil Image View", depthBufferDesc.name))
			.SetImageAspect(ImageAspect::Stencil)
			.SetViewType(TextureSubresourceViewType::StencilOnly);

		imageViewStencilOnly = CreateImageView(m_device.get(), imageViewDesc);
	}
	else
	{
		imageViewDepthOnly = imageViewDepthStencil;
		imageViewStencilOnly = imageViewDepthStencil;
	}

	auto imageInfoDepth = VkDescriptorImageInfo{
		.sampler		= VK_NULL_HANDLE,
		.imageView		= *imageViewDepthOnly,
		.imageLayout	= GetImageLayout(ResourceState::ShaderResource)
	};
	auto imageInfoStencil = VkDescriptorImageInfo{
		.sampler		= VK_NULL_HANDLE,
		.imageView		= *imageViewStencilOnly,
		.imageLayout	= GetImageLayout(ResourceState::ShaderResource)
	};

	ResourceData resourceData{
		.image			= image.get(),
		.buffer			= nullptr,
		.usageState		= ResourceState::Undefined
	};

	DepthBufferData depthBufferData{
		.imageViewDepthStencil	= imageViewDepthStencil.get(),
		.imageViewDepthOnly		= imageViewDepthOnly.get(),
		.imageViewStencilOnly	= imageViewStencilOnly.get(),
		.imageInfoDepth			= imageInfoDepth,
		.imageInfoStencil		= imageInfoStencil
	};

	return make_pair(resourceData, depthBufferData);
}


pair<ResourceData, GpuBufferData> ResourceManager::CreateGpuBuffer_Internal(const GpuBufferDesc& gpuBufferDesc)
{
	constexpr VkBufferUsageFlags transferFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VkBufferCreateInfo bufferCreateInfo{
		.sType	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size	= gpuBufferDesc.elementCount * gpuBufferDesc.elementSize,
		.usage	= GetBufferUsageFlags(gpuBufferDesc.resourceType) | transferFlags
	};

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.flags = GetMemoryFlags(gpuBufferDesc.memoryAccess);
	allocCreateInfo.usage = GetMemoryUsage(gpuBufferDesc.memoryAccess);

	VkBuffer vkBuffer{ VK_NULL_HANDLE };
	VmaAllocation vmaBufferAllocation{ VK_NULL_HANDLE };

	auto res = vmaCreateBuffer(*m_allocator, &bufferCreateInfo, &allocCreateInfo, &vkBuffer, &vmaBufferAllocation, nullptr);
	assert(res == VK_SUCCESS);

	SetDebugName(*m_device, vkBuffer, gpuBufferDesc.name);

	auto buffer = Create<CVkBuffer>(m_device.get(), m_allocator.get(), vkBuffer, vmaBufferAllocation);

	wil::com_ptr<CVkBufferView> bufferView;
	if (gpuBufferDesc.resourceType == ResourceType::TypedBuffer)
	{
		VkBufferViewCreateInfo bufferViewCreateInfo{
			.sType		= VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
			.buffer		= vkBuffer,
			.format		= FormatToVulkan(gpuBufferDesc.format),
			.offset		= 0,
			.range		= VK_WHOLE_SIZE
		};
		VkBufferView vkBufferView{ VK_NULL_HANDLE };
		vkCreateBufferView(*m_device, &bufferViewCreateInfo, nullptr, &vkBufferView);
		bufferView = Create<CVkBufferView>(m_device.get(), vkBufferView);
	}

	ResourceData resourceData{
		.image			= nullptr,
		.buffer			= buffer,
		.usageState		= ResourceState::Common
	};

	GpuBufferData gpuBufferData{
		.bufferView		= bufferView,
		.bufferInfo		= { .buffer = vkBuffer, .offset = 0, .range = VK_WHOLE_SIZE	}
	};

	return make_pair(resourceData, gpuBufferData);
}


wil::com_ptr<CVkPipeline> ResourceManager::CreateGraphicsPipeline_Internal(const GraphicsPipelineDesc& pipelineDesc)
{
	// Shaders
	vector<VkPipelineShaderStageCreateInfo> shaderStages;

	auto AddShaderStageCreateInfo = [this, &shaderStages](ShaderType type, const ShaderNameAndEntry& shaderAndEntry)
		{
			if (shaderAndEntry)
			{
				Shader* shader = LoadShader(type, shaderAndEntry);
				assert(shader);
				auto shaderModule = CreateShaderModule(shader);

				VkPipelineShaderStageCreateInfo createInfo{};
				FillShaderStageCreateInfo(createInfo, *shaderModule, shader);
				shaderStages.push_back(createInfo);
			}
		};

	AddShaderStageCreateInfo(ShaderType::Vertex, pipelineDesc.vertexShader);
	AddShaderStageCreateInfo(ShaderType::Pixel, pipelineDesc.pixelShader);
	AddShaderStageCreateInfo(ShaderType::Geometry, pipelineDesc.geometryShader);
	AddShaderStageCreateInfo(ShaderType::Hull, pipelineDesc.hullShader);
	AddShaderStageCreateInfo(ShaderType::Domain, pipelineDesc.domainShader);

	// Vertex streams
	vector<VkVertexInputBindingDescription> vertexInputBindings;
	const auto& vertexStreams = pipelineDesc.vertexStreams;
	const uint32_t numStreams = (uint32_t)vertexStreams.size();
	if (numStreams > 0)
	{
		vertexInputBindings.resize(numStreams);
		for (uint32_t i = 0; i < numStreams; ++i)
		{
			vertexInputBindings[i].binding = vertexStreams[i].inputSlot;
			vertexInputBindings[i].inputRate = InputClassificationToVulkan(vertexStreams[i].inputClassification);
			vertexInputBindings[i].stride = vertexStreams[i].stride;
		}
	}

	// Vertex elements
	vector<VkVertexInputAttributeDescription> vertexAttributes;
	const auto& vertexElements = pipelineDesc.vertexElements;
	const uint32_t numElements = (uint32_t)vertexElements.size();
	if (numElements > 0)
	{
		vertexAttributes.resize(numElements);
		for (uint32_t i = 0; i < numElements; ++i)
		{
			vertexAttributes[i].binding = vertexElements[i].inputSlot;
			vertexAttributes[i].location = i;
			vertexAttributes[i].format = FormatToVulkan(vertexElements[i].format);
			vertexAttributes[i].offset = vertexElements[i].alignedByteOffset;
		}
	}

	// Vertex input layout
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{
		.sType								= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount		= (uint32_t)vertexInputBindings.size(),
		.pVertexBindingDescriptions			= vertexInputBindings.data(),
		.vertexAttributeDescriptionCount	= (uint32_t)vertexAttributes.size(),
		.pVertexAttributeDescriptions		= vertexAttributes.data()
	};

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
		.sType						= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology					= PrimitiveTopologyToVulkan(pipelineDesc.topology),
		.primitiveRestartEnable		= pipelineDesc.indexBufferStripCut == IndexBufferStripCutValue::Disabled ? VK_FALSE : VK_TRUE
	};

	// Tessellation state
	VkPipelineTessellationStateCreateInfo tessellationInfo{
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		.patchControlPoints		= GetControlPointCount(pipelineDesc.topology)
	};

	// Viewport state
	VkPipelineViewportStateCreateInfo viewportStateInfo{
		.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount	= 1,
		.pViewports		= nullptr, // dynamic state
		.scissorCount	= 1,
		.pScissors		= nullptr  // dynamic state
	};

	// Rasterizer state
	const auto rasterizerState = pipelineDesc.rasterizerState;
	VkPipelineRasterizationStateCreateInfo rasterizerInfo
	{
		.sType						= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable			= VK_FALSE,
		.rasterizerDiscardEnable	= VK_FALSE,
		.polygonMode				= FillModeToVulkan(rasterizerState.fillMode),
		.cullMode					= CullModeToVulkan(rasterizerState.cullMode),
		.frontFace					= rasterizerState.frontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable			= (rasterizerState.depthBias != 0 || rasterizerState.slopeScaledDepthBias != 0.0f) ? VK_TRUE : VK_FALSE,
		.depthBiasConstantFactor	= *reinterpret_cast<const float*>(&rasterizerState.depthBias),
		.depthBiasClamp				= rasterizerState.depthBiasClamp,
		.depthBiasSlopeFactor		= rasterizerState.slopeScaledDepthBias,
		.lineWidth					= 1.0f
	};

	// Multisample state
	VkPipelineMultisampleStateCreateInfo multisampleInfo{
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples	= GetSampleCountFlags(pipelineDesc.msaaCount),
		.sampleShadingEnable	= VK_FALSE,
		.minSampleShading		= 0.0f,
		.pSampleMask			= nullptr,
		.alphaToCoverageEnable	= pipelineDesc.blendState.alphaToCoverageEnable ? VK_TRUE : VK_FALSE,
		.alphaToOneEnable		= VK_FALSE
	};

	// Depth stencil state
	const auto& depthStencilState = pipelineDesc.depthStencilState;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable		= depthStencilState.depthEnable ? VK_TRUE : VK_FALSE,
		.depthWriteEnable		= depthStencilState.depthWriteMask == DepthWrite::All ? VK_TRUE : VK_FALSE,
		.depthCompareOp			= ComparisonFuncToVulkan(depthStencilState.depthFunc),
		.depthBoundsTestEnable	= VK_FALSE,
		.stencilTestEnable		= depthStencilState.stencilEnable ? VK_TRUE : VK_FALSE,
		.front = {
			.failOp			= StencilOpToVulkan(depthStencilState.frontFace.stencilFailOp),
			.passOp			= StencilOpToVulkan(depthStencilState.frontFace.stencilPassOp),
			.depthFailOp	= StencilOpToVulkan(depthStencilState.frontFace.stencilDepthFailOp),
			.compareOp		= ComparisonFuncToVulkan(depthStencilState.frontFace.stencilFunc),
			.compareMask	= depthStencilState.stencilReadMask,
			.writeMask		= depthStencilState.stencilWriteMask,
			.reference		= 0
		},
		.back = {
			.failOp			= StencilOpToVulkan(depthStencilState.backFace.stencilFailOp),
			.passOp			= StencilOpToVulkan(depthStencilState.backFace.stencilPassOp),
			.depthFailOp	= StencilOpToVulkan(depthStencilState.backFace.stencilDepthFailOp),
			.compareOp		= ComparisonFuncToVulkan(depthStencilState.backFace.stencilFunc),
			.compareMask	= depthStencilState.stencilReadMask,
			.writeMask		= depthStencilState.stencilWriteMask,
			.reference		= 0
		},
		.minDepthBounds			= 0.0f,
		.maxDepthBounds			= 1.0f
	};

	// Blend state
	const auto& blendState = pipelineDesc.blendState;
	VkPipelineColorBlendStateCreateInfo blendStateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };

	array<VkPipelineColorBlendAttachmentState, 8> blendAttachments;

	for (uint32_t i = 0; i < 8; ++i)
	{
		const auto& rt = blendState.renderTargetBlend[i];
		blendAttachments[i].blendEnable = rt.blendEnable ? VK_TRUE : VK_FALSE;
		blendAttachments[i].srcColorBlendFactor = BlendToVulkan(rt.srcBlend);
		blendAttachments[i].dstColorBlendFactor = BlendToVulkan(rt.dstBlend);
		blendAttachments[i].colorBlendOp = BlendOpToVulkan(rt.blendOp);
		blendAttachments[i].srcAlphaBlendFactor = BlendToVulkan(rt.srcBlendAlpha);
		blendAttachments[i].dstAlphaBlendFactor = BlendToVulkan(rt.dstBlendAlpha);
		blendAttachments[i].alphaBlendOp = BlendOpToVulkan(rt.blendOpAlpha);
		blendAttachments[i].colorWriteMask = ColorWriteToVulkan(rt.writeMask);

		// First render target with logic op enabled gets to set the state
		if (rt.logicOpEnable && (VK_FALSE == blendStateInfo.logicOpEnable))
		{
			blendStateInfo.logicOpEnable = VK_TRUE;
			blendStateInfo.logicOp = LogicOpToVulkan(rt.logicOp);
		}
	}
	const uint32_t numRtvs = (uint32_t)pipelineDesc.rtvFormats.size();
	blendStateInfo.attachmentCount = numRtvs;
	blendStateInfo.pAttachments = blendAttachments.data();

	// Dynamic states
	vector<VkDynamicState> dynamicStates;
	dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
	dynamicStates.push_back(VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY);

	VkPipelineDynamicStateCreateInfo dynamicStateInfo{
		.sType				= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount	= (uint32_t)dynamicStates.size(),
		.pDynamicStates		= dynamicStates.data()
	};

	// TODO: Check for dynamic rendering support here.  Will need proper extension/feature system.
	vector<VkFormat> rtvFormats(numRtvs);
	if (numRtvs > 0)
	{
		for (uint32_t i = 0; i < numRtvs; ++i)
		{
			rtvFormats[i] = FormatToVulkan(pipelineDesc.rtvFormats[i]);
		}
	}

	const Format dsvFormat = pipelineDesc.dsvFormat;
	VkPipelineRenderingCreateInfo dynamicRenderingInfo{
		.sType						= VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.viewMask					= 0,
		.colorAttachmentCount		= (uint32_t)rtvFormats.size(),
		.pColorAttachmentFormats	= rtvFormats.data(),
		.depthAttachmentFormat		= FormatToVulkan(dsvFormat),
		.stencilAttachmentFormat	= IsStencilFormat(dsvFormat) ? FormatToVulkan(dsvFormat) : VK_FORMAT_UNDEFINED
	};

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{
		.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext					= &dynamicRenderingInfo,
		.stageCount				= (uint32_t)shaderStages.size(),
		.pStages				= shaderStages.data(),
		.pVertexInputState		= &vertexInputInfo,
		.pInputAssemblyState	= &inputAssemblyInfo,
		.pTessellationState		= &tessellationInfo,
		.pViewportState			= &viewportStateInfo,
		.pRasterizationState	= &rasterizerInfo,
		.pMultisampleState		= &multisampleInfo,
		.pDepthStencilState		= &depthStencilInfo,
		.pColorBlendState		= &blendStateInfo,
		.pDynamicState			= &dynamicStateInfo,
		.layout					= GetPipelineLayout(pipelineDesc.rootSignature.get()),
		.renderPass				= VK_NULL_HANDLE,
		.subpass				= 0,
		.basePipelineHandle		= VK_NULL_HANDLE,
		.basePipelineIndex		= 0
	};

	VkPipeline vkPipeline{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkCreateGraphicsPipelines(*m_device, *m_pipelineCache, 1, &pipelineCreateInfo, nullptr, &vkPipeline)))
	{
		auto pipeline = Create<CVkPipeline>(m_device.get(), vkPipeline);
		return pipeline;
	}
	else
	{
		LogError(LogVulkan) << "Failed to create VkPipeline (graphics).  Error code: " << res << endl;
	}

	return nullptr;
}


RootSignatureData ResourceManager::CreateRootSignature_Internal(const RootSignatureDesc& rootSignatureDesc)
{
	// Validate RootSignatureDesc
	if (!rootSignatureDesc.Validate())
	{
		LogError(LogVulkan) << "RootSignature is not valid!" << endl;
		return RootSignatureData{};
	}

	std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts;
	std::vector<wil::com_ptr<CVkDescriptorSetLayout>> descriptorSetLayouts;
	std::vector<VkPushConstantRange> vkPushConstantRanges;
	std::unordered_map<uint32_t, std::vector<DescriptorBindingDesc>> layoutBindingMap;
	std::unordered_map<uint32_t, uint32_t> rootParameterIndexToDescriptorSetMap;
	uint32_t pushConstantOffset{ 0 };

	size_t hashCode = Utility::g_hashStart;

	uint32_t rootParamIndex = 0;
	for (const auto& rootParameter : rootSignatureDesc.rootParameters)
	{
		std::vector<VkDescriptorSetLayoutBinding> vkLayoutBindings;
		std::vector<DescriptorBindingDesc> layoutBindingDescs;

		VkShaderStageFlags shaderStageFlags = ShaderStageToVulkan(rootParameter.shaderVisibility);

		bool createLayout = true;
		uint32_t offset = 0;

		if (rootParameter.parameterType == RootParameterType::RootConstants)
		{
			VkPushConstantRange& vkPushConstantRange = vkPushConstantRanges.emplace_back();
			vkPushConstantRange.offset = pushConstantOffset;
			vkPushConstantRange.size = rootParameter.num32BitConstants * 4;
			vkPushConstantRange.stageFlags = shaderStageFlags;
			pushConstantOffset += rootParameter.num32BitConstants * 4;

			hashCode = Utility::HashState(&vkPushConstantRange, 1, hashCode);

			createLayout = false;
		}
		else if (rootParameter.parameterType == RootParameterType::RootCBV ||
			rootParameter.parameterType == RootParameterType::RootSRV ||
			rootParameter.parameterType == RootParameterType::RootUAV)
		{
			VkDescriptorSetLayoutBinding& vkBinding = vkLayoutBindings.emplace_back();
			vkBinding.descriptorType = RootParameterTypeToVulkanDescriptorType(rootParameter.parameterType);

			offset = GetDescriptorOffset(rootSignatureDesc.bindingOffsets, vkBinding.descriptorType);

			vkBinding.binding = rootParameter.startRegister + offset;
			vkBinding.descriptorCount = 1;
			vkBinding.stageFlags = shaderStageFlags;
			vkBinding.pImmutableSamplers = nullptr;

			hashCode = Utility::HashState(&vkBinding, 1, hashCode);

			DescriptorBindingDesc& bindingDesc = layoutBindingDescs.emplace_back();
			bindingDesc.descriptorType = vkBinding.descriptorType;
			bindingDesc.startSlot = rootParameter.startRegister;
			bindingDesc.numDescriptors = vkBinding.descriptorCount;
			bindingDesc.offset = offset;
		}
		else if (rootParameter.parameterType == RootParameterType::Table)
		{
			for (const auto& range : rootParameter.table)
			{
				VkDescriptorSetLayoutBinding& vkBinding = vkLayoutBindings.emplace_back();
				vkBinding.descriptorCount = range.numDescriptors;
				vkBinding.descriptorType = DescriptorTypeToVulkan(range.descriptorType);

				offset = GetDescriptorOffset(rootSignatureDesc.bindingOffsets, vkBinding.descriptorType);

				vkBinding.binding = range.startRegister + offset;
				vkBinding.stageFlags = shaderStageFlags;
				vkBinding.pImmutableSamplers = nullptr;

				hashCode = Utility::HashState(&vkBinding, 1, hashCode);

				DescriptorBindingDesc& bindingDesc = layoutBindingDescs.emplace_back();
				bindingDesc.descriptorType = vkBinding.descriptorType;
				bindingDesc.startSlot = rootParameter.startRegister;
				bindingDesc.numDescriptors = vkBinding.descriptorCount;
				bindingDesc.offset = offset;
			}
		}

		if (createLayout)
		{

			VkDescriptorSetLayoutCreateInfo createInfo{
				.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.bindingCount	= (uint32_t)vkLayoutBindings.size(),
				.pBindings		= vkLayoutBindings.data()
			};

			VkDescriptorSetLayout vkDescriptorSetLayout{ VK_NULL_HANDLE };
			vkCreateDescriptorSetLayout(*m_device, &createInfo, nullptr, &vkDescriptorSetLayout);
			vkDescriptorSetLayouts.push_back(vkDescriptorSetLayout);

			wil::com_ptr<CVkDescriptorSetLayout> descriptorSetLayout = Create<CVkDescriptorSetLayout>(m_device.get(), vkDescriptorSetLayout);
			descriptorSetLayouts.push_back(descriptorSetLayout);

			layoutBindingMap[rootParamIndex] = layoutBindingDescs;
			rootParameterIndexToDescriptorSetMap[rootParamIndex] = (uint32_t)descriptorSetLayouts.size() - 1;
		}

		++rootParamIndex;
	}

	CVkPipelineLayout** ppPipelineLayout;
	bool firstCompile = false;
	{
		lock_guard<mutex> CS(m_pipelineLayoutMutex);

		auto iter = m_pipelineLayoutHashMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_pipelineLayoutHashMap.end())
		{
			ppPipelineLayout = m_pipelineLayoutHashMap[hashCode].addressof();
			firstCompile = true;
		}
		else
		{
			ppPipelineLayout = &iter->second;
		}
	}

	if (firstCompile)
	{
		VkPipelineLayoutCreateInfo createInfo{
			.sType						= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount				= (uint32_t)vkDescriptorSetLayouts.size(),
			.pSetLayouts				= vkDescriptorSetLayouts.data(),
			.pushConstantRangeCount		= (uint32_t)vkPushConstantRanges.size(),
			.pPushConstantRanges		= vkPushConstantRanges.data()
		};

		VkPipelineLayout vkPipelineLayout{ VK_NULL_HANDLE };
		vkCreatePipelineLayout(*m_device, &createInfo, nullptr, &vkPipelineLayout);

		wil::com_ptr<CVkPipelineLayout> pipelineLayout = Create<CVkPipelineLayout>(m_device.get(), vkPipelineLayout);

		*ppPipelineLayout = pipelineLayout.get();

		(*ppPipelineLayout)->AddRef();
	}

	RootSignatureData rootSignatureData{
		.pipelineLayout							= *ppPipelineLayout,
		.layoutBindingMap						= layoutBindingMap,
		.rootParameterIndexToDescriptorSetMap	= rootParameterIndexToDescriptorSetMap,
		.descriptorSetLayouts					= descriptorSetLayouts
	};

	return rootSignatureData;
}


wil::com_ptr<CVkShaderModule> ResourceManager::CreateShaderModule(Shader* shader)
{
	CVkShaderModule** ppShaderModule = nullptr;
	bool firstCompile = false;
	{
		lock_guard<mutex> CS(m_shaderModuleMutex);

		size_t hashCode = shader->GetHash();
		auto iter = m_shaderModuleHashMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_shaderModuleHashMap.end())
		{
			ppShaderModule = m_shaderModuleHashMap[hashCode].addressof();
			firstCompile = true;
		}
		else
		{
			ppShaderModule = &iter->second;
		}
	}

	if (firstCompile)
	{
		VkShaderModuleCreateInfo createInfo{
			.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize	= shader->GetByteCodeSize(),
			.pCode		= reinterpret_cast<const uint32_t*>(shader->GetByteCode())
		};

		VkShaderModule vkShaderModule{ VK_NULL_HANDLE };
		vkCreateShaderModule(*m_device, &createInfo, nullptr, &vkShaderModule);

		wil::com_ptr<CVkShaderModule> shaderModule = Create<CVkShaderModule>(m_device.get(), vkShaderModule);

		*ppShaderModule = shaderModule.get();

		(*ppShaderModule)->AddRef();
	}

	return *ppShaderModule;
}


wil::com_ptr<CVkPipelineCache> ResourceManager::CreatePipelineCache() const
{
	VkPipelineCacheCreateInfo createInfo{
		.sType				= VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
		.initialDataSize	= 0,
		.pInitialData		= nullptr
	};

	VkPipelineCache vkPipelineCache{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkCreatePipelineCache(*m_device, &createInfo, nullptr, &vkPipelineCache)))
	{
		return Create<CVkPipelineCache>(m_device.get(), vkPipelineCache);
	}
	else
	{
		LogError(LogVulkan) << "Failed to create VkPipelineCache.  Error code: " << res << endl;
	}

	return nullptr;
}


ResourceManager* const GetVulkanResourceManager()
{
	return g_resourceManager;
}

} // namespace Luna::VK