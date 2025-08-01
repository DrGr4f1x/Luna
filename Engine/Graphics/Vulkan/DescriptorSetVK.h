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
class Device;


using DescriptorData = std::variant<VkDescriptorImageInfo, VkDescriptorBufferInfo, VkBufferView>;


class DescriptorSet : public IDescriptorSet
{
	friend class Device;

public:
	void SetSRV(uint32_t slot, ColorBufferPtr colorBuffer) override;
	void SetSRV(uint32_t slot, DepthBufferPtr depthBuffer, bool depthSrv = true) override;
	void SetSRV(uint32_t slot, GpuBufferPtr gpuBuffer) override;
	void SetSRV(uint32_t slot, TexturePtr texture) override;

	void SetUAV(uint32_t slot, ColorBufferPtr colorBuffer, uint32_t uavIndex = 0) override;
	void SetUAV(uint32_t slot, DepthBufferPtr depthBuffer) override;
	void SetUAV(uint32_t slot, GpuBufferPtr gpuBuffer) override;

	void SetCBV(uint32_t slot, GpuBufferPtr gpuBuffer) override;

	void SetSampler(uint32_t slot, SamplerPtr sampler) override;

	void SetDynamicOffset(uint32_t offset) override;

	void UpdateGpuDescriptors() override;

	bool HasDescriptors() const;
	VkDescriptorSet GetDescriptorSet() const { return m_descriptorSet; }
	uint32_t GetDynamicOffset() const { return m_dynamicOffset; }
	bool IsDynamicBuffer() const { return m_isDynamicBuffer; }

protected:
	Device* m_device{ nullptr };

	VkDescriptorSet m_descriptorSet{ VK_NULL_HANDLE };
	std::array<VkWriteDescriptorSet, MaxDescriptorsPerTable> m_writeDescriptorSets;
	std::array<DescriptorData, MaxDescriptorsPerTable> m_descriptorData;
	uint32_t m_numDescriptors{ 0 };
	uint32_t m_dirtyBits{ 0 };
	uint32_t m_dynamicOffset{ 0 };
	bool m_isDynamicBuffer{ false };
};

} // namespace Luna::VK