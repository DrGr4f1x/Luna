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

class __declspec(uuid("60397F42-FBB5-493C-8949-90FF458B02C8")) ICommandBufferPool : public IUnknown
{
public:
	virtual ~ICommandBufferPool() = default;

	virtual VkCommandBuffer RequestCommandBuffer(uint64_t completedFenceValue) = 0;
	virtual void DiscardCommandBuffer(uint64_t fenceValue, VkCommandBuffer commandBuffer) = 0;

	virtual size_t Size() const noexcept = 0;
};


class __declspec(uuid("6F49B8DC-36C6-4E93-9DE3-23034DE13DB2")) CommandBufferPool 
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ICommandBufferPool>
	, public NonCopyable
{
public:
	CommandBufferPool(IVkCommandPool* commandPool, CommandListType commandListType) noexcept
		: m_vkCommandPool{ commandPool }
		, m_commandListType{ commandListType }
	{
	}

	~CommandBufferPool()
	{
		Destroy();
	}

	VkCommandBuffer RequestCommandBuffer(uint64_t completedFenceValue) final;
	void DiscardCommandBuffer(uint64_t fenceValue, VkCommandBuffer commandBuffer) final;

	size_t Size() const noexcept final { return m_commandBuffers.size(); }

private:
	void Destroy();

private:
	wil::com_ptr<IVkCommandPool> m_vkCommandPool;
	const CommandListType m_commandListType{ CommandListType::Direct };

	std::vector<VkCommandBuffer> m_commandBuffers;
	std::queue<std::pair<uint64_t, VkCommandBuffer>> m_readyCommandBuffers;
	std::mutex m_commandBufferMutex;
};

} // namespace Luna::VK
