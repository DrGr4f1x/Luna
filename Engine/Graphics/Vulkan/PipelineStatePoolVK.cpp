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

PipelineStatePool* g_pipelineStatePool{ nullptr };


PipelineStatePool::PipelineStatePool(CVkDevice* device)
	: m_device{ device }
{
	assert(g_pipelineStatePool == nullptr);

	// Populate freelist and data arrays
	for (uint32_t i = 0; i < MaxItems; ++i)
	{
		m_freeList.push(i);
		m_pipelines[i].reset();
		m_descs[i] = GraphicsPipelineDesc{};
	}

	g_pipelineStatePool = this;
}


PipelineStatePool::~PipelineStatePool()
{
	g_pipelineStatePool = nullptr;
}


PipelineStateHandle PipelineStatePool::CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_freeList.empty());

	uint32_t index = m_freeList.front();
	m_freeList.pop();

	m_descs[index] = pipelineDesc;

	// TODO: do pipeline creation here, not in the device
	m_pipelines[index] = GetVulkanGraphicsDevice()->AllocateGraphicsPipeline(pipelineDesc);

	return Create<PipelineStateHandleType>(index, this);
}


void PipelineStatePool::DestroyHandle(PipelineStateHandleType* handle)
{
	std::lock_guard guard(m_allocationMutex);

	// TODO: queue this up to execute in one big deallocation batch, e.g. at the end of the frame

	uint32_t index = handle->GetIndex();
	m_descs[index] = GraphicsPipelineDesc{};
	m_pipelines[index].reset();

	m_freeList.push(index);
}


VkPipeline PipelineStatePool::GetPipeline(PipelineStateHandleType* handle) const
{
	uint32_t index = handle->GetIndex();
	return m_pipelines[index]->Get();
}


PipelineStatePool* const GetVulkanPipelineStatePool()
{
	return g_pipelineStatePool;
}

} // namespace Luna::VK