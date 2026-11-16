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
	std::vector<RayTracingShaderLibraryEntry> shaderLibrary;
	std::vector<ShaderGroupDesc> shaderGroups;
	uint32_t maxRecursionDepth{ 1 };
	uint32_t maxPayloadSize{ 0 };
	uint32_t maxRayHitAttributeSize{ 0 };
	RayTracingPipelineFlags flags{ RayTracingPipelineFlags::None };
};


class IRayTracingPipeline
{
public:
	virtual ~IRayTracingPipeline() = default;

	virtual bool HasRayGenShader() const = 0;
	virtual bool HasMissShader() const = 0;
	virtual uint32_t GetNumHitGroups() const = 0;
};

using RayTracingPipelinePtr = std::shared_ptr<IRayTracingPipeline>;

} // namespace Luna
