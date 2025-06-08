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
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna
{

// Forward declarations
class IResourceManager;

} // namespace Luna


namespace Luna::VK
{

struct RootSignatureRecord
{
	std::weak_ptr<ResourceHandleType> weakHandle;
	std::atomic<bool> isReady{ false };
};


struct DescriptorBindingDesc
{
	VkDescriptorType descriptorType;
	uint32_t startSlot{ 0 };
	uint32_t numDescriptors{ 1 };
	uint32_t offset{ 0 };
};


struct RootSignatureData
{
	wil::com_ptr<CVkPipelineLayout> pipelineLayout;
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
	std::unordered_map<uint32_t, std::vector<DescriptorBindingDesc>> layoutBindingMap;
	std::unordered_map<uint32_t, uint32_t> rootParameterIndexToDescriptorSetMap;
	std::vector<wil::com_ptr<CVkDescriptorSetLayout>> descriptorSetLayouts;
};


class RootSignatureFactory : public RootSignatureFactoryBase
{
public:
	RootSignatureFactory(IResourceManager* owner, CVkDevice* device);

	ResourceHandle CreateRootSignature(const RootSignatureDesc& rootSignatureDesc);
	void Destroy(uint32_t index);

	const RootSignatureDesc& GetDesc(uint32_t index) const;
	VkPipelineLayout GetPipelineLayout(uint32_t index) const;
	CVkDescriptorSetLayout* GetDescriptorSetLayout(uint32_t index, uint32_t rootParameterIndex) const;
	const std::vector<DescriptorBindingDesc>& GetLayoutBindings(uint32_t index, uint32_t paramIndex) const;

private:
	void ResetData(uint32_t index)
	{
		m_rootSignatureData[index] = RootSignatureData{};
	}

	void ResetHash(uint32_t index)
	{
		m_hashList[index] = 0;
	}

private:
	IResourceManager* m_owner{ nullptr };
	wil::com_ptr<CVkDevice> m_device;

	std::mutex m_mutex;

	std::queue<uint32_t> m_freeList;

	std::map<size_t, std::unique_ptr<RootSignatureRecord>> m_hashToRecordMap;

	// Root signatures
	std::array<RootSignatureData, MaxResources> m_rootSignatureData;

	// Hash keys
	std::array<size_t, MaxResources> m_hashList;
};

} // namespace Luna::VK