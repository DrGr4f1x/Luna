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
	assert(index < m_rayGenShaderIdentifierInfo.numIdentifiers);
	return (void*)(m_shaderHandleStorage.data() + m_rayGenShaderIdentifierInfo.offsetInBytes + index * m_shaderIdentifierSize);
}


void* RayTracingPipeline::GetMissShaderIdentifier(uint32_t index) const
{
	assert(index < m_missShaderIdentifierInfo.numIdentifiers);
	return (void*)(m_shaderHandleStorage.data() + m_missShaderIdentifierInfo.offsetInBytes + index * m_shaderIdentifierSize);
}


void* RayTracingPipeline::GetHitGroupIdentifier(uint32_t index) const
{
	assert(index < m_hitGroupIdentifierInfo.numIdentifiers);
	return (void*)(m_shaderHandleStorage.data() + m_hitGroupIdentifierInfo.offsetInBytes + index * m_shaderIdentifierSize);
}


void* RayTracingPipeline::GetCallableShaderIdentifier(uint32_t index) const
{
	assert(index < m_callableShaderIdentifierInfo.numIdentifiers);
	return (void*)(m_shaderHandleStorage.data() + m_callableShaderIdentifierInfo.offsetInBytes + index * m_shaderIdentifierSize);
}


void RayTracingPipeline::GatherShaderIdentifiers(Device* device, uint32_t numRayGenShaders, uint32_t numMissShaders, uint32_t numHitGroups, uint32_t numCallableShaders)
{
	const auto& caps = device->GetDeviceCaps();

	// Get the shader handle size
	m_shaderIdentifierSize = caps.shaderStage.rayTracing.shaderGroupIdentifierSize;

	// Compute total size of shader handles
	const uint32_t numHandles = numRayGenShaders + numMissShaders + numHitGroups + numCallableShaders;
	const uint32_t totalShaderIdentifierSize = (uint32_t)m_shaderIdentifierSize * numHandles;

	// Get the shader group handles
	m_shaderHandleStorage.clear();
	m_shaderHandleStorage.resize(totalShaderIdentifierSize);
	vkGetRayTracingShaderGroupHandlesKHR(device->GetVulkanDevice(), m_pipeline->Get(), 0, numHandles, totalShaderIdentifierSize, m_shaderHandleStorage.data());

	// Setup ray generation shader identifier info
	m_rayGenShaderIdentifierInfo.numIdentifiers = numRayGenShaders;
	m_rayGenShaderIdentifierInfo.offsetInBytes = 0;

	// Setup miss shader identifier info
	m_missShaderIdentifierInfo.numIdentifiers = numMissShaders;
	m_missShaderIdentifierInfo.offsetInBytes = m_rayGenShaderIdentifierInfo.offsetInBytes + (m_rayGenShaderIdentifierInfo.numIdentifiers * m_shaderIdentifierSize);

	// Setup hit group identifier info
	m_hitGroupIdentifierInfo.numIdentifiers = numHitGroups;
	m_hitGroupIdentifierInfo.offsetInBytes = m_missShaderIdentifierInfo.offsetInBytes + (m_missShaderIdentifierInfo.numIdentifiers * m_shaderIdentifierSize);

	// Finally, setup the callable shader identifier info
	m_callableShaderIdentifierInfo.numIdentifiers = numCallableShaders;
	m_callableShaderIdentifierInfo.offsetInBytes = m_hitGroupIdentifierInfo.offsetInBytes + (m_hitGroupIdentifierInfo.numIdentifiers * m_shaderIdentifierSize);
}

} // namespace Luna::VK