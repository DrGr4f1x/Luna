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

#include "DeviceVK.h"

#include "ColorBufferVK.h"
#include "DepthBufferVK.h"
#include "GpuBufferVK.h"

using namespace std;

extern Luna::IGraphicsDevice* g_graphicsDevice;


namespace Luna::VK
{

GraphicsDevice* g_vulkanGraphicsDevice = nullptr;


// TODO - Move this elsewhere?
bool QueryLinearTilingFeature(VkFormatProperties properties, VkFormatFeatureFlagBits flags)
{
	return (properties.linearTilingFeatures & flags) != 0;
}


bool QueryOptimalTilingFeature(VkFormatProperties properties, VkFormatFeatureFlagBits flags)
{
	return (properties.optimalTilingFeatures & flags) != 0;
}


bool QueryBufferFeature(VkFormatProperties properties, VkFormatFeatureFlagBits flags)
{
	return (properties.bufferFeatures & flags) != 0;
}


GraphicsDevice::GraphicsDevice(const GraphicsDeviceDesc& desc)
	: m_desc{ desc }
	, m_vkDevice{ desc.device }
{
	g_graphicsDevice = this;
	g_vulkanGraphicsDevice = this;
}


GraphicsDevice::~GraphicsDevice()
{
	LogInfo(LogVulkan) << "Destroying Vulkan device." << endl;

	g_graphicsDevice = nullptr;
	g_vulkanGraphicsDevice = nullptr;
}


wil::com_ptr<IPlatformData> GraphicsDevice::CreateColorBufferData(ColorBufferDesc& desc, ResourceState& initialState)
{
	// Create image
	auto imageDesc = ImageDesc{
		.name				= desc.name,
		.width				= desc.width,
		.height				= desc.height,
		.arraySizeOrDepth	= desc.arraySizeOrDepth,
		.format				= desc.format,
		.numMips			= desc.numMips,
		.numSamples			= desc.numSamples,
		.resourceType		= desc.resourceType,
		.imageUsage			= GpuImageUsage::ColorBuffer,
		.memoryAccess		= MemoryAccess::GpuReadWrite
	};

	if (HasFlag(desc.resourceType, ResourceType::Texture3D))
	{
		imageDesc.SetNumMips(1);
		imageDesc.SetDepth(desc.arraySizeOrDepth);
	}
	else if (HasAnyFlag(desc.resourceType, ResourceType::Texture2D_Type))
	{
		if (HasAnyFlag(desc.resourceType, ResourceType::TextureArray_Type))
		{
			imageDesc.SetResourceType(desc.numSamples == 1 ? ResourceType::Texture2D_Array : ResourceType::Texture2DMS_Array);
			imageDesc.SetArraySize(desc.arraySizeOrDepth);
		}
		else
		{
			imageDesc.SetResourceType(desc.numSamples == 1 ? ResourceType::Texture2D : ResourceType::Texture2DMS_Array);
		}
	}

	auto image = CreateImage(imageDesc);

	// Render target view
	auto imageViewDesc = ImageViewDesc{
		.image				= image.get(),
		.name				= desc.name,
		.resourceType		= ResourceType::Texture2D,
		.imageUsage			= GpuImageUsage::RenderTarget,
		.format				= desc.format,
		.imageAspect		= ImageAspect::Color,
		.baseMipLevel		= 0,
		.mipCount			= desc.numMips,
		.baseArraySlice		= 0,
		.arraySize			= desc.arraySizeOrDepth
	};
	auto imageViewRtv = CreateImageView(imageViewDesc);

	// Shader resource view
	imageViewDesc.SetImageUsage(GpuImageUsage::ShaderResource);
	auto imageViewSrv = CreateImageView(imageViewDesc);

	// Descriptors
	auto imageInfoSrv = VkDescriptorImageInfo{
		.sampler		= VK_NULL_HANDLE,
		.imageView		= *imageViewSrv,
		.imageLayout	= GetImageLayout(ResourceState::ShaderResource)
	};
	auto imageInfoUav = VkDescriptorImageInfo{
		.sampler		= VK_NULL_HANDLE,
		.imageView		= *imageViewSrv,
		.imageLayout	= GetImageLayout(ResourceState::UnorderedAccess)
	};

	auto descExt = ColorBufferDescExt{
		.image			= image.get(),
		.imageViewRtv	= imageViewRtv.get(),
		.imageViewSrv	= imageViewSrv.get(),
		.imageInfoSrv	= imageInfoSrv,
		.imageInfoUav	= imageInfoUav,
		.usageState		= ResourceState::Common
	};

	initialState = ResourceState::Common;

	return Make<ColorBufferData>(descExt);
}


wil::com_ptr<IPlatformData> GraphicsDevice::CreateDepthBufferData(DepthBufferDesc& desc, ResourceState& initialState)
{
	// Create depth image
	auto imageDesc = ImageDesc{
		.name				= desc.name,
		.width				= desc.width,
		.height				= desc.height,
		.arraySizeOrDepth	= desc.arraySizeOrDepth,
		.format				= desc.format,
		.numMips			= desc.numMips,
		.numSamples			= desc.numSamples,
		.resourceType		= desc.resourceType,
		.imageUsage			= GpuImageUsage::DepthBuffer,
		.memoryAccess		= MemoryAccess::GpuReadWrite
	};
	
	auto image = CreateImage(imageDesc);

	// Create image views and descriptors
	const bool bHasStencil = IsStencilFormat(desc.format);

	auto imageAspect = ImageAspect::Depth;
	if (bHasStencil)
	{
		imageAspect |= ImageAspect::Stencil;
	}

	auto imageViewDesc = ImageViewDesc{
		.image				= image.get(),
		.name				= desc.name,
		.resourceType		= ResourceType::Texture2D,
		.imageUsage			= GpuImageUsage::DepthStencilTarget,
		.format				= desc.format,
		.imageAspect		= ImageAspect::Color,
		.baseMipLevel		= 0,
		.mipCount			= desc.numMips,
		.baseArraySlice		= 0,
		.arraySize			= desc.arraySizeOrDepth
	};

	auto imageViewDepthStencil = CreateImageView(imageViewDesc);
	wil::com_ptr<CVkImageView> imageViewDepthOnly;
	wil::com_ptr<CVkImageView> imageViewStencilOnly;

	if (bHasStencil)
	{
		imageViewDesc
			.SetName(format("{} Depth Image View", desc.name))
			.SetImageAspect(ImageAspect::Depth)
			.SetViewType(TextureSubresourceViewType::DepthOnly);

		imageViewDepthOnly = CreateImageView(imageViewDesc);

		imageViewDesc
			.SetName(format("{} Stencil Image View", desc.name))
			.SetImageAspect(ImageAspect::Stencil)
			.SetViewType(TextureSubresourceViewType::StencilOnly);

		imageViewStencilOnly = CreateImageView(imageViewDesc);
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

	auto descExt = DepthBufferDescExt{
		.image					= image.get(),
		.imageViewDepthStencil	= imageViewDepthStencil.get(),
		.imageViewDepthOnly		= imageViewDepthOnly.get(),
		.imageViewStencilOnly	= imageViewStencilOnly.get(),
		.imageInfoDepth			= imageInfoDepth,
		.imageInfoStencil		= imageInfoStencil,
		.usageState				= ResourceState::DepthRead | ResourceState::DepthWrite
	};

	return Make<DepthBufferData>(descExt);
}


wil::com_ptr<IPlatformData> GraphicsDevice::CreateGpuBufferData(GpuBufferDesc& desc, ResourceState& initialState)
{
	constexpr VkBufferUsageFlags transferFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	auto createInfo = VkBufferCreateInfo{
		.sType	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext	= nullptr,
		.size	= desc.elementCount * desc.elementSize,
		.usage	= GetBufferUsageFlags(desc.resourceType) | transferFlags
	};

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.flags = GetMemoryFlags(desc.memoryAccess);
	allocCreateInfo.usage = GetMemoryUsage(desc.memoryAccess);

	VkBuffer vkBuffer{ VK_NULL_HANDLE };
	VmaAllocation vmaBufferAllocation{ VK_NULL_HANDLE };

	auto res = vmaCreateBuffer(*m_vmaAllocator, &createInfo, &allocCreateInfo, &vkBuffer, &vmaBufferAllocation, nullptr);
	assert(res == VK_SUCCESS);

	SetDebugName(*m_vkDevice, vkBuffer, desc.name);

	auto buffer = Create<CVkBuffer>(m_vkDevice.get(), m_vmaAllocator.get(), vkBuffer, vmaBufferAllocation);

	auto descExt = GpuBufferDescExt{
		.buffer			= buffer.get(),
		.bufferInfo		= {
			.buffer		= vkBuffer,
			.offset		= 0,
			.range		= VK_WHOLE_SIZE
		}
	};
	return Make<GpuBufferData>(descExt);
}


void GraphicsDevice::CreateResources()
{
	m_vmaAllocator = CreateVmaAllocator();
}


wil::com_ptr<CVkFence> GraphicsDevice::CreateFence(bool bSignaled) const
{
	auto createInfo = VkFenceCreateInfo{ 
		.sType		= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext		= nullptr,
		.flags		= bSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0u
	};

	VkFence fence{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkCreateFence(*m_vkDevice, &createInfo, nullptr, &fence)))
	{
		return Create<CVkFence>(m_vkDevice.get(), fence);
	}

	return nullptr;
}


wil::com_ptr<CVkSemaphore> GraphicsDevice::CreateSemaphore(VkSemaphoreType semaphoreType, uint64_t initialValue) const
{
	VkSemaphoreTypeCreateInfo typeCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
	typeCreateInfo.semaphoreType = semaphoreType;
	typeCreateInfo.initialValue = initialValue;

	VkSemaphoreCreateInfo createInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	createInfo.pNext = &typeCreateInfo;

	VkSemaphore semaphore{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkCreateSemaphore(*m_vkDevice, &createInfo, nullptr, &semaphore)))
	{
		return Create<CVkSemaphore>(m_vkDevice.get(), semaphore);
	}

