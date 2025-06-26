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

#include "Graphics\PipelineState.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK 
{

// Forward declarations
class Device;


class GraphicsPipelineState : public IGraphicsPipelineState
{
	friend class Device;

public:
	VkPipeline GetPipelineState() const { return m_pipelineState->Get(); }

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<CVkPipeline> m_pipelineState;
};

} // namespace Luna::VK