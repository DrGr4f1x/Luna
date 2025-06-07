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

#include "DescriptorSetFactoryVK.h"

#include "DescriptorPoolVK.h"
#include "ResourceManagerVK.h"
#include "VulkanUtil.h"


namespace Luna::VK
{

DescriptorSetFactory::DescriptorSetFactory(IResourceManager* owner, CVkDevice* device)
	: m_owner{ owner }
	, m_device{ device }
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		m_freeList.push(i);
	}

	ClearDescs();
	ClearData();
}


ResourceHandle DescriptorSetFactory::CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc)
{ 
	std::lock_guard guard(m_mutex);

	// Find or create descriptor set pool
	VkDescriptorSetLayout vkDescriptorSetLayout = descriptorSetDesc.descriptorSetLayout->Get();
	DescriptorPool* pool{ nullptr };
	auto it = m_setPoolMapping.find(vkDescriptorSetLayout);
	if (it == m_setPoolMapping.end())
	{
		DescriptorPoolDesc descriptorPoolDesc{
			.device						= m_device.get(),
			.layout						= descriptorSetDesc.descriptorSetLayout,
			.rootParameter				= descriptorSetDesc.rootParameter,
			.poolSize					= MaxSetsPerPool,
			.allowFreeDescriptorSets	= true
		};

		auto poolHandle = make_unique<DescriptorPool>(descriptorPoolDesc);
		pool = poolHandle.get();
		m_setPoolMapping.emplace(vkDescriptorSetLayout, std::move(poolHandle));
	}
	else
	{
		pool = it->second.get();
	}

	// Allocate descriptor set from pool
	VkDescriptorSet vkDescriptorSet = pool->AllocateDescriptorSet();

	DescriptorSetData data{
		.descriptorSet		= vkDescriptorSet,
		.bindingOffsets		= descriptorSetDesc.bindingOffsets,
		.numDescriptors		= descriptorSetDesc.numDescriptors,
		.isDynamicBuffer	= descriptorSetDesc.isDynamicBuffer
	};

	assert(data.numDescriptors <= MaxDescriptorsPerTable);

	// Get a descriptor set index allocation
	assert(!m_freeList.empty());
	uint32_t descriptorSetIndex = m_freeList.front();
	m_freeList.pop();

	m_descs[descriptorSetIndex] = descriptorSetDesc;
	m_data[descriptorSetIndex] = data;

	return make_shared<ResourceHandleType>(descriptorSetIndex, IResourceManager::ManagedDescriptorSet, m_owner);
}


void DescriptorSetFactory::Destroy(uint32_t index)
{
	m_freeList.push(index);
	ResetDesc(index);
	ResetData(index);
}


