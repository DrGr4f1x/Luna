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

#include "DepthBufferManagerVK.h"

#include "VulkanUtil.h"


namespace Luna::VK
{

DepthBufferManager* g_depthBufferManager{ nullptr };


DepthBufferManager::DepthBufferManager(CVkDevice* device, CVmaAllocator* allocator)
	: m_device{ device }
	, m_allocator{ allocator }
{
	assert(g_depthBufferManager == nullptr);

	for (uint32_t i = 0; i < MaxItems; ++i)
	{
		m_freeList.push(i);
		m_descs[i] = DepthBufferDesc{};
		m_depthBufferData[i] = DepthBufferData{};
	}

	g_depthBufferManager = this;
}


DepthBufferManager::~DepthBufferManager()
{
	g_depthBufferManager = nullptr;
}


DepthBufferHandle DepthBufferManager::CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_freeList.empty());

	uint32_t index = m_freeList.front();
	m_freeList.pop();

	m_descs[index] = depthBufferDesc;
	m_depthBufferData[index] = CreateDepthBuffer_Internal(depthBufferDesc);

	return Create<DepthBufferHandleType>(index, this);
}


void DepthBufferManager::DestroyHandle(DepthBufferHandleType* handle)
{
	assert(handle != nullptr);

	std::lock_guard guard(m_allocationMutex);

	// TODO: queue this up to execute in one big batch, e.g. at the end of the frame

	uint32_t index = handle->GetIndex();
	m_descs[index] = DepthBufferDesc{};
	m_depthBufferData[index] = DepthBufferData{};

	m_freeList.push(index);
}


ResourceType DepthBufferManager::GetResourceType(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).resourceType;
}


ResourceState DepthBufferManager::GetUsageState(DepthBufferHandleType* handle) const
{
	return GetData(handle).usageState;
}


void DepthBufferManager::SetUsageState(DepthBufferHandleType* handle, ResourceState newState)
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	m_depthBufferData[index].usageState = newState;
}


uint64_t DepthBufferManager::GetWidth(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).width;
}


uint32_t DepthBufferManager::GetHeight(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).height;
}


uint32_t DepthBufferManager::GetDepthOrArraySize(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).arraySizeOrDepth;
}


uint32_t DepthBufferManager::GetNumMips(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).numMips;
}


uint32_t DepthBufferManager::GetNumSamples(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).numSamples;
}


uint32_t DepthBufferManager::GetPlaneCount(DepthBufferHandleType* handle) const
{
	return GetData(handle).planeCount;
}


Format DepthBufferManager::GetFormat(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).format;
}


float DepthBufferManager::GetClearDepth(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).clearDepth;
}


uint8_t DepthBufferManager::GetClearStencil(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).clearStencil;
}


VkImage DepthBufferManager::GetImage(DepthBufferHandleType* handle) const
{
	return GetData(handle).image->Get();
}


VkImageView DepthBufferManager::GetImageView(DepthBufferHandleType* handle, DepthStencilAspect depthStencilAspect) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();

	switch (depthStencilAspect)
	{
	case DepthStencilAspect::DepthReadOnly:		return m_depthBufferData[index].imageViewDepthOnly->Get();
	case DepthStencilAspect::StencilReadOnly:	return m_depthBufferData[index].imageViewStencilOnly->Get();
	default:									return m_depthBufferData[index].imageViewDepthStencil->Get();
	}
}


VkDescriptorImageInfo DepthBufferManager::GetImageInfo(DepthBufferHandleType* handle, bool depthSrv) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return depthSrv ? m_depthBufferData[index].imageInfoDepth : m_depthBufferData[index].imageInfoStencil;
}


const DepthBufferDesc& DepthBufferManager::GetDesc(DepthBufferHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_descs[index];
}


const DepthBufferData& DepthBufferManager::GetData(DepthBufferHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_depthBufferData[index];
}


DepthBufferData DepthBufferManager::CreateDepthBuffer_Internal(const DepthBufferDesc& depthBufferDesc)
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

	DepthBufferData data{
		.image					= image.get(),
		.imageViewDepthStencil	= imageViewDepthStencil.get(),
		.imageViewDepthOnly		= imageViewDepthOnly.get(),
		.imageViewStencilOnly	= imageViewStencilOnly.get(),
		.imageInfoDepth			= imageInfoDepth,
		.imageInfoStencil		= imageInfoStencil,
		.usageState				= ResourceState::Undefined
	};

	return data;
}


DepthBufferManager* const GetVulkanDepthBufferManager()
{
	return g_depthBufferManager;
}

} // namespace Luna::VK