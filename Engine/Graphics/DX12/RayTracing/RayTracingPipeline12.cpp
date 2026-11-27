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

#include "RayTracingPipeline12.h"

#include "Graphics\DX12\Device12.h"

using namespace std;


namespace Luna::DX12
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


void RayTracingPipeline::GatherShaderIdentifiers(
	const vector<wstring>& rayGenShaderNames,
	const vector<wstring>& missShaderNames,
	const vector<wstring>& callableShaderNames,
	const vector<wstring>& hitGroupNames)
{
	auto stateObjectProperties = m_stateObject.query<ID3D12StateObjectProperties>();
	assert(stateObjectProperties);

	auto GatherIdentifiers = [stateObjectProperties](const vector<wstring>& names, vector<void*>& identifiers)
		{
			for (const auto& name : names)
			{
				void* identifier = stateObjectProperties->GetShaderIdentifier(name.c_str());
				assert(identifier != nullptr);
				identifiers.push_back(identifier);
			}
		};

	GatherIdentifiers(rayGenShaderNames, m_rayGenShaderIdentifiers);
	GatherIdentifiers(missShaderNames, m_missShaderIdentifiers);
	GatherIdentifiers(callableShaderNames, m_callableShaderIdentifiers);
	GatherIdentifiers(hitGroupNames, m_hitGroupIdentifiers);
}

} // namespace Luna::DX12