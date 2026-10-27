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


DescriptorBufferAllocation DescriptorHeap::AllocateDescriptorMemory(VkDescriptorSetLayout vkDescriptorSetLayout)
{
	return m_impl->AllocateDescriptorMemory(vkDescriptorSetLayout);
}


VulkanDescriptorHeapClass DescriptorHeap::GetClass() const
{
	return m_impl->GetClass();
}


VkBuffer DescriptorHeap::GetBuffer() const
{
	return m_impl->GetBuffer();
}


DescriptorBufferAllocation DescriptorBufferImpl::AllocateDescriptorMemory(VkDescriptorSetLayout vkDescriptorSetLayout)
{
	assert(m_bufferSize > 0);

	VkDeviceSize size{ 0 };
	vkGetDescriptorSetLayoutSizeEXT(m_device->GetVulkanDevice(), vkDescriptorSetLayout, &size);

	size_t alignedSize = Math::AlignUp(size, m_alignment);
	assert(m_freeSpace >= alignedSize);

	std::byte* allocation = m_bufferHead;
	m_bufferHead += alignedSize;

	m_freeSpace -= alignedSize;

	return DescriptorBufferAllocation{
		.mem		= allocation,
		.offset		= (size_t)(allocation - m_initialHead)
	};
}

} // namespace Luna::VK