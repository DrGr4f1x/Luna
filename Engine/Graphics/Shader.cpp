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

#include "Shader.h"

#include "GraphicsCommon.h"


namespace Luna
{

bool Shader::Initialize(const ShaderDesc& desc)
{
	return false;
}


const std::byte* Shader::GetByteCode() const
{
	if (m_shaderData)
	{
		return m_shaderData->GetByteCode();
	}
	return nullptr;
}


size_t Shader::GetByteCodeSize() const
{
	if (m_shaderData)
	{
		return m_shaderData->GetByteCodeSize();
	}
	return 0;
}

} // namespace Luna