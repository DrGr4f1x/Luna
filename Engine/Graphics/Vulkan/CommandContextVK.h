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


struct ContextState
{
	friend class ComputeContext;
	friend class GraphicsContext;

	~ContextState();

	std::string id;
	CommandListType type;

private:
	// Debug events and markers
	void BeginEvent(const std::string& label);
	void EndEvent();
	void SetMarker(const std::string& label);

	void Reset();
	void Initialize();

	void Begin(const std::string& id);
	uint64_t Finish(bool bWaitForCompletion);

	void TransitionResource(ColorBuffer& colorBuffer, ResourceState newState, bool bFlushImmediate);
	void InsertUAVBarrier(ColorBuffer& colorBuffer, bool bFlushImmediate);
	void FlushResourceBarriers();

	void ClearColor(ColorBuffer& colorBuffer);
	void ClearColor(ColorBuffer& colorBuffer, Color clearColor);

	void BindDescriptorHeaps();

	size_t GetPendingBarrierCount() const noexcept { return m_textureBarriers.size() + m_bufferBarriers.size(); }

private:
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


class __declspec(uuid("EAA87286-6C5C-4C61-A18C-D45010F95E1E")) ComputeContext
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IComputeContext, ICommandContext>>
	, NonCopyable
{
	friend class DeviceManager;

public:
	ComputeContext();
	virtual ~ComputeContext() = default;

	// ICommandContext implementation
	void SetId(const std::string& id) final { m_state.id = id; }
	CommandListType GetType() const final { return m_state.type; }

	// Debug events and markers
	void BeginEvent(const std::string& label) final { m_state.BeginEvent(label); }
	void EndEvent() final { m_state.EndEvent(); }
	void SetMarker(const std::string& label) final { m_state.SetMarker(label); }

	void Reset() final { m_state.Reset(); }
	void Initialize() final { m_state.Initialize(); }

	void Begin(const std::string& id) final { m_state.Begin(id); }
	uint64_t Finish(bool bWaitForCompletion = false) final;

	void TransitionResource(ColorBuffer& colorBuffer, ResourceState newState, bool bFlushImmediate = false) final
	{
		m_state.TransitionResource(colorBuffer, newState, bFlushImmediate);
	}

private:
	ContextState m_state;
};


class __declspec(uuid("7038C5F8-60C1-48F5-881A-3FF19982AB4D")) GraphicsContext
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IGraphicsContext, IComputeContext, ICommandContext>>
	, NonCopyable
{
	friend class DeviceManager;

public:
	GraphicsContext();
	virtual ~GraphicsContext() = default;

	// ICommandContext implementation
	void SetId(const std::string& id) final { m_state.id = id; }
	CommandListType GetType() const final { return m_state.type; }

	// Debug events and markers
	void BeginEvent(const std::string& label) final { m_state.BeginEvent(label); }
	void EndEvent() final { m_state.EndEvent(); }
	void SetMarker(const std::string& label) final { m_state.SetMarker(label); }

	void Reset() final { m_state.Reset(); }
	void Initialize() final { m_state.Initialize(); }

	void Begin(const std::string& id) final { m_state.Begin(id); }
	uint64_t Finish(bool bWaitForCompletion = false) final;

	void TransitionResource(ColorBuffer& colorBuffer, ResourceState newState, bool bFlushImmediate = false) final
	{
		m_state.TransitionResource(colorBuffer, newState, bFlushImmediate);
	}

	void ClearColor(ColorBuffer& colorBuffer) final { m_state.ClearColor(colorBuffer); }
	void ClearColor(ColorBuffer& colorBuffer, Color clearColor) final { m_state.ClearColor(colorBuffer, clearColor); }

private:
	ContextState m_state;
};

} // namespace Luna::VK