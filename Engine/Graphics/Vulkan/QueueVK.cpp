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

#include "QueueVK.h"

#include "VulkanUtil.h"

using namespace std;

#ifdef CreateSemaphore
#undef CreateSemaphore
#endif

namespace Luna::VK
{

Queue::Queue(CVkDevice* device, VkQueue queue, QueueType queueType, uint32_t queueFamilyIndex)
	: m_device{ device }
	, m_vkQueue  { queue }
	, m_queueType{ queueType }
	, m_queueFamilyIndex{ queueFamilyIndex }
	, m_nextFenceValue{ (uint64_t)queueType << 56 | 1 }
	, m_lastCompletedFenceValue{ (uint64_t)queueType << 56 }
	, m_lastSubmittedFenceValue{ (uint64_t)queueType << 56 }
{
	m_timelineSemaphore = CreateSemaphore(device, VK_SEMAPHORE_TYPE_TIMELINE, m_lastCompletedFenceValue);
	assert(m_timelineSemaphore);
	m_timelineSemaphore->name = std::format("{} Queue Timeline Semaphore", EngineTypeToString(queueType));

	const auto commandListType = QueueTypeToCommandListType(queueType);

	auto commandPool = CreateCommandPool();
	assert(commandPool);

	m_commandBufferPool.Initialize(commandPool.get(), commandListType);
}


void Queue::AddWaitSemaphore(SemaphorePtr semaphore, uint64_t value)
{
	if (!semaphore)
	{
		return;
	}

	m_waitSemaphores.push_back(semaphore);
	m_waitSemaphoreValues.push_back(value);
	m_waitDstStageMask.push_back(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
}


void Queue::AddSignalSemaphore(SemaphorePtr semaphore, uint64_t value)
{
	if (!semaphore)
	{
		return;
	}

	m_signalSemaphores.push_back(semaphore);
	m_signalSemaphoreValues.push_back(value);
}


uint64_t Queue::IncrementFence()
{
	lock_guard<mutex> guard{ m_fenceMutex };

	// Have the queue signal the timeline semaphore
	VkTimelineSemaphoreSubmitInfo timelineInfo{ VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
	timelineInfo.signalSemaphoreValueCount = 1;
	timelineInfo.pSignalSemaphoreValues = &m_nextFenceValue;

	VkSemaphore timelineSemaphore = m_timelineSemaphore->semaphore->Get();

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.pNext = &timelineInfo;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &timelineSemaphore;

	vkQueueSubmit(m_vkQueue, 1, &submitInfo, VK_NULL_HANDLE);

	return m_nextFenceValue++;
}


bool Queue::IsFenceComplete(uint64_t fenceValue)
{
	// Avoid querying the fence value by testing against the last one seen.
	// The max() is to protect against an unlikely race condition that could cause the last
	// completed fence value to regress.
	if (fenceValue > m_lastCompletedFenceValue)
	{
		uint64_t semaphoreCounterValue{ 0 };
		auto res = vkGetSemaphoreCounterValue(m_timelineSemaphore->semaphore->GetDevice(), m_timelineSemaphore->semaphore->Get(), &semaphoreCounterValue);
		assert(res == VK_SUCCESS);
		m_lastCompletedFenceValue = std::max(m_lastCompletedFenceValue, semaphoreCounterValue);
	}

	return fenceValue <= m_lastCompletedFenceValue;
}


void Queue::WaitForFence(uint64_t fenceValue)
{
	if (IsFenceComplete(fenceValue))
	{
		return;
	}

	lock_guard<mutex> guard{ m_fenceMutex };

	VkSemaphore timelineSemaphore = m_timelineSemaphore->semaphore->Get();

	VkSemaphoreWaitInfo waitInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
	waitInfo.semaphoreCount = 1;
	waitInfo.pSemaphores = &timelineSemaphore;
	waitInfo.pValues = &fenceValue;

	auto res = vkWaitSemaphores(m_timelineSemaphore->semaphore->GetDevice(), &waitInfo, UINT64_MAX);
	assert(res == VK_SUCCESS);

	m_lastCompletedFenceValue = fenceValue;
}


uint64_t Queue::ExecuteCommandList(VkCommandBuffer cmdList, VkFence fence)
{
	lock_guard<mutex> guard{ m_fenceMutex };

	const bool incrementFenceAndSignal = (cmdList != VK_NULL_HANDLE);

	if (incrementFenceAndSignal)
	{
		AddSignalSemaphore(m_timelineSemaphore, m_nextFenceValue);
	}

	std::vector<VkSemaphore> waitSemaphores;
	uint32_t index = 0;
	for (auto semaphore : m_waitSemaphores)
	{
		waitSemaphores.push_back(semaphore->semaphore->Get());
		++index;
	}

	std::vector<VkSemaphore> signalSemaphores;
	index = 0;
	for (auto semaphore : m_signalSemaphores)
	{
		signalSemaphores.push_back(semaphore->semaphore->Get());
	}

	auto timelineInfo = VkTimelineSemaphoreSubmitInfo{
		.sType						= VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
		.waitSemaphoreValueCount	= (uint32_t)m_waitSemaphoreValues.size(),
		.pWaitSemaphoreValues		= m_waitSemaphoreValues.data(),
		.signalSemaphoreValueCount	= (uint32_t)m_signalSemaphoreValues.size(),
		.pSignalSemaphoreValues		= m_signalSemaphoreValues.data()
	};

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.pNext = &timelineInfo;
	submitInfo.waitSemaphoreCount = (uint32_t)waitSemaphores.size();
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = m_waitDstStageMask.data();
	submitInfo.signalSemaphoreCount = (uint32_t)signalSemaphores.size();
	submitInfo.pSignalSemaphores = signalSemaphores.data();
	submitInfo.commandBufferCount = cmdList ? 1 : 0;
	submitInfo.pCommandBuffers = cmdList ? &cmdList : nullptr;

	auto res = vkQueueSubmit(m_vkQueue, 1, &submitInfo, fence);
	assert(res == VK_SUCCESS);

	ClearSemaphores();

	// Increment the fence value.
	if (incrementFenceAndSignal)
	{
		m_lastSubmittedFenceValue = m_nextFenceValue;
		return m_nextFenceValue++;
	}
	
	return m_nextFenceValue;
}


VkCommandBuffer Queue::RequestCommandBuffer()
{
	uint64_t completedFence{ 0 };
	vkGetSemaphoreCounterValue(m_timelineSemaphore->semaphore->GetDevice(), m_timelineSemaphore->semaphore->Get(), &completedFence);

	return m_commandBufferPool.RequestCommandBuffer(completedFence);
}


void Queue::DiscardCommandBuffer(uint64_t fenceValueForReset, VkCommandBuffer commandBuffer)
{
	m_commandBufferPool.DiscardCommandBuffer(fenceValueForReset, commandBuffer);
}


wil::com_ptr<CVkCommandPool> Queue::CreateCommandPool()
{
	VkCommandPoolCreateInfo createInfo{
		.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex	= m_queueFamilyIndex
	};

	VkCommandPool vkCommandPool{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkCreateCommandPool(*m_device, &createInfo, nullptr, &vkCommandPool)))
	{
		return Create<CVkCommandPool>(m_device.get(), vkCommandPool);
	}

	return nullptr;
}


void Queue::ClearSemaphores()
{
	m_waitSemaphores.clear();
	m_waitSemaphoreValues.clear();
	m_waitDstStageMask.clear();
	m_signalSemaphores.clear();
	m_signalSemaphoreValues.clear();
}
} // namespace Luna::VK