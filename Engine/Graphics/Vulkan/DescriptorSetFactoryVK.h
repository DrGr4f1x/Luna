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
class ColorBuffer;
class DepthBuffer;
class GpuBuffer;
class IResourceManager;

} // namespace Luna


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


class DescriptorSetFactory
{
	static const uint32_t MaxResources = (1 << 10);
	static const uint32_t InvalidAllocation = ~0u;

public:
	DescriptorSetFactory(IResourceManager* owner, CVkDevice* device);

	ResourceHandle CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc);
	void Destroy(uint32_t index);

	void SetSRV(uint32_t index, int slot, const ColorBuffer& colorBuffer);
	void SetSRV(uint32_t index, int slot, const DepthBuffer& depthBuffer, bool depthSrv = true);
	void SetSRV(uint32_t index, int slot, const GpuBuffer& gpuBuffer);
	void SetUAV(uint32_t index, int slot, const ColorBuffer& colorBuffer, uint32_t uavIndex = 0);
	void SetUAV(uint32_t index, int slot, const DepthBuffer& depthBuffer);
	void SetUAV(uint32_t index, int slot, const GpuBuffer& gpuBuffer);
	void SetCBV(uint32_t index, int slot, const GpuBuffer& gpuBuffer);
	void SetDynamicOffset(uint32_t index, uint32_t offset);
	void UpdateGpuDescriptors(uint32_t index);

	bool HasDescriptors(uint32_t index) const;
	VkDescriptorSet GetDescriptorSet(uint32_t index) const;
	uint32_t GetDynamicOffset(uint32_t index) const;
	bool IsDynamicBuffer(uint32_t index) const;

private:
	void ResetDesc(uint32_t index)
	{
		m_descs[index] = DescriptorSetDesc{};
	}

	void ResetData(uint32_t index)
	{
		m_data[index] = DescriptorSetData{};
	}

	void ClearDescs()
	{
		for (uint32_t i = 0; i < MaxResources; ++i)
		{
			ResetDesc(i);
		}
	}

	void ClearData()
	{
		for (uint32_t i = 0; i < MaxResources; ++i)
		{
			ResetData(i);
		}
	}

private:
	IResourceManager* m_owner{ nullptr };
	wil::com_ptr<CVkDevice> m_device;

	std::mutex m_mutex;

	std::queue<uint32_t> m_freeList;

	std::array<DescriptorSetDesc, MaxResources> m_descs;
	std::array<DescriptorSetData, MaxResources> m_data;
	std::unordered_map<VkDescriptorSetLayout, std::unique_ptr<DescriptorPool>> m_setPoolMapping;
};

} // namespace Luna::VK