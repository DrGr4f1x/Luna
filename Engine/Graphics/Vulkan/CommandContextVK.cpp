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

#include "Graphics\GpuImage.h"
#include "Graphics\PixelBuffer.h"
#include "Graphics\Vulkan\ColorBufferVK.h"
#include "Graphics\Vulkan\DeviceManagerVK.h"
#include "Graphics\Vulkan\QueueVK.h"

using namespace std;


namespace Luna::VK
{

ContextState::~ContextState() = default;


void ContextState::BeginEvent(const string& label)
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


void ContextState::EndEvent()
{
#if ENABLE_VULKAN_DEBUG_MARKERS
	vkCmdEndDebugUtilsLabelEXT(m_commandBuffer);
#endif
}


void ContextState::SetMarker(const string& label)
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


void ContextState::Reset()
{
	assert(m_commandBuffer == VK_NULL_HANDLE);
	m_commandBuffer = GetVulkanDeviceManager()->GetQueue(type).RequestCommandBuffer();
}


void ContextState::Initialize()
{
	assert(m_commandBuffer == VK_NULL_HANDLE);
	m_commandBuffer = GetVulkanDeviceManager()->GetQueue(type).RequestCommandBuffer();
}


void ContextState::Begin(const string& id)
{
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.pNext = nullptr;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
}


uint64_t ContextState::Finish(bool bWaitForCompletion)
{
	assert(type == CommandListType::Direct || type == CommandListType::Compute);

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

	auto& queue = deviceManager->GetQueue(type);

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


void ContextState::TransitionResource(IGpuImage* gpuImage, ResourceState newState, bool bFlushImmediate)
{
	wil::com_ptr<IPixelBuffer> pixelBuffer;
	gpuImage->QueryInterface(IID_PPV_ARGS(&pixelBuffer));

	TextureBarrier barrier{};
	barrier.image = gpuImage->GetNativeObject(NativeObjectType::VK_Image);
	barrier.format = FormatToVulkan(pixelBuffer->GetFormat());
	barrier.imageAspect = GetImageAspect(pixelBuffer->GetFormat());
	barrier.beforeState = gpuImage->GetUsageState();
	barrier.afterState = newState;
	barrier.numMips = pixelBuffer->GetNumMips();
	barrier.mipLevel = 0;
	barrier.arraySizeOrDepth = pixelBuffer->GetArraySize();
	barrier.arraySlice = 0;
	barrier.bWholeTexture = true;

	m_textureBarriers.push_back(barrier);

	gpuImage->SetUsageState(newState);

	if (bFlushImmediate || GetPendingBarrierCount() >= 16)
	{
		FlushResourceBarriers();
	}
}


void ContextState::InsertUAVBarrier(IGpuImage* gpuImage, bool bFlushImmediate)
{
	assert_msg(HasFlag(gpuImage->GetUsageState(), ResourceState::UnorderedAccess), "Resource must be in UnorderedAccess state to insert a UAV barrier");

	TransitionResource(gpuImage, gpuImage->GetUsageState(), bFlushImmediate);
}


void ContextState::FlushResourceBarriers()
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


void ContextState::ClearColor(IColorBuffer* colorBuffer)
{
	ResourceState oldState = colorBuffer->GetUsageState();

	TransitionResource(colorBuffer, ResourceState::CopyDest, false);

	VkClearColorValue colVal;
	Color clearColor = colorBuffer->GetClearColor();
	colVal.float32[0] = clearColor.R();
	colVal.float32[1] = clearColor.G();
	colVal.float32[2] = clearColor.B();
	colVal.float32[3] = clearColor.A();

	VkImageSubresourceRange range;
	range.baseArrayLayer = 0;
	range.baseMipLevel = 0;
	range.layerCount = colorBuffer->GetArraySize();
	range.levelCount = colorBuffer->GetNumMips();
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	FlushResourceBarriers();
	VkImage image = colorBuffer->GetNativeObject(NativeObjectType::VK_Image);
	vkCmdClearColorImage(m_commandBuffer, image, GetImageLayout(colorBuffer->GetUsageState()), &colVal, 1, &range);

	TransitionResource(colorBuffer, oldState, false);
}


void ContextState::ClearColor(IColorBuffer* colorBuffer, Color clearColor)
{
	ResourceState oldState = colorBuffer->GetUsageState();

	TransitionResource(colorBuffer, ResourceState::CopyDest, false);

	VkClearColorValue colVal;
	colVal.float32[0] = clearColor.R();
	colVal.float32[1] = clearColor.G();
	colVal.float32[2] = clearColor.B();
	colVal.float32[3] = clearColor.A();

	VkImageSubresourceRange range;
	range.baseArrayLayer = 0;
	range.baseMipLevel = 0;
	range.layerCount = colorBuffer->GetArraySize();
	range.levelCount = colorBuffer->GetNumMips();
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	FlushResourceBarriers();
	VkImage image = colorBuffer->GetNativeObject(NativeObjectType::VK_Image);
	vkCmdClearColorImage(m_commandBuffer, image, GetImageLayout(colorBuffer->GetUsageState()), &colVal, 1, &range);

	TransitionResource(colorBuffer, oldState, false);
}


ComputeContext::ComputeContext()
{
	m_state.type = CommandListType::Compute;
}


uint64_t ComputeContext::Finish(bool bWaitForCompletion)
{
	uint64_t fenceValue = m_state.Finish(bWaitForCompletion);

	GetVulkanDeviceManager()->FreeContext(this);

	return fenceValue;
}


GraphicsContext::GraphicsContext()
{
	m_state.type = CommandListType::Direct;
}


uint64_t GraphicsContext::Finish(bool bWaitForCompletion)
{
	uint64_t fenceValue = m_state.Finish(bWaitForCompletion);

	GetVulkanDeviceManager()->FreeContext(this);

	return fenceValue;
}

} // namespace Luna::VK