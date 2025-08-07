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

#include "DescriptorSetVK.h"

#include "ColorBufferVK.h"
#include "DepthBufferVK.h"
#include "DeviceVK.h"
#include "GpuBufferVK.h"
#include "SamplerVK.h"
#include "TextureVK.h"

using namespace std;


namespace Luna::VK
{

void DescriptorSet::SetSRV(uint32_t slot, ColorBufferPtr colorBuffer)
{
	// TODO: Try this with GetPlatformObject()

	const ColorBuffer* colorBufferVK = (const ColorBuffer*)colorBuffer.get();
	assert(colorBufferVK != nullptr);

	VkDescriptorImageInfo info{
		.imageView		= colorBufferVK->GetSrvDescriptor().GetImageView(),
		.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= slot,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.pImageInfo			= &info
	};

	vkUpdateDescriptorSets(
		m_device->GetVulkanDevice(),
		1,
		&writeDescriptorSet,
		0,
		nullptr);
}


void DescriptorSet::SetSRV(uint32_t slot, DepthBufferPtr depthBuffer, bool depthSrv)
{
	// TODO: Try this with GetPlatformObject()

	const DepthBuffer* depthBufferVK = (const DepthBuffer*)depthBuffer.get();
	assert(depthBufferVK != nullptr);

	const auto& descriptor = depthSrv ? depthBufferVK->GetDescriptor(DepthStencilAspect::DepthReadOnly) : depthBufferVK->GetDescriptor(DepthStencilAspect::StencilReadOnly);

	VkDescriptorImageInfo info{
		.imageView		= descriptor.GetImageView(),
		.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= slot,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.pImageInfo			= &info
	};

	vkUpdateDescriptorSets(
		m_device->GetVulkanDevice(),
		1,
		&writeDescriptorSet,
		0,
		nullptr);
}


void DescriptorSet::SetSRV(uint32_t slot, GpuBufferPtr gpuBuffer)
{
	// TODO: Try this with GetPlatformObject()

	const GpuBuffer* gpuBufferVK = (const GpuBuffer*)gpuBuffer.get();
	assert(gpuBufferVK != nullptr);

	if (m_isDynamicBuffer)
	{
		assert(slot == 0);
	}

	const auto& descriptor = gpuBufferVK->GetDescriptor();

	VkBufferView texelBufferView = VK_NULL_HANDLE;
	VkDescriptorBufferInfo info{};

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= slot,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= m_isDynamicBuffer ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	};

	if (gpuBuffer->GetResourceType() == ResourceType::TypedBuffer)
	{
		texelBufferView = descriptor.GetBufferView();

		writeDescriptorSet.pTexelBufferView = &texelBufferView;
	}
	else
	{
		info.buffer = gpuBufferVK->GetBuffer();
		info.offset = 0;
		info.range = m_isDynamicBuffer ? gpuBuffer->GetElementSize() : VK_WHOLE_SIZE;

		writeDescriptorSet.pBufferInfo = &info;
	}

	vkUpdateDescriptorSets(
		m_device->GetVulkanDevice(),
		1,
		&writeDescriptorSet,
		0,
		nullptr);
}


void DescriptorSet::SetSRV(uint32_t slot, TexturePtr texture)
{
	// TODO: Try this with GetPlatformObject()

	const Texture* textureVK = (const Texture*)texture.Get();
	assert(textureVK != nullptr);

	VkDescriptorImageInfo info{
		.imageView		= textureVK->GetDescriptor().GetImageView(),
		.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= slot,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.pImageInfo			= &info
	};

	vkUpdateDescriptorSets(
		m_device->GetVulkanDevice(),
		1,
		&writeDescriptorSet,
		0,
		nullptr);
}


