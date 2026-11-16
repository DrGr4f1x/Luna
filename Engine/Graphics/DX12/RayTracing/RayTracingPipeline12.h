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
	friend class Device;

public:
	const RayTracingPipelineDesc& GetPipelineDesc() const { return m_desc; }

	bool HasRayGenShader() const override { return !m_rayGenShaderName.empty(); }
	bool HasMissShader() const override { return !m_missShaderName.empty(); }
	uint32_t GetNumHitGroups() const override { return (uint32_t)m_hitGroupNames.size(); }

	const wchar_t* GetRayGenerationShaderName() const { return m_rayGenShaderName.c_str(); }
	const wchar_t* GetMissShaderName() const { return m_missShaderName.c_str(); }

	const wchar_t* GetHitGroupName(uint32_t index) const;

	ID3D12StateObject* GetStateObject() const { return m_stateObject.get(); }
	ID3D12StateObjectProperties* GetStateObjectProperties() const { return m_stateObjectProperties.get(); }

protected:
	RayTracingPipelineDesc m_desc{};
	wil::com_ptr<ID3D12StateObject> m_stateObject;
	wil::com_ptr<ID3D12StateObjectProperties> m_stateObjectProperties;

	std::wstring m_rayGenShaderName;
	std::wstring m_missShaderName;
	std::vector<std::wstring> m_hitGroupNames;
};

} // Luna::DX12