	return nullptr;
}


wil::com_ptr<CVkCommandPool> GraphicsDevice::CreateCommandPool(CommandListType commandListType) const
{
	uint32_t queueFamilyIndex{ 0 };
	switch (commandListType)
	{
	case CommandListType::Compute: 
		queueFamilyIndex = m_desc.queueFamilyIndices.compute; 
		break;

	case CommandListType::Copy: 
		queueFamilyIndex = m_desc.queueFamilyIndices.transfer; 
		break;

	default: 
		queueFamilyIndex = m_desc.queueFamilyIndices.graphics; 
		break;
	}

	VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = queueFamilyIndex;

	VkCommandPool vkCommandPool{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkCreateCommandPool(*m_vkDevice, &createInfo, nullptr, &vkCommandPool)))
	{
		return Create<CVkCommandPool>(m_vkDevice.get(), vkCommandPool);
	}

	return nullptr;
}


wil::com_ptr<CVmaAllocator> GraphicsDevice::CreateVmaAllocator() const
{
	VmaVulkanFunctions vmaFunctions{};
	vmaFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vmaFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo createInfo{};
	createInfo.physicalDevice = m_vkDevice->GetPhysicalDevice();
	createInfo.device = *m_vkDevice;
	createInfo.instance = m_desc.instance;
	createInfo.pVulkanFunctions = &vmaFunctions;

	VmaAllocator vmaAllocator{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vmaCreateAllocator(&createInfo, &vmaAllocator)))
	{
		return Create<CVmaAllocator>(m_vkDevice.get(), vmaAllocator);
	}
	else
	{
		LogError(LogVulkan) << "Failed to create VmaAllocator.  Error code: " << res << endl;
	}

	return nullptr;
}


