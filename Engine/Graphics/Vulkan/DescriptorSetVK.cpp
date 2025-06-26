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

using namespace std;


namespace Luna::VK
{

void DescriptorSet::SetSRV(uint32_t slot, ColorBufferPtr colorBuffer)
{
	// TODO: Try this with GetPlatformObject()

	const ColorBuffer* colorBufferVK = (const ColorBuffer*)colorBuffer.get();
	assert(colorBufferVK != nullptr);

	VkWriteDescriptorSet& writeSet = m_writeDescriptorSets[slot];

	writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.descriptorCount = 1;
	writeSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	writeSet.dstSet = m_descriptorSet;
	writeSet.dstBinding = slot + m_bindingOffsets.shaderResource;
	writeSet.dstArrayElement = 0;

	m_descriptorData[slot] = colorBufferVK->GetImageInfoSrv();
	writeSet.pImageInfo = std::get_if<VkDescriptorImageInfo>(&m_descriptorData[slot]);

	m_dirtyBits |= (1 << slot);
}


void DescriptorSet::SetSRV(uint32_t slot, DepthBufferPtr depthBuffer, bool depthSrv)
{
	// TODO: Try this with GetPlatformObject()

	const DepthBuffer* depthBufferVK = (const DepthBuffer*)depthBuffer.get();
	assert(depthBufferVK != nullptr);

	VkWriteDescriptorSet& writeSet = m_writeDescriptorSets[slot];

	writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.descriptorCount = 1;
	writeSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	writeSet.dstSet = m_descriptorSet;
	writeSet.dstBinding = slot + m_bindingOffsets.shaderResource;
	writeSet.dstArrayElement = 0;

	m_descriptorData[slot] = depthSrv ? depthBufferVK->GetImageInfoDepth() : depthBufferVK->GetImageInfoStencil();
	writeSet.pImageInfo = std::get_if<VkDescriptorImageInfo>(&m_descriptorData[slot]);

	m_dirtyBits |= (1 << slot);
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

	VkWriteDescriptorSet& writeSet = m_writeDescriptorSets[slot];

	writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.descriptorCount = 1;
	writeSet.dstSet = m_descriptorSet;
	writeSet.dstBinding = slot + m_bindingOffsets.shaderResource;
	writeSet.dstArrayElement = 0;

	if (gpuBuffer->GetResourceType() == ResourceType::TypedBuffer)
	{
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		m_descriptorData[slot] = gpuBufferVK->GetBufferView();
		writeSet.pTexelBufferView = std::get_if<VkBufferView>(&m_descriptorData[slot]);
	}
	else
	{
		writeSet.descriptorType = m_isDynamicBuffer ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		m_descriptorData[slot] = gpuBufferVK->GetBufferInfo();
		writeSet.pBufferInfo = std::get_if<VkDescriptorBufferInfo>(&m_descriptorData[slot]);
	}

	m_dirtyBits |= (1 << slot);
}


void DescriptorSet::SetUAV(uint32_t slot, ColorBufferPtr colorBuffer, uint32_t uavIndex)
{
	// TODO: Try this with GetPlatformObject()

	const ColorBuffer* colorBufferVK = (const ColorBuffer*)colorBuffer.get();
	assert(colorBufferVK != nullptr);

	VkWriteDescriptorSet& writeSet = m_writeDescriptorSets[slot];

	writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.descriptorCount = 1;
	writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	writeSet.dstSet = m_descriptorSet;
	writeSet.dstBinding = slot + m_bindingOffsets.unorderedAccess;
	writeSet.dstArrayElement = 0;

	m_descriptorData[slot] = colorBufferVK->GetImageInfoUav();
	writeSet.pImageInfo = std::get_if<VkDescriptorImageInfo>(&m_descriptorData[slot]);

	m_dirtyBits |= (1 << slot);
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

	VkWriteDescriptorSet& writeSet = m_writeDescriptorSets[slot];

	writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.descriptorCount = 1;
	writeSet.dstSet = m_descriptorSet;
	writeSet.dstBinding = slot + m_bindingOffsets.unorderedAccess;
	writeSet.dstArrayElement = 0;

	if (gpuBuffer->GetResourceType() == ResourceType::TypedBuffer)
	{
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		m_descriptorData[slot] = gpuBufferVK->GetBufferView();
		writeSet.pTexelBufferView = std::get_if<VkBufferView>(&m_descriptorData[slot]);
	}
	else
	{
		writeSet.descriptorType = m_isDynamicBuffer ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		m_descriptorData[slot] = gpuBufferVK->GetBufferInfo();
		writeSet.pBufferInfo = std::get_if<VkDescriptorBufferInfo>(&m_descriptorData[slot]);
	}

	m_dirtyBits |= (1 << slot);
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

	VkWriteDescriptorSet& writeSet = m_writeDescriptorSets[slot];

	writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.descriptorCount = 1;
	writeSet.descriptorType = m_isDynamicBuffer ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeSet.dstSet = m_descriptorSet;
	writeSet.dstBinding = slot + m_bindingOffsets.constantBuffer;
	writeSet.dstArrayElement = 0;
	m_descriptorData[slot] = gpuBufferVK->GetBufferInfo();
	writeSet.pBufferInfo = std::get_if<VkDescriptorBufferInfo>(&m_descriptorData[slot]);

	m_dirtyBits |= (1 << slot);
}


void DescriptorSet::SetDynamicOffset(uint32_t offset)
{
	assert(m_isDynamicBuffer);

	m_dynamicOffset = offset;
}


void DescriptorSet::UpdateGpuDescriptors()
{
	if (m_dirtyBits == 0)
	{
		return;
	}

	array<VkWriteDescriptorSet, MaxDescriptorsPerTable> liveDescriptors;

	unsigned long setBit{ 0 };
	uint32_t paramIndex{ 0 };
	while (_BitScanForward(&setBit, m_dirtyBits))
	{
		liveDescriptors[paramIndex++] = m_writeDescriptorSets[setBit];
		m_dirtyBits &= ~(1 << setBit);
	}

	assert(m_dirtyBits == 0);

	vkUpdateDescriptorSets(
		m_device->GetVulkanDevice(),
		(uint32_t)paramIndex,
		liveDescriptors.data(),
		0,
		nullptr);
}


bool DescriptorSet::HasDescriptors() const
{
	return (m_numDescriptors != 0) || m_isDynamicBuffer;
}

} // namespace Luna::VK