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

#include "CommandContextVK.h"

#include "Graphics\Vulkan\ColorBufferVK.h"
#include "Graphics\Vulkan\DepthBufferVK.h"
#include "Graphics\Vulkan\DeviceManagerVK.h"
#include "Graphics\Vulkan\QueueVK.h"

using namespace std;


namespace Luna::VK
{

template <class T>
inline VkImage GetImage(const T& obj)
{
	const auto platformData = obj.GetPlatformData();
	wil::com_ptr<IGpuImageData> gpuImageData;
	assert_succeeded(platformData->QueryInterface(IID_PPV_ARGS(&gpuImageData)));
	return gpuImageData->GetImage();
}


void CommandContextVK::BeginEvent(const string& label)
{
#if ENABLE_VULKAN_DEBUG_MARKERS
	VkDebugUtilsLabelEXT labelInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
	labelInfo.color[0] = 0.0f;
	labelInfo.color[1] = 0.0f;
	labelInfo.color[2] = 0.0f;
	labelInfo.color[3] = 0.0f;
	labelInfo.pLabelName = label.c_str();
	vkCmdBeginDebugUtilsLabelEXT(m_commandBuffer, &labelInfo);
#endif
}


void CommandContextVK::EndEvent()
{
#if ENABLE_VULKAN_DEBUG_MARKERS
	vkCmdEndDebugUtilsLabelEXT(m_commandBuffer);
#endif
}


void CommandContextVK::SetMarker(const string& label)
{
#if ENABLE_VULKAN_DEBUG_MARKERS
	VkDebugUtilsLabelEXT labelInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
	labelInfo.color[0] = 0.0f;
	labelInfo.color[1] = 0.0f;
	labelInfo.color[2] = 0.0f;
	labelInfo.color[3] = 0.0f;
	labelInfo.pLabelName = label.c_str();
	vkCmdInsertDebugUtilsLabelEXT(m_commandBuffer, &labelInfo);
#endif
}


// TODO - Move this logic into CommandManager::AllocateContext
void CommandContextVK::Reset()
{
	assert(m_commandBuffer == VK_NULL_HANDLE);
	m_commandBuffer = GetVulkanDeviceManager()->GetQueue(m_type).RequestCommandBuffer();
}


// TODO - Move this logic into CommandManager::AllocateContext
void CommandContextVK::Initialize()
{
	assert(m_commandBuffer == VK_NULL_HANDLE);
	m_commandBuffer = GetVulkanDeviceManager()->GetQueue(m_type).RequestCommandBuffer();
}


void CommandContextVK::BeginFrame()
{
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.pNext = nullptr;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
}


uint64_t CommandContextVK::Finish(bool bWaitForCompletion)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);

	FlushResourceBarriers();

	// TODO
#if 0
	if (m_ID.length() > 0)
	{
		EngineProfiling::EndBlock(this);
	}
#endif

#if ENABLE_VULKAN_DEBUG_MARKERS
	if (m_hasPendingDebugEvent)
	{
		EndEvent();
		m_hasPendingDebugEvent = false;
	}
#endif

	vkEndCommandBuffer(m_commandBuffer);

	auto deviceManager = GetVulkanDeviceManager();

	auto& queue = deviceManager->GetQueue(m_type);

	uint64_t fenceValue = queue.ExecuteCommandList(m_commandBuffer);
	queue.DiscardCommandBuffer(fenceValue, m_commandBuffer);
	m_commandBuffer = VK_NULL_HANDLE;

	// Recycle dynamic allocations
	//m_cpuLinearAllocator.CleanupUsedPages(fenceValue);
	//m_dynamicDescriptorPool.CleanupUsedPools(fenceValue);

	if (bWaitForCompletion)
	{
		queue.WaitForFence(fenceValue);
	}

	return fenceValue;
}


void CommandContextVK::TransitionResource(ColorBuffer& colorBuffer, ResourceState newState, bool bFlushImmediate)
{
	TextureBarrier barrier{};
	barrier.image = GetImage(colorBuffer);
	barrier.format = FormatToVulkan(colorBuffer.GetFormat());
	barrier.imageAspect = GetImageAspect(colorBuffer.GetFormat());
	barrier.beforeState = colorBuffer.GetUsageState();
	barrier.afterState = newState;
	barrier.numMips = colorBuffer.GetNumMips();
	barrier.mipLevel = 0;
	barrier.arraySizeOrDepth = colorBuffer.GetArraySize();
	barrier.arraySlice = 0;
	barrier.bWholeTexture = true;

	m_textureBarriers.push_back(barrier);

	// TODO: we should probably do this after the barriers are flushed
	colorBuffer.SetUsageState(newState);

	if (bFlushImmediate || GetPendingBarrierCount() >= 16)
	{
		FlushResourceBarriers();
	}
}


void CommandContextVK::TransitionResource(DepthBuffer& depthBuffer, ResourceState newState, bool bFlushImmediate)
{
	TextureBarrier barrier{};
	barrier.image = GetImage(depthBuffer);
	barrier.format = FormatToVulkan(depthBuffer.GetFormat());
	barrier.imageAspect = GetImageAspect(depthBuffer.GetFormat());
	barrier.beforeState = depthBuffer.GetUsageState();
	barrier.afterState = newState;
	barrier.numMips = depthBuffer.GetNumMips();
	barrier.mipLevel = 0;
	barrier.arraySizeOrDepth = depthBuffer.GetArraySize();
	barrier.arraySlice = 0;
	barrier.bWholeTexture = true;

	m_textureBarriers.push_back(barrier);

	// TODO: we should probably do this after the barriers are flushed
	depthBuffer.SetUsageState(newState);

	if (bFlushImmediate || GetPendingBarrierCount() >= 16)
	{
		FlushResourceBarriers();
	}
}


void CommandContextVK::InsertUAVBarrier(ColorBuffer& colorBuffer, bool bFlushImmediate)
{
	assert_msg(HasFlag(colorBuffer.GetUsageState(), ResourceState::UnorderedAccess), "Resource must be in UnorderedAccess state to insert a UAV barrier");

	TransitionResource(colorBuffer, colorBuffer.GetUsageState(), bFlushImmediate);
}


void CommandContextVK::FlushResourceBarriers()
{
	for (const auto& barrier : m_textureBarriers)
	{
		ResourceStateMapping before = GetResourceStateMapping(barrier.beforeState);
		ResourceStateMapping after = GetResourceStateMapping(barrier.afterState);

		assert(after.imageLayout != VK_IMAGE_LAYOUT_UNDEFINED);

		VkImageSubresourceRange subresourceRange{};
		subresourceRange.aspectMask = barrier.imageAspect;
		subresourceRange.baseArrayLayer = barrier.arraySlice;
		subresourceRange.baseMipLevel = barrier.mipLevel;
		subresourceRange.layerCount = barrier.arraySizeOrDepth;
		subresourceRange.levelCount = barrier.numMips;

		VkImageMemoryBarrier2 vkBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		vkBarrier.srcAccessMask = before.accessFlags;
		vkBarrier.dstAccessMask = after.accessFlags;
		vkBarrier.srcStageMask = before.stageFlags;
		vkBarrier.dstStageMask = after.stageFlags;
		vkBarrier.oldLayout = before.imageLayout;
		vkBarrier.newLayout = after.imageLayout;
		vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkBarrier.image = barrier.image;
		vkBarrier.subresourceRange = subresourceRange;

		m_imageMemoryBarriers.push_back(vkBarrier);
	}

	if (!m_imageMemoryBarriers.empty())
	{
		VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
		dependencyInfo.imageMemoryBarrierCount = (uint32_t)m_imageMemoryBarriers.size();
		dependencyInfo.pImageMemoryBarriers = m_imageMemoryBarriers.data();

		vkCmdPipelineBarrier2(m_commandBuffer, &dependencyInfo);

		m_imageMemoryBarriers.clear();
	}

	m_textureBarriers.clear();

	// TODO - Vulkan GPU buffer support
}


void CommandContextVK::ClearColor(ColorBuffer& colorBuffer)
{
	ClearColor(colorBuffer, colorBuffer.GetClearColor());
}


void CommandContextVK::ClearColor(ColorBuffer& colorBuffer, Color clearColor)
{
	ResourceState oldState = colorBuffer.GetUsageState();

	TransitionResource(colorBuffer, ResourceState::CopyDest, false);

	VkClearColorValue colVal;
	colVal.float32[0] = clearColor.R();
	colVal.float32[1] = clearColor.G();
	colVal.float32[2] = clearColor.B();
	colVal.float32[3] = clearColor.A();

	VkImageSubresourceRange range{
		.aspectMask			= VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel		= 0,
		.levelCount			= colorBuffer.GetNumMips(),
		.baseArrayLayer		= 0,
		.layerCount			= colorBuffer.GetArraySize()
	};

	FlushResourceBarriers();

	vkCmdClearColorImage(m_commandBuffer, GetImage(colorBuffer), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &colVal, 1, &range);

	TransitionResource(colorBuffer, oldState, false);
}


void CommandContextVK::ClearDepth(DepthBuffer& depthBuffer)
{
	ClearDepthAndStencil_Internal(depthBuffer, VK_IMAGE_ASPECT_DEPTH_BIT);
}


void CommandContextVK::ClearStencil(DepthBuffer& depthBuffer)
{
	ClearDepthAndStencil_Internal(depthBuffer, VK_IMAGE_ASPECT_STENCIL_BIT);
}


void CommandContextVK::ClearDepthAndStencil(DepthBuffer& depthBuffer)
{
	ClearDepthAndStencil_Internal(depthBuffer, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
}


void CommandContextVK::ClearDepthAndStencil_Internal(DepthBuffer& depthBuffer, VkImageAspectFlags flags)
{
	ResourceState oldState = depthBuffer.GetUsageState();

	TransitionResource(depthBuffer, ResourceState::CopyDest, false);

	VkClearDepthStencilValue depthVal{
		.depth		= depthBuffer.GetClearDepth(),
		.stencil	= depthBuffer.GetClearStencil()
	};

	VkImageSubresourceRange range{
		.aspectMask			= flags,
		.baseMipLevel		= 0,
		.levelCount			= depthBuffer.GetNumMips(),
		.baseArrayLayer		= 0,
		.layerCount			= depthBuffer.GetArraySize()
	};

	FlushResourceBarriers();
	vkCmdClearDepthStencilImage(m_commandBuffer, GetImage(depthBuffer), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &depthVal, 1, &range);

	TransitionResource(depthBuffer, oldState, false);
}

} // namespace Luna::VK