void DescriptorSetFactory::SetSRV(uint32_t index, int slot, const ColorBuffer& colorBuffer)
{
	auto& data = m_data[index];

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	auto colorBufferHandle = colorBuffer.GetHandle();

	writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.descriptorCount = 1;
	writeSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.shaderResource;
	writeSet.dstArrayElement = 0;

	data.descriptorData[slot] = GetVulkanResourceManager()->GetImageInfoSrv(colorBufferHandle.get());
	writeSet.pImageInfo = std::get_if<VkDescriptorImageInfo>(&data.descriptorData[slot]);

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetFactory::SetSRV(uint32_t index, int slot, const DepthBuffer& depthBuffer, bool depthSrv)
{
	auto& data = m_data[index];

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	auto depthBufferHandle = depthBuffer.GetHandle();

	writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.descriptorCount = 1;
	writeSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.shaderResource;
	writeSet.dstArrayElement = 0;

	data.descriptorData[slot] = GetVulkanResourceManager()->GetImageInfoDepth(depthBufferHandle.get(), depthSrv);
	writeSet.pImageInfo = std::get_if<VkDescriptorImageInfo>(&data.descriptorData[slot]);

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetFactory::SetSRV(uint32_t index, int slot, const GpuBuffer& gpuBuffer)
{
	auto& data = m_data[index];

	if (data.isDynamicBuffer)
	{
		assert(slot == 0);
	}

	auto gpuBufferHandle = gpuBuffer.GetHandle();

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.descriptorCount = 1;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.shaderResource;
	writeSet.dstArrayElement = 0;

	if (gpuBuffer.GetResourceType() == ResourceType::TypedBuffer)
	{
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		data.descriptorData[slot] = GetVulkanResourceManager()->GetBufferView(gpuBufferHandle.get());
		writeSet.pTexelBufferView = std::get_if<VkBufferView>(&data.descriptorData[slot]);
	}
	else
	{
		writeSet.descriptorType = data.isDynamicBuffer ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		data.descriptorData[slot] = GetVulkanResourceManager()->GetBufferInfo(gpuBufferHandle.get());
		writeSet.pBufferInfo = std::get_if<VkDescriptorBufferInfo>(&data.descriptorData[slot]);
	}

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetFactory::SetUAV(uint32_t index, int slot, const ColorBuffer& colorBuffer, uint32_t uavIndex)
{
	auto& data = m_data[index];

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	auto colorBufferHandle = colorBuffer.GetHandle();

	writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.descriptorCount = 1;
	writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.unorderedAccess;
	writeSet.dstArrayElement = 0;

	data.descriptorData[slot] = GetVulkanResourceManager()->GetImageInfoUav(colorBufferHandle.get());
	writeSet.pImageInfo = std::get_if<VkDescriptorImageInfo>(&data.descriptorData[slot]);

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetFactory::SetUAV(uint32_t index, int slot, const DepthBuffer& depthBuffer)
{
	auto& data = m_data[index];

	assert_msg(false, "Depth UAVs not yet supported");
}


void DescriptorSetFactory::SetUAV(uint32_t index, int slot, const GpuBuffer& gpuBuffer)
{
	auto& data = m_data[index];

	if (data.isDynamicBuffer)
	{
		assert(slot == 0);
	}

	auto gpuBufferHandle = gpuBuffer.GetHandle();

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.descriptorCount = 1;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.unorderedAccess;
	writeSet.dstArrayElement = 0;

	if (gpuBuffer.GetResourceType() == ResourceType::TypedBuffer)
	{
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		data.descriptorData[slot] = GetVulkanResourceManager()->GetBufferView(gpuBufferHandle.get());
		writeSet.pTexelBufferView = std::get_if<VkBufferView>(&data.descriptorData[slot]);
	}
	else
	{
		writeSet.descriptorType = data.isDynamicBuffer ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		data.descriptorData[slot] = GetVulkanResourceManager()->GetBufferInfo(gpuBufferHandle.get());
		writeSet.pBufferInfo = std::get_if<VkDescriptorBufferInfo>(&data.descriptorData[slot]);
	}

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetFactory::SetCBV(uint32_t index, int slot, const GpuBuffer& gpuBuffer)
{
	auto& data = m_data[index];

	if (data.isDynamicBuffer)
	{
		assert(slot == 0);
	}

	auto gpuBufferHandle = gpuBuffer.GetHandle();

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.descriptorCount = 1;
	writeSet.descriptorType = data.isDynamicBuffer ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.constantBuffer;
	writeSet.dstArrayElement = 0;
	data.descriptorData[slot] = GetVulkanResourceManager()->GetBufferInfo(gpuBufferHandle.get());
	writeSet.pBufferInfo = std::get_if<VkDescriptorBufferInfo>(&data.descriptorData[slot]);

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetFactory::SetDynamicOffset(uint32_t index, uint32_t offset)
{
	auto& data = m_data[index];

	assert(data.isDynamicBuffer);

	data.dynamicOffset = offset;
}


void DescriptorSetFactory::UpdateGpuDescriptors(uint32_t index)
{
	auto& data = m_data[index];

	if (data.dirtyBits == 0)
		return;

	array<VkWriteDescriptorSet, MaxDescriptorsPerTable> liveDescriptors;

	unsigned long setBit{ 0 };
	uint32_t paramIndex{ 0 };
	while (_BitScanForward(&setBit, data.dirtyBits))
	{
		liveDescriptors[paramIndex++] = data.writeDescriptorSets[setBit];
		data.dirtyBits &= ~(1 << setBit);
	}

	assert(data.dirtyBits == 0);

	vkUpdateDescriptorSets(
		m_device->Get(),
		(uint32_t)paramIndex,
		liveDescriptors.data(),
		0,
		nullptr);
}


bool DescriptorSetFactory::HasDescriptors(uint32_t index) const
{
	return (m_data[index].numDescriptors != 0) || m_data[index].isDynamicBuffer;
}


VkDescriptorSet DescriptorSetFactory::GetDescriptorSet(uint32_t index) const
{
	return m_data[index].descriptorSet;
}


uint32_t DescriptorSetFactory::GetDynamicOffset(uint32_t index) const
{
	return m_data[index].dynamicOffset;
}


bool DescriptorSetFactory::IsDynamicBuffer(uint32_t index) const
{
	return m_data[index].isDynamicBuffer;
}

} // namespace Luna::VK