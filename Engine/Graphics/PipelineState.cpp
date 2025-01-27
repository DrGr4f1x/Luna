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

#include "PipelineState.h"

#include "GraphicsCommon.h"


namespace Luna
{

bool GraphicsPSO::Initialize(GraphicsPSODesc& desc)
{
	if (auto device = GetGraphicsDevice())
	{
		m_platformData = device->CreateGraphicsPSOData(desc);
	}

	return m_platformData != nullptr;
}

} // namespace Luna