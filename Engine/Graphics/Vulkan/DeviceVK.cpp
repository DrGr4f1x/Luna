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

#include "FileSystem.h"

#include "CommandContextVK.h"
#include "DescriptorAllocatorVK.h"

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
	, m_colorBufferPool{ m_vkDevice.get() }
	, m_depthBufferPool{ m_vkDevice.get() }
	, m_descriptorSetPool{ m_vkDevice.get() }
	, m_gpuBufferPool{ m_vkDevice.get() }
	, m_pipelinePool{ m_vkDevice.get() }
	, m_rootSignaturePool{ m_vkDevice.get() }
{
	g_graphicsDevice = this;
	g_vulkanGraphicsDevice = this;
}


GraphicsDevice::~GraphicsDevice()
{
	LogInfo(LogVulkan) << "Destroying Vulkan device." << endl;

	DescriptorSetAllocator::DestroyAll();

	g_graphicsDevice = nullptr;
	g_vulkanGraphicsDevice = nullptr;
}


void GraphicsDevice::CreateResources()
{
	m_vmaAllocator = CreateVmaAllocator();
	m_colorBufferPool.SetAllocator(m_vmaAllocator.get());
	m_depthBufferPool.SetAllocator(m_vmaAllocator.get());
	m_gpuBufferPool.SetAllocator(m_vmaAllocator.get());
}


wil::com_ptr<CVkFence> GraphicsDevice::CreateFence(bool bSignaled) const
{
	VkFenceCreateInfo createInfo{ 
		.sType		= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
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
	uint32_t queueFamilyIndex = 0;
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

	VkCommandPoolCreateInfo createInfo{ 
		.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex	= queueFamilyIndex
	};

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
	VkImageCreateInfo imageCreateInfo{ 
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

	memcpy(stagingAllocInfo.pMappedData, initialData, numBytes);

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