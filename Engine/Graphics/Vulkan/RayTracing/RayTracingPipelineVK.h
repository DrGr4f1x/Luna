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
	~RayTracingPipeline() noexcept override = default;

	RayTracingPipeline(const RayTracingPipelineDesc& desc, CVkPipeline* pipeline)
		: m_desc{ desc }
		, m_pipeline{ pipeline }
	{}

	void* GetRayGenShaderIdentifier(uint32_t index) const;
	void* GetMissShaderIdentifier(uint32_t index) const;
	void* GetHitGroupIdentifier(uint32_t index) const;
	void* GetCallableShaderIdentifier(uint32_t index) const;

	const RayTracingPipelineDesc& GetPipelineDesc() const { return m_desc; }

	VkPipeline GetPipeline() const { return m_pipeline ? m_pipeline->Get() : VK_NULL_HANDLE; }

	void GatherShaderIdentifiers(Device* device, uint32_t numRayGenShaders, uint32_t numMissShaders, uint32_t numHitGroups, uint32_t numCallableShaders);

protected:
	RayTracingPipelineDesc m_desc{};
	wil::com_ptr<CVkPipeline> m_pipeline;
	
	std::vector<void*> m_rayGenShaderIdentifiers;
	std::vector<void*> m_missShaderIdentifiers;
	std::vector<void*> m_hitGroupIdentifiers;
	std::vector<void*> m_callableShaderIdentifiers;

	std::vector<std::byte> m_shaderHandleStorage;
};

} // namespace Luna::VK