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


DescriptorAllocation DescriptorHeap::AllocateDescriptors(DescriptorHeapType heapType, size_t sizeInBytes)
{
	return m_impl->AllocateDescriptors(heapType, sizeInBytes);
}


VulkanDescriptorHeapClass DescriptorHeap::GetClass() const
{
	return m_impl->GetClass();
}


VkBuffer DescriptorHeap::GetBuffer(DescriptorHeapType heapType) const
{
	return m_impl->GetBuffer(heapType);
}


VkBindHeapInfoEXT DescriptorHeap::GetBindInfo(DescriptorHeapType heapType) const
{
	return m_impl->GetBindInfo(heapType);
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


DescriptorAllocation DescriptorBufferImpl::AllocateDescriptors(DescriptorHeapType heapType, size_t sizeInBytes)
{
	assert(false);
	return DescriptorAllocation{};
}


VkBuffer DescriptorBufferImpl::GetBuffer(DescriptorHeapType heapType) const
{
	return (heapType == DescriptorHeapType::Resource) ? 
		m_resourceBuffer.descriptorBuffer->Get() :
		m_samplerBuffer.descriptorBuffer->Get();
}


DescriptorAllocation DescriptorHeapImpl::AllocateDescriptorMemory(DescriptorHeapType heapType, VkDescriptorSetLayout vkDescriptorSetLayout)
{
	assert(false);
	return DescriptorAllocation{};
}


DescriptorAllocation DescriptorHeapImpl::AllocateDescriptors(DescriptorHeapType heapType, size_t sizeInBytes)
{
	const auto& caps = m_device->GetDeviceCaps();

	HeapState& heapState = (heapType == DescriptorHeapType::Resource) ?
		m_resourceState :
		m_samplerState;

	size_t alignedSize = Math::AlignUp(sizeInBytes, caps.descriptorHeap.largestResourceDescriptorAlignment);
	assert(heapState.freeSpace >= alignedSize);

	std::byte* allocation = heapState.bufferHead;
	heapState.bufferHead += alignedSize;

	heapState.freeSpace -= alignedSize;

	return DescriptorAllocation{
		.mem = allocation,
		.offset = (size_t)(allocation - heapState.initialHead)
	};
}


VkBuffer DescriptorHeapImpl::GetBuffer(DescriptorHeapType heapType) const
{
	return (heapType == DescriptorHeapType::Resource) ?
		m_resourceState.descriptorHeap->Get() :
		m_samplerState.descriptorHeap->Get();
}


VkBindHeapInfoEXT DescriptorHeapImpl::GetBindInfo(DescriptorHeapType heapType) const
{
	return (heapType == DescriptorHeapType::Resource) ?
		m_resourceState.bindInfo :
		m_samplerState.bindInfo;
}

} // namespace Luna::VK