wil::com_ptr<CVkImage> GraphicsDevice::CreateImage(const ImageDesc& desc) const
{
	auto imageCreateInfo = VkImageCreateInfo{ 
		.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.flags			= (VkImageCreateFlags)GetImageCreateFlags(desc.resourceType),
		.imageType		= GetImageType(desc.resourceType),
		.format			= FormatToVulkan(desc.format),
		.extent			= { 
							.width	= (uint32_t)desc.width, 
							.height = desc.height, 
							.depth	= desc.arraySizeOrDepth },
		.mipLevels		= desc.numMips,
		.arrayLayers	= HasAnyFlag(desc.resourceType, ResourceType::TextureArray_Type) ? desc.arraySizeOrDepth : 1,
		.samples		= GetSampleCountFlags(desc.numSamples),
		.tiling			= VK_IMAGE_TILING_OPTIMAL,
		.usage			= GetImageUsageFlags(desc.imageUsage)
	};
	
	// Remove storage flag if this format doesn't support it.
	// TODO - Make a table with all the format properties?
	VkFormatProperties properties = GetFormatProperties(desc.format);
	if (!QueryOptimalTilingFeature(properties, VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
	{
		imageCreateInfo.usage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
	}

	VmaAllocationCreateInfo imageAllocCreateInfo{};
	imageAllocCreateInfo.flags = GetMemoryFlags(desc.memoryAccess);
	imageAllocCreateInfo.usage = GetMemoryUsage(desc.memoryAccess);

	VkImage vkImage{ VK_NULL_HANDLE };
	VmaAllocation vmaAllocation{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vmaCreateImage(*m_vmaAllocator, &imageCreateInfo, &imageAllocCreateInfo, &vkImage, &vmaAllocation, nullptr)))
	{
		SetDebugName(*m_vkDevice, vkImage, desc.name);

		return Create<CVkImage>(m_vkDevice.get(), m_vmaAllocator.get(), vkImage, vmaAllocation);
	}
	else
	{
		LogError(LogVulkan) << "Failed to create VkImage.  Error code: " << res << endl;
	}

	return nullptr;
}


wil::com_ptr<CVkImageView> GraphicsDevice::CreateImageView(const ImageViewDesc& desc) const
{
	VkImageViewCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	createInfo.viewType = GetImageViewType(desc.resourceType, desc.imageUsage);
	createInfo.format = FormatToVulkan(desc.format);
	if (IsColorFormat(desc.format))
	{
		createInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	}
	createInfo.subresourceRange = {};
	createInfo.subresourceRange.aspectMask = GetImageAspect(desc.imageAspect);
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = desc.mipCount;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = desc.resourceType == ResourceType::Texture3D ? 1 : desc.arraySize;
	createInfo.image = desc.image->Get();

	VkImageView vkImageView{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkCreateImageView(*m_vkDevice, &createInfo, nullptr, &vkImageView)))
	{
		return Create<CVkImageView>(m_vkDevice.get(), desc.image, vkImageView);
	}
	else
	{
		LogWarning(LogVulkan) << "Failed to create VkImageView.  Error code: " << res << endl;
	}

	return nullptr;
}


