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

#include "Graphics\RayTracing\RayTracingPipeline.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

class RayTracingPipeline : public IRayTracingPipeline
{
	friend class Device;

public:
	const RayTracingPipelineDesc& GetPipelineDesc() const { return m_desc; }

	bool HasRayGenShader() const { return m_hasRayGenShader; }
	bool HasMissShader() const { return m_hasMissShader; }
	uint32_t GetNumHitGroups() const { return m_numHitGroups; }

	VkPipeline GetPipeline() const { return m_pipeline ? m_pipeline->Get() : VK_NULL_HANDLE; }

protected:
	RayTracingPipelineDesc m_desc{};
	wil::com_ptr<CVkPipeline> m_pipeline;
	bool m_hasRayGenShader{ false };
	bool m_hasMissShader{ false };
	uint32_t m_numHitGroups{ 0 };
};

} // namespace Luna::VK