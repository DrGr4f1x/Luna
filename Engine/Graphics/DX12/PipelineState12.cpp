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

#include "PipelineState12.h"


namespace Luna::DX12
{

GraphicsPSO12::GraphicsPSO12(const GraphicsPSODesc& graphicsPSODesc, const GraphicsPSODescExt& graphicsPSODescExt)
	: m_pipelineState{ graphicsPSODescExt.pipelineState }
{}


NativeObjectPtr GraphicsPSO12::GetNativeObject() const noexcept
{
	return NativeObjectPtr(m_pipelineState.get());
}

} // namespace Luna::DX12