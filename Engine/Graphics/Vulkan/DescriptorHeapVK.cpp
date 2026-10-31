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

#include "DescriptorHeapVK.h"

#include "DeviceVK.h"


namespace Luna::VK
{

DescriptorHeap::~DescriptorHeap()
{
	delete m_impl;
}


DescriptorAllocation DescriptorHeap::AllocateDescriptorMemory(DescriptorHeapType heapType, VkDescriptorSetLayout vkDescriptorSetLayout)
{
	return m_impl->AllocateDescriptorMemory(heapType, vkDescriptorSetLayout);
}


VulkanDescriptorHeapClass DescriptorHeap::GetClass() const
{
	return m_impl->GetClass();
}


VkBuffer DescriptorHeap::GetBuffer(DescriptorHeapType heapType) const
{
	return m_impl->GetBuffer(heapType);
}


DescriptorAllocation DescriptorBufferImpl::AllocateDescriptorMemory(DescriptorHeapType heapType, VkDescriptorSetLayout vkDescriptorSetLayout)
{
	BufferState& bufferState = (heapType == DescriptorHeapType::Resource) ?
		m_resourceBuffer :
		m_samplerBuffer;

	assert(bufferState.bufferSize > 0);

	VkDeviceSize size{ 0 };
	vkGetDescriptorSetLayoutSizeEXT(m_device->GetVulkanDevice(), vkDescriptorSetLayout, &size);

	size_t alignedSize = Math::AlignUp(size, bufferState.alignment);
	assert(bufferState.freeSpace >= alignedSize);

	std::byte* allocation = bufferState.bufferHead;
	bufferState.bufferHead += alignedSize;

	bufferState.freeSpace -= alignedSize;

	return DescriptorAllocation{
		.mem		= allocation,
		.offset		= (size_t)(allocation - bufferState.initialHead)
	};
}


VkBuffer DescriptorBufferImpl::GetBuffer(DescriptorHeapType heapType) const
{
	return (heapType == DescriptorHeapType::Resource) ? 
		m_resourceBuffer.descriptorBuffer->Get() :
		m_samplerBuffer.descriptorBuffer->Get();
}


DescriptorAllocation DescriptorHeapImpl::AllocateDescriptorMemory(DescriptorHeapType heapType, VkDescriptorSetLayout vkDescriptorSetLayout)
{
	return DescriptorAllocation{};
}


VkBuffer DescriptorHeapImpl::GetBuffer(DescriptorHeapType heapType) const
{
	return m_descriptorHeap->Get();
}


VkBindHeapInfoEXT DescriptorHeapImpl::GetBindInfo(DescriptorHeapType heapType) const
{
	return m_bindInfo;
}

} // namespace Luna::VK