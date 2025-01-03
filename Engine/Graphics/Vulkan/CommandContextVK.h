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

#include "Graphics\CommandContext.h"
#include "Graphics\Vulkan\VulkanCommon.h"

using namespace Microsoft::WRL;


namespace Luna
{

// Forward declarations
class ColorBuffer;
class DepthBuffer;

} // namespace Luna


namespace Luna::VK
{

// Forward declarations
class ComputeContext;
class GraphicsContext;


struct TextureBarrier
{
	VkImage image{ VK_NULL_HANDLE };
	VkFormat format{ VK_FORMAT_UNDEFINED };
	VkImageAspectFlags imageAspect{ 0 };
	ResourceState beforeState{ ResourceState::Undefined };
	ResourceState afterState{ ResourceState::Undefined };
	uint32_t numMips{ 1 };
	uint32_t mipLevel{ 0 };
	uint32_t arraySizeOrDepth{ 1 };
	uint32_t arraySlice{ 0 };
	bool bWholeTexture{ false };
};


struct BufferBarrier
{
	// TODO - Vulkan GPU buffer support
};


class __declspec(uuid("63C5358D-F31C-43DA-90DA-8676E272BE4A")) CommandContextVK final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ICommandContext>
{
public:
	explicit CommandContextVK(CommandListType type)
		: m_type{ type }
	{}

	void SetId(const std::string& id) override { m_id = id; }
	CommandListType GetType() const override { return m_type; }

	// Debug events and markers
	void BeginEvent(const std::string& label) override;
	void EndEvent() override;
	void SetMarker(const std::string& label) override;

	void Reset() override;
	void Initialize() override;

	void BeginFrame() override;
	uint64_t Finish(bool bWaitForCompletion) override;

	void TransitionResource(ColorBuffer& colorBuffer, ResourceState newState, bool bFlushImmediate) override;
	void TransitionResource(DepthBuffer& depthBuffer, ResourceState newState, bool bFlushImmediate) override;
	void InsertUAVBarrier(ColorBuffer& colorBuffer, bool bFlushImmediate) override;
	void FlushResourceBarriers() override;

	void ClearColor(ColorBuffer& colorBuffer) override;
	void ClearColor(ColorBuffer& colorBuffer, Color clearColor) override;
	void ClearDepth(DepthBuffer& depthBuffer) override;
	void ClearStencil(DepthBuffer& depthBuffer) override;
	void ClearDepthAndStencil(DepthBuffer& depthBuffer) override;

private:
	// TODO Put this in namespace Internal
	void ClearDepthAndStencil_Internal(DepthBuffer& depthBuffer, VkImageAspectFlags flags);

	void BindDescriptorHeaps() {}

	size_t GetPendingBarrierCount() const noexcept { return m_textureBarriers.size() + m_bufferBarriers.size(); }

private:
	std::string m_id;
	CommandListType m_type;

	VkCommandBuffer m_commandBuffer{ VK_NULL_HANDLE };

	bool m_bInvertedViewport{ true };
	bool m_hasPendingDebugEvent{ false };

	// Resource barriers
	std::vector<TextureBarrier> m_textureBarriers;
	std::vector<BufferBarrier> m_bufferBarriers;
	std::vector<VkMemoryBarrier2> m_memoryBarriers;
	std::vector<VkBufferMemoryBarrier2> m_bufferMemoryBarriers;
	std::vector<VkImageMemoryBarrier2> m_imageMemoryBarriers;
};

} // namespace Luna::VK