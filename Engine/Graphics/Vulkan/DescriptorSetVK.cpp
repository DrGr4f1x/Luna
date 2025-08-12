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
#include "DescriptorVK.h"
#include "DeviceVK.h"
#include "GpuBufferVK.h"
#include "SamplerVK.h"
#include "TextureVK.h"

using namespace std;


namespace Luna::VK
{

DescriptorSet::DescriptorSet(Device* device, const RootParameter& rootParameter)
	: m_device{ device }
	, m_rootParameter{ rootParameter }
{}


void DescriptorSet::SetSRV(uint32_t slot, const IDescriptor* descriptor)
{
	SetSRVUAV<true>(slot, descriptor);
}


void DescriptorSet::SetUAV(uint32_t slot, const IDescriptor* descriptor)
{
	SetSRVUAV<false>(slot, descriptor);
}


void DescriptorSet::SetCBV(uint32_t slot, const IDescriptor* descriptor)
{
	const Descriptor* descriptorVK = (const Descriptor*)descriptor;

	assert(descriptorVK->GetDescriptorClass() == DescriptorClass::Buffer);
	assert(m_rootParameter.parameterType == RootParameterType::RootCBV || m_rootParameter.parameterType == RootParameterType::Table);

	if (!m_isDynamicBuffer)
	{
		assert(m_rootParameter.GetDescriptorType(slot) == DescriptorType::ConstantBuffer);
	}

	VkDescriptorBufferInfo info{
		.buffer		= descriptorVK->GetBuffer(),
		.offset		= 0,
		.range		= m_isDynamicBuffer ? descriptorVK->GetElementSize() : VK_WHOLE_SIZE
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

	UpdateDescriptorSet(writeDescriptorSet);
}


void DescriptorSet::SetSampler(uint32_t slot, const IDescriptor* descriptor)
{
	const Descriptor* descriptorVK = (const Descriptor*)descriptor;

	assert(descriptorVK->GetDescriptorClass() == DescriptorClass::Sampler);
	assert(m_rootParameter.GetDescriptorType(slot) == DescriptorType::Sampler);

	VkDescriptorImageInfo info{
		.sampler		= descriptorVK->GetSampler(),
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

	UpdateDescriptorSet(writeDescriptorSet);
}


void DescriptorSet::SetSRV(uint32_t slot, ColorBufferPtr colorBuffer)
{
	VkDescriptorImageInfo info{
		.imageView		= ((const Descriptor*)colorBuffer->GetSrvDescriptor())->GetImageView(),
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

	UpdateDescriptorSet(writeDescriptorSet);
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

	UpdateDescriptorSet(writeDescriptorSet);
}


void DescriptorSet::SetSRV(uint32_t slot, GpuBufferPtr gpuBuffer)
{
	const Descriptor* descriptor = (const Descriptor*)gpuBuffer->GetSrvDescriptor();

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
		texelBufferView = descriptor->GetBufferView();

		writeDescriptorSet.pTexelBufferView = &texelBufferView;
	}
	else
	{
		info.buffer = ((const Descriptor*)gpuBuffer->GetSrvDescriptor())->GetBuffer();
		info.offset = 0;
		info.range = m_isDynamicBuffer ? gpuBuffer->GetElementSize() : VK_WHOLE_SIZE;

		writeDescriptorSet.pBufferInfo = &info;
	}

	UpdateDescriptorSet(writeDescriptorSet);
}


void DescriptorSet::SetSRV(uint32_t slot, TexturePtr texture)
{
	// TODO: Try this with GetPlatformObject()

	const Texture* textureVK = (const Texture*)texture.Get();
	assert(textureVK != nullptr);

	VkDescriptorImageInfo info{
		.imageView		= ((const Descriptor*)textureVK->GetDescriptor())->GetImageView(),
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

	UpdateDescriptorSet(writeDescriptorSet);
}


void DescriptorSet::SetUAV(uint32_t slot, ColorBufferPtr colorBuffer, uint32_t uavIndex)
{
	VkDescriptorImageInfo info{
		.imageView		= ((const Descriptor*)colorBuffer->GetUavDescriptor(uavIndex))->GetImageView(),
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

	UpdateDescriptorSet(writeDescriptorSet);
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
	const Descriptor* descriptor = (const Descriptor*)gpuBuffer->GetUavDescriptor();

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
		texelBufferView = descriptor->GetBufferView();

		writeDescriptorSet.pTexelBufferView = &texelBufferView;
	}
	else
	{
		info.buffer = ((const Descriptor*)gpuBuffer->GetSrvDescriptor())->GetBuffer();
		info.offset = 0;
		info.range = m_isDynamicBuffer ? gpuBuffer->GetElementSize() : VK_WHOLE_SIZE;

		writeDescriptorSet.pBufferInfo = &info;
	}

	UpdateDescriptorSet(writeDescriptorSet);
}


void DescriptorSet::SetCBV(uint32_t slot, GpuBufferPtr gpuBuffer)
{
	VkDescriptorBufferInfo info{
		.buffer		= ((const Descriptor*)gpuBuffer->GetCbvDescriptor())->GetBuffer(),
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

	UpdateDescriptorSet(writeDescriptorSet);
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

	UpdateDescriptorSet(writeDescriptorSet);
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


void DescriptorSet::UpdateDescriptorSet(const VkWriteDescriptorSet& writeDescriptorSet)
{
	vkUpdateDescriptorSets(
		m_device->GetVulkanDevice(),
		1,
		&writeDescriptorSet,
		0,
		nullptr);
}


template<bool isSrv>
void DescriptorSet::SetSRVUAV(uint32_t slot, const IDescriptor* descriptor)
{
	const Descriptor* descriptorVK = (const Descriptor*)descriptor;

	if (IsRootDescriptorType(m_rootParameter.parameterType))
	{
		assert(descriptorVK->GetBuffer() != VK_NULL_HANDLE);
		assert(descriptorVK->GetElementSize() != 0);

		VkDescriptorBufferInfo info{
			.buffer = descriptorVK->GetBuffer(),
			.offset = 0,
			.range = descriptorVK->GetElementSize()
		};

		VkWriteDescriptorSet writeDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = m_descriptorSet,
			.dstBinding = slot,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
		};

		UpdateDescriptorSet(writeDescriptorSet);
	}
	else
	{
		DescriptorType descriptorType = m_rootParameter.GetDescriptorType(slot);
		assert(IsDescriptorTypeSRV(descriptorType));

		// Images (i.e. ColorBuffer, DepthBuffer, or Texture)
		if (descriptorVK->GetDescriptorClass() == DescriptorClass::Image)
		{
			VkDescriptorImageInfo info{
				.imageView		= descriptorVK->GetImageView(),
				.imageLayout	= isSrv ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL
			};

			VkWriteDescriptorSet writeDescriptorSet{
				.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet				= m_descriptorSet,
				.dstBinding			= slot,
				.dstArrayElement	= 0,
				.descriptorCount	= 1,
				.descriptorType		= isSrv ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.pImageInfo			= &info
			};

			UpdateDescriptorSet(writeDescriptorSet);
		}
		// Buffers (i.e. GpuBuffer)
		else if (descriptorVK->GetDescriptorClass() == DescriptorClass::Buffer)
		{
			VkBufferView texelBufferView = VK_NULL_HANDLE;
			VkDescriptorBufferInfo info{};

			VkWriteDescriptorSet writeDescriptorSet{
				.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet				= m_descriptorSet,
				.dstBinding			= slot,
				.dstArrayElement	= 0,
				.descriptorCount	= 1,
			};

			// StructuredBuffers and RawBuffers use VkDescriptorBufferInfo
			if (descriptorType == DescriptorType::StructuredBufferSRV || descriptorType == DescriptorType::StructuredBufferUAV ||
				descriptorType == DescriptorType::RawBufferSRV || descriptorType == DescriptorType::RawBufferUAV)
			{
				info.buffer = descriptorVK->GetBuffer();
				info.offset = 0;
				info.range = m_isDynamicBuffer ? descriptorVK->GetElementSize() : VK_WHOLE_SIZE;

				writeDescriptorSet.descriptorType = m_isDynamicBuffer ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				writeDescriptorSet.pBufferInfo = &info;
			}
			// TypedBuffers use VkBufferView instead
			else
			{
				texelBufferView = descriptorVK->GetBufferView();

				writeDescriptorSet.descriptorType = isSrv ? VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
				writeDescriptorSet.pTexelBufferView = &texelBufferView;
			}

			UpdateDescriptorSet(writeDescriptorSet);
		}
	}
}

} // namespace Luna::VK