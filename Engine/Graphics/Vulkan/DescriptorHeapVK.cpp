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

using namespace std;


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


DescriptorAllocation DescriptorHeap::AllocateDescriptors(DescriptorHeapType heapType, uint32_t numDescriptors, bool imageDescriptors)
{
	return m_impl->AllocateDescriptors(heapType, numDescriptors, imageDescriptors);
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

	VkDeviceSize size{ 0 };
	vkGetDescriptorSetLayoutSizeEXT(m_device->GetVulkanDevice(), vkDescriptorSetLayout, &size);

	size_t alignedSize = Math::AlignUp(size, bufferState.alignment);
	assert(bufferState.freeSpace >= alignedSize);

	std::byte* allocation = bufferState.bufferHead;
	bufferState.bufferHead += alignedSize;

	bufferState.freeSpace -= alignedSize;

	return DescriptorAllocation{
		.mem			= allocation,
		.offsetInBytes	= (size_t)(allocation - bufferState.initialHead),
		.offsetInItems	= 0
	};
}


DescriptorAllocation DescriptorBufferImpl::AllocateDescriptors(DescriptorHeapType heapType, uint32_t numDescriptors, bool imageDescriptors)
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


DescriptorAllocation DescriptorHeapImpl::AllocateDescriptors(DescriptorHeapType heapType, uint32_t numDescriptors, bool imageDescriptors)
{
	const auto& caps = m_device->GetDeviceCaps();

	std::byte* mem = (heapType == DescriptorHeapType::Resource) ?
		m_resourceState.bufferHead :
		m_samplerState.bufferHead;

	uint32_t offset = 0;

	if (heapType == DescriptorHeapType::Resource)
	{
		if (imageDescriptors)
		{
			assert(m_numAllocatedImageDescriptors + numDescriptors <= m_numImageDescriptors);

			mem += m_imageDescriptorOffset;
			mem += m_numAllocatedImageDescriptors * GetImageDescriptorSize();
			offset = m_numAllocatedImageDescriptors;
			m_numAllocatedImageDescriptors += numDescriptors;
		}
		else
		{
			assert(m_numAllocatedBufferDescriptors + numDescriptors <= m_numBufferDescriptors);

			mem += m_bufferDescriptorOffset;
			mem += m_numAllocatedBufferDescriptors * GetBufferDescriptorSize();
			offset = m_numAllocatedBufferDescriptors;
			m_numAllocatedBufferDescriptors += numDescriptors;
		}
	}
	else
	{
		assert(m_numAllocatedSamplerDescriptors + numDescriptors <= m_numSamplerDescriptors);

		mem += m_samplerDescriptorOffset;
		mem += m_numAllocatedSamplerDescriptors * GetSamplerDescriptorSize();
		offset = m_numAllocatedSamplerDescriptors;
		m_numAllocatedSamplerDescriptors += numDescriptors;
	}

	return DescriptorAllocation{
		.mem			= mem,
		.offsetInBytes	= 0,
		.offsetInItems	= offset
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


size_t DescriptorHeapImpl::GetImageDescriptorSize() const
{
	return m_device->GetDeviceCaps().descriptorHeap.imageDescriptorSize;
}


size_t DescriptorHeapImpl::GetBufferDescriptorSize() const
{
	return m_device->GetDeviceCaps().descriptorHeap.bufferDescriptorSize;
}


size_t DescriptorHeapImpl::GetSamplerDescriptorSize() const
{
	return m_device->GetDeviceCaps().descriptorHeap.samplerDescriptorSize;
}

} // namespace Luna::VK