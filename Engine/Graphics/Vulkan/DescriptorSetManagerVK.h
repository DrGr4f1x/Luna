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

#include "Graphics\DescriptorSet.h"
#include "Graphics\RootSignature.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

// Forward declarations
class DescriptorPool;


struct DescriptorSetDesc
{
	CVkDescriptorSetLayout* descriptorSetLayout{ nullptr };
	RootParameter rootParameter{};
	VulkanBindingOffsets bindingOffsets{};
	uint32_t numDescriptors{ 0 };
	bool isDynamicBuffer{ false };
};


using DescriptorData = std::variant<VkDescriptorImageInfo, VkDescriptorBufferInfo, VkBufferView>;


struct DescriptorSetData
{
	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	VulkanBindingOffsets bindingOffsets;
	std::array<VkWriteDescriptorSet, MaxDescriptorsPerTable> writeDescriptorSets;
	std::array<DescriptorData, MaxDescriptorsPerTable> descriptorData;
	uint32_t numDescriptors{ 0 };
	uint32_t dirtyBits{ 0 };
	uint32_t dynamicOffset{ 0 };
	bool isDynamicBuffer{ false };
};


class DescriptorSetManager : public IDescriptorSetManager
{
	static const uint32_t MaxItems = (1 << 16);

public:
	explicit DescriptorSetManager(CVkDevice* device);
	~DescriptorSetManager();

	// Create/Destroy descriptor set
	DescriptorSetHandle CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc);
	void DestroyHandle(DescriptorSetHandleType* handle) override;

	// Platform agnostic functions
	void SetSRV(DescriptorSetHandleType* handle, int slot, const ColorBuffer& colorBuffer) override;
	void SetSRV(DescriptorSetHandleType* handle, int slot, const DepthBuffer& depthBuffer, bool depthSrv = true) override;
	void SetSRV(DescriptorSetHandleType* handle, int slot, const GpuBuffer& gpuBuffer) override;

	void SetUAV(DescriptorSetHandleType* handle, int slot, const ColorBuffer& colorBuffer, uint32_t uavIndex = 0) override;
	void SetUAV(DescriptorSetHandleType* handle, int slot, const DepthBuffer& depthBuffer) override;
	void SetUAV(DescriptorSetHandleType* handle, int slot, const GpuBuffer& gpuBuffer) override;

	void SetCBV(DescriptorSetHandleType* handle, int slot, const GpuBuffer& gpuBuffer) override;

	void SetDynamicOffset(DescriptorSetHandleType* handle, uint32_t offset) override;

	void UpdateGpuDescriptors(DescriptorSetHandleType* handle) override;

	// Platform specific functions
	bool HasDescriptors(DescriptorSetHandleType* handle) const;
	VkDescriptorSet GetDescriptorSet(DescriptorSetHandleType* handle) const;
	uint32_t GetDynamicOffset(DescriptorSetHandleType* handle) const;
	bool IsDynamicBuffer(DescriptorSetHandleType* handle) const;

private:
	wil::com_ptr<CVkDevice> m_device;

	// Allocation mutex
	std::mutex m_allocationMutex;

	// Free list
	std::queue<uint32_t> m_freeList;

	// Cold data
	std::array<DescriptorSetDesc, MaxItems> m_descs;

	// Hot data
	std::array<DescriptorSetData, MaxItems> m_descriptorData;

	// Descriptor set pools
	std::unordered_map<VkDescriptorSetLayout, std::unique_ptr<DescriptorPool>> m_setPoolMapping;
};


DescriptorSetManager* const GetVulkanDescriptorSetManager();

} // namespace Luna::VK