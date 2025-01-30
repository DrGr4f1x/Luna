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

GraphicsPipeline::GraphicsPipeline(ID3D12PipelineState* pipelineState)
	: m_pipelineState{ pipelineState }
{}


NativeObjectPtr GraphicsPipeline::GetNativeObject() const noexcept
{
	return NativeObjectPtr(m_pipelineState.get());
}

} // namespace Luna::DX12