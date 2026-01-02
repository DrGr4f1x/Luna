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

#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

class CommandBufferPool : public NonCopyable
{
public:
	CommandBufferPool() noexcept = default;

	~CommandBufferPool()
	{
		Destroy();
	}

	void Initialize(CVkCommandPool* commandPool, CommandListType commandListType);

	VkCommandBuffer RequestCommandBuffer(uint64_t completedFenceValue);
	void DiscardCommandBuffer(uint64_t fenceValue, VkCommandBuffer commandBuffer);

	size_t Size() const noexcept { return m_commandBuffers.size(); }

private:
	void Destroy();

private:
	wil::com_ptr<CVkCommandPool> m_vkCommandPool;
	CommandListType m_commandListType{ CommandListType::Graphics };

	std::vector<VkCommandBuffer> m_commandBuffers;
	std::queue<std::pair<uint64_t, VkCommandBuffer>> m_readyCommandBuffers;
	std::mutex m_commandBufferMutex;
};

} // namespace Luna::VK
