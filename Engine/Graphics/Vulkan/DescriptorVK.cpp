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


VkImage Descriptor::GetImage() const
{
	if (m_image)
	{
		return m_image->Get();
	}
	return VK_NULL_HANDLE;
}


VkBuffer Descriptor::GetBuffer() const
{
	if (m_buffer)
	{
		return m_buffer->Get();
	}
	return VK_NULL_HANDLE;
}


void Descriptor::SetImageView(CVkImage* image, CVkImageView* imageView)
{
	m_image = image;
	m_imageView = imageView;
	m_descriptorClass = DescriptorClass::Image;
}


void Descriptor::SetBufferView(CVkBuffer* buffer, CVkBufferView* bufferView, size_t elementSize, size_t bufferSize, VkFormat format)
{
	m_buffer = buffer;
	m_bufferView = bufferView;
	m_elementSize = elementSize;
	m_bufferSize = bufferSize;
	m_format = format;
	m_descriptorClass = DescriptorClass::Buffer;
}


void Descriptor::SetSampler(CVkSampler* sampler)
{
	m_sampler = sampler;
	m_descriptorClass = DescriptorClass::Sampler;
}


void Descriptor::ReadRawDescriptor(Device* device, DescriptorType descriptorType)
{
	assert(m_rawDescriptorSize == 0);

	VkDevice vkDevice = device->GetVulkanDevice();

	switch (descriptorType)
	{

	case DescriptorType::ConstantBuffer:
	{
		assert(m_buffer != nullptr);

		VkDescriptorAddressInfoEXT addressInfo{
			.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
			.address	= GetBufferDeviceAddress(vkDevice, m_buffer->Get()),
			.range		= m_bufferSize,
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
		break;

	case DescriptorType::TextureSRV:
	{
		assert(m_imageView != nullptr);

		VkDescriptorImageInfo info{
			.imageView		= m_imageView->Get(),
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
		break;

	case DescriptorType::TextureUAV:
	case DescriptorType::SamplerFeedbackTextureUAV:
	{
		assert(m_imageView != nullptr);

		VkDescriptorImageInfo info{
			.imageView		= m_imageView->Get(),
			.imageLayout	= VK_IMAGE_LAYOUT_GENERAL
		};

		VkDescriptorGetInfoEXT descriptorGetInfo{
			.sType	= VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.data	= {.pStorageImage = &info }
		};

		m_rawDescriptorSize = device->GetDeviceCaps().descriptorBuffer.descriptorSize.storageImage;
		assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

		vkGetDescriptorEXT(vkDevice, &descriptorGetInfo, m_rawDescriptorSize, (void*)(&m_rawDescriptor[0]));
	}
		break;

	case DescriptorType::TypedBufferSRV:
	{
		assert(m_buffer != nullptr);
		assert(m_format != VK_FORMAT_UNDEFINED);

		VkDescriptorAddressInfoEXT addressInfo{
			.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
			.address	= GetBufferDeviceAddress(vkDevice, m_buffer->Get()),
			.range		= m_bufferSize,
			.format		= m_format
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
		break;

	case DescriptorType::TypedBufferUAV:
	{
		assert(m_buffer != nullptr);
		assert(m_format != VK_FORMAT_UNDEFINED);

		VkDescriptorAddressInfoEXT addressInfo{
			.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
			.address	= GetBufferDeviceAddress(vkDevice, m_buffer->Get()),
			.range		= m_bufferSize,
			.format		= m_format
		};

		VkDescriptorGetInfoEXT descriptorGetInfo{
			.sType	= VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type	= VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
			.data	= { .pStorageTexelBuffer = &addressInfo }
		};

		m_rawDescriptorSize = device->GetDeviceCaps().descriptorBuffer.descriptorSize.storageTexelBuffer;
		assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

		vkGetDescriptorEXT(vkDevice, &descriptorGetInfo, m_rawDescriptorSize, (void*)(&m_rawDescriptor[0]));
	}
		break;

	case DescriptorType::StructuredBufferSRV:
	case DescriptorType::StructuredBufferUAV:
	case DescriptorType::RawBufferSRV:
	case DescriptorType::RawBufferUAV:
	{
		assert(m_buffer != nullptr);

		VkDescriptorAddressInfoEXT addressInfo{
			.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
			.address	= GetBufferDeviceAddress(vkDevice, m_buffer->Get()),
			.range		= m_bufferSize,
			.format		= VK_FORMAT_UNDEFINED
		};

		VkDescriptorGetInfoEXT descriptorGetInfo{
			.sType	= VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type	= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.data	 { .pStorageBuffer = &addressInfo }
		};

		m_rawDescriptorSize = device->GetDeviceCaps().descriptorBuffer.descriptorSize.storageBuffer;
		assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

		vkGetDescriptorEXT(vkDevice, &descriptorGetInfo, m_rawDescriptorSize, (void*)(&m_rawDescriptor[0]));
	}
		break;

	case DescriptorType::Sampler:
	{
		assert(m_sampler != nullptr);
		VkSampler sampler = m_sampler->Get();

		VkDescriptorGetInfoEXT descriptorGetInfo{
			.sType	= VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type	= VK_DESCRIPTOR_TYPE_SAMPLER,
			.data	= { .pSampler = &sampler }
		};

		m_rawDescriptorSize = device->GetDeviceCaps().descriptorBuffer.descriptorSize.sampler;
		assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

		vkGetDescriptorEXT(vkDevice, &descriptorGetInfo, m_rawDescriptorSize, (void*)(&m_rawDescriptor[0]));
	}
		break;

	case DescriptorType::RayTracingAccelStruct:
	{
		assert(m_buffer != nullptr);

		VkDescriptorGetInfoEXT descriptorGetInfo{
			.sType	= VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type	= VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
			.data	= { .accelerationStructure = GetBufferDeviceAddress(vkDevice, m_buffer->Get()) }
		};

		m_rawDescriptorSize = device->GetDeviceCaps().descriptorBuffer.descriptorSize.accelerationStructure;
		assert(m_rawDescriptorSize <= kMaxRawDescriptorSize);

		vkGetDescriptorEXT(vkDevice, &descriptorGetInfo, m_rawDescriptorSize, (void*)(&m_rawDescriptor[0]));
	}
		break;
	}

	assert(m_rawDescriptorSize != 0);
}


size_t Descriptor::CopyRawDescriptor(void* dest) const
{
	assert(m_rawDescriptorSize != 0);
	assert(dest != nullptr);

	memcpy(dest, &m_rawDescriptor[0], m_rawDescriptorSize);

	return m_rawDescriptorSize;
}

} // namespace Luna::VK