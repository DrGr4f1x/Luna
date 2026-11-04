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

#include "DescriptorVK.h"

#include "DeviceVK.h"


namespace Luna::VK
{

Descriptor::Descriptor()
{
	ZeroMemory(&m_rawDescriptor[0], m_rawDescriptor.size());
}


void Descriptor::ReadSampledImageDescriptor(bool useDescriptorHeap, Device* device, VkImageViewCreateInfo viewInfo, VkImageView imageView)
{
	VkDevice vkDevice = device->GetVulkanDevice();

	if (useDescriptorHeap)
	{
		ReadImageDescriptorForHeap(
			vkDevice,
			device->GetDeviceCaps().descriptorHeap.imageDescriptorSize,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			viewInfo);
	}
	else
	{
		VkDescriptorImageInfo info{
			.imageView		= imageView,
			.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		VkDescriptorGetInfoEXT descriptorGetInfo{ 
			.sType	= VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type	= VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			.data	= { .pSampledImage = &info }
		};

		m_rawDescriptorSize = device->GetDeviceCaps().descriptorBuffer.descriptorSize.sampledImage;
		assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

		vkGetDescriptorEXT(vkDevice, &descriptorGetInfo, m_rawDescriptorSize, (void*)(&m_rawDescriptor[0]));
	}
}


void Descriptor::ReadStorageImageDescriptor(bool useDescriptorHeap, Device* device, VkImageViewCreateInfo viewInfo, VkImageView imageView)
{
	VkDevice vkDevice = device->GetVulkanDevice();

	if (useDescriptorHeap)
	{
		ReadImageDescriptorForHeap(
			vkDevice,
			device->GetDeviceCaps().descriptorHeap.imageDescriptorSize,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			viewInfo);
	}
	else
	{
		VkDescriptorImageInfo info{
			.imageView		= imageView,
			.imageLayout	= VK_IMAGE_LAYOUT_GENERAL
		};

		VkDescriptorGetInfoEXT descriptorGetInfo{
			.sType	= VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.data	= { .pStorageImage = &info }
		};

		m_rawDescriptorSize = device->GetDeviceCaps().descriptorBuffer.descriptorSize.storageImage;
		assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

		vkGetDescriptorEXT(vkDevice, &descriptorGetInfo, m_rawDescriptorSize, (void*)(&m_rawDescriptor[0]));
	}
}


void Descriptor::ReadUniformBufferDescriptor(bool useDescriptorHeap, Device* device, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize)
{
	VkDevice vkDevice = device->GetVulkanDevice();

	if (useDescriptorHeap)
	{
		ReadBufferDescriptorForHeap(
			vkDevice,
			device->GetDeviceCaps().descriptorHeap.bufferDescriptorSize,
			bufferAddress,
			bufferSize,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	}
	else
	{
		VkDescriptorAddressInfoEXT addressInfo{
			.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
			.address	= bufferAddress,
			.range		= bufferSize,
			.format		= VK_FORMAT_UNDEFINED
		};

		VkDescriptorGetInfoEXT descriptorGetInfo{
			.sType	= VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type	= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.data	= { .pUniformBuffer = &addressInfo }
		};

		m_rawDescriptorSize = device->GetDeviceCaps().descriptorBuffer.descriptorSize.uniformBuffer;
		assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

		vkGetDescriptorEXT(vkDevice, &descriptorGetInfo, m_rawDescriptorSize, (void*)(&m_rawDescriptor[0]));
	}
}


void Descriptor::ReadUniformTexelBufferDescriptor(bool useDescriptorHeap, Device* device, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize, VkFormat format)
{
	VkDevice vkDevice = device->GetVulkanDevice();

	if (useDescriptorHeap)
	{
		ReadTexelBufferDescriptorForHeap(
			vkDevice,
			device->GetDeviceCaps().descriptorHeap.bufferDescriptorSize,
			bufferAddress,
			bufferSize,
			VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
			format);
	}
	else
	{
		VkDescriptorAddressInfoEXT addressInfo{
			.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
			.address	= bufferAddress,
			.range		= bufferSize,
			.format		= format
		};

		VkDescriptorGetInfoEXT descriptorGetInfo{
			.sType	= VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type	= VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
			.data	= { .pUniformTexelBuffer = &addressInfo }
		};

		m_rawDescriptorSize = device->GetDeviceCaps().descriptorBuffer.descriptorSize.uniformTexelBuffer;
		assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

		vkGetDescriptorEXT(vkDevice, &descriptorGetInfo, m_rawDescriptorSize, (void*)(&m_rawDescriptor[0]));
	}
}


void Descriptor::ReadStorageTexelBufferDescriptor(bool useDescriptorHeap, Device* device, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize, VkFormat format)
{
	VkDevice vkDevice = device->GetVulkanDevice();

	if (useDescriptorHeap)
	{
		ReadTexelBufferDescriptorForHeap(
			vkDevice,
			device->GetDeviceCaps().descriptorHeap.bufferDescriptorSize,
			bufferAddress,
			bufferSize,
			VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
			format);
	}
	else
	{
		VkDescriptorAddressInfoEXT addressInfo{
			.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
			.address	= bufferAddress,
			.range		= bufferSize,
			.format		= format
		};

		VkDescriptorGetInfoEXT descriptorGetInfo{
			.sType	= VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type	= VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
			.data	= { .pStorageTexelBuffer = &addressInfo }
		};

		m_rawDescriptorSize = device->GetDeviceCaps().descriptorBuffer.descriptorSize.uniformTexelBuffer;
		assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

		vkGetDescriptorEXT(vkDevice, &descriptorGetInfo, m_rawDescriptorSize, (void*)(&m_rawDescriptor[0]));
	}
}


void Descriptor::ReadStorageBufferDescriptor(bool useDescriptorHeap, Device* device, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize)
{
	VkDevice vkDevice = device->GetVulkanDevice();

	if (useDescriptorHeap)
	{
		ReadBufferDescriptorForHeap(
			vkDevice,
			device->GetDeviceCaps().descriptorHeap.bufferDescriptorSize,
			bufferAddress,
			bufferSize,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	}
	else
	{
		VkDescriptorAddressInfoEXT addressInfo{
			.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
			.address	= bufferAddress,
			.range		= bufferSize,
			.format		= VK_FORMAT_UNDEFINED
		};

		VkDescriptorGetInfoEXT descriptorGetInfo{
			.sType	= VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type	= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.data	= { .pStorageBuffer = &addressInfo }
		};

		m_rawDescriptorSize = device->GetDeviceCaps().descriptorBuffer.descriptorSize.uniformBuffer;
		assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

		vkGetDescriptorEXT(vkDevice, &descriptorGetInfo, m_rawDescriptorSize, (void*)(&m_rawDescriptor[0]));
	}
}


void Descriptor::ReadAccelerationStructureDescriptor(bool useDescriptorHeap, Device* device, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize)
{
	VkDevice vkDevice = device->GetVulkanDevice();

	if (useDescriptorHeap)
	{
		ReadBufferDescriptorForHeap(
			vkDevice,
			device->GetDeviceCaps().descriptorHeap.bufferDescriptorSize,
			bufferAddress,
			bufferSize,
			VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
	}
	else
	{
		VkDescriptorGetInfoEXT descriptorGetInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
			.data = { .accelerationStructure = bufferAddress }
		};

		m_rawDescriptorSize = device->GetDeviceCaps().descriptorBuffer.descriptorSize.uniformBuffer;
		assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

		vkGetDescriptorEXT(vkDevice, &descriptorGetInfo, m_rawDescriptorSize, (void*)(&m_rawDescriptor[0]));
	}
}


void Descriptor::ReadSamplerDescriptor(bool useDescriptorHeap, Device* device, VkSampler sampler, const VkSamplerCreateInfo& info)
{
	VkDevice vkDevice = device->GetVulkanDevice();

	if (useDescriptorHeap)
	{
		m_rawDescriptorSize = device->GetDeviceCaps().descriptorHeap.samplerDescriptorSize;
		assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

		VkHostAddressRangeEXT dst{
			.address	= (void*)&m_rawDescriptor[0],
			.size		= m_rawDescriptorSize
		};

		vkWriteSamplerDescriptorsEXT(vkDevice, 1, &info, &dst);
	}
	else
	{
		m_rawDescriptorSize = device->GetDeviceCaps().descriptorBuffer.descriptorSize.sampler;
		assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

		VkDescriptorGetInfoEXT descriptorGetInfo{
			.sType	= VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type	= VK_DESCRIPTOR_TYPE_SAMPLER,
			.data	= { .pSampler = &sampler }
		};

		vkGetDescriptorEXT(vkDevice, &descriptorGetInfo, m_rawDescriptorSize, (void*)(&m_rawDescriptor[0]));
	}
}


size_t Descriptor::CopyRawDescriptor(void* dest) const
{
	assert(m_rawDescriptorSize != 0);
	assert(dest != nullptr);

	memcpy(dest, &m_rawDescriptor[0], m_rawDescriptorSize);

	return m_rawDescriptorSize;
}

void Descriptor::ReadImageDescriptorForHeap(VkDevice device, size_t descriptorSize, VkImageLayout layout, VkDescriptorType descriptorType, const VkImageViewCreateInfo& viewInfo)
{
	VkImageDescriptorInfoEXT imageDescInfo{
		.sType	= VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
		.pView	= &viewInfo,
		.layout	= layout
	};

	VkResourceDescriptorInfoEXT resInfo{
		.sType	= VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
		.type	= descriptorType,
		.data	= { .pImage = &imageDescInfo }
	};

	m_rawDescriptorSize = descriptorSize;
	assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

	VkHostAddressRangeEXT dst{};
	dst.address = (void*)&m_rawDescriptor[0];
	dst.size = m_rawDescriptorSize;
	vkWriteResourceDescriptorsEXT(device, 1, &resInfo, &dst);
}


void Descriptor::ReadBufferDescriptorForHeap(VkDevice device, size_t descriptorSize, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize, VkDescriptorType descriptorType)
{
	VkDeviceAddressRangeEXT addressRange{
		.address	= bufferAddress,
		.size		= bufferSize
	};

	VkResourceDescriptorInfoEXT resInfo{
		.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
		.type = descriptorType,
		.data = {.pAddressRange = &addressRange }
	};

	m_rawDescriptorSize = descriptorSize;
	assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

	VkHostAddressRangeEXT dst{};
	dst.address = (void*)&m_rawDescriptor[0];
	dst.size = m_rawDescriptorSize;
	vkWriteResourceDescriptorsEXT(device, 1, &resInfo, &dst);
}

void Descriptor::ReadTexelBufferDescriptorForHeap(VkDevice device, size_t descriptorSize, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize, VkDescriptorType descriptorType, VkFormat format)
{
	VkTexelBufferDescriptorInfoEXT texelBufferInfo{
		.sType = VK_STRUCTURE_TYPE_TEXEL_BUFFER_DESCRIPTOR_INFO_EXT,
		.format = format,
		.addressRange = { .address = bufferAddress, .size = bufferSize }
	};

	VkResourceDescriptorInfoEXT resInfo{
		.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
		.type = descriptorType,
		.data = { .pTexelBuffer = &texelBufferInfo }
	};

	m_rawDescriptorSize = descriptorSize;
	assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

	VkHostAddressRangeEXT dst{};
	dst.address = (void*)&m_rawDescriptor[0];
	dst.size = m_rawDescriptorSize;
	vkWriteResourceDescriptorsEXT(device, 1, &resInfo, &dst);
}

} // namespace Luna::VK