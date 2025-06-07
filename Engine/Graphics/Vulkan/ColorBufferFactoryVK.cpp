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

#include "ColorBufferFactoryVK.h"

#include "Graphics\ResourceManager.h"
#include "VulkanUtil.h"


namespace Luna::VK
{

ColorBufferFactory::ColorBufferFactory(IResourceManager* owner, CVkDevice* device, CVmaAllocator* allocator)
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


ResourceHandle ColorBufferFactory::CreateColorBuffer(const ColorBufferDesc& colorBufferDesc)
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

	ImageData imageData{
		.image			= image.get(),
		.usageState		= ResourceState::Common
	};

	ColorBufferData colorBufferData{
		.imageViewRtv = imageViewRtv.get(),
		.imageViewSrv = imageViewSrv.get(),
		.imageInfoSrv = imageInfoSrv,
		.imageInfoUav = imageInfoUav
	};

	// Create handle and store cached data
	{
		std::lock_guard lock(m_mutex);

		assert(!m_freeList.empty());

		// Get an index allocation
		uint32_t index = m_freeList.front();
		m_freeList.pop();

		m_descs[index] = colorBufferDesc;
		m_images[index] = imageData;
		m_data[index] = colorBufferData;

		return std::make_shared<ResourceHandleType>(index, IResourceManager::ManagedColorBuffer, m_owner);
	}
}


void ColorBufferFactory::Destroy(uint32_t index)
{
	std::lock_guard lock(m_mutex);

	ResetDesc(index);
	ResetImage(index);
	ResetData(index);

	m_freeList.push(index);
}


ResourceHandle ColorBufferFactory::CreateColorBufferFromSwapChainImage(CVkImage* swapChainImage, uint32_t width, uint32_t height, Format format, uint32_t imageIndex)
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

	ImageData imageData{
		.image			= swapChainImage,
		.usageState		= ResourceState::Undefined
	};

	ColorBufferData colorBufferData{
		.imageViewRtv = imageViewRtv.get(),
		.imageViewSrv = imageViewSrv.get(),
		.imageInfoSrv = imageInfoSrv,
		.imageInfoUav = imageInfoUav
	};

	// Create handle and store cached data
	{
		std::lock_guard lock(m_mutex);

		assert(!m_freeList.empty());

		// Get an index allocation
		uint32_t index = m_freeList.front();
		m_freeList.pop();

		m_descs[index] = colorBufferDesc;
		m_images[index] = imageData;
		m_data[index] = colorBufferData;

		return std::make_shared<ResourceHandleType>(index, IResourceManager::ManagedColorBuffer, m_owner);
	}
}


void ColorBufferFactory::ResetImage(uint32_t index)
{
	m_images[index] = ImageData{};
}


void ColorBufferFactory::ResetData(uint32_t index)
{
	m_data[index] = ColorBufferData{};
}


void ColorBufferFactory::ClearImages()
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		ResetImage(i);
	}
}


void ColorBufferFactory::ClearData()
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		ResetData(i);
	}
}

} // namespace Luna::VK