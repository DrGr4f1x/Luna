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
#include "Graphics\Vulkan\DeviceVK.h"
#include "Graphics\Vulkan\DeviceManagerVK.h"
#include "Graphics\Vulkan\GpuBufferVK.h"
#include "Graphics\Vulkan\QueueVK.h"

using namespace std;


namespace Luna::VK
{

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


void CommandContextVK::TransitionResource(IGpuResource* gpuResource, ResourceState newState, bool bFlushImmediate)
{
	auto resourceType = gpuResource->GetResourceType();
	
	bool didInsertBarrier = false;

	if (IsTextureResource(resourceType))
	{
		PixelBufferHandle pixelBuffer;
		gpuResource->QueryInterface(IID_PPV_ARGS(&pixelBuffer));
		assert(pixelBuffer);

		TextureBarrier barrier{
			.image				= gpuResource->GetNativeObject(NativeObjectType::VK_Image),
			.format				= FormatToVulkan(pixelBuffer->GetFormat()),
			.imageAspect		= GetImageAspect(pixelBuffer->GetFormat()),
			.beforeState		= gpuResource->GetUsageState(),
			.afterState			= newState,
			.numMips			= pixelBuffer->GetNumMips(),
			.mipLevel			= 0,
			.arraySizeOrDepth	= pixelBuffer->GetArraySize(),
			.arraySlice			= 0,
			.bWholeTexture		= true
		};

		m_textureBarriers.push_back(barrier);

		didInsertBarrier = true;
	}
	else if (IsBufferResource(resourceType))
	{
		GpuBufferHandle gpuBuffer;
		gpuResource->QueryInterface(IID_PPV_ARGS(&gpuBuffer));
		assert(gpuBuffer);

		BufferBarrier barrier{
			.buffer			= gpuResource->GetNativeObject(NativeObjectType::VK_Buffer),
			.beforeState	= gpuResource->GetUsageState(),
			.afterState		= newState,
			.size			= gpuBuffer->GetSize()
		};

		m_bufferBarriers.push_back(barrier);
	}
	
	if (didInsertBarrier)
	{
		gpuResource->SetUsageState(newState);

		if (bFlushImmediate || GetPendingBarrierCount() >= 16)
		{
			FlushResourceBarriers();
		}
	}
}


void CommandContextVK::InsertUAVBarrier(IGpuResource* gpuResource, bool bFlushImmediate)
{
	assert_msg(HasFlag(gpuResource->GetUsageState(), ResourceState::UnorderedAccess), "Resource must be in UnorderedAccess state to insert a UAV barrier");

	TransitionResource(gpuResource, gpuResource->GetUsageState(), bFlushImmediate);
}


void CommandContextVK::FlushResourceBarriers()
{
	for (const auto& barrier : m_textureBarriers)
	{
		VkImageSubresourceRange subresourceRange{};
		subresourceRange.aspectMask = barrier.imageAspect;
		subresourceRange.baseArrayLayer = barrier.arraySlice;
		subresourceRange.baseMipLevel = barrier.mipLevel;
		subresourceRange.layerCount = barrier.arraySizeOrDepth;
		subresourceRange.levelCount = barrier.numMips;

		VkImageMemoryBarrier2 vkBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		vkBarrier.srcAccessMask = GetAccessMask(barrier.beforeState);
		vkBarrier.dstAccessMask = GetAccessMask(barrier.afterState);
		vkBarrier.srcStageMask = GetPipelineStage(barrier.beforeState);
		vkBarrier.dstStageMask = GetPipelineStage(barrier.afterState);
		vkBarrier.oldLayout = GetImageLayout(barrier.beforeState);
		vkBarrier.newLayout = GetImageLayout(barrier.afterState);
		vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkBarrier.image = barrier.image;
		vkBarrier.subresourceRange = subresourceRange;

		m_imageMemoryBarriers.push_back(vkBarrier);
	}	

	for (const auto& barrier : m_bufferBarriers)
	{
		VkBufferMemoryBarrier2 vkBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
		vkBarrier.srcAccessMask = GetAccessMask(barrier.beforeState);
		vkBarrier.dstAccessMask = GetAccessMask(barrier.afterState);
		vkBarrier.srcStageMask = GetPipelineStage(barrier.beforeState);
		vkBarrier.dstStageMask = GetPipelineStage(barrier.afterState);
		vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkBarrier.buffer = barrier.buffer;
		vkBarrier.offset = 0;
		vkBarrier.size = barrier.size;

		m_bufferMemoryBarriers.push_back(vkBarrier);
	}

	if (!m_imageMemoryBarriers.empty() || !m_bufferMemoryBarriers.empty())
	{
		VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
		dependencyInfo.imageMemoryBarrierCount = (uint32_t)m_imageMemoryBarriers.size();
		dependencyInfo.pImageMemoryBarriers = m_imageMemoryBarriers.data();
		dependencyInfo.bufferMemoryBarrierCount = (uint32_t)m_bufferMemoryBarriers.size();
		dependencyInfo.pBufferMemoryBarriers = m_bufferMemoryBarriers.data();

		vkCmdPipelineBarrier2(m_commandBuffer, &dependencyInfo);

		m_imageMemoryBarriers.clear();
		m_bufferMemoryBarriers.clear();
	}

	m_textureBarriers.clear();
	m_bufferBarriers.clear();
}


void CommandContextVK::ClearColor(IColorBuffer* colorBuffer)
{
	ClearColor(colorBuffer, colorBuffer->GetClearColor());
}


void CommandContextVK::ClearColor(IColorBuffer* colorBuffer, Color clearColor)
{
	ResourceState oldState = colorBuffer->GetUsageState();

	TransitionResource(colorBuffer, ResourceState::CopyDest, false);

	VkClearColorValue colVal;
	colVal.float32[0] = clearColor.R();
	colVal.float32[1] = clearColor.G();
	colVal.float32[2] = clearColor.B();
	colVal.float32[3] = clearColor.A();

	VkImageSubresourceRange range{
		.aspectMask			= VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel		= 0,
		.levelCount			= colorBuffer->GetNumMips(),
		.baseArrayLayer		= 0,
		.layerCount			= colorBuffer->GetArraySize()
	};

	FlushResourceBarriers();

	vkCmdClearColorImage(m_commandBuffer, colorBuffer->GetNativeObject(NativeObjectType::VK_Image), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &colVal, 1, &range);

	TransitionResource(colorBuffer, oldState, false);
}


void CommandContextVK::ClearDepth(IDepthBuffer* depthBuffer)
{
	ClearDepthAndStencil_Internal(depthBuffer, VK_IMAGE_ASPECT_DEPTH_BIT);
}


void CommandContextVK::ClearStencil(IDepthBuffer* depthBuffer)
{
	ClearDepthAndStencil_Internal(depthBuffer, VK_IMAGE_ASPECT_STENCIL_BIT);
}


void CommandContextVK::ClearDepthAndStencil(IDepthBuffer* depthBuffer)
{
	ClearDepthAndStencil_Internal(depthBuffer, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
}


void CommandContextVK::ClearDepthAndStencil_Internal(IDepthBuffer* depthBuffer, VkImageAspectFlags flags)
{
	ResourceState oldState = depthBuffer->GetUsageState();

	TransitionResource(depthBuffer, ResourceState::CopyDest, false);

	VkClearDepthStencilValue depthVal{
		.depth		= depthBuffer->GetClearDepth(),
		.stencil	= depthBuffer->GetClearStencil()
	};

	VkImageSubresourceRange range{
		.aspectMask			= flags,
		.baseMipLevel		= 0,
		.levelCount			= depthBuffer->GetNumMips(),
		.baseArrayLayer		= 0,
		.layerCount			= depthBuffer->GetArraySize()
	};

	FlushResourceBarriers();
	vkCmdClearDepthStencilImage(m_commandBuffer, depthBuffer->GetNativeObject(NativeObjectType::VK_Image), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &depthVal, 1, &range);

	TransitionResource(depthBuffer, oldState, false);
}


void CommandContextVK::BeginRendering(IColorBuffer* renderTarget)
{
	// TODO
}


void CommandContextVK::BeginRendering(IColorBuffer* renderTarget, IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect)
{
	// TODO
}


void CommandContextVK::BeginRendering(IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect)
{
	// TODO
}


void CommandContextVK::BeginRendering(std::span<IColorBuffer*> renderTargets)
{
	// TODO
}


void CommandContextVK::BeginRendering(std::span<IColorBuffer*> renderTargets, IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect)
{
	// TODO
}

void CommandContextVK::EndRendering()
{
	// TODO
}


void CommandContextVK::InitializeBuffer_Internal(IGpuBuffer* destBuffer, const void* bufferData, size_t numBytes, size_t offset)
{
	auto stagingBuffer = GetVulkanGraphicsDevice()->CreateStagingBuffer(bufferData, numBytes);

	// Copy from the upload buffer to the destination buffer
	TransitionResource(destBuffer, ResourceState::CopyDest, true);

	VkBufferCopy copyRegion{ .size = numBytes };
	vkCmdCopyBuffer(m_commandBuffer, *stagingBuffer, destBuffer->GetNativeObject(NativeObjectType::VK_Buffer), 1, &copyRegion);

	TransitionResource(destBuffer, ResourceState::GenericRead, true);

	GetVulkanDeviceManager()->ReleaseBuffer(stagingBuffer.get());
}

} // namespace Luna::VK