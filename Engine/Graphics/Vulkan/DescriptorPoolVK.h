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

#include "Graphics\RootSignature.h"
#include "Graphics\Vulkan\RefCountingImplVK.h"


namespace Luna
{

struct RootParameter;

} // namespace Luna


namespace Luna::VK
{

constexpr uint32_t MaxSetsPerPool = 16;


struct DescriptorPoolDesc
{
	CVkDevice* device{ nullptr };
	CVkDescriptorSetLayout* layout{ nullptr };
	RootParameter rootParameter{};
	uint32_t poolSize{ MaxSetsPerPool };
	bool allowFreeDescriptorSets{ false };
};


class DescriptorPool
{
public:
	DescriptorPool(const DescriptorPoolDesc& descriptorPoolDesc);
	~DescriptorPool() = default;

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

	// Whether to allow descriptor sets to be freed
	const bool m_allowFreeDescriptorSets{ false };

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


class DescriptorPoolCache
{
public:
	DescriptorPoolCache(const DescriptorPoolDesc& descriptorPoolDesc);
	~DescriptorPoolCache();

	VkDescriptorSet AllocateDescriptorSet();

	void RetirePool(uint64_t fenceValue);

	VkDescriptorSetLayout GetLayout() const;

private:
	std::shared_ptr<DescriptorPool> RequestDescriptorPool();

private:
	// Vulkan handles
	wil::com_ptr<CVkDevice> m_device;
	wil::com_ptr<CVkDescriptorSetLayout> m_layout;

	// Root parameter
	RootParameter m_rootParameter;

	// Number of sets to allocate for each pool
	const uint32_t m_poolMaxSets{ 0 };

	// Active descriptor set pool
	std::shared_ptr<DescriptorPool> m_activePool;

	// Static members
	static std::mutex sm_mutex;
	static std::queue<std::shared_ptr<DescriptorPool>> sm_availablePools;
	static std::queue<std::pair<uint64_t, std::shared_ptr<DescriptorPool>>> sm_retiredPools;
};

} // namespace Luna::VK