wil::com_ptr<CVkBuffer> GraphicsDevice::CreateStagingBuffer(const void* initialData, size_t numBytes) const
{
	VkBufferCreateInfo stagingBufferInfo{ 
		.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO ,
		.pNext			= nullptr,
		.size			= numBytes,
		.usage			= VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.sharingMode	= VK_SHARING_MODE_EXCLUSIVE
	};

	VmaAllocationCreateInfo stagingAllocCreateInfo{
		.flags	= VMA_ALLOCATION_CREATE_MAPPED_BIT,
		.usage	= VMA_MEMORY_USAGE_CPU_ONLY
	};
	
	VkBuffer stagingBuffer{ VK_NULL_HANDLE };
	VmaAllocation stagingBufferAlloc{ VK_NULL_HANDLE };
	VmaAllocationInfo stagingAllocInfo{};

	auto allocator = GetVulkanGraphicsDevice()->GetAllocator();
	vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocCreateInfo, &stagingBuffer, &stagingBufferAlloc, &stagingAllocInfo);

	SIMDMemCopy(stagingAllocInfo.pMappedData, initialData, numBytes);

	return Create<CVkBuffer>(m_vkDevice.get(), m_vmaAllocator.get(), stagingBuffer, stagingBufferAlloc);
}


VkFormatProperties GraphicsDevice::GetFormatProperties(Format format) const
{
	VkFormat vkFormat = static_cast<VkFormat>(format);
	VkFormatProperties properties{};

	vkGetPhysicalDeviceFormatProperties(m_vkDevice->GetPhysicalDevice(), vkFormat, &properties);

	return properties;
}


GraphicsDevice* GetVulkanGraphicsDevice()
{
	return g_vulkanGraphicsDevice;
}

} // namespace Luna::VK