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

#include "Graphics\RootSignature.h"
#include "Graphics\PipelineState.h"
#include "Graphics\RayTracing\ShaderBindingTable.h"


namespace Luna
{

struct ShaderGroupDesc
{
	ShaderGroupDesc(int32_t i)
	{
		shaderIndices[0] = i;
		shaderIndices[1] = -1;
		shaderIndices[2] = -1;
	}

	ShaderGroupDesc(int32_t i, int32_t j)
	{
		shaderIndices[0] = i;
		shaderIndices[1] = j;
		shaderIndices[2] = -1;
	}

	ShaderGroupDesc(int32_t i, int32_t j, int32_t k)
	{
		shaderIndices[0] = i;
		shaderIndices[1] = j;
		shaderIndices[2] = k;
	}

	int32_t shaderIndices[3]{ -1, -1, -1 };
};


struct RayTracingShaderLibraryEntry
{
	ShaderType type{};
	ShaderNameAndEntry shaderNameAndEntry{};
};


struct RayTracingPipelineDesc
{
	RootSignaturePtr rootSignature;

	// Lists of shaders.  Must always have at least one raygen and miss shader.
	std::vector<RayTracingShaderLibraryEntry> rayGenShaders;
	std::vector<RayTracingShaderLibraryEntry> missShaders;
	std::vector<RayTracingShaderLibraryEntry> hitGroupShaders;

	// Indices into the hitGroupShaders list for each hit group
	std::vector<ShaderGroupDesc> hitGroups;

	uint32_t maxRecursionDepth{ 1 };
	uint32_t maxPayloadSize{ 0 };
	uint32_t maxRayHitAttributeSize{ 0 };
	RayTracingPipelineFlags flags{ RayTracingPipelineFlags::None };
};


class IRayTracingPipeline
{
public:
	virtual ~IRayTracingPipeline() noexcept = default;
};

using RayTracingPipelinePtr = std::shared_ptr<IRayTracingPipeline>;

} // namespace Luna
