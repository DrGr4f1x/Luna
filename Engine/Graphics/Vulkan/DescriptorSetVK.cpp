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


void DescriptorSet::SetSRV(uint32_t srvRegister, const IDescriptor* descriptor)
{
	SetSRVUAV<true>(srvRegister, descriptor);
}


void DescriptorSet::SetUAV(uint32_t uavRegister, const IDescriptor* descriptor)
{
	SetSRVUAV<false>(uavRegister, descriptor);
}


void DescriptorSet::SetCBV(uint32_t cbvRegister, const IDescriptor* descriptor)
{
	const Descriptor* descriptorVK = (const Descriptor*)descriptor;

	assert(descriptorVK->GetDescriptorClass() == DescriptorClass::Buffer);
	assert(m_rootParameter.parameterType == RootParameterType::Table);

	VkDescriptorBufferInfo info{
		.buffer		= descriptorVK->GetBuffer(),
		.offset		= 0,
		.range		= VK_WHOLE_SIZE
	};

	const uint32_t regShift = GetRegisterShiftCBV();

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= regShift + cbvRegister,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo		= &info
	};

	UpdateDescriptorSet(writeDescriptorSet);
}


void DescriptorSet::SetSampler(uint32_t samplerRegister, const IDescriptor* descriptor)
{
	const Descriptor* descriptorVK = (const Descriptor*)descriptor;

	assert(descriptorVK->GetDescriptorClass() == DescriptorClass::Sampler);

	VkDescriptorImageInfo info{
		.sampler		= descriptorVK->GetSampler(),
		.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	const uint32_t regShift = GetRegisterShiftSampler();

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= regShift + samplerRegister,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_SAMPLER,
		.pImageInfo			= &info
	};

	UpdateDescriptorSet(writeDescriptorSet);
}


void DescriptorSet::SetBindlessSRVs(uint32_t srvRegister, std::span<const IDescriptor*> descriptors)
{
	SetDescriptors_Internal<DescriptorRegisterType::SRV>(srvRegister, descriptors);
}


void DescriptorSet::SetSRV(uint32_t srvRegister, ColorBufferPtr colorBuffer)
{
	VkDescriptorImageInfo info{
		.imageView		= ((const Descriptor*)colorBuffer->GetSrvDescriptor())->GetImageView(),
		.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	const uint32_t regShift = GetRegisterShiftSRV();

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= regShift + srvRegister,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.pImageInfo			= &info
	};

	UpdateDescriptorSet(writeDescriptorSet);
}


void DescriptorSet::SetSRV(uint32_t srvRegister, DepthBufferPtr depthBuffer, bool depthSrv)
{
	VkDescriptorImageInfo info{
		.imageView		= ((const Descriptor*)depthBuffer->GetSrvDescriptor(depthSrv))->GetImageView(),
		.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	const uint32_t regShift = GetRegisterShiftSRV();

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= regShift + srvRegister,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.pImageInfo			= &info
	};

	UpdateDescriptorSet(writeDescriptorSet);
}


void DescriptorSet::SetSRV(uint32_t srvRegister, GpuBufferPtr gpuBuffer)
{
	const Descriptor* descriptor = (const Descriptor*)gpuBuffer->GetSrvDescriptor();

	const uint32_t regShift = GetRegisterShiftSRV();

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= regShift + srvRegister,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	};

	VkBufferView texelBufferView = VK_NULL_HANDLE;
	VkDescriptorBufferInfo info{};

	if (gpuBuffer->GetResourceType() == ResourceType::TypedBuffer)
	{
		texelBufferView = descriptor->GetBufferView();
		writeDescriptorSet.pTexelBufferView = &texelBufferView;
	}
	else
	{
		info.buffer = ((const Descriptor*)gpuBuffer->GetSrvDescriptor())->GetBuffer();
		info.offset = 0;
		info.range = VK_WHOLE_SIZE;
		
		writeDescriptorSet.pBufferInfo = &info;
	}

	UpdateDescriptorSet(writeDescriptorSet);
}


void DescriptorSet::SetSRV(uint32_t srvRegister, TexturePtr texture)
{
	const Texture* textureVK = (const Texture*)texture.Get();
	assert(textureVK != nullptr);

	VkDescriptorImageInfo info{
		.imageView		= ((const Descriptor*)textureVK->GetDescriptor())->GetImageView(),
		.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	const uint32_t regShift = GetRegisterShiftSRV();

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= regShift + srvRegister,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.pImageInfo			= &info
	};

	UpdateDescriptorSet(writeDescriptorSet);
}


