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

#include "ColorBufferPoolVK.h"

#include "DeviceVK.h"


namespace Luna::VK
{

ColorBufferPool* g_colorBufferPool{ nullptr };


ColorBufferPool::ColorBufferPool(CVkDevice* device)
	: m_device{ device }
{
	assert(g_colorBufferPool == nullptr);

	for (uint32_t i = 0; i < MaxItems; ++i)
	{
		m_freeList.push(i);
		m_descs[i] = ColorBufferDesc{};
		m_colorBufferData[i] = ColorBufferData{};
	}

	g_colorBufferPool = this;
}


ColorBufferPool::~ColorBufferPool()
{
	g_colorBufferPool = nullptr;
}


ColorBufferHandle ColorBufferPool::CreateColorBuffer(const ColorBufferDesc& colorBufferDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_freeList.empty());

	uint32_t index = m_freeList.front();
	m_freeList.pop();

	m_descs[index] = colorBufferDesc;
	m_colorBufferData[index] = CreateColorBuffer_Internal(colorBufferDesc);

	return Create<ColorBufferHandleType>(index, this);
}


void ColorBufferPool::DestroyHandle(ColorBufferHandleType* handle)
{
	assert(handle != nullptr);

	std::lock_guard guard(m_allocationMutex);

	// TODO: queue this up to execute in one big batch, e.g. at the end of the frame

	uint32_t index = handle->GetIndex();
	m_descs[index] = ColorBufferDesc{};
	m_colorBufferData[index] = ColorBufferData{};

	m_freeList.push(index);
}


ResourceType ColorBufferPool::GetResourceType(ColorBufferHandleType* handle) const
{
	return GetDesc(handle).resourceType;
}


ResourceState ColorBufferPool::GetUsageState(ColorBufferHandleType* handle) const
{
	return GetData(handle).usageState;
}


void ColorBufferPool::SetUsageState(ColorBufferHandleType* handle, ResourceState newState)
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	m_colorBufferData[index].usageState = newState;
}


uint64_t ColorBufferPool::GetWidth(ColorBufferHandleType* handle) const
{
	return GetDesc(handle).width;
}


uint32_t ColorBufferPool::GetHeight(ColorBufferHandleType* handle) const
{
	return GetDesc(handle).height;
}


uint32_t ColorBufferPool::GetDepthOrArraySize(ColorBufferHandleType* handle) const
{
	return GetDesc(handle).arraySizeOrDepth;
}


uint32_t ColorBufferPool::GetNumMips(ColorBufferHandleType* handle) const
{
	return GetDesc(handle).numMips;
}


uint32_t ColorBufferPool::GetNumSamples(ColorBufferHandleType* handle) const
{
	return GetDesc(handle).numSamples;
}


uint32_t ColorBufferPool::GetPlaneCount(ColorBufferHandleType* handle) const
{
	return GetData(handle).planeCount;
}


Format ColorBufferPool::GetFormat(ColorBufferHandleType* handle) const
{
	return GetDesc(handle).format;
}


Color ColorBufferPool::GetClearColor(ColorBufferHandleType* handle) const
{
	return GetDesc(handle).clearColor;
}


ColorBufferHandle ColorBufferPool::CreateColorBufferFromSwapChainImage(CVkImage* swapChainImage, uint32_t width, uint32_t height, Format format, uint32_t imageIndex)
{
	auto device = GetVulkanGraphicsDevice();

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

	auto imageViewRtv = device->CreateImageView(imageViewDesc);

	// SRV view
	imageViewDesc
		.SetImageUsage(GpuImageUsage::ShaderResource)
		.SetName(std::format("Primary SwapChain {} SRV Image View", imageIndex));

	auto imageViewSrv = device->CreateImageView(imageViewDesc);

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


VkImage ColorBufferPool::GetImage(ColorBufferHandleType* handle) const
{
	return GetData(handle).image->Get();
}


VkImageView ColorBufferPool::GetImageViewSrv(ColorBufferHandleType* handle) const
{
	return GetData(handle).imageViewSrv->Get();
}


VkImageView ColorBufferPool::GetImageViewRtv(ColorBufferHandleType* handle) const
{
	return GetData(handle).imageViewRtv->Get();
}


VkDescriptorImageInfo ColorBufferPool::GetImageInfoSrv(ColorBufferHandleType* handle) const
{
	return GetData(handle).imageInfoSrv;
}


VkDescriptorImageInfo ColorBufferPool::GetImageInfoUav(ColorBufferHandleType* handle) const
{
	return GetData(handle).imageInfoUav;
}


const ColorBufferDesc& ColorBufferPool::GetDesc(ColorBufferHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_descs[index];
}


const ColorBufferData& ColorBufferPool::GetData(ColorBufferHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_colorBufferData[index];
}


ColorBufferData ColorBufferPool::CreateColorBuffer_Internal(const ColorBufferDesc& colorBufferDesc)
{
	auto device = GetVulkanGraphicsDevice();

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

	auto image = device->CreateImage(imageDesc);

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
	auto imageViewRtv = device->CreateImageView(imageViewDesc);

	// Shader resource view
	imageViewDesc.SetImageUsage(GpuImageUsage::ShaderResource);
	auto imageViewSrv = device->CreateImageView(imageViewDesc);

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


ColorBufferPool* const GetVulkanColorBufferPool()
{
	return g_colorBufferPool;
}

} // namespace Luna::VK