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

#include "Shader12.h"

namespace Luna::DX12
{

ShaderData::ShaderData(ShaderDescExt& descExt)
	: m_byteCode{ std::move(descExt.byteCode) }
	, m_byteCodeSize{ descExt.byteCodeSize }
{}

} // namespace Luna::DX12