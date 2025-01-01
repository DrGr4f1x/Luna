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

#include "DepthBuffer12.h"

namespace Luna::DX12
{

DepthBufferData::DepthBufferData(const DepthBufferDescExt& descExt) noexcept
	: m_resource{ descExt.resource }
	, m_dsvHandles{ descExt.dsvHandles }
	, m_depthSrvHandle{ descExt.depthSrvHandle }
	, m_stencilSrvHandle{ descExt.stencilSrvHandle }
{}

} // namespace Luna::DX12