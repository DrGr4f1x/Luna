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

#include "Graphics\RootSignature.h"

using namespace std;


namespace Luna::VK
{

DescriptorSetPool::DescriptorSetPool(CVkDevice* device, CVkDescriptorSetLayout* layout, const RootParameter& rootParam, uint32_t poolSize)
	: m_device{ device }
	, m_layout{ layout }
	, m_poolMaxSets { poolSize }
{
	ParseRootParameter(rootParam);
}


VkDescriptorSet DescriptorSetPool::AllocateDescriptorSet()
{
	m_poolIndex = FindAvailablePool(m_poolIndex);

	// Increment allocated set count for the current pool
	++m_allocatedSetsPerPool[m_poolIndex];

	VkDescriptorSetLayout vkDescriptorSetLayout = m_layout->Get();

	VkDescriptorSetAllocateInfo allocInfo{
		.sType					= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool			= m_pools[m_poolIndex]->Get(),
		.descriptorSetCount		= 1,
		.pSetLayouts			= &vkDescriptorSetLayout
	};

	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkAllocateDescriptorSets(m_device->Get(), &allocInfo, &descriptorSet)))
	{
		// Store the index of the pool the new set was allocated from
		m_setPoolMapping.emplace(descriptorSet, m_poolIndex);
	}
	else
	{
		// Failed to allocate, so decrement the allocated set count
		--m_allocatedSetsPerPool[m_poolIndex];
	}

	return descriptorSet;
}


void DescriptorSetPool::FreeDescriptorSet(VkDescriptorSet descriptorSet)
{
	auto it = m_setPoolMapping.find(descriptorSet);
	assert(it != m_setPoolMapping.end());

	auto poolIndex = it->second;

	// Free descriptor set
	vkFreeDescriptorSets(m_device->Get(), m_pools[poolIndex]->Get(), 1, &descriptorSet);

	// Remove descriptor set mapping
	m_setPoolMapping.erase(it);

	// Decrement allocated set count for the pool
	--m_allocatedSetsPerPool[poolIndex];

	// Change the current pool index to use the available pool
	m_poolIndex = poolIndex;
}


uint32_t DescriptorSetPool::GetNumLiveDescriptorSets() const
{
	uint32_t sum = 0;

	for (uint32_t num : m_allocatedSetsPerPool)
	{
		sum += num;
	}

	return sum;
}


void DescriptorSetPool::Reset()
{
	// Reset all descriptor pools
	for (auto pool : m_pools)
	{
		vkResetDescriptorPool(m_device->Get(), pool->Get(), 0);
	}

	// Clear internal tracking of descriptor set allocations
	std::fill(m_allocatedSetsPerPool.begin(), m_allocatedSetsPerPool.end(), 0);
	m_setPoolMapping.clear();

	// Reset the pool index from which descriptor sets are allocated
	m_poolIndex = 0;
}


void DescriptorSetPool::ParseRootParameter(const RootParameter& rootParam)
{
	map<VkDescriptorType, uint32_t> descriptorTypeCounts;

	// Count each type of descriptor needed by a descriptor set
	if (rootParam.parameterType == RootParameterType::RootCBV)
	{
		descriptorTypeCounts[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC] = 1;
	}
	else if (rootParam.parameterType == RootParameterType::RootSRV || rootParam.parameterType == RootParameterType::RootUAV)
	{
		descriptorTypeCounts[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC] = 1;
	}
	else if (rootParam.parameterType == RootParameterType::Table)
	{
		for (const auto& range : rootParam.table)
		{
			descriptorTypeCounts[DescriptorTypeToVulkan(range.descriptorType)] += range.numDescriptors;
		}
	}

	// Allocate pool sizes array
	m_poolSizes.resize(descriptorTypeCounts.size());

	// FIll pool size for each descriptor type multiplied by the pool size
	auto poolSizeIt = m_poolSizes.begin();

	for(auto& [type, count] : descriptorTypeCounts)
	{ 
		poolSizeIt->type = type;
		poolSizeIt->descriptorCount = count * m_poolMaxSets;
		++poolSizeIt;
	}
}


uint32_t DescriptorSetPool::FindAvailablePool(uint32_t searchIndex)
{
	// Create a new pool
	if (searchIndex >= (uint32_t)m_pools.size())
	{
		VkDescriptorPoolCreateInfo createInfo
		{
			.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags			= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
			.maxSets		= m_poolMaxSets,
			.poolSizeCount	= (uint32_t)m_poolSizes.size(),
			.pPoolSizes		= m_poolSizes.data()
			// TODO: Add support for VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT
		};

		VkDescriptorPool vkDescriptorPool{ VK_NULL_HANDLE };
		vkCreateDescriptorPool(m_device->Get(), &createInfo, nullptr, &vkDescriptorPool);

		auto pool = Create<CVkDescriptorPool>(m_device.get(), vkDescriptorPool);

		// Store the Vulkan handle
		m_pools.push_back(pool);

		// Initialize the allocated set count for the new pool to 0
		m_allocatedSetsPerPool.push_back(0);

		return searchIndex;
	}
	else if (m_allocatedSetsPerPool[searchIndex] < m_poolMaxSets)
	{
		return searchIndex;
	}

	// Increment pool index
	return FindAvailablePool(++searchIndex);
}

} // namespace Luna::VK