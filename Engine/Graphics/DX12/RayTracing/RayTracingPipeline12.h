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
#include "Graphics\DX12\DirectXCommon.h"

namespace Luna::DX12
{

class RayTracingPipeline : public IRayTracingPipeline
{
public:
	RayTracingPipeline(const RayTracingPipelineDesc& desc, ID3D12StateObject* stateObject)
		: m_desc{ desc }
		, m_stateObject{ stateObject }
	{}

	const RayTracingPipelineDesc& GetPipelineDesc() const { return m_desc; }

	ID3D12StateObject* GetStateObject() const { return m_stateObject.get(); }

	void* GetRayGenShaderIdentifier(uint32_t index) const;
	void* GetMissShaderIdentifier(uint32_t index) const;
	void* GetCallableShaderIdentifier(uint32_t index) const;
	void* GetHitGroupIdentifier(uint32_t index) const;

	void GatherShaderIdentifiers(
		const std::vector<std::wstring>& rayGenShaderNames,
		const std::vector<std::wstring>& missShaderNames,
		const std::vector<std::wstring>& callableShaderNames,
		const std::vector<std::wstring>& hitGroupNames);

protected:
	RayTracingPipelineDesc m_desc{};
	wil::com_ptr<ID3D12StateObject> m_stateObject;

	std::vector<void*> m_rayGenShaderIdentifiers;
	std::vector<void*> m_missShaderIdentifiers;
	std::vector<void*> m_callableShaderIdentifiers;
	std::vector<void*> m_hitGroupIdentifiers;
};

} // Luna::DX12