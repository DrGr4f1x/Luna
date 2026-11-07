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

static bool IsResourceDescriptorClass(DescriptorClass descriptorClass)
{
	using enum DescriptorClass;

	return descriptorClass == Image || descriptorClass == Buffer;
}


DescriptorBuffer::~DescriptorBuffer()
{
	vmaUnmapMemory(m_resourceBuffer.descriptorBuffer->GetAllocator(), m_resourceBuffer.descriptorBuffer->GetAllocation());
	vmaUnmapMemory(m_samplerBuffer.descriptorBuffer->GetAllocator(), m_samplerBuffer.descriptorBuffer->GetAllocation());
}


DescriptorAllocation DescriptorBuffer::AllocateDescriptorMemory(DescriptorHeapType heapType, VkDescriptorSetLayout vkDescriptorSetLayout)
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


DescriptorAllocation DescriptorBuffer::AllocateDescriptors(DescriptorClass descriptorClass, uint32_t numDescriptors)
{
	assert(false);
	return DescriptorAllocation{};
}


VkBuffer DescriptorBuffer::GetBuffer(DescriptorHeapType heapType) const
{
	return (heapType == DescriptorHeapType::Resource) ? 
		m_resourceBuffer.descriptorBuffer->Get() :
		m_samplerBuffer.descriptorBuffer->Get();
}


DescriptorHeap::~DescriptorHeap()
{
	vmaUnmapMemory(m_resourceState.descriptorHeap->GetAllocator(), m_resourceState.descriptorHeap->GetAllocation());
	vmaUnmapMemory(m_samplerState.descriptorHeap->GetAllocator(), m_samplerState.descriptorHeap->GetAllocation());
}


DescriptorAllocation DescriptorHeap::AllocateDescriptorMemory(DescriptorHeapType heapType, VkDescriptorSetLayout vkDescriptorSetLayout)
{
	assert(false);
	return DescriptorAllocation{};
}


DescriptorAllocation DescriptorHeap::AllocateDescriptors(DescriptorClass descriptorClass, uint32_t numDescriptors)
{
	const auto& caps = m_device->GetDeviceCaps();

	std::byte* mem = (descriptorClass == DescriptorClass::Sampler) ?
		m_samplerState.bufferHead :
		m_resourceState.bufferHead;

	uint32_t offset = 0;

	if (IsResourceDescriptorClass(descriptorClass))
	{
		if (descriptorClass == DescriptorClass::Image)
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


VkBuffer DescriptorHeap::GetBuffer(DescriptorHeapType heapType) const
{
	return (heapType == DescriptorHeapType::Resource) ?
		m_resourceState.descriptorHeap->Get() :
		m_samplerState.descriptorHeap->Get();
}


VkBindHeapInfoEXT DescriptorHeap::GetBindInfo(DescriptorHeapType heapType) const
{
	return (heapType == DescriptorHeapType::Resource) ?
		m_resourceState.bindInfo :
		m_samplerState.bindInfo;
}


size_t DescriptorHeap::GetImageDescriptorSize() const
{
	return m_device->GetDeviceCaps().descriptorHeap.imageDescriptorSize;
}


size_t DescriptorHeap::GetBufferDescriptorSize() const
{
	return m_device->GetDeviceCaps().descriptorHeap.bufferDescriptorSize;
}


size_t DescriptorHeap::GetSamplerDescriptorSize() const
{
	return m_device->GetDeviceCaps().descriptorHeap.samplerDescriptorSize;
}

} // namespace Luna::VK