void DescriptorSet::SetUAV(uint32_t slot, ColorBufferPtr colorBuffer, uint32_t uavIndex)
{
	// TODO: Try this with GetPlatformObject()

	const ColorBuffer* colorBufferVK = (const ColorBuffer*)colorBuffer.get();
	assert(colorBufferVK != nullptr);

	VkDescriptorImageInfo info{
		.imageView		= colorBufferVK->GetSrvDescriptor().GetImageView(),
		.imageLayout	= VK_IMAGE_LAYOUT_GENERAL
	};

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= slot,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo			= &info
	};

	vkUpdateDescriptorSets(
		m_device->GetVulkanDevice(),
		1,
		&writeDescriptorSet,
		0,
		nullptr);
}


void DescriptorSet::SetUAV(uint32_t slot, DepthBufferPtr depthBuffer)
{
	// TODO: Try this with GetPlatformObject()

	const DepthBuffer* depthBufferVK = (const DepthBuffer*)depthBuffer.get();
	assert(depthBufferVK != nullptr);

	assert_msg(false, "Depth UAVs not yet supported");
}


void DescriptorSet::SetUAV(uint32_t slot, GpuBufferPtr gpuBuffer)
{
	// TODO: Try this with GetPlatformObject()

	const GpuBuffer* gpuBufferVK = (const GpuBuffer*)gpuBuffer.get();
	assert(gpuBufferVK != nullptr);

	if (m_isDynamicBuffer)
	{
		assert(slot == 0);
	}

	const auto& descriptor = gpuBufferVK->GetDescriptor();

	VkBufferView texelBufferView = VK_NULL_HANDLE;
	VkDescriptorBufferInfo info{};

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= slot,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= m_isDynamicBuffer ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	};

	if (gpuBuffer->GetResourceType() == ResourceType::TypedBuffer)
	{
		texelBufferView = descriptor.GetBufferView();

		writeDescriptorSet.pTexelBufferView = &texelBufferView;
	}
	else
	{
		info.buffer = gpuBufferVK->GetBuffer();
		info.offset = 0;
		info.range = m_isDynamicBuffer ? gpuBuffer->GetElementSize() : VK_WHOLE_SIZE;

		writeDescriptorSet.pBufferInfo = &info;
	}

	vkUpdateDescriptorSets(
		m_device->GetVulkanDevice(),
		1,
		&writeDescriptorSet,
		0,
		nullptr);
}


void DescriptorSet::SetCBV(uint32_t slot, GpuBufferPtr gpuBuffer)
{
	// TODO: Try this with GetPlatformObject()

	const GpuBuffer* gpuBufferVK = (const GpuBuffer*)gpuBuffer.get();
	assert(gpuBufferVK != nullptr);

	if (m_isDynamicBuffer)
	{
		assert(slot == 0);
	}

	const auto& descriptor = gpuBufferVK->GetDescriptor();

	VkDescriptorBufferInfo info{
		.buffer		= gpuBufferVK->GetBuffer(),
		.offset		= 0,
		.range		= m_isDynamicBuffer ? gpuBuffer->GetElementSize() : VK_WHOLE_SIZE
	};

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= slot,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= m_isDynamicBuffer ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo		= &info
	};

	vkUpdateDescriptorSets(
		m_device->GetVulkanDevice(),
		1,
		&writeDescriptorSet,
		0,
		nullptr);
}


void DescriptorSet::SetSampler(uint32_t slot, SamplerPtr sampler)
{
	// TODO: Try this with GetPlatformObject()

	const Sampler* samplerVK = (const Sampler*)sampler.get();
	assert(samplerVK != nullptr);

	VkDescriptorImageInfo info{
		.sampler		= samplerVK->GetDescriptor().GetSampler(),
		.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= slot,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_SAMPLER,
		.pImageInfo			= &info
	};

	vkUpdateDescriptorSets(
		m_device->GetVulkanDevice(),
		1,
		&writeDescriptorSet,
		0,
		nullptr);
}


void DescriptorSet::SetDynamicOffset(uint32_t offset)
{
	assert(m_isDynamicBuffer);

	m_dynamicOffset = offset;
}


bool DescriptorSet::HasDescriptors() const
{
	return (m_numDescriptors != 0) || m_isDynamicBuffer;
}

} // namespace Luna::VK