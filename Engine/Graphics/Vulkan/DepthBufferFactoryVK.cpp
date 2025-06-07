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

#include "DepthBufferFactoryVK.h"

#include "Graphics\ResourceManager.h"
#include "VulkanUtil.h"


namespace Luna::VK
{

DepthBufferFactory::DepthBufferFactory(IResourceManager* owner, CVkDevice* device, CVmaAllocator* allocator)
	: m_owner{ owner }
	, m_device{ device }
	, m_allocator{ allocator }
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		m_freeList.push(i);
	}

	ClearImages();
	ClearData();
}


ResourceHandle DepthBufferFactory::CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc)
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

	ImageData imageData{
		.image			= image.get(),
		.usageState		= ResourceState::Undefined
	};

	DepthBufferData depthBufferData{
		.imageViewDepthStencil	= imageViewDepthStencil.get(),
		.imageViewDepthOnly		= imageViewDepthOnly.get(),
		.imageViewStencilOnly	= imageViewStencilOnly.get(),
		.imageInfoDepth			= imageInfoDepth,
		.imageInfoStencil		= imageInfoStencil
	};

	// Create handle and store cached data
	{
		std::lock_guard lock(m_mutex);

		assert(!m_freeList.empty());

		// Get an index allocation
		uint32_t index = m_freeList.front();
		m_freeList.pop();

		m_descs[index] = depthBufferDesc;
		m_images[index] = imageData;
		m_data[index] = depthBufferData;

		return std::make_shared<ResourceHandleType>(index, IResourceManager::ManagedDepthBuffer, m_owner);
	}
}


void DepthBufferFactory::Destroy(uint32_t index)
{
	std::lock_guard lock(m_mutex);

	ResetDesc(index);
	ResetImage(index);
	ResetData(index);

	m_freeList.push(index);
}


VkImageView DepthBufferFactory::GetImageView(uint32_t index, DepthStencilAspect depthStencilAspect) const
{
	switch (depthStencilAspect)
	{
	case DepthStencilAspect::DepthReadOnly:		return m_data[index].imageViewDepthOnly->Get();
	case DepthStencilAspect::StencilReadOnly:	return m_data[index].imageViewStencilOnly->Get();
	default:									return m_data[index].imageViewDepthStencil->Get();
	}
}


VkDescriptorImageInfo DepthBufferFactory::GetImageInfo(uint32_t index, bool depthSrv) const
{
	return depthSrv ? m_data[index].imageInfoDepth : m_data[index].imageInfoStencil;
}


void DepthBufferFactory::ResetImage(uint32_t index)
{
	m_images[index] = ImageData{};
}


void DepthBufferFactory::ResetData(uint32_t index)
{
	m_data[index] = DepthBufferData{};
}


void DepthBufferFactory::ClearImages()
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		ResetImage(i);
	}
}


void DepthBufferFactory::ClearData()
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		ResetData(i);
	}
}

} // namespace Luna::VK