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

#include "Graphics\Vulkan\DeviceVK.h"

using namespace std;


namespace Luna::VK
{

mutex DescriptorSetAllocator::sm_allocationMutex;
vector<VkDescriptorPool> DescriptorSetAllocator::sm_descriptorPoolList;


DescriptorSetAllocator g_descriptorSetAllocator;


VkDescriptorSet DescriptorSetAllocator::Allocate(VkDescriptorSetLayout layout)
{
	if (m_descriptorPool == VK_NULL_HANDLE)
	{
		m_descriptorPool = RequestNewPool();
	}

	VkDevice device = GetVulkanGraphicsDevice()->GetDevice();

	VkDescriptorSetAllocateInfo allocInfo{
		.sType					= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool			= m_descriptorPool,
		.descriptorSetCount		= 1,
		.pSetLayouts			= &layout
	};

	VkDescriptorSet descSet{ VK_NULL_HANDLE };
	auto res = vkAllocateDescriptorSets(device, &allocInfo, &descSet);

	if (res != VK_SUCCESS)
	{
		m_descriptorPool = RequestNewPool();
		allocInfo.descriptorPool = m_descriptorPool;
	}

	ThrowIfFailed(vkAllocateDescriptorSets(device, &allocInfo, &descSet));

	return descSet;
}


void DescriptorSetAllocator::DestroyAll()
{
	lock_guard<mutex> CS(sm_allocationMutex);

	VkDevice device = GetVulkanGraphicsDevice()->GetDevice();

	for (auto& pool : sm_descriptorPoolList)
	{
		vkResetDescriptorPool(device, pool, 0);
		vkDestroyDescriptorPool(device, pool, nullptr);
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

	auto device = GetVulkanGraphicsDevice()->GetDevice();

	VkDescriptorPool pool{ VK_NULL_HANDLE };
	ThrowIfFailed(vkCreateDescriptorPool(device, &createInfo, nullptr, &pool));

	sm_descriptorPoolList.emplace_back(pool);

	return pool;
}

} // namespace Luna::VK