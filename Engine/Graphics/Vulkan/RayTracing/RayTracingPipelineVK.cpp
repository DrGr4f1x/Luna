//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Stdafx.h"

#include "RayTracingPipelineVK.h"

#include "Graphics\Vulkan\DeviceVK.h"

using namespace std;


namespace Luna::VK
{

void* RayTracingPipeline::GetRayGenShaderIdentifier(uint32_t index) const
{
	assert(index < m_rayGenShaderIdentifiers.size());
	return m_rayGenShaderIdentifiers[index];
}


void* RayTracingPipeline::GetMissShaderIdentifier(uint32_t index) const
{
	assert(index < m_missShaderIdentifiers.size());
	return m_missShaderIdentifiers[index];
}


void* RayTracingPipeline::GetHitGroupIdentifier(uint32_t index) const
{
	assert(index < m_hitGroupIdentifiers.size());
	return m_hitGroupIdentifiers[index];
}


void* RayTracingPipeline::GetCallableShaderIdentifier(uint32_t index) const
{
	assert(index < m_callableShaderIdentifiers.size());
	return m_callableShaderIdentifiers[index];
}


void RayTracingPipeline::GatherShaderIdentifiers(Device* device, uint32_t numRayGenShaders, uint32_t numMissShaders, uint32_t numHitGroups, uint32_t numCallableShaders)
{
	const auto& caps = device->GetDeviceCaps();

	// Get the shader handle size
	const size_t shaderIdentifierSize = caps.shaderStage.rayTracing.shaderGroupIdentifierSize;

	// Compute total size of shader handles
	const uint32_t numHandles = numRayGenShaders + numMissShaders + numHitGroups + numCallableShaders;
	const uint32_t totalShaderIdentifierSize = (uint32_t)shaderIdentifierSize * numHandles;

	// Get the shader group handles
	m_shaderHandleStorage.clear();
	m_shaderHandleStorage.resize(totalShaderIdentifierSize);
	vkGetRayTracingShaderGroupHandlesKHR(device->GetVulkanDevice(), m_pipeline->Get(), 0, numHandles, totalShaderIdentifierSize, m_shaderHandleStorage.data());

	// Get the ray generation shader handles
	std::byte* ptr = m_shaderHandleStorage.data();
	for (uint32_t i = 0; i < numRayGenShaders; ++i)
	{
		m_rayGenShaderIdentifiers.push_back(ptr);
		ptr += shaderIdentifierSize;
	}

	// Get the miss shader handles
	for (uint32_t i = 0; i < numMissShaders; ++i)
	{
		m_missShaderIdentifiers.push_back(ptr);
		ptr += shaderIdentifierSize;
	}

	// Get the hit group handles
	for (uint32_t i = 0; i < numHitGroups; ++i)
	{
		m_hitGroupIdentifiers.push_back(ptr);
		ptr += shaderIdentifierSize;
	}

	// Finally, get the callable shader handles
	for (uint32_t i = 0; i < numCallableShaders; ++i)
	{
		m_callableShaderIdentifiers.push_back(ptr);
		ptr += shaderIdentifierSize;
	}
}

} // namespace Luna::VK