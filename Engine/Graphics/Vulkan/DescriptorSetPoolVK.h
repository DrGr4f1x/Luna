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

struct DescriptorSetDesc
{
	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	VulkanBindingOffsets bindingOffsets;
	uint32_t numDescriptors{ 0 };
	bool isDynamicBuffer{ false };
};


struct DescriptorSetData
{
	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	VulkanBindingOffsets bindingOffsets;
	std::array<VkWriteDescriptorSet, MaxDescriptorsPerTable> writeDescriptorSets;
	uint32_t numDescriptors{ 0 };
	uint32_t dirtyBits{ 0 };
	uint32_t dynamicOffset{ 0 };
	bool isDynamicBuffer{ false };
};


class DescriptorSetPool : public IDescriptorSetPool
{
	static const uint32_t MaxItems = (1 << 16);

public:
	explicit DescriptorSetPool(CVkDevice* device);
	~DescriptorSetPool();

	// Create/Destroy descriptor set
	DescriptorSetHandle CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc);
	void DestroyHandle(DescriptorSetHandleType* handle) override;

	// Platform agnostic functions
	void SetSRV(DescriptorSetHandleType* handle, int slot, const IColorBuffer* colorBuffer) override;
	void SetSRV(DescriptorSetHandleType* handle, int slot, const IDepthBuffer* depthBuffer, bool depthSrv = true) override;
	void SetSRV(DescriptorSetHandleType* handle, int slot, const IGpuBuffer* gpuBuffer) override;

	void SetUAV(DescriptorSetHandleType* handle, int slot, const IColorBuffer* colorBuffer, uint32_t uavIndex = 0) override;
	void SetUAV(DescriptorSetHandleType* handle, int slot, const IDepthBuffer* depthBuffer) override;
	void SetUAV(DescriptorSetHandleType* handle, int slot, const IGpuBuffer* gpuBuffer) override;

	void SetCBV(DescriptorSetHandleType* handle, int slot, const IGpuBuffer* gpuBuffer) override;

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

	// Hot data
	std::array<DescriptorSetData, MaxItems> m_descriptorData;
};


DescriptorSetPool* const GetVulkanDescriptorSetPool();

} // namespace Luna::VK