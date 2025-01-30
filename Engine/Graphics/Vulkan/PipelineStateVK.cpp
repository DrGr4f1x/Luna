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

#include "PipelineStateVK.h"


namespace Luna::VK
{

GraphicsPipeline::GraphicsPipeline(CVkPipeline* pipeline)
	: m_pipeline{ pipeline }
{}


NativeObjectPtr GraphicsPipeline::GetNativeObject() const noexcept
{
	return NativeObjectPtr(*m_pipeline);
}

} // namespace Luna::VK