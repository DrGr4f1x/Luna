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

#include "Graphics\Shader.h"

#include "Graphics\PipelineState.h"

using namespace Microsoft::WRL;


namespace Luna::DX12
{

struct ShaderDescExt
{
	std::unique_ptr<std::byte[]> byteCode;
	size_t byteCodeSize{ 0 };
};


Shader* LoadShader(ShaderType type, const ShaderNameAndEntry& shaderNameAndEntry);

} // namespace Luna::DX12