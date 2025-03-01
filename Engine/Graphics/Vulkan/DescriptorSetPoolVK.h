//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#pragma once

#include "Graphics\Vulkan\VulkanCommon.h"

namespace Luna
{

struct RootParameter;

} // namespace Luna


namespace Luna::VK
{

class DescriptorSetPool
{
public:
	static const uint32_t MaxSetsPerPool = 16;

	DescriptorSetPool(CVkDevice* device, CVkDescriptorSetLayout* layout, const RootParameter& rootParam, uint32_t poolSize = MaxSetsPerPool);
	~DescriptorSetPool() = default;

	VkDescriptorSet AllocateDescriptorSet();
	void FreeDescriptorSet(VkDescriptorSet descriptorSet);

	// Get the number of live descriptor sets allocated, but not yet freed
	uint32_t GetNumLiveDescriptorSets() const;

	void Reset();

private:
	// Parse root parameter to determine Vulkan descriptor counts by type for this set of pools
	void ParseRootParameter(const RootParameter& rootParam);

	// Find next pool index or create new pool
	uint32_t FindAvailablePool(uint32_t searchIndex);

private:
	// Vulkan handles
	wil::com_ptr<CVkDevice> m_device;
	wil::com_ptr<CVkDescriptorSetLayout> m_layout;

	// Number of sets to allocate for each pool
	const uint32_t m_poolMaxSets{ 0 };

	// Descriptor pool size
	std::vector<VkDescriptorPoolSize> m_poolSizes;

	// Count of allocated sets per pool
	std::vector<uint32_t> m_allocatedSetsPerPool;

	// Descriptor pools
	std::vector<wil::com_ptr<CVkDescriptorPool>> m_pools;

	// Current pool index
	uint32_t m_poolIndex{ 0 };

	// Map between allocated descriptor sets and pool index
	std::unordered_map<VkDescriptorSet, uint32_t> m_setPoolMapping;
};

} // namespace Luna::VK