void DescriptorSet::SetUAV(uint32_t uavRegister, ColorBufferPtr colorBuffer, uint32_t uavIndex)
{
	VkDescriptorImageInfo info{
		.imageView		= ((const Descriptor*)colorBuffer->GetUavDescriptor(uavIndex))->GetImageView(),
		.imageLayout	= VK_IMAGE_LAYOUT_GENERAL
	};

	const uint32_t regShift = GetRegisterShiftUAV();

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= regShift + uavRegister,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo			= &info
	};

	UpdateDescriptorSet(writeDescriptorSet);
}


void DescriptorSet::SetUAV(uint32_t uavRegister, DepthBufferPtr depthBuffer)
{
	const DepthBuffer* depthBufferVK = (const DepthBuffer*)depthBuffer.get();
	assert(depthBufferVK != nullptr);

	assert_msg(false, "Depth UAVs not yet supported");
}


void DescriptorSet::SetUAV(uint32_t uavRegister, GpuBufferPtr gpuBuffer)
{
	const Descriptor* descriptor = (const Descriptor*)gpuBuffer->GetUavDescriptor();

	const uint32_t regShift = GetRegisterShiftUAV();

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= regShift + uavRegister,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	};

	VkBufferView texelBufferView = VK_NULL_HANDLE;
	VkDescriptorBufferInfo info{};

	if (gpuBuffer->GetResourceType() == ResourceType::TypedBuffer)
	{
		texelBufferView = descriptor->GetBufferView();
		writeDescriptorSet.pTexelBufferView = &texelBufferView;
	}
	else
	{
		info.buffer = ((const Descriptor*)gpuBuffer->GetSrvDescriptor())->GetBuffer();
		info.offset = 0;
		info.range = VK_WHOLE_SIZE;
		
		writeDescriptorSet.pBufferInfo = &info;
	}

	UpdateDescriptorSet(writeDescriptorSet);
}


void DescriptorSet::SetCBV(uint32_t cbvRegister, GpuBufferPtr gpuBuffer)
{
	VkDescriptorBufferInfo info{
		.buffer		= ((const Descriptor*)gpuBuffer->GetCbvDescriptor())->GetBuffer(),
		.offset		= 0,
		.range		= VK_WHOLE_SIZE
	};

	const uint32_t regShift = GetRegisterShiftCBV();

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= regShift + cbvRegister,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo		= &info
	};

	UpdateDescriptorSet(writeDescriptorSet);
}


void DescriptorSet::SetSampler(uint32_t samplerRegister, SamplerPtr sampler)
{
	VkDescriptorImageInfo info{
		.sampler		= ((const Descriptor*)sampler->GetDescriptor())->GetSampler(),
		.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	const uint32_t regShift = GetRegisterShiftSampler();

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= regShift + samplerRegister,
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_SAMPLER,
		.pImageInfo			= &info
	};

	UpdateDescriptorSet(writeDescriptorSet);
}


bool DescriptorSet::HasDescriptors() const
{
	return (m_numDescriptors != 0);
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
void DescriptorSet::SetSRVUAV(uint32_t srvUavRegister, const IDescriptor* descriptor)
{
	const Descriptor* descriptorVK = (const Descriptor*)descriptor;

	assert(m_rootParameter.parameterType == RootParameterType::Table);

	const uint32_t regShift = isSrv ? GetRegisterShiftSRV() : GetRegisterShiftUAV();

	const DescriptorRegisterType registerType = isSrv ? DescriptorRegisterType::SRV : DescriptorRegisterType::UAV;
	const uint32_t rangeIndex = m_rootParameter.FindMatchingRangeIndex(registerType, srvUavRegister);
	assert(rangeIndex != ~0u);
	const DescriptorType descriptorType = m_rootParameter.table[rangeIndex].descriptorType;

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
			.dstBinding			= regShift + srvUavRegister,
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
			.dstBinding			= regShift + srvUavRegister,
			.dstArrayElement	= 0,
			.descriptorCount	= 1,
		};

		// StructuredBuffers and RawBuffers use VkDescriptorBufferInfo
		if (descriptorType == DescriptorType::StructuredBufferSRV || descriptorType == DescriptorType::StructuredBufferUAV ||
			descriptorType == DescriptorType::RawBufferSRV || descriptorType == DescriptorType::RawBufferUAV)
		{
			info.buffer = descriptorVK->GetBuffer();
			info.offset = 0;
			info.range = VK_WHOLE_SIZE;

			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
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


template <DescriptorRegisterType registerType>
void DescriptorSet::SetDescriptors_Internal(uint32_t descriptorRegister, std::span<const IDescriptor*> descriptors)
{
	const uint32_t rangeIndex = m_rootParameter.FindMatchingRangeIndex(registerType, descriptorRegister);
	
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

	const uint32_t regShift = GetRegisterShift(range.descriptorType);

	VkWriteDescriptorSet writeDescriptorSet{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet				= m_descriptorSet,
		.dstBinding			= regShift + range.startRegister,
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
		bufferInfos[i].range = VK_WHOLE_SIZE;
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