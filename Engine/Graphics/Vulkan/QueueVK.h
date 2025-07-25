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
#include "Graphics\Vulkan\SemaphoreVK.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

// Forward declarations
class GraphicsDevice;


class Queue
{
public:
	Queue(CVkDevice* device, VkQueue queue, QueueType queueType, uint32_t queueFamilyIndex);

	void AddWaitSemaphore(SemaphorePtr semaphore, uint64_t value);
	void AddSignalSemaphore(SemaphorePtr semaphore, uint64_t value);

	VkQueue GetVkQueue() const noexcept { return m_vkQueue; }
	SemaphorePtr GetTimelineSemaphore() const noexcept { return m_timelineSemaphore; }

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

	uint64_t ExecuteCommandList(VkCommandBuffer cmdList, VkFence fence = VK_NULL_HANDLE);
	VkCommandBuffer RequestCommandBuffer();
	void DiscardCommandBuffer(uint64_t fenceValueForReset, VkCommandBuffer commandBuffer);

private:
	wil::com_ptr<CVkCommandPool> CreateCommandPool();
	void ClearSemaphores();

private:
	wil::com_ptr<CVkDevice> m_device;

	VkQueue m_vkQueue{};
	QueueType m_queueType{};
	uint32_t m_queueFamilyIndex{ 0 };

	CommandBufferPool m_commandBufferPool;
	std::mutex m_fenceMutex;

	SemaphorePtr m_timelineSemaphore;
	uint64_t m_nextFenceValue{ 0 };
	uint64_t m_lastCompletedFenceValue{ 0 };
	uint64_t m_lastSubmittedFenceValue{ 0 };

	std::vector<SemaphorePtr> m_waitSemaphores;
	std::vector<uint64_t> m_waitSemaphoreValues;
	std::vector<VkPipelineStageFlags> m_waitDstStageMask;
	std::vector<SemaphorePtr> m_signalSemaphores;
	std::vector<uint64_t> m_signalSemaphoreValues;
};

} // namespace Luna::VK