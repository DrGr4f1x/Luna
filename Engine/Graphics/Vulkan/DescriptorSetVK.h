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
#include "Graphics\Vulkan\VulkanCommon.h"

using namespace Microsoft::WRL;


namespace Luna::VK 
{

struct DescriptorSetDescExt
{
	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	uint32_t numDescriptors{ 0 };
	bool isDynamicBuffer{ false };
};


class __declspec(uuid("EB8306AB-A0A9-419B-9825-4B70B95A9F05")) DescriptorSet final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IDescriptorSet>
	, NonCopyable
{
public:
	explicit DescriptorSet(const DescriptorSetDescExt& descriptorSetDescExt);

	void SetSRV(int slot, const IColorBuffer* colorBuffer) override;
	void SetSRV(int slot, const IDepthBuffer* depthBuffer, bool depthSrv = true) override;
	void SetSRV(int slot, const IGpuBuffer* gpuBuffer) override;

	void SetUAV(int slot, const IColorBuffer* colorBuffer, uint32_t uavIndex = 0) override;
	void SetUAV(int slot, const IDepthBuffer* depthBuffer) override;
	void SetUAV(int slot, const IGpuBuffer* gpuBuffer) override;

	void SetCBV(int slot, const IGpuBuffer* gpuBuffer) override;

	void SetDynamicOffset(uint32_t offset) override;

private:
	bool IsDirty() const noexcept { return m_dirtyBits != 0; }
	void Update();

private:
	VkDescriptorSet m_descriptorSet{ VK_NULL_HANDLE };
	std::array<VkWriteDescriptorSet, MaxDescriptorsPerTable> m_writeDescriptorSets;
	uint32_t m_numDescriptors{ 0 };
	uint32_t m_dirtyBits{ 0 };
	uint32_t m_dynamicOffset{ 0 };
	bool m_isDynamicBuffer{ false };

	bool m_isInitialized{ false };
};

} // namespace Luna::VK