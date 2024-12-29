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

#include "Graphics\Vulkan\CommandBufferPoolVK.h"
#include "Graphics\Vulkan\VulkanCommon.h"

namespace Luna::VK
{

// Forward declarations
class GraphicsDevice;


class Queue
{
public:
	Queue(GraphicsDevice* device, VkQueue queue, QueueType queueType);

	void AddWaitSemaphore(VkSemaphore semaphore, uint64_t value);
	void AddSignalSemaphore(VkSemaphore semaphore, uint64_t value);

	VkQueue GetVkQueue() const noexcept { return m_vkQueue; }
	VkSemaphore GetTimelineSemaphore() const noexcept { return m_vkTimelineSemaphore->Get(); }

	uint64_t IncrementFence();
	bool IsFenceComplete(uint64_t fenceValue);
	void WaitForFence(uint64_t fenceValue);
	void WaitForIdle()
	{
		WaitForFence(IncrementFence());
	}

	uint64_t GetNextFenceValue() const noexcept { return m_nextFenceValue; }
	uint64_t GetLastCompletedFenceValue() const noexcept { return m_lastCompletedFenceValue; }
	uint64_t GetLastSubmittedFenceValue() const noexcept { return m_lastSubmittedFenceValue; }

	uint64_t ExecuteCommandList(VkCommandBuffer cmdList);
	VkCommandBuffer RequestCommandBuffer();
	void DiscardCommandBuffer(uint64_t fenceValueForReset, VkCommandBuffer commandBuffer);

private:
	void ClearSemaphores();

private:
	VkQueue m_vkQueue{};
	QueueType m_queueType{};

	CommandBufferPool m_commandBufferPool;
	std::mutex m_fenceMutex;

	wil::com_ptr<CVkSemaphore> m_vkTimelineSemaphore;
	uint64_t m_nextFenceValue{ 0 };
	uint64_t m_lastCompletedFenceValue{ 0 };
	uint64_t m_lastSubmittedFenceValue{ 0 };

	std::vector<VkSemaphore> m_waitSemaphores;
	std::vector<uint64_t> m_waitSemaphoreValues;
	std::vector<VkPipelineStageFlags> m_waitDstStageMask;
	std::vector<VkSemaphore> m_signalSemaphores;
	std::vector<uint64_t> m_signalSemaphoreValues;
};

} // namespace Luna::VK