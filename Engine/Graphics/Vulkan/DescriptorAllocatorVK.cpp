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

#include "DescriptorAllocatorVK.h"

#include "DeviceVK.h"
#include "DeviceManagerVK.h"

using namespace std;


namespace Luna::VK
{

#if USE_DESCRIPTOR_BUFFERS
mutex DescriptorBufferAllocator::sm_allocationMutex;

DescriptorBufferAllocator g_userDescriptorBufferAllocator[] = {
	{ DescriptorBufferType::Resource, 64 * (1 << 16) },
	{ DescriptorBufferType::Sampler, 32 * (1 << 10) }
};


DescriptorBufferAllocator::DescriptorBufferAllocator(DescriptorBufferType bufferType, size_t sizeInBytes)
	: m_type{ bufferType }
	, m_bufferSize{ sizeInBytes }
{
}


DescriptorBufferAllocation DescriptorBufferAllocator::Allocate(VkDescriptorSetLayout layout)
{
	lock_guard<mutex> guard{ sm_allocationMutex };

	VkDeviceSize size{ 0 };
	vkGetDescriptorSetLayoutSizeEXT(m_device->GetVulkanDevice(), layout, &size);

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


void DescriptorBufferAllocator::CreateAll()
{
	lock_guard<mutex> guard{ sm_allocationMutex };

	for (uint32_t i = 0; i < _countof(g_userDescriptorBufferAllocator); ++i)
	{
		auto& allocator = g_userDescriptorBufferAllocator[i];
		allocator.Create();
	}
}


void DescriptorBufferAllocator::DestroyAll()
{
	lock_guard<mutex> guard{ sm_allocationMutex };

	for (uint32_t i = 0; i < _countof(g_userDescriptorBufferAllocator); ++i)
	{
		auto& allocator = g_userDescriptorBufferAllocator[i];

		if (allocator.m_descriptorBuffer)
		{
			vmaUnmapMemory(allocator.m_descriptorBuffer->GetAllocator(), allocator.m_descriptorBuffer->GetAllocation());
			allocator.m_descriptorBuffer.reset();
		}
	}
}


VkBuffer DescriptorBufferAllocator::GetBuffer() const noexcept
{
	if (m_descriptorBuffer)
	{
		return m_descriptorBuffer->Get();
	}
	return VK_NULL_HANDLE;
}


void DescriptorBufferAllocator::Create()
{
	m_device = GetVulkanDevice();
	m_alignment = m_device->GetDeviceCaps().descriptorBuffer.descriptorBufferOffsetAlignment;

	m_descriptorBuffer = m_device->CreateDescriptorBuffer(m_type, m_bufferSize);

	ThrowIfFailed(vmaMapMemory(m_descriptorBuffer->GetAllocator(), m_descriptorBuffer->GetAllocation(), (void**)&m_bufferHead));
	m_initialHead = m_bufferHead;
	m_freeSpace = m_bufferSize;
}
#endif // USE_DESCRIPTOR_BUFFERS


#if USE_LEGACY_DESCRIPTOR_SETS
mutex DescriptorSetAllocator::sm_allocationMutex;
vector<VkDescriptorPool> DescriptorSetAllocator::sm_descriptorPoolList;

DescriptorSetAllocator g_descriptorSetAllocator;


VkDescriptorSet DescriptorSetAllocator::Allocate(VkDescriptorSetLayout layout)
{
	if (m_descriptorPool == VK_NULL_HANDLE)
	{
		m_descriptorPool = RequestNewPool();
	}

	VkDescriptorSetAllocateInfo allocInfo{
		.sType					= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool			= m_descriptorPool,
		.descriptorSetCount		= 1,
		.pSetLayouts			= &layout
	};

	auto device = GetVulkanDevice();

	VkDescriptorSet descSet{ VK_NULL_HANDLE };
	auto res = vkAllocateDescriptorSets(device->GetVulkanDevice(), &allocInfo, &descSet);

	if (res != VK_SUCCESS)
	{
		m_descriptorPool = RequestNewPool();
		allocInfo.descriptorPool = m_descriptorPool;
	}

	ThrowIfFailed(vkAllocateDescriptorSets(device->GetVulkanDevice(), &allocInfo, &descSet));

	return descSet;
}


void DescriptorSetAllocator::DestroyAll()
{
	lock_guard<mutex> CS(sm_allocationMutex);

	auto device = GetVulkanDevice();

	for (auto& pool : sm_descriptorPoolList)
	{
		vkResetDescriptorPool(device->GetVulkanDevice(), pool, 0);
		vkDestroyDescriptorPool(device->GetVulkanDevice(), pool, nullptr);
	}
	sm_descriptorPoolList.clear();
}


VkDescriptorPool DescriptorSetAllocator::RequestNewPool()
{
	lock_guard<mutex> CS(sm_allocationMutex);

	static const uint32_t kNumDescriptors = 256u;

	VkDescriptorPoolSize typeCounts[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, kNumDescriptors },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kNumDescriptors },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kNumDescriptors },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kNumDescriptors },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, kNumDescriptors },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, kNumDescriptors },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kNumDescriptors },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, kNumDescriptors },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, kNumDescriptors },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, kNumDescriptors },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, kNumDescriptors }
	};

	VkDescriptorPoolCreateInfo createInfo{
		.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets		= sm_numDescriptorsPerPool,
		.poolSizeCount	= _countof(typeCounts),
		.pPoolSizes		= typeCounts
	};

	auto device = GetVulkanDevice();

	VkDescriptorPool pool{ VK_NULL_HANDLE };
	ThrowIfFailed(vkCreateDescriptorPool(device->GetVulkanDevice(), &createInfo, nullptr, &pool));

	sm_descriptorPoolList.emplace_back(pool);

	return pool;
}
#endif // USE_LEGACY_DESCRIPTOR_SETS

} // namespace Luna::VK