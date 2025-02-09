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

#include "PipelineStatePoolVK.h"

#include "Graphics\PlatformData.h"

#include "DeviceVK.h"


namespace Luna::VK
{

PipelineStatePool::PipelineStatePool(CVkDevice* device)
	: m_device{ device }
{
	// Populate freelist and data arrays
	for (uint32_t i = 0; i < MaxItems; ++i)
	{
		m_freeList.push(i);
		m_pipelines[i].reset();
		m_descs[i] = GraphicsPipelineDesc{};
	}
}


PipelineStateHandle PipelineStatePool::CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_freeList.empty());

	uint32_t index = m_freeList.front();
	m_freeList.pop();

	m_descs[index] = pipelineDesc;

	// TODO: do the pipeline creation here, not in the device
	auto pipeline = GetVulkanGraphicsDevice()->CreateGraphicsPipeline(pipelineDesc);
	m_pipelines[index] = pipeline->GetVulkan();

	return Create<PipelineStateHandleType>(index, this);
}


void PipelineStatePool::DestroyHandle(PipelineStateHandleType* handle)
{
	std::lock_guard guard(m_allocationMutex);

	// TODO: queue this up to execute in one big batch, e.g. at the end of the frame

	uint32_t index = handle->GetIndex();
	m_descs[index] = GraphicsPipelineDesc{};
	m_pipelines[index].reset();

	m_freeList.push(index);
}

} // namespace Luna::VK