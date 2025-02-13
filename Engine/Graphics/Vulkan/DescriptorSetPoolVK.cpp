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

#include "DescriptorSetPoolVK.h"

#include "Graphics\ColorBuffer.h"
#include "Graphics\DepthBuffer.h"
#include "Graphics\GpuBuffer.h"


namespace Luna::VK
{

DescriptorSetPool* g_descriptorSetPool{ nullptr };


const VkDescriptorImageInfo* GetImageInfo(const IGpuResource* gpuResource)
{
	return gpuResource->GetNativeObject(NativeObjectType::VK_ImageInfo_SRV);
}


const VkDescriptorImageInfo* GetImageInfoUAV(const IGpuResource* gpuResource, uint32_t index)
{
	return gpuResource->GetNativeObject(NativeObjectType::VK_ImageInfo_UAV, index);
}


const VkDescriptorImageInfo* GetImageInfoDepth(const IDepthBuffer* depthBuffer, bool depthSrv)
{
	NativeObjectType type = depthSrv ? NativeObjectType::VK_ImageInfo_SRV_Depth : NativeObjectType::VK_ImageInfo_SRV_Stencil;
	return depthBuffer->GetNativeObject(type);
}


const VkDescriptorBufferInfo* GetBufferInfo(const IGpuResource* gpuResource)
{
	return gpuResource->GetNativeObject(NativeObjectType::VK_BufferInfo);
}


const VkBufferView* GetBufferView(const IGpuResource* gpuResource)
{
	return gpuResource->GetNativeObject(NativeObjectType::VK_BufferView);
}


DescriptorSetPool::DescriptorSetPool(CVkDevice* device)
	: m_device{ device }
{
	assert(g_descriptorSetPool == nullptr);

	// Populate free list and data arrays
	for (uint32_t i = 0; i < MaxItems; ++i)
	{
		m_freeList.push(i);
		m_descriptorData[i] = DescriptorSetData{};
	}

	g_descriptorSetPool = this;
}


DescriptorSetPool::~DescriptorSetPool()
{
	g_descriptorSetPool = nullptr;
}


DescriptorSetHandle DescriptorSetPool::CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_freeList.empty());

	uint32_t index = m_freeList.front();
	m_freeList.pop();

	DescriptorSetData data{
		.descriptorSet = descriptorSetDesc.descriptorSet,
		.bindingOffsets = descriptorSetDesc.bindingOffsets,
		.numDescriptors = descriptorSetDesc.numDescriptors,
		.isDynamicBuffer = descriptorSetDesc.isDynamicBuffer
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


void DescriptorSetPool::DestroyHandle(DescriptorSetHandleType* handle)
{
	assert(handle != nullptr);

	std::lock_guard guard(m_allocationMutex);

	// TODO: queue this up to execute in one big batch, e.g. at the end of the frame

	uint32_t index = handle->GetIndex();
	m_descriptorData[index] = DescriptorSetData{};

	m_freeList.push(index);
}


void DescriptorSetPool::SetSRV(DescriptorSetHandleType* handle, int slot, const IColorBuffer* colorBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	const VkDescriptorImageInfo* imageInfo = GetImageInfo(colorBuffer);

	if (writeSet.pImageInfo == imageInfo)
		return;

	writeSet.descriptorCount = 1;
	writeSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.shaderResource;
	writeSet.dstArrayElement = 0;
	writeSet.pImageInfo = imageInfo;

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetPool::SetSRV(DescriptorSetHandleType* handle, int slot, const IDepthBuffer* depthBuffer, bool depthSrv)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	const VkDescriptorImageInfo* imageInfo = GetImageInfoDepth(depthBuffer, depthSrv);

	if (writeSet.pImageInfo == imageInfo)
		return;

	writeSet.descriptorCount = 1;
	writeSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.shaderResource;
	writeSet.dstArrayElement = 0;
	writeSet.pImageInfo = imageInfo;

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetPool::SetSRV(DescriptorSetHandleType* handle, int slot, const IGpuBuffer* gpuBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	if (data.isDynamicBuffer)
	{
		assert(slot == 0);
	}

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	const VkDescriptorBufferInfo* bufferInfo = GetBufferInfo(gpuBuffer);

	if (writeSet.pBufferInfo == bufferInfo)
		return;

	writeSet.descriptorCount = 1;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.shaderResource;
	writeSet.dstArrayElement = 0;

	if (gpuBuffer->GetResourceType() == ResourceType::TypedBuffer)
	{
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		writeSet.pTexelBufferView = GetBufferView(gpuBuffer);
	}
	else
	{
		writeSet.descriptorType = data.isDynamicBuffer ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writeSet.pBufferInfo = bufferInfo;
	}

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetPool::SetUAV(DescriptorSetHandleType* handle, int slot, const IColorBuffer* colorBuffer, uint32_t uavIndex)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	const VkDescriptorImageInfo* imageInfo = GetImageInfoUAV(colorBuffer, uavIndex);
	if (writeSet.pImageInfo == imageInfo)
		return;

	writeSet.descriptorCount = 1;
	writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.unorderedAccess;
	writeSet.dstArrayElement = 0;
	writeSet.pImageInfo = imageInfo;

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetPool::SetUAV(DescriptorSetHandleType* handle, int slot, const IDepthBuffer* depthBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	assert_msg(false, "Depth UAVs not yet supported");
}


void DescriptorSetPool::SetUAV(DescriptorSetHandleType* handle, int slot, const IGpuBuffer* gpuBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	if (data.isDynamicBuffer)
	{
		assert(slot == 0);
	}

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	writeSet.descriptorCount = 1;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.unorderedAccess;
	writeSet.dstArrayElement = 0;

	if (gpuBuffer->GetResourceType() == ResourceType::TypedBuffer)
	{
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		writeSet.pTexelBufferView = GetBufferView(gpuBuffer);
	}
	else
	{
		writeSet.descriptorType = data.isDynamicBuffer ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writeSet.pBufferInfo = GetBufferInfo(gpuBuffer);
	}

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetPool::SetCBV(DescriptorSetHandleType* handle, int slot, const IGpuBuffer* gpuBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	if (data.isDynamicBuffer)
	{
		assert(slot == 0);
	}

	VkWriteDescriptorSet& writeSet = data.writeDescriptorSets[slot];

	const VkDescriptorBufferInfo* bufferInfo = GetBufferInfo(gpuBuffer);
	if (writeSet.pBufferInfo == bufferInfo)
		return;

	writeSet.descriptorCount = 1;
	writeSet.descriptorType = data.isDynamicBuffer ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeSet.dstSet = data.descriptorSet;
	writeSet.dstBinding = slot + data.bindingOffsets.constantBuffer;
	writeSet.dstArrayElement = 0;
	writeSet.pBufferInfo = bufferInfo;

	data.dirtyBits |= (1 << slot);
}


void DescriptorSetPool::SetDynamicOffset(DescriptorSetHandleType* handle, uint32_t offset)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	assert(data.isDynamicBuffer);

	data.dynamicOffset = offset;
}


void DescriptorSetPool::UpdateGpuDescriptors(DescriptorSetHandleType* handle)
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


bool DescriptorSetPool::HasDescriptors(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return (data.numDescriptors != 0) || data.isDynamicBuffer;
}


VkDescriptorSet DescriptorSetPool::GetDescriptorSet(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return data.descriptorSet;
}


uint32_t DescriptorSetPool::GetDynamicOffset(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return data.dynamicOffset;
}


bool DescriptorSetPool::IsDynamicBuffer(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return data.isDynamicBuffer;
}


DescriptorSetPool* const GetVulkanDescriptorSetPool()
{
	return g_descriptorSetPool;
}

} // namespace Luna::VK