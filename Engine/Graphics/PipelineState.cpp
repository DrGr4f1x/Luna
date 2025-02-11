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

PipelineStateHandleType::~PipelineStateHandleType()
{
	assert(m_pool);
	m_pool->DestroyHandle(this);
}


PrimitiveTopology GraphicsPipelineState::GetPrimitiveTopology() const
{
	return GetDesc().topology;
}


void GraphicsPipelineState::Initialize(GraphicsPipelineDesc& desc)
{
	m_handle = GetGraphicsDevice()->CreateGraphicsPipeline(desc);
}


const GraphicsPipelineDesc& GraphicsPipelineState::GetDesc() const
{
	return GetPipelineStatePool()->GetDesc(m_handle.get());
}

} // namespace Luna