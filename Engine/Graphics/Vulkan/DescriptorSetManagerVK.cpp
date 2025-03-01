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

#include "DescriptorSetManagerVK.h"

#include "ColorBufferManagerVK.h"
#include "DepthBufferManagerVK.h"
#include "DescriptorSetPoolVK.h"
#include "GpuBufferManagerVK.h"


namespace Luna::VK
{

DescriptorSetManager* g_descriptorSetManager{ nullptr };


DescriptorSetManager::DescriptorSetManager(CVkDevice* device)
	: m_device{ device }
{
	assert(g_descriptorSetManager == nullptr);

	// Populate free list and data arrays
	for (uint32_t i = 0; i < MaxItems; ++i)
	{
		m_freeList.push(i);
		m_descriptorData[i] = DescriptorSetData{};
	}

	g_descriptorSetManager = this;
}


DescriptorSetManager::~DescriptorSetManager()
{
	g_descriptorSetManager = nullptr;
}


DescriptorSetHandle DescriptorSetManager::CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_freeList.empty());

	uint32_t index = m_freeList.front();
	m_freeList.pop();

	// Find or create descriptor set pool
	VkDescriptorSetLayout vkDescriptorSetLayout = descriptorSetDesc.descriptorSetLayout->Get();
	DescriptorSetPool* pool{ nullptr };
	auto it = m_setPoolMapping.find(vkDescriptorSetLayout);
	if (it == m_setPoolMapping.end())
	{
		auto poolHandle = make_unique<DescriptorSetPool>(m_device.get(), descriptorSetDesc.descriptorSetLayout, descriptorSetDesc.rootParameter);
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

	for (uint32_t j = 0; j < MaxDescriptorsPerTable; ++j)
	{
		VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[j];
		writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSet.pNext = nullptr;
		writeSet.dstBinding = 0;
		writeSet.dstArrayElement = 0;
		writeSet.descriptorCount = 0;
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
		writeSet.pImageInfo = nullptr;
		writeSet.pBufferInfo = nullptr;
		writeSet.pTexelBufferView = nullptr;
	}

	m_descriptorData[index] = data;

	return Create<DescriptorSetHandleType>(index, this);
}


void DescriptorSetManager::DestroyHandle(DescriptorSetHandleType* handle)
{
	assert(handle != nullptr);

	std::lock_guard guard(m_allocationMutex);

	// TODO: queue this up to execute in one big batch, e.g. at the end of the frame

	uint32_t index = handle->GetIndex();
	m_descriptorData[index] = DescriptorSetData{};

	m_freeList.push(index);
}


void DescriptorSetManager::SetSRV(DescriptorSetHandleType* handle, int slot, const ColorBuffer& colorBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	auto colorBufferManager = GetVulkanColorBufferManager();
	auto colorBufferHandle = colorBuffer.GetHandle();

	writeSet.descriptorCount = 1;
	writeSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.shaderResource;
	writeSet.dstArrayElement = 0;
	
	data.descriptorData[slot] = colorBufferManager->GetImageInfoSrv(colorBufferHandle.get());
	writeSet.pImageInfo = std::get_if<VkDescriptorImageInfo>(&data.descriptorData[slot]);

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetManager::SetSRV(DescriptorSetHandleType* handle, int slot, const DepthBuffer& depthBuffer, bool depthSrv)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	auto depthBufferManager = GetVulkanDepthBufferManager();
	auto depthBufferHandle = depthBuffer.GetHandle();

	writeSet.descriptorCount = 1;
	writeSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.shaderResource;
	writeSet.dstArrayElement = 0;

	data.descriptorData[slot] = depthBufferManager->GetImageInfo(depthBufferHandle.get(), depthSrv);
	writeSet.pImageInfo = std::get_if<VkDescriptorImageInfo>(&data.descriptorData[slot]);

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetManager::SetSRV(DescriptorSetHandleType* handle, int slot, const GpuBuffer& gpuBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	if (data.isDynamicBuffer)
	{
		assert(slot == 0);
	}

	auto gpuBufferManager = GetVulkanGpuBufferManager();
	GpuBufferHandle gpuBufferHandle = gpuBuffer.GetHandle();

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	writeSet.descriptorCount = 1;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.shaderResource;
	writeSet.dstArrayElement = 0;

	if (gpuBuffer.GetResourceType() == ResourceType::TypedBuffer)
	{
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		data.descriptorData[slot] = gpuBufferManager->GetBufferView(gpuBufferHandle.get());
		writeSet.pTexelBufferView = std::get_if<VkBufferView>(&data.descriptorData[slot]);
	}
	else
	{
		writeSet.descriptorType = data.isDynamicBuffer ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		data.descriptorData[slot] = gpuBufferManager->GetBufferInfo(gpuBufferHandle.get());
		writeSet.pBufferInfo = std::get_if<VkDescriptorBufferInfo>(&data.descriptorData[slot]);
	}

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetManager::SetUAV(DescriptorSetHandleType* handle, int slot, const ColorBuffer& colorBuffer, uint32_t uavIndex)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	auto colorBufferManager = GetVulkanColorBufferManager();
	auto colorBufferHandle = colorBuffer.GetHandle();

	writeSet.descriptorCount = 1;
	writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.unorderedAccess;
	writeSet.dstArrayElement = 0;
	
	data.descriptorData[slot] = colorBufferManager->GetImageInfoUav(colorBufferHandle.get());
	writeSet.pImageInfo = std::get_if<VkDescriptorImageInfo>(&data.descriptorData[slot]);

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetManager::SetUAV(DescriptorSetHandleType* handle, int slot, const DepthBuffer& depthBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	assert_msg(false, "Depth UAVs not yet supported");
}


void DescriptorSetManager::SetUAV(DescriptorSetHandleType* handle, int slot, const GpuBuffer& gpuBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	if (data.isDynamicBuffer)
	{
		assert(slot == 0);
	}

	auto gpuBufferManager = GetVulkanGpuBufferManager();
	GpuBufferHandle gpuBufferHandle = gpuBuffer.GetHandle();

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	writeSet.descriptorCount = 1;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.unorderedAccess;
	writeSet.dstArrayElement = 0;

	if (gpuBuffer.GetResourceType() == ResourceType::TypedBuffer)
	{
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		data.descriptorData[slot] = gpuBufferManager->GetBufferView(gpuBufferHandle.get());
		writeSet.pTexelBufferView = std::get_if<VkBufferView>(&data.descriptorData[slot]);
	}
	else
	{
		writeSet.descriptorType = data.isDynamicBuffer ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		data.descriptorData[slot] = gpuBufferManager->GetBufferInfo(gpuBufferHandle.get());
		writeSet.pBufferInfo = std::get_if<VkDescriptorBufferInfo>(&data.descriptorData[slot]);
	}

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetManager::SetCBV(DescriptorSetHandleType* handle, int slot, const GpuBuffer& gpuBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	if (data.isDynamicBuffer)
	{
		assert(slot == 0);
	}

	auto gpuBufferManager = GetVulkanGpuBufferManager();
	GpuBufferHandle gpuBufferHandle = gpuBuffer.GetHandle();

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	writeSet.descriptorCount = 1;
	writeSet.descriptorType = data.isDynamicBuffer ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.constantBuffer;
	writeSet.dstArrayElement = 0;
	data.descriptorData[slot] = gpuBufferManager->GetBufferInfo(gpuBufferHandle.get());
	writeSet.pBufferInfo = std::get_if<VkDescriptorBufferInfo>(&data.descriptorData[slot]);

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetManager::SetDynamicOffset(DescriptorSetHandleType* handle, uint32_t offset)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	assert(data.isDynamicBuffer);

	data.dynamicOffset = offset;
}


void DescriptorSetManager::UpdateGpuDescriptors(DescriptorSetHandleType* handle)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

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


bool DescriptorSetManager::HasDescriptors(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return (data.numDescriptors != 0) || data.isDynamicBuffer;
}


VkDescriptorSet DescriptorSetManager::GetDescriptorSet(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return data.descriptorSet;
}


uint32_t DescriptorSetManager::GetDynamicOffset(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return data.dynamicOffset;
}


bool DescriptorSetManager::IsDynamicBuffer(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return data.isDynamicBuffer;
}


DescriptorSetManager* const GetVulkanDescriptorSetManager()
{
	return g_descriptorSetManager;
}

} // namespace Luna::VK