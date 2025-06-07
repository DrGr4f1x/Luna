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

#include "Graphics\Vulkan\RefCountingImplVK.h"


namespace Luna
{

class DescriptorSetPool;
class RootSignature;
struct RootParameter;

} // namespace Luna


namespace Luna::VK
{

// Forward declarations
class DescriptorPoolCache;


class IDynamicDescriptorHeap
{
public:
	virtual void SetDescriptorImageInfo(uint32_t rootParameter, uint32_t offset, VkDescriptorImageInfo descriptorImageInfo, bool graphicsPipe = true) = 0;
	virtual void SetDescriptorBufferInfo(uint32_t rootParameter, uint32_t offset, VkDescriptorBufferInfo descriptorBufferInfo, bool graphicsPipe = true) = 0;
	virtual void SetDescriptorBufferView(uint32_t rootParameter, uint32_t offset, VkBufferView descriptorBufferView, bool graphicsPipe = true) = 0;

	virtual void CleanupUsedPools(uint64_t fenceValue) = 0;

	virtual void ParseRootSignature(const RootSignature& rootSignature, bool graphicsPipe = true) = 0;

	virtual void UpdateAndBindDescriptorSets(VkCommandBuffer commandBuffer, bool graphicsPipe = true) = 0;
};


class DefaultDynamicDescriptorHeap final : public IDynamicDescriptorHeap
{
public:
	explicit DefaultDynamicDescriptorHeap(CVkDevice* device);

	void SetDescriptorImageInfo(uint32_t rootParameter, uint32_t offset, VkDescriptorImageInfo descriptorImageInfo, bool graphicsPipe = true) override;
	void SetDescriptorBufferInfo(uint32_t rootParameter, uint32_t offset, VkDescriptorBufferInfo descriptorBufferInfo, bool graphicsPipe = true) override;
	void SetDescriptorBufferView(uint32_t rootParameter, uint32_t offset, VkBufferView descriptorBufferView, bool graphicsPipe = true) override;

	void CleanupUsedPools(uint64_t fenceValue) override;

	void ParseRootSignature(const RootSignature& rootSignature, bool graphicsPipe = true) override;

	void UpdateAndBindDescriptorSets(VkCommandBuffer commandBuffer, bool graphicsPipe = true) override;

private:
	DescriptorPoolCache* FindOrCreateDescriptorPoolCache(CVkDescriptorSetLayout* layout, const RootParameter& rootParameter);
	void Reset();

private:
	struct DescriptorBinding
	{
		VkDescriptorType descriptorType;
		uint32_t offset;
		std::variant<VkDescriptorImageInfo, VkDescriptorBufferInfo, VkBufferView> descriptorInfo;
	};

	struct DescriptorCache
	{
		VkDevice device{ VK_NULL_HANDLE };
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
		uint32_t rootParameterIndex{ 0 };
		bool dirty{ false };

		void SetDescriptorImageInfo(uint32_t offset, VkDescriptorImageInfo descriptorImageInfo);
		void SetDescriptorBufferInfo(uint32_t offset, VkDescriptorBufferInfo descriptorBufferInfo);
		void SetDescriptorBufferView(uint32_t, VkBufferView descriptorBufferView);

		void Reset();
		bool UpdateDescriptorSet();
		void SetDescriptorBinding(uint32_t bindingSlot, VkDescriptorType descriptorType, uint32_t offset);

		// TODO: Support dynamic offsets properly.
		bool HasDynamicOffset() const { return hasDynamicOffset; }
		uint32_t GetDynamicOffset() const { return dynamicOffset; }

	private:
		std::unordered_map<uint32_t, DescriptorBinding> descriptorBindings;
		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		bool hasDynamicOffset{ false };
		uint32_t dynamicOffset{ 0 };
	};

private:
	wil::com_ptr<CVkDevice> m_device;

	// Pipeline layouts (index = 0 graphics, 1 = compute)
	VkPipelineLayout m_pipelineLayout[2];

	// Descriptor set caches (index = 0 graphics, 1 = compute)
	std::array<DescriptorCache, MaxRootParameters> m_descriptorCaches[2];
	
	std::unordered_map<VkDescriptorSetLayout, std::shared_ptr<DescriptorPoolCache>> m_layoutToPoolMap;
};

} // namespace Luna::VK