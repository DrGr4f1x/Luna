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

#include "CommandBufferPoolVK.h"

using namespace std;


namespace Luna::VK
{

void CommandBufferPool::Initialize(CVkCommandPool* commandPool, CommandListType commandListType)
{
	m_vkCommandPool = commandPool;
	m_commandListType = commandListType;
}


VkCommandBuffer CommandBufferPool::RequestCommandBuffer(uint64_t completedFenceValue)
{
	lock_guard<mutex> guard{ m_commandBufferMutex };

	VkCommandBuffer commandBuffer{ VK_NULL_HANDLE };

	if (!m_readyCommandBuffers.empty())
	{
		auto& [fence, buffer] = m_readyCommandBuffers.front();

		if (fence <= completedFenceValue)
		{
			commandBuffer = buffer;
			if (VK_FAILED(vkResetCommandBuffer(commandBuffer, 0)))
			{
				LogError(LogVulkan) << "Failed to reset command buffer.  Error code: " << res << endl;
				return VK_NULL_HANDLE;
			}
			m_readyCommandBuffers.pop();
		}
	}

	if (commandBuffer == VK_NULL_HANDLE)
	{
		VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocInfo.commandPool = m_vkCommandPool->Get();
		allocInfo.commandBufferCount = 1;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		if (VK_FAILED(vkAllocateCommandBuffers(m_vkCommandPool->GetDevice(), &allocInfo, &commandBuffer)))
		{
			LogError(LogVulkan) << "Failed to allocate command buffer.  Error code: " << res << endl;
			return VK_NULL_HANDLE;
		}

		SetDebugName(m_vkCommandPool->GetDevice(), commandBuffer, format("CommandBuffer {}", m_commandBuffers.size()));

		m_commandBuffers.push_back(commandBuffer);
	}

	return commandBuffer;
}


void CommandBufferPool::DiscardCommandBuffer(uint64_t fenceValue, VkCommandBuffer commandBuffer)
{
	lock_guard<mutex> guard{ m_commandBufferMutex };

	// Fence indicates we are free to re-use the command buffer
	m_readyCommandBuffers.push(make_pair(fenceValue, commandBuffer));
}


void CommandBufferPool::Destroy()
{
	if (m_vkCommandPool == nullptr || m_vkCommandPool->Get() == VK_NULL_HANDLE)
	{
		return;
	}

	lock_guard<mutex> guard{ m_commandBufferMutex };

	while (!m_readyCommandBuffers.empty())
	{
		m_readyCommandBuffers.pop();
	}

	if (!m_commandBuffers.empty())
	{
		vkFreeCommandBuffers(m_vkCommandPool->GetDevice(), m_vkCommandPool->Get(), (uint32_t)m_commandBuffers.size(), m_commandBuffers.data());
		m_commandBuffers.clear();
	}

	m_vkCommandPool.reset();
}

} // namespace Luna::VK