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

#include "VulkanUtil.h"

#include "SemaphoreVK.h"
#include "VulkanCommon.h"

using namespace std;


namespace Luna::VK
{

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


VkFormatProperties GetFormatProperties(VkPhysicalDevice physicalDevice, Format format)
{
	VkFormat vkFormat = FormatToVulkan(format);
	VkFormatProperties properties{};

	vkGetPhysicalDeviceFormatProperties(physicalDevice, vkFormat, &properties);

	return properties;
}


wil::com_ptr<CVkImage> CreateImage(CVkDevice* device, CVmaAllocator* allocator, const ImageDesc& desc)
{
	VkImageCreateInfo imageCreateInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.flags = (VkImageCreateFlags)GetImageCreateFlags(desc.resourceType),
		.imageType = GetImageType(desc.resourceType),
		.format = FormatToVulkan(desc.format),
		.extent = {
			.width = (uint32_t)desc.width,
			.height = desc.height,
			.depth = desc.arraySizeOrDepth },
		.mipLevels = desc.numMips,
		.arrayLayers = HasAnyFlag(desc.resourceType, ResourceType::TextureArray_Type) ? desc.arraySizeOrDepth : 1,
		.samples = GetSampleCountFlags(desc.numSamples),
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = GetImageUsageFlags(desc.imageUsage)
	};

	// Remove storage flag if this format doesn't support it.
	// TODO - Make a table with all the format properties?
	VkFormatProperties properties = GetFormatProperties(device->GetPhysicalDevice(), desc.format);
	if (!QueryOptimalTilingFeature(properties, VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
	{
		imageCreateInfo.usage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
	}

	VmaAllocationCreateInfo imageAllocCreateInfo{};
	imageAllocCreateInfo.flags = GetMemoryFlags(desc.memoryAccess);
	imageAllocCreateInfo.usage = GetMemoryUsage(desc.memoryAccess);

	VkImage vkImage{ VK_NULL_HANDLE };
	VmaAllocation vmaAllocation{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vmaCreateImage(*allocator, &imageCreateInfo, &imageAllocCreateInfo, &vkImage, &vmaAllocation, nullptr)))
	{
		SetDebugName(*device, vkImage, desc.name);

		return Create<CVkImage>(device, allocator, vkImage, vmaAllocation);
	}
	else
	{
		LogError(LogVulkan) << "Failed to create VkImage.  Error code: " << res << endl;
	}

	return nullptr;
}


wil::com_ptr<CVkImageView> CreateImageView(CVkDevice* device, const ImageViewDesc& desc)
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
	if (VK_SUCCEEDED(vkCreateImageView(*device, &createInfo, nullptr, &vkImageView)))
	{
		return Create<CVkImageView>(device, desc.image, vkImageView);
	}
	else
	{
		LogWarning(LogVulkan) << "Failed to create VkImageView.  Error code: " << res << endl;
	}

	return nullptr;
}


wil::com_ptr<CVkFence> CreateFence(CVkDevice* device, bool bSignaled)
{
	VkFenceCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = bSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0u
	};

	VkFence fence{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkCreateFence(*device, &createInfo, nullptr, &fence)))
	{
		return Create<CVkFence>(device, fence);
	}

	return nullptr;
}


SemaphorePtr CreateSemaphore(CVkDevice* device, VkSemaphoreType semaphoreType, uint64_t initialValue)
{
	VkSemaphoreTypeCreateInfo typeCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
	typeCreateInfo.semaphoreType = semaphoreType;
	typeCreateInfo.initialValue = initialValue;

	VkSemaphoreCreateInfo createInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	createInfo.pNext = &typeCreateInfo;

	VkSemaphore vkSemaphore{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkCreateSemaphore(*device, &createInfo, nullptr, &vkSemaphore)))
	{
		auto semaphoreRef = Create<CVkSemaphore>(device, vkSemaphore);

		auto pSemaphore = std::make_shared<Semaphore>();
		pSemaphore->semaphore = semaphoreRef;
		pSemaphore->isBinary = semaphoreType == VK_SEMAPHORE_TYPE_BINARY;
		pSemaphore->value = initialValue;

		return pSemaphore;
	}

	return nullptr;
}

// TODO: make these configurable
constexpr uint32_t s_tRegShift = 0;
constexpr uint32_t s_sRegShift = 128;
constexpr uint32_t s_bRegShift = 256;
constexpr uint32_t s_uRegShift = 384;


uint32_t GetRegisterShift(DescriptorType descriptorType)
{
	switch (descriptorType)
	{
	case DescriptorType::ConstantBuffer:
		return s_bRegShift;
	case DescriptorType::TextureSRV:
		return s_tRegShift;
	case DescriptorType::TextureUAV:
		return s_uRegShift;
	case DescriptorType::TypedBufferSRV:
		return s_tRegShift;
	case DescriptorType::TypedBufferUAV:
		return s_uRegShift;
	case DescriptorType::StructuredBufferSRV:
		return s_tRegShift;
	case DescriptorType::StructuredBufferUAV:
		return s_uRegShift;
	case DescriptorType::RawBufferSRV:
		return s_tRegShift;
	case DescriptorType::RawBufferUAV:
		return s_uRegShift;
	case DescriptorType::Sampler:
		return s_sRegShift;
	case DescriptorType::RayTracingAccelStruct:
		return s_tRegShift;
	case DescriptorType::PushConstants:
		return s_bRegShift;
	case DescriptorType::SamplerFeedbackTextureUAV:
		return s_uRegShift;
	default:
		return 0;
	}
}

uint32_t GetRegisterShift(RootParameterType rootParameterType)
{
	switch (rootParameterType)
	{
	case RootParameterType::RootConstants:
		return s_bRegShift;
	case RootParameterType::RootCBV:
		return s_bRegShift;
	case RootParameterType::RootSRV:
		return s_tRegShift;
	case RootParameterType::RootUAV:
		return s_uRegShift;
	default:
		return 0;
	}
}

uint32_t GetRegisterClass(DescriptorType descriptorType)
{
	switch (descriptorType)
	{
	case DescriptorType::ConstantBuffer:
		return 2;
	case DescriptorType::TextureSRV:
		return 0;
	case DescriptorType::TextureUAV:
		return 3;
	case DescriptorType::TypedBufferSRV:
		return 0;
	case DescriptorType::TypedBufferUAV:
		return 3;
	case DescriptorType::StructuredBufferSRV:
		return 0;
	case DescriptorType::StructuredBufferUAV:
		return 3;
	case DescriptorType::RawBufferSRV:
		return 0;
	case DescriptorType::RawBufferUAV:
		return 3;
	case DescriptorType::Sampler:
		return 1;
	case DescriptorType::RayTracingAccelStruct:
		return 0;
	case DescriptorType::PushConstants:
		return 2;
	case DescriptorType::SamplerFeedbackTextureUAV:
		return 3;
	default:
		return 0;
	}
}

uint32_t GetRegisterShiftSRV() { return s_tRegShift; }
uint32_t GetRegisterShiftCBV() { return s_bRegShift; }
uint32_t GetRegisterShiftUAV() { return s_uRegShift; }
uint32_t GetRegisterShiftSampler() { return s_sRegShift; }

} // namespace Luna::VK