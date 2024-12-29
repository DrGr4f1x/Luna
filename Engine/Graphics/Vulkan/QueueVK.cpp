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

#include "DeviceVK.h"

using namespace std;


namespace Luna::VK
{
Queue::Queue(GraphicsDevice* device, VkQueue queue, QueueType queueType)
	: m_vkQueue{ queue }
	, m_queueType{ queueType }
	, m_nextFenceValue{ (uint64_t)queueType << 56 | 1 }
	, m_lastCompletedFenceValue{ (uint64_t)queueType << 56 }
	, m_lastSubmittedFenceValue{ (uint64_t)queueType << 56 }
{
	m_vkTimelineSemaphore = device->CreateSemaphore(VK_SEMAPHORE_TYPE_TIMELINE, m_lastCompletedFenceValue);
	assert(m_vkTimelineSemaphore);

	const auto commandListType = QueueTypeToCommandListType(queueType);

	auto commandPool = device->CreateCommandPool(commandListType);
	assert(commandPool);

	m_commandBufferPool.Initialize(commandPool.get(), commandListType);
}


void Queue::AddWaitSemaphore(VkSemaphore semaphore, uint64_t value)
{
	if (!semaphore)
	{
		return;
	}

	m_waitSemaphores.push_back(semaphore);
	m_waitSemaphoreValues.push_back(value);
	m_waitDstStageMask.push_back(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
}


void Queue::AddSignalSemaphore(VkSemaphore semaphore, uint64_t value)
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

	VkSemaphore timelineSemaphore = *m_vkTimelineSemaphore;

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
		auto res = vkGetSemaphoreCounterValue(m_vkTimelineSemaphore->GetDevice(), m_vkTimelineSemaphore->Get(), &semaphoreCounterValue);
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

	VkSemaphore timelineSemaphore = *m_vkTimelineSemaphore;

	VkSemaphoreWaitInfo waitInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
	waitInfo.semaphoreCount = 1;
	waitInfo.pSemaphores = &timelineSemaphore;
	waitInfo.pValues = &fenceValue;

	auto res = vkWaitSemaphores(m_vkTimelineSemaphore->GetDevice(), &waitInfo, UINT64_MAX);
	assert(res == VK_SUCCESS);

	m_lastCompletedFenceValue = fenceValue;
}


uint64_t Queue::ExecuteCommandList(VkCommandBuffer cmdList)
{
	lock_guard<mutex> guard{ m_fenceMutex };

	AddSignalSemaphore(*m_vkTimelineSemaphore, m_nextFenceValue);

	auto timelineInfo = VkTimelineSemaphoreSubmitInfo{
		.sType						= VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
		.pNext						= nullptr,
		.waitSemaphoreValueCount	= (uint32_t)m_waitSemaphoreValues.size(),
		.pWaitSemaphoreValues		= m_waitSemaphoreValues.data(),
		.signalSemaphoreValueCount	= (uint32_t)m_signalSemaphoreValues.size(),
		.pSignalSemaphoreValues		= m_signalSemaphoreValues.data()
	};

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.pNext = &timelineInfo;
	submitInfo.waitSemaphoreCount = (uint32_t)m_waitSemaphores.size();
	submitInfo.pWaitSemaphores = m_waitSemaphores.data();
	submitInfo.pWaitDstStageMask = m_waitDstStageMask.data();
	submitInfo.signalSemaphoreCount = (uint32_t)m_signalSemaphores.size();
	submitInfo.pSignalSemaphores = m_signalSemaphores.data();
	submitInfo.commandBufferCount = cmdList ? 1 : 0;
	submitInfo.pCommandBuffers = &cmdList;

	auto res = vkQueueSubmit(m_vkQueue, 1, &submitInfo, VK_NULL_HANDLE);
	assert(res == VK_SUCCESS);

	ClearSemaphores();

	// Increment the fence value.
	m_lastSubmittedFenceValue = m_nextFenceValue;
	return m_nextFenceValue++;
}


VkCommandBuffer Queue::RequestCommandBuffer()
{
	uint64_t completedFence{ 0 };
	vkGetSemaphoreCounterValue(m_vkTimelineSemaphore->GetDevice(), m_vkTimelineSemaphore->Get(), &completedFence);

	return m_commandBufferPool.RequestCommandBuffer(completedFence);
}


void Queue::DiscardCommandBuffer(uint64_t fenceValueForReset, VkCommandBuffer commandBuffer)
{
	m_commandBufferPool.DiscardCommandBuffer(fenceValueForReset, commandBuffer);
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