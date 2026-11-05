//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#pragma once

#include "Graphics\Descriptor.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

// Forward declarations
class Device;


class Descriptor : public IDescriptor
{
public:
	Descriptor();
	~Descriptor() override {}

	void ReadSampledImageDescriptor(bool useDescriptorHeap, Device* device, VkImageViewCreateInfo viewInfo, VkImageView imageView);
	void ReadStorageImageDescriptor(bool useDescriptorHeap, Device* device, VkImageViewCreateInfo viewInfo, VkImageView imageView);

	void ReadUniformBufferDescriptor(bool useDescriptorHeap, Device* device, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize);
	void ReadUniformTexelBufferDescriptor(bool useDescriptorHeap, Device* device, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize, VkFormat format);
	void ReadStorageTexelBufferDescriptor(bool useDescriptorHeap, Device* device, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize, VkFormat format);
	void ReadStorageBufferDescriptor(bool useDescriptorHeap, Device* device, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize);
	void ReadAccelerationStructureDescriptor(bool useDescriptorHeap, Device* device, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize);

	void ReadSamplerDescriptor(bool useDescriptorHeap, Device* device, VkSampler sampler, const VkSamplerCreateInfo& info);

	size_t CopyRawDescriptor(void* dest) const;
	size_t GetRawDescriptorSize() const noexcept { return m_rawDescriptorSize; }

private:
	// Descriptor heap
	void ReadImageDescriptorForHeap(VkDevice device, size_t descriptorSize, VkImageLayout layout, VkDescriptorType descriptorType, const VkImageViewCreateInfo& viewInfo);
	void ReadBufferDescriptorForHeap(VkDevice device, size_t descriptorSize, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize, VkDescriptorType descriptorType);
	void ReadTexelBufferDescriptorForHeap(VkDevice device, size_t descriptorSize, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize, VkDescriptorType descriptorType, VkFormat format);
	void SetSizeAndReadDescriptorForHeap(VkDevice device, size_t descriptorSize, const VkResourceDescriptorInfoEXT& resInfo);

	// Descriptor buffer
	void ReadImageDescriptorForBuffer(VkDevice device, size_t descriptorSize, VkImageLayout layout, VkDescriptorType descriptorType, VkImageView imageView);
	void ReadBufferDescriptorForBuffer(VkDevice device, size_t descriptorSize, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize, VkDescriptorType descriptorType, VkFormat format = VK_FORMAT_UNDEFINED);

private:
	std::array<std::byte, kMaxRawDescriptorSize> m_rawDescriptor;
	size_t m_rawDescriptorSize{ 0 };
};

} // namespace Luna::VK