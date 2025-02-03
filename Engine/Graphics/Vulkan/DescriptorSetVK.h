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

class __declspec(uuid("EB8306AB-A0A9-419B-9825-4B70B95A9F05")) DescriptorSet final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IDescriptorSet>
	, NonCopyable
{
	enum { MaxDescriptors = 32 };

public:
	DescriptorSet();

	void SetSRV(int paramIndex, const IColorBuffer* colorBuffer) override;
	void SetSRV(int paramIndex, const IDepthBuffer* depthBuffer, bool depthSrv = true) override;
	void SetSRV(int paramIndex, const IGpuBuffer* gpuBuffer) override;

	void SetUAV(int paramIndex, const IColorBuffer* colorBuffer, uint32_t uavIndex = 0) override;
	void SetUAV(int paramIndex, const IDepthBuffer* depthBuffer) override;
	void SetUAV(int paramIndex, const IGpuBuffer* gpuBuffer) override;

	void SetCBV(int paramIndex, const IGpuBuffer* gpuBuffer) override;

	void SetDynamicOffset(uint32_t offset) override;

private:
	VkDescriptorSet m_descriptorSet{ VK_NULL_HANDLE };
	std::array<VkWriteDescriptorSet, MaxDescriptors> m_writeDescriptorSets;
	uint32_t m_dirtyBits{ 0 };
	uint32_t m_dynamicOffset{ 0 };
	bool m_isDynamicCBV{ false };

	bool m_bIsRootCBV{ false };

	bool m_bIsInitialized{ false };
};

} // namespace Luna::VK