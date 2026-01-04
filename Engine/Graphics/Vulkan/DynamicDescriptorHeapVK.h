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

#if USE_DESCRIPTOR_BUFFERS
#include "Graphics\Vulkan\DescriptorAllocatorVK.h"
#endif // USE_DESCRIPTOR_BUFFERS

namespace Luna
{

#if USE_LEGACY_DESCRIPTOR_SETS
class DescriptorSetPool;
#endif // USE_LEGACY_DESCRIPTOR_SETS

class IColorBuffer;
class IDepthBuffer;
class IGpuBuffer;
class ISampler;
class ITexture;
struct RootParameter;

} // namespace Luna


namespace Luna::VK
{

// Forward declarations
#if USE_LEGACY_DESCRIPTOR_SETS
class DescriptorPoolCache;
#endif // USE_LEGACY_DESCRIPTOR_SETS

class CommandContextVK;
class Descriptor;
class DescriptorSetLayout;
class RootSignature;


#if USE_DESCRIPTOR_BUFFERS
class DynamicDescriptorBuffer
{
public:
	DynamicDescriptorBuffer(CommandContextVK& owningContext, DescriptorBufferType bufferType);
	~DynamicDescriptorBuffer();

	void CleanupUsedBuffers(uint64_t fenceValue);

	void ParseRootSignature(const RootSignature& rootSignature, bool graphicsPipe);

	void UpdateAndBindDescriptorSets(VkCommandBuffer commandBuffer, bool graphicsPipe);

	void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t srvRegister, uint32_t arrayIndex, const IColorBuffer* colorBuffer);
	void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t srvRegister, uint32_t arrayIndex, const IDepthBuffer* depthBuffer, bool depthSrv);
	void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t srvRegister, uint32_t arrayIndex, const IGpuBuffer* gpuBuffer);
	void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t srvRegister, uint32_t arrayIndex, const ITexture* texture);

	void SetUAV(CommandListType type, uint32_t rootIndex, uint32_t uavRegister, uint32_t arrayIndex, const IColorBuffer* colorBuffer);
	void SetUAV(CommandListType type, uint32_t rootIndex, uint32_t uavRegister, uint32_t arrayIndex, const IDepthBuffer* depthBuffer);
	void SetUAV(CommandListType type, uint32_t rootIndex, uint32_t uavRegister, uint32_t arrayIndex, const IGpuBuffer* gpuBuffer);

	void SetCBV(CommandListType type, uint32_t rootIndex, uint32_t cbvRegister, uint32_t arrayIndex, const IGpuBuffer* gpuBuffer);

	void SetSampler(CommandListType type, uint32_t rootIndex, uint32_t samplerRegister, uint32_t arrayIndex, const ISampler* sampler);

private:
	// Static methods
	static CVkBuffer* RequestDescriptorBuffer(DescriptorBufferType bufferType);
	static void DiscardDescriptorBuffers(DescriptorBufferType bufferType, uint64_t fenceValueForReset, const std::vector<CVkBuffer*>& usedBuffers);

	bool HasSpace(size_t neededSpace)
	{
		return (m_currentBufferPtr != nullptr && m_currentOffset + neededSpace <= m_freeSpace);
	}

	void RetireCurrentBuffer();
	void RetireUsedBuffers(uint64_t fenceValue);
	VkBuffer GetCurrentBuffer();

	void ClearGraphicsTableAllocations();
	void ClearComputeTableAllocations();

private:
	// Static members
	static const size_t sm_bufferSize = 64 * (1 << 16);
	static std::mutex sm_mutex;
	static std::vector<wil::com_ptr<CVkBuffer>> sm_descriptorBufferPool[2];
	static std::queue<std::pair<uint64_t, CVkBuffer*>> sm_retiredDescriptorBuffers[2];
	static std::queue<CVkBuffer*> sm_availableDescriptorBuffers[2];

	// Non-static members
	CommandContextVK& m_owningContext;
	CVkBuffer* m_currentBufferPtr{ nullptr };
	const DescriptorBufferType m_bufferType;

	std::byte* m_bufferStart{ nullptr };
	size_t m_currentOffset{ 0 };
	size_t m_freeSpace{ 0 };
	size_t m_offsetAlignment{ 0 };

	std::vector<CVkBuffer*> m_retiredBuffers;

	struct DescriptorCache
	{
		static const size_t kNumDescriptors = 256;
		static const size_t kMaxDescriptorSize = 64;
		static const size_t kScratchMemorySize = kNumDescriptors * kMaxDescriptorSize;

		// Pointers to mapped memory for writing descriptors with vkGetDescriptorEXT
		std::array<DescriptorBufferAllocation, MaxRootParameters> tableAllocations;

		// Descriptor set layouts
		std::array<DescriptorSetLayout*, MaxRootParameters> descriptorSetLayouts;

		std::byte* memory{ nullptr };
		size_t allocationSize{ 0 };

		VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };

		void ParseRootSignature(const RootSignature& rootSignature);

		void SetDescriptor(uint32_t rootIndex, uint32_t srvRegister, uint32_t arrayIndex, const Descriptor* descriptor);
	};

	DescriptorCache m_graphicsCache;
	DescriptorCache m_computeCache;
};
#endif // USE_DESCRIPTOR_BUFFERS


#if USE_LEGACY_DESCRIPTOR_SETS
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
#endif // USE_LEGACY_DESCRIPTOR_SETS

} // namespace Luna::VK