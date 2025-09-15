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


void DescriptorSet::SetBindlessSRVs(uint32_t slot, std::span<const IDescriptor*> descriptors)
{
	SetDescriptors_Internal(slot, descriptors);
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
	VkDescriptorImageInfo info{
		.imageView		= ((const Descriptor*)depthBuffer->GetSrvDescriptor(depthSrv))->GetImageView(),
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
	VkDescriptorImageInfo info{
		.sampler		= ((const Descriptor*)sampler->GetDescriptor())->GetSampler(),
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
			.buffer		= descriptorVK->GetBuffer(),
			.offset		= 0,
			.range		= descriptorVK->GetElementSize()
		};

		VkWriteDescriptorSet writeDescriptorSet{
			.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet				= m_descriptorSet,
			.dstBinding			= slot,
			.dstArrayElement	= 0,
			.descriptorCount	= 1,
			.descriptorType		= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
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
				.dstBinding			= m_rootParameter.GetRegisterForSlot(slot),
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
				.dstBinding			= m_rootParameter.GetRegisterForSlot(slot),
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


void DescriptorSet::SetDescriptors_Internal(uint32_t slot, std::span<const IDescriptor*> descriptors)
{
	uint32_t rangeIndex = m_rootParameter.GetRangeIndex(slot);
	assert(rangeIndex != ~0u);
	const auto& range = m_rootParameter.table[rangeIndex];

	// TODO: Relax this requirement so that we can set a subset of a bindless array
	assert(range.numDescriptors == descriptors.size());

	// Allocate scratch memory
	size_t scratchSize = 0;
	unique_ptr<std::byte[]> scratchMemory;
	switch (range.descriptorType)
	{
	case DescriptorType::ConstantBuffer:
	case DescriptorType::StructuredBufferSRV:
	case DescriptorType::StructuredBufferUAV:
	case DescriptorType::RawBufferSRV:
	case DescriptorType::RawBufferUAV:
		scratchSize = sizeof(VkDescriptorBufferInfo) * descriptors.size();
		scratchMemory.reset(new std::byte[scratchSize]);
		break;

	case DescriptorType::TextureSRV:
	case DescriptorType::TextureUAV:
	case DescriptorType::Sampler:
		scratchSize = sizeof(VkDescriptorImageInfo) * descriptors.size();
		scratchMemory.reset(new std::byte[scratchSize]);
		break;

	case DescriptorType::TypedBufferSRV:
	case DescriptorType::TypedBufferUAV:
		scratchSize = sizeof(VkBufferView) * descriptors.size();
		scratchMemory.reset(new std::byte[scratchSize]);
		break;

	default:
		assert(false);
		break;
	}

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= m_rootParameter.GetRangeStartRegister(rangeIndex),
		.dstArrayElement	= 0,
		.descriptorCount	= range.numDescriptors,
		.descriptorType		= DescriptorTypeToVulkan(range.descriptorType)
	};

	std::byte* data = scratchMemory.get();

	switch (range.descriptorType)
	{
	case DescriptorType::Sampler:
		WriteSamplers(writeDescriptorSet, data, descriptors);
		break;

	case DescriptorType::ConstantBuffer:
	case DescriptorType::StructuredBufferSRV:
	case DescriptorType::StructuredBufferUAV:
	case DescriptorType::RawBufferSRV:
	case DescriptorType::RawBufferUAV:
		WriteBuffers(writeDescriptorSet, data, descriptors);
		break;

	case DescriptorType::TextureSRV:
	case DescriptorType::TextureUAV:
		WriteTextures(writeDescriptorSet, data, descriptors);
		break;

	case DescriptorType::TypedBufferSRV:
	case DescriptorType::TypedBufferUAV:
		WriteTypedBuffers(writeDescriptorSet, data, descriptors);
		break;
	}

	UpdateDescriptorSet(writeDescriptorSet);
}


void DescriptorSet::WriteSamplers(VkWriteDescriptorSet& writeDescriptorSet, std::byte* scratchData, std::span<const IDescriptor*> descriptors)
{
	VkDescriptorImageInfo* imageInfos = (VkDescriptorImageInfo*)scratchData;

	for (size_t i = 0; i < descriptors.size(); ++i)
	{
		imageInfos[i].imageView = VK_NULL_HANDLE;
		imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfos[i].sampler = ((const Descriptor*)descriptors[i])->GetSampler();
	}

	writeDescriptorSet.pImageInfo = imageInfos;
}


void DescriptorSet::WriteBuffers(VkWriteDescriptorSet& writeDescriptorSet, std::byte* scratchData, std::span<const IDescriptor*> descriptors)
{
	VkDescriptorBufferInfo* bufferInfos = (VkDescriptorBufferInfo*)scratchData;

	for (size_t i = 0; i < descriptors.size(); ++i)
	{
		const Descriptor* descriptorVK = (const Descriptor*)descriptors[i];

		bufferInfos[i].buffer = descriptorVK->GetBuffer();
		bufferInfos[i].offset = 0;
		bufferInfos[i].range = m_isDynamicBuffer ? descriptorVK->GetElementSize() : VK_WHOLE_SIZE;
	}

	writeDescriptorSet.pBufferInfo = bufferInfos;
}


void DescriptorSet::WriteTextures(VkWriteDescriptorSet& writeDescriptorSet, std::byte* scratchData, std::span<const IDescriptor*> descriptors)
{
	VkDescriptorImageInfo* imageInfos = (VkDescriptorImageInfo*)scratchData;

	for (size_t i = 0; i < descriptors.size(); ++i)
	{
		imageInfos[i].imageView = ((const Descriptor*)descriptors[i])->GetImageView();;
		imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfos[i].sampler = VK_NULL_HANDLE;
	}

	writeDescriptorSet.pImageInfo = imageInfos;
}


void DescriptorSet::WriteTypedBuffers(VkWriteDescriptorSet& writeDescriptorSet, std::byte* scratchData, std::span<const IDescriptor*> descriptors)
{
	VkBufferView* bufferViews = (VkBufferView*)scratchData;

	for (size_t i = 0; i < descriptors.size(); ++i)
	{
		bufferViews[i] = ((const Descriptor*)descriptors[i])->GetBufferView();
	}

	writeDescriptorSet.pTexelBufferView = bufferViews;
}

} // namespace Luna::VK