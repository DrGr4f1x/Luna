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

#include "ColorBufferManagerVK.h"

#include "DeviceManagerVK.h"
#include "VulkanUtil.h"


using namespace std;


namespace Luna::VK
{

ColorBufferManager* g_colorBufferManager{ nullptr };


ColorBufferManager::ColorBufferManager(CVkDevice* device, CVmaAllocator* allocator)
	: m_device{ device }
	, m_allocator{ allocator }
{
	assert(g_colorBufferManager == nullptr);

	for (uint32_t i = 0; i < MaxItems; ++i)
	{
		m_freeList.push(i);
		m_descs[i] = ColorBufferDesc{};
		m_colorBufferData[i] = ColorBufferData{};
	}

	g_colorBufferManager = this;
}


ColorBufferManager::~ColorBufferManager()
{
	g_colorBufferManager = nullptr;
}


ColorBufferHandle ColorBufferManager::CreateColorBuffer(const ColorBufferDesc& colorBufferDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_freeList.empty());

	uint32_t index = m_freeList.front();
	m_freeList.pop();

	m_descs[index] = colorBufferDesc;
	m_colorBufferData[index] = CreateColorBuffer_Internal(colorBufferDesc);

	return Create<ColorBufferHandleType>(index, this);
}


void ColorBufferManager::DestroyHandle(ColorBufferHandleType* handle)
{
	assert(handle != nullptr);

	std::lock_guard guard(m_allocationMutex);

	// TODO: queue this up to execute in one big batch, e.g. at the end of the frame

	uint32_t index = handle->GetIndex();
	m_descs[index] = ColorBufferDesc{};
	m_colorBufferData[index] = ColorBufferData{};

	m_freeList.push(index);
}


ResourceType ColorBufferManager::GetResourceType(ColorBufferHandleType* handle) const
{
	return GetDesc(handle).resourceType;
}


ResourceState ColorBufferManager::GetUsageState(ColorBufferHandleType* handle) const
{
	return GetData(handle).usageState;
}


void ColorBufferManager::SetUsageState(ColorBufferHandleType* handle, ResourceState newState)
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	m_colorBufferData[index].usageState = newState;
}


uint64_t ColorBufferManager::GetWidth(ColorBufferHandleType* handle) const
{
	return GetDesc(handle).width;
}


uint32_t ColorBufferManager::GetHeight(ColorBufferHandleType* handle) const
{
	return GetDesc(handle).height;
}


uint32_t ColorBufferManager::GetDepthOrArraySize(ColorBufferHandleType* handle) const
{
	return GetDesc(handle).arraySizeOrDepth;
}


uint32_t ColorBufferManager::GetNumMips(ColorBufferHandleType* handle) const
{
	return GetDesc(handle).numMips;
}


uint32_t ColorBufferManager::GetNumSamples(ColorBufferHandleType* handle) const
{
	return GetDesc(handle).numSamples;
}


uint32_t ColorBufferManager::GetPlaneCount(ColorBufferHandleType* handle) const
{
	return GetData(handle).planeCount;
}


Format ColorBufferManager::GetFormat(ColorBufferHandleType* handle) const
{
	return GetDesc(handle).format;
}


Color ColorBufferManager::GetClearColor(ColorBufferHandleType* handle) const
{
	return GetDesc(handle).clearColor;
}


ColorBufferHandle ColorBufferManager::CreateColorBufferFromSwapChainImage(CVkImage* swapChainImage, uint32_t width, uint32_t height, Format format, uint32_t imageIndex)
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

	ColorBufferData data{
		.image = swapChainImage,
		.imageViewRtv = imageViewRtv.get(),
		.imageViewSrv = imageViewSrv.get(),
		.imageInfoSrv = imageInfoSrv,
		.imageInfoUav = imageInfoUav,
		.usageState = ResourceState::Undefined
	};

	std::lock_guard guard(m_allocationMutex);

	assert(!m_freeList.empty());

	uint32_t index = m_freeList.front();
	m_freeList.pop();

	m_descs[index] = colorBufferDesc;
	m_colorBufferData[index] = data;

	return Create<ColorBufferHandleType>(index, this);
}


VkImage ColorBufferManager::GetImage(ColorBufferHandleType* handle) const
{
	return GetData(handle).image->Get();
}


VkImageView ColorBufferManager::GetImageViewSrv(ColorBufferHandleType* handle) const
{
	return GetData(handle).imageViewSrv->Get();
}


VkImageView ColorBufferManager::GetImageViewRtv(ColorBufferHandleType* handle) const
{
	return GetData(handle).imageViewRtv->Get();
}


VkDescriptorImageInfo ColorBufferManager::GetImageInfoSrv(ColorBufferHandleType* handle) const
{
	return GetData(handle).imageInfoSrv;
}


VkDescriptorImageInfo ColorBufferManager::GetImageInfoUav(ColorBufferHandleType* handle) const
{
	return GetData(handle).imageInfoUav;
}


const ColorBufferDesc& ColorBufferManager::GetDesc(ColorBufferHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_descs[index];
}


const ColorBufferData& ColorBufferManager::GetData(ColorBufferHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_colorBufferData[index];
}


ColorBufferData ColorBufferManager::CreateColorBuffer_Internal(const ColorBufferDesc& colorBufferDesc)
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

	ColorBufferData data{
		.image = image.get(),
		.imageViewRtv = imageViewRtv.get(),
		.imageViewSrv = imageViewSrv.get(),
		.imageInfoSrv = imageInfoSrv,
		.imageInfoUav = imageInfoUav,
		.usageState = ResourceState::Common
	};

	return data;
}


ColorBufferManager* const GetVulkanColorBufferManager()
{
	return g_colorBufferManager;
}

} // namespace Luna::VK