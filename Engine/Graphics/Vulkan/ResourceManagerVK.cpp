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

#include "VulkanUtil.h"

using namespace std;


namespace Luna::VK
{

ResourceManager* g_resourceManager{ nullptr };


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

		// ColorBuffer data
		m_colorBufferFreeList.push(i);
		m_colorBufferDescs[i] = ColorBufferDesc{};
		m_colorBufferData[i] = ColorBufferData{};

		// DepthBuffer data
		m_depthBufferFreeList.push(i);
		m_depthBufferDescs[i] = DepthBufferDesc{};
		m_depthBufferData[i] = DepthBufferData{};

		// GpuBuffer data
		m_gpuBufferFreeList.push(i);
		m_gpuBufferDescs[i] = GpuBufferDesc{};
		m_gpuBufferData[i] = GpuBufferData{};
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
	uint32_t colorBufferIndex = m_colorBufferFreeList.front();
	m_colorBufferFreeList.pop();

	// Make sure we don't already have a color buffer allocation
	assert(m_resourceIndices[resourceIndex] == InvalidAllocation);

	// Store the color buffer allocation index
	m_resourceIndices[resourceIndex] = colorBufferIndex;

	// Allocate the color buffer and resource
	auto [resourceData, colorBufferData] = CreateColorBuffer_Internal(colorBufferDesc);

	m_colorBufferDescs[colorBufferIndex] = colorBufferDesc;
	m_colorBufferData[colorBufferIndex] = colorBufferData;
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
	uint32_t depthBufferIndex = m_depthBufferFreeList.front();
	m_depthBufferFreeList.pop();

	// Make sure we don't already have a depth buffer allocation
	assert(m_resourceIndices[resourceIndex] == InvalidAllocation);

	// Store the depth buffer allocation index
	m_resourceIndices[resourceIndex] = depthBufferIndex;

	// Allocate the depth buffer and resource
	auto [resourceData, depthBufferData] = CreateDepthBuffer_Internal(depthBufferDesc);

	m_depthBufferDescs[depthBufferIndex] = depthBufferDesc;
	m_depthBufferData[depthBufferIndex] = depthBufferData;
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
	uint32_t gpuBufferIndex = m_gpuBufferFreeList.front();
	m_gpuBufferFreeList.pop();

	// Make sure we don't already have a gpu buffer allocation
	assert(m_resourceIndices[resourceIndex] == InvalidAllocation);

	// Store the gpu buffer allocation index
	m_resourceIndices[resourceIndex] = gpuBufferIndex;

	// Allocate the gpu buffer and resource
	auto [resourceData, gpuBufferData] = CreateGpuBuffer_Internal(gpuBufferDesc);

	m_gpuBufferDescs[gpuBufferIndex] = gpuBufferDesc;
	m_gpuBufferData[gpuBufferIndex] = gpuBufferData;
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
		m_colorBufferDescs[resourceIndex] = ColorBufferDesc{};
		m_colorBufferData[resourceIndex] = ColorBufferData{};
		m_colorBufferFreeList.push(resourceIndex);
		break;

	case ManagedDepthBuffer:
		m_depthBufferDescs[resourceIndex] = DepthBufferDesc{};
		m_depthBufferData[resourceIndex] = DepthBufferData{};
		m_depthBufferFreeList.push(resourceIndex);
		break;

	case ManagedGpuBuffer:
		m_gpuBufferDescs[resourceIndex] = GpuBufferDesc{};
		m_gpuBufferData[resourceIndex] = GpuBufferData{};
		m_gpuBufferFreeList.push(resourceIndex);
		break;

	default:
		LogError(LogVulkan) << "ResourceManager: Attempting to destroy handle of unknown type " << type << endl;
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
	case ManagedColorBuffer: return make_optional(m_colorBufferDescs[resourceIndex].resourceType);
	case ManagedDepthBuffer: return make_optional(m_depthBufferDescs[resourceIndex].resourceType);
	case ManagedGpuBuffer: return make_optional(m_gpuBufferDescs[resourceIndex].resourceType);

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
	case ManagedColorBuffer: return make_optional(m_colorBufferDescs[resourceIndex].format);
	case ManagedDepthBuffer: return make_optional(m_depthBufferDescs[resourceIndex].format);
	case ManagedGpuBuffer: return make_optional(m_gpuBufferDescs[resourceIndex].format);

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
	case ManagedColorBuffer: return make_optional(m_colorBufferDescs[resourceIndex].width);
	case ManagedDepthBuffer: return make_optional(m_depthBufferDescs[resourceIndex].width);
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
	case ManagedColorBuffer: return make_optional(m_colorBufferDescs[resourceIndex].height);
	case ManagedDepthBuffer: return make_optional(m_depthBufferDescs[resourceIndex].height);
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
	case ManagedColorBuffer: return make_optional(m_colorBufferDescs[resourceIndex].arraySizeOrDepth);
	case ManagedDepthBuffer: return make_optional(m_depthBufferDescs[resourceIndex].arraySizeOrDepth);
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
	case ManagedColorBuffer: return make_optional(m_colorBufferDescs[resourceIndex].numMips);
	case ManagedDepthBuffer: return make_optional(m_depthBufferDescs[resourceIndex].numMips);
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
	case ManagedColorBuffer: return make_optional(m_colorBufferDescs[resourceIndex].numSamples);
	case ManagedDepthBuffer: return make_optional(m_depthBufferDescs[resourceIndex].numSamples);
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
	case ManagedColorBuffer: return make_optional(m_colorBufferData[resourceIndex].planeCount);
	case ManagedDepthBuffer: return make_optional(m_depthBufferData[resourceIndex].planeCount);
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
	case ManagedColorBuffer: return make_optional(m_colorBufferDescs[resourceIndex].clearColor);
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
	case ManagedDepthBuffer: return make_optional(m_depthBufferDescs[resourceIndex].clearDepth);
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
	case ManagedDepthBuffer: return make_optional(m_depthBufferDescs[resourceIndex].clearStencil);
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
	case ManagedGpuBuffer: return make_optional(m_gpuBufferDescs[resourceIndex].elementCount * m_gpuBufferDescs[resourceIndex].elementSize);
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
	case ManagedGpuBuffer: return make_optional(m_gpuBufferDescs[resourceIndex].elementCount);
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
	case ManagedGpuBuffer: return make_optional(m_gpuBufferDescs[resourceIndex].elementSize);
	default:
		assert(false);
		LogError(LogGraphics) << "ResourceManager: unknown resource type " << type << endl;
		return std::nullopt;
	}
}


void ResourceManager::Update(ResourceHandleType* handle, size_t sizeInBytes, size_t offset, const void* data) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert((sizeInBytes + offset) <= (m_gpuBufferDescs[resourceIndex].elementSize * m_gpuBufferDescs[resourceIndex].elementCount));
	assert(HasFlag(m_gpuBufferDescs[resourceIndex].memoryAccess, MemoryAccess::CpuWrite));

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
	uint32_t colorBufferIndex = m_colorBufferFreeList.front();
	m_colorBufferFreeList.pop();

	// Make sure we don't already have a color buffer allocation
	assert(m_resourceIndices[resourceIndex] == InvalidAllocation);

	// Store the color buffer allocation index
	m_resourceIndices[resourceIndex] = colorBufferIndex;

	m_colorBufferDescs[colorBufferIndex] = colorBufferDesc;
	m_colorBufferData[colorBufferIndex] = colorBufferData;
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

	return m_colorBufferData[resourceIndex].imageViewSrv->Get();
}


VkImageView ResourceManager::GetImageViewRtv(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedColorBuffer);

	return m_colorBufferData[resourceIndex].imageViewRtv->Get();
}


VkImageView ResourceManager::GetImageViewDepth(ResourceHandleType* handle, DepthStencilAspect depthStencilAspect) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedDepthBuffer);

	switch (depthStencilAspect)
	{
	case DepthStencilAspect::DepthReadOnly:		return m_depthBufferData[resourceIndex].imageViewDepthOnly->Get();
	case DepthStencilAspect::StencilReadOnly:	return m_depthBufferData[resourceIndex].imageViewStencilOnly->Get();
	default:									return m_depthBufferData[resourceIndex].imageViewDepthStencil->Get();
	}
}


VkDescriptorImageInfo ResourceManager::GetImageInfoSrv(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedColorBuffer);

	return m_colorBufferData[resourceIndex].imageInfoSrv;
}


VkDescriptorImageInfo ResourceManager::GetImageInfoUav(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedColorBuffer);

	return m_colorBufferData[resourceIndex].imageInfoUav;
}


VkDescriptorImageInfo ResourceManager::GetImageInfoDepth(ResourceHandleType* handle, bool depthSrv) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedDepthBuffer);

	return depthSrv ? m_depthBufferData[resourceIndex].imageInfoDepth : m_depthBufferData[resourceIndex].imageInfoStencil;
}


VkDescriptorBufferInfo ResourceManager::GetBufferInfo(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedGpuBuffer);

	return m_gpuBufferData[resourceIndex].bufferInfo;
}


VkBufferView ResourceManager::GetBufferView(ResourceHandleType* handle) const
{
	const auto [index, type, resourceIndex] = UnpackHandle(handle);

	assert(type == ManagedGpuBuffer);

	return m_gpuBufferData[resourceIndex].bufferView->Get();
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


ResourceManager* const GetVulkanResourceManager()
{
	return g_resourceManager;
}

} // namespace Luna::VK