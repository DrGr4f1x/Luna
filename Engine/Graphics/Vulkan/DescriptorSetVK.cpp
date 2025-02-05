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

#include "Graphics\ColorBuffer.h"
#include "Graphics\DepthBuffer.h"
#include "Graphics\GpuBuffer.h"
#include "Graphics\GraphicsCommon.h"
#include "Graphics\Vulkan\DescriptorAllocatorVK.h"
#include "Graphics\Vulkan\DeviceVK.h"
#include "Graphics\Vulkan\VulkanCommon.h"

using namespace std;


namespace Luna::VK
{

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


DescriptorSet::DescriptorSet(const DescriptorSetDescExt& descriptorSetDescExt)
	: m_descriptorSet{descriptorSetDescExt.descriptorSet}
	, m_numDescriptors{descriptorSetDescExt.numDescriptors}
	, m_isDynamicCBV{ descriptorSetDescExt.isDynamicCbv }
	, m_isRootCBV{ descriptorSetDescExt.isRootCbv }
{
	assert(m_numDescriptors <= MaxDescriptorsPerTable);

	for (uint32_t j = 0; j < MaxDescriptorsPerTable; ++j)
	{
		VkWriteDescriptorSet& writeSet = m_writeDescriptorSets[j];
		writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSet.pNext = nullptr;
		writeSet.pImageInfo = nullptr;
		writeSet.pBufferInfo = nullptr;
		writeSet.pTexelBufferView = nullptr;
	}
}


void DescriptorSet::SetSRV(int slot, const IColorBuffer* colorBuffer)
{
	VkWriteDescriptorSet& writeSet = m_writeDescriptorSets[slot];

	const VkDescriptorImageInfo* imageInfo = GetImageInfo(colorBuffer);

	if (writeSet.pImageInfo == imageInfo)
		return;

	writeSet.descriptorCount = 1;
	writeSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	writeSet.dstSet = m_descriptorSet;
	writeSet.dstBinding = slot;
	writeSet.pImageInfo = imageInfo;

	m_dirtyBits |= (1 << slot);
}


void DescriptorSet::SetSRV(int slot, const IDepthBuffer* depthBuffer, bool depthSrv)
{
	VkWriteDescriptorSet& writeSet = m_writeDescriptorSets[slot];

	const VkDescriptorImageInfo* imageInfo = GetImageInfoDepth(depthBuffer, depthSrv);

	if (writeSet.pImageInfo == imageInfo)
		return;

	writeSet.descriptorCount = 1;
	writeSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	writeSet.dstSet = m_descriptorSet;
	writeSet.dstBinding = slot;
	writeSet.pImageInfo = imageInfo;

	m_dirtyBits |= (1 << slot);
}


void DescriptorSet::SetSRV(int slot, const IGpuBuffer* gpuBuffer)
{
	VkWriteDescriptorSet& writeSet = m_writeDescriptorSets[slot];

	const VkDescriptorBufferInfo* bufferInfo = GetBufferInfo(gpuBuffer);

	if (writeSet.pBufferInfo == bufferInfo)
		return;

	writeSet.descriptorCount = 1;
	writeSet.dstSet = m_descriptorSet;
	writeSet.dstBinding = slot;

	if (gpuBuffer->GetResourceType() == ResourceType::TypedBuffer)
	{
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		writeSet.pTexelBufferView = GetBufferView(gpuBuffer);
	}
	else
	{
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writeSet.pBufferInfo = bufferInfo;
	}

	m_dirtyBits |= (1 << slot);
}


void DescriptorSet::SetUAV(int slot, const IColorBuffer* colorBuffer, uint32_t uavIndex)
{
	VkWriteDescriptorSet& writeSet = m_writeDescriptorSets[slot];

	const VkDescriptorImageInfo* imageInfo = GetImageInfoUAV(colorBuffer, uavIndex);
	if (writeSet.pImageInfo == imageInfo)
		return;

	writeSet.descriptorCount = 1;
	writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	writeSet.dstSet = m_descriptorSet;
	writeSet.dstBinding = slot;
	writeSet.pImageInfo = imageInfo;

	m_dirtyBits |= (1 << slot);
}


void DescriptorSet::SetUAV(int slot, const IDepthBuffer* depthBuffer)
{
	assert_msg(false, "Depth UAVs not yet supported");
}


void DescriptorSet::SetUAV(int slot, const IGpuBuffer* gpuBuffer)
{
	VkWriteDescriptorSet& writeSet = m_writeDescriptorSets[slot];

	writeSet.descriptorCount = 1;
	writeSet.dstSet = m_descriptorSet;
	writeSet.dstBinding = slot;

	if (gpuBuffer->GetResourceType() == ResourceType::TypedBuffer)
	{
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		writeSet.pTexelBufferView = GetBufferView(gpuBuffer);
	}
	else
	{
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writeSet.pBufferInfo = GetBufferInfo(gpuBuffer);
	}

	m_dirtyBits |= (1 << slot);
}


void DescriptorSet::SetCBV(int paramIndex, const IGpuBuffer* gpuBuffer)
{
	VkWriteDescriptorSet& writeSet = m_writeDescriptorSets[paramIndex];

	const VkDescriptorBufferInfo* bufferInfo = GetBufferInfo(gpuBuffer);
	if (writeSet.pBufferInfo == bufferInfo)
		return;

	writeSet.descriptorCount = 1;
	// TODO: refactor dynamic CBVs
	writeSet.descriptorType = m_isDynamicCBV ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeSet.dstSet = m_descriptorSet;
	writeSet.dstBinding = paramIndex;
	writeSet.pBufferInfo = bufferInfo;

	m_dirtyBits |= (1 << paramIndex);
}


void DescriptorSet::SetDynamicOffset(uint32_t offset)
{
	assert(m_isDynamicCBV);

	m_dynamicOffset = offset;
}


void DescriptorSet::Update()
{
	if (!IsDirty() || m_numDescriptors == 0)
		return;

	array<VkWriteDescriptorSet, MaxDescriptorsPerTable> liveDescriptors;

	unsigned long setBit{ 0 };
	uint32_t index{ 0 };
	while (_BitScanForward(&setBit, m_dirtyBits))
	{
		liveDescriptors[index++] = m_writeDescriptorSets[setBit];
		m_dirtyBits &= ~(1 << setBit);
	}

	assert(m_dirtyBits == 0);

	auto device = GetVulkanGraphicsDevice()->GetDevice();

	vkUpdateDescriptorSets(
		device,
		(uint32_t)index,
		liveDescriptors.data(),
		0,
		nullptr);
}

} // namespace Luna::VK