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

#include "DeviceVK.h"
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


VkDeviceAddress GetBufferDeviceAddress(VkDevice device, VkBuffer buffer)
{
	VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = buffer
	};
	return vkGetBufferDeviceAddress(device, &bufferDeviceAddressInfo);
}


VkDeviceSize GetBufferDeviceSize(VkDevice device, VkBuffer buffer)
{
	VkMemoryRequirements memReqs{};
	vkGetBufferMemoryRequirements(device, buffer, &memReqs);
	return memReqs.size;
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


size_t GetDescriptorSize(DescriptorType descriptorType)
{
	const auto& caps = GetVulkanDevice()->GetDeviceCaps();

	switch (descriptorType)
	{
	case DescriptorType::ConstantBuffer:
		return caps.descriptorBuffer.descriptorSize.uniformBuffer;
	case DescriptorType::TextureSRV:
		return caps.descriptorBuffer.descriptorSize.sampledImage;
	case DescriptorType::TextureUAV:
		return caps.descriptorBuffer.descriptorSize.storageImage;
	case DescriptorType::TypedBufferSRV:
		return caps.descriptorBuffer.descriptorSize.uniformTexelBuffer;
	case DescriptorType::TypedBufferUAV:
		return caps.descriptorBuffer.descriptorSize.storageTexelBuffer;
	case DescriptorType::StructuredBufferSRV:
		return caps.descriptorBuffer.descriptorSize.storageBuffer;
	case DescriptorType::StructuredBufferUAV:
		return caps.descriptorBuffer.descriptorSize.storageBuffer;
	case DescriptorType::RawBufferSRV:
		return caps.descriptorBuffer.descriptorSize.storageBuffer;
	case DescriptorType::RawBufferUAV:
		return caps.descriptorBuffer.descriptorSize.storageBuffer;
	case DescriptorType::Sampler:
		return caps.descriptorBuffer.descriptorSize.sampler;
	case DescriptorType::RayTracingAccelStruct:
		return caps.descriptorBuffer.descriptorSize.accelerationStructure;
	case DescriptorType::SamplerFeedbackTextureUAV:
		return caps.descriptorBuffer.descriptorSize.storageImage;
	default:
		return 0;
	}
}

} // namespace Luna::VK