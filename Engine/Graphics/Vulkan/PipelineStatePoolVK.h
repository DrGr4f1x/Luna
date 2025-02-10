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

class PipelineStatePool : public IPipelineStatePool
{
	static const uint32_t MaxItems = (1 << 12);

public:
	explicit PipelineStatePool(CVkDevice* device);
	~PipelineStatePool();

	// Create/Destroy pipeline state
	PipelineStateHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc) override;
	void DestroyHandle(PipelineStateHandleType* handle) override;

	// Getters
	VkPipeline GetPipeline(PipelineStateHandleType* handle) const;

private:
	wil::com_ptr<CVkDevice> m_device;

	// Allocation mutex
	std::mutex m_allocationMutex;

	// Free list
	std::queue<uint32_t> m_freeList;

	// Hot data: CVkPipeline
	std::array<wil::com_ptr<CVkPipeline>, MaxItems> m_pipelines;

	// Cold data: GraphicsPipelineDesc
	std::array<GraphicsPipelineDesc, MaxItems> m_descs;
};


PipelineStatePool* const GetVulkanPipelineStatePool();

} // namespace Luna::VK
