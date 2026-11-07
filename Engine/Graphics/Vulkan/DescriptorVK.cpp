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


void Descriptor::ReadSampledImageDescriptor(Device* device, VkImageViewCreateInfo viewInfo, VkImageView imageView)
{
	VkDevice vkDevice = device->GetVulkanDevice();

	if (device->UseDescriptorHeap())
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
		ReadImageDescriptorForBuffer(
			vkDevice,
			device->GetDeviceCaps().descriptorBuffer.descriptorSize.sampledImage,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			imageView);
	}
}


void Descriptor::ReadStorageImageDescriptor(Device* device, VkImageViewCreateInfo viewInfo, VkImageView imageView)
{
	VkDevice vkDevice = device->GetVulkanDevice();

	if (device->UseDescriptorHeap())
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
		ReadImageDescriptorForBuffer(
			vkDevice,
			device->GetDeviceCaps().descriptorBuffer.descriptorSize.storageImage,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			imageView);
	}
}


void Descriptor::ReadUniformBufferDescriptor(Device* device, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize)
{
	VkDevice vkDevice = device->GetVulkanDevice();

	if (device->UseDescriptorHeap())
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
		ReadBufferDescriptorForBuffer(
			vkDevice,
			device->GetDeviceCaps().descriptorBuffer.descriptorSize.uniformBuffer,
			bufferAddress,
			bufferSize,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	}
}


void Descriptor::ReadUniformTexelBufferDescriptor(Device* device, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize, VkFormat format)
{
	VkDevice vkDevice = device->GetVulkanDevice();

	if (device->UseDescriptorHeap())
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
		ReadBufferDescriptorForBuffer(
			vkDevice,
			device->GetDeviceCaps().descriptorBuffer.descriptorSize.uniformTexelBuffer,
			bufferAddress,
			bufferSize,
			VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
			format);
	}
}


void Descriptor::ReadStorageTexelBufferDescriptor(Device* device, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize, VkFormat format)
{
	VkDevice vkDevice = device->GetVulkanDevice();

	if (device->UseDescriptorHeap())
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
		ReadBufferDescriptorForBuffer(
			vkDevice,
			device->GetDeviceCaps().descriptorBuffer.descriptorSize.storageTexelBuffer,
			bufferAddress,
			bufferSize,
			VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
			format);
	}
}


void Descriptor::ReadStorageBufferDescriptor(Device* device, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize)
{
	VkDevice vkDevice = device->GetVulkanDevice();

	if (device->UseDescriptorHeap())
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
		ReadBufferDescriptorForBuffer(
			vkDevice,
			device->GetDeviceCaps().descriptorBuffer.descriptorSize.storageBuffer,
			bufferAddress,
			bufferSize,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	}
}


void Descriptor::ReadAccelerationStructureDescriptor(Device* device, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize)
{
	VkDevice vkDevice = device->GetVulkanDevice();

	if (device->UseDescriptorHeap())
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
		ReadBufferDescriptorForBuffer(
			vkDevice,
			device->GetDeviceCaps().descriptorBuffer.descriptorSize.accelerationStructure,
			bufferAddress,
			bufferSize,
			VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
	}
}


void Descriptor::ReadSamplerDescriptor(Device* device, VkSampler sampler, const VkSamplerCreateInfo& info)
{
	VkDevice vkDevice = device->GetVulkanDevice();

	if (device->UseDescriptorHeap())
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

	SetSizeAndReadDescriptorForHeap(device, descriptorSize, resInfo);
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

	SetSizeAndReadDescriptorForHeap(device, descriptorSize, resInfo);
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

	SetSizeAndReadDescriptorForHeap(device, descriptorSize, resInfo);
}


void Descriptor::SetSizeAndReadDescriptorForHeap(VkDevice device, size_t descriptorSize, const VkResourceDescriptorInfoEXT& resInfo)
{
	m_rawDescriptorSize = descriptorSize;
	assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

	VkHostAddressRangeEXT range{ .address = (void*)&m_rawDescriptor[0], .size = m_rawDescriptorSize };
	vkWriteResourceDescriptorsEXT(device, 1, &resInfo, &range);
}


void Descriptor::ReadImageDescriptorForBuffer(VkDevice device, size_t descriptorSize, VkImageLayout layout, VkDescriptorType descriptorType, VkImageView imageView)
{
	VkDescriptorImageInfo info{
		.imageView		= imageView,
		.imageLayout	= layout
	};

	VkDescriptorGetInfoEXT descriptorGetInfo{
		.sType	= VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
		.type	= descriptorType
	};

	switch (descriptorType)
	{
	case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		descriptorGetInfo.data.pSampledImage = &info;
		break;

	case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
		descriptorGetInfo.data.pStorageImage = &info;
		break;

	default:
		assert(false);
		break;
	}

	m_rawDescriptorSize = descriptorSize;
	assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

	vkGetDescriptorEXT(device, &descriptorGetInfo, m_rawDescriptorSize, (void*)(&m_rawDescriptor[0]));
}


void Descriptor::ReadBufferDescriptorForBuffer(VkDevice device, size_t descriptorSize, VkDeviceAddress bufferAddress, VkDeviceSize bufferSize, VkDescriptorType descriptorType, VkFormat format)
{
	VkDescriptorAddressInfoEXT addressInfo{
		.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
		.address	= bufferAddress,
		.range		= bufferSize,
		.format		= VK_FORMAT_UNDEFINED
	};

	VkDescriptorGetInfoEXT descriptorGetInfo{
		.sType	= VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
		.type	= descriptorType
	};

	switch (descriptorType)
	{
	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		descriptorGetInfo.data.pUniformBuffer = &addressInfo;
		break;

	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		descriptorGetInfo.data.pStorageBuffer = &addressInfo;
		break;

	case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		descriptorGetInfo.data.pUniformTexelBuffer = &addressInfo;
		break;

	case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
		descriptorGetInfo.data.pStorageTexelBuffer = &addressInfo;
		break;

	case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
		descriptorGetInfo.data.accelerationStructure = bufferAddress;
		break;

	default:
		assert(false);
		break;
	}

	m_rawDescriptorSize = descriptorSize;
	assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

	vkGetDescriptorEXT(device, &descriptorGetInfo, m_rawDescriptorSize, (void*)(&m_rawDescriptor[0]));
}

} // namespace Luna::VK