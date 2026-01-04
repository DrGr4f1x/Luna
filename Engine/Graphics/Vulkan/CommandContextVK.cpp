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

#include "Graphics\PipelineState.h"
#include "Graphics\ResourceSet.h"

#include "ColorBufferVK.h"
#include "DepthBufferVK.h"
#include "DescriptorSetVK.h"
#include "DeviceManagerVK.h"
#include "GpuBufferVK.h"
#include "QueryHeapVK.h"
#include "QueueVK.h"
#include "PipelineStateVK.h"
#include "RootSignatureVK.h"
#include "TextureVK.h"
#include "VulkanUtil.h"

#if ENABLE_VULKAN_DEBUG_MARKERS
#include <pix3.h>
#endif

using namespace std;


namespace Luna::VK
{

VkClearValue GetColorClearValue(Color color)
{
	VkClearValue clearValue{};
	clearValue.color.float32[0] = color[0];
	clearValue.color.float32[1] = color[1];
	clearValue.color.float32[2] = color[2];
	clearValue.color.float32[3] = color[3];
	return clearValue;
}


VkClearValue GetDepthStencilClearValue(float depth, uint32_t stencil)
{
	VkClearValue clearValue{};
	clearValue.depthStencil.depth = depth;
	clearValue.depthStencil.stencil = stencil;
	return clearValue;
}


VkRenderingAttachmentInfo GetRenderingAttachmentInfo(const ColorBuffer& renderTarget)
{
	VkImageView imageView = ((const Descriptor*)renderTarget.GetRtvDescriptor())->GetImageView();

	VkRenderingAttachmentInfo info{
		.sType			= VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView		= imageView,
		.imageLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.resolveMode	= VK_RESOLVE_MODE_NONE,
		.loadOp			= VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp		= VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue		= GetColorClearValue(renderTarget.GetClearColor())
	};

	return info;
}


VkRenderingAttachmentInfo GetRenderingAttachmentInfo(const DepthBuffer& depthTarget, DepthStencilAspect depthStencilAspect)
{
	VkImageView imageView = ((const Descriptor*)depthTarget.GetDsvDescriptor(depthStencilAspect))->GetImageView();

	VkImageLayout imageLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
	switch (depthStencilAspect)
	{
	case DepthStencilAspect::ReadWrite:
		imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		break;

	case DepthStencilAspect::ReadOnly:
		imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		break;

	case DepthStencilAspect::DepthReadOnly:
		imageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
		break;

	case DepthStencilAspect::StencilReadOnly:
		imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
		break;
	}

	VkRenderingAttachmentInfo info{
		.sType			= VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView		= imageView,
		.imageLayout	= imageLayout,
		.resolveMode	= VK_RESOLVE_MODE_NONE,
		.loadOp			= VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp		= VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue		= GetDepthStencilClearValue(depthTarget.GetClearDepth(), depthTarget.GetClearStencil())
	};

	return info;
}


VkBufferImageCopy TextureSubResourceDataToVulkan(const TextureSubresourceData& data, VkImageAspectFlags aspectMask)
{
	VkBufferImageCopy bufferImageCopy{
		.bufferOffset = data.bufferOffset,
		.imageSubresource = {
			.aspectMask			= aspectMask,
			.mipLevel			= data.mipLevel,
			.baseArrayLayer		= data.baseArrayLayer,
			.layerCount			= data.layerCount },
		.imageExtent = {
			.width = data.width,
			.height = data.height,
			.depth = data.depth }
	};

	return bufferImageCopy;
}


CommandContextVK::CommandContextVK(CVkDevice* device, CommandListType type)
	: m_device{ device }
	, m_commandListType{ type }
#if USE_DESCRIPTOR_BUFFERS
	, m_dynamicResourceDescriptorBuffer{ *this, DescriptorBufferType::Resource }
	, m_dynamicSamplerDescriptorBuffer{ *this, DescriptorBufferType::Sampler }
#endif // USE_DESCRIPTOR_BUFFERS
{
#if USE_DESCRIPTOR_BUFFERS
	ZeroMemory(m_currentDescriptorBuffers, sizeof(m_currentDescriptorBuffers));
#endif // USE_DESCRIPTOR_BUFFERS
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

	PIXBeginEvent(0, label.c_str());
#endif
}


void CommandContextVK::EndEvent()
{
#if ENABLE_VULKAN_DEBUG_MARKERS
	vkCmdEndDebugUtilsLabelEXT(m_commandBuffer);

	PIXEndEvent();
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

	PIXSetMarker(0, label.c_str());
#endif
}


// TODO - Move this logic into CommandManager::AllocateContext
void CommandContextVK::Reset()
{
	assert(m_commandBuffer == VK_NULL_HANDLE);
	m_commandBuffer = GetVulkanDeviceManager()->GetQueue(m_commandListType).RequestCommandBuffer();

	m_graphicsPipelineLayout = VK_NULL_HANDLE;
	m_computePipelineLayout = VK_NULL_HANDLE;
	m_graphicsPipeline = VK_NULL_HANDLE;
	m_computePipeline = VK_NULL_HANDLE;
	m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;

	m_graphicsRootSignature = nullptr;
	m_computeRootSignature = nullptr;
	m_isGraphicsRootSignatureParsed = false;
	m_isComputeRootSignatureParsed = false;
	m_hasDirtyGraphicsDescriptors = false;
	m_hasDirtyComputeDescriptors = false;

	m_renderingArea.offset.x = -1;
	m_renderingArea.offset.y = -1;
	m_renderingArea.extent.width = 0;
	m_renderingArea.extent.height = 0;

#if USE_DESCRIPTOR_BUFFERS
	ZeroMemory(m_currentDescriptorBuffers, sizeof(m_currentDescriptorBuffers));
#endif // USE_DESCRIPTOR_BUFFERS
}


// TODO - Move this logic into CommandManager::AllocateContext
void CommandContextVK::Initialize()
{
	assert(m_commandBuffer == VK_NULL_HANDLE);
	m_commandBuffer = GetVulkanDeviceManager()->GetQueue(m_commandListType).RequestCommandBuffer();

#if USE_LEGACY_DESCRIPTOR_SETS
	m_dynamicDescriptorHeap = make_unique<DefaultDynamicDescriptorHeap>(m_device.get());
#endif // USE_LEGACY_DESCRIPTOR_SETS
}


void CommandContextVK::BeginFrame()
{
	VkCommandBufferBeginInfo beginInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
}


uint64_t CommandContextVK::Finish(bool bWaitForCompletion)
{
	assert(m_commandListType == CommandListType::Graphics || m_commandListType == CommandListType::Compute);

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

	auto& queue = deviceManager->GetQueue(m_commandListType);

	uint64_t fenceValue = queue.ExecuteCommandList(m_commandBuffer);
	queue.DiscardCommandBuffer(fenceValue, m_commandBuffer);
	m_commandBuffer = VK_NULL_HANDLE;

	// Recycle dynamic allocations
#if USE_DESCRIPTOR_BUFFERS
	m_dynamicResourceDescriptorBuffer.CleanupUsedBuffers(fenceValue);
	m_dynamicSamplerDescriptorBuffer.CleanupUsedBuffers(fenceValue);
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	m_dynamicDescriptorHeap->CleanupUsedPools(fenceValue);
#endif // USE_LEGACY_DESCRIPTOR_SETS

	m_cpuLinearAllocator.CleanupUsedPages(fenceValue);

	if (bWaitForCompletion)
	{
		queue.WaitForFence(fenceValue);
	}

	return fenceValue;
}


void CommandContextVK::TransitionResource(IColorBuffer* colorBuffer, ResourceState newState, bool bFlushImmediate)
{
	BeginEvent("TransitionResource");

	// TODO: Try this with GetPlatformObject()
	ColorBuffer* colorBufferVK = (ColorBuffer*)colorBuffer;
	assert(colorBufferVK != nullptr);

	TextureBarrier barrier{
		.image				= colorBufferVK->GetImage(),
		.format				= FormatToVulkan(colorBuffer->GetFormat()),
		.imageAspect		= GetImageAspect(colorBuffer->GetFormat()),
		.beforeState		= colorBuffer->GetUsageState(),
		.afterState			= newState,
		.numMips			= colorBuffer->GetNumMips(),
		.mipLevel			= 0,
		.arraySizeOrDepth	= colorBuffer->GetArraySize(),
		.arraySlice			= 0,
		.bWholeTexture		= true
	};

	m_textureBarriers.push_back(barrier);

	colorBuffer->SetUsageState(newState);

	if (bFlushImmediate || GetPendingBarrierCount() >= 16)
	{
		FlushResourceBarriers();
	}

	EndEvent();
}


void CommandContextVK::TransitionResource(IDepthBuffer* depthBuffer, ResourceState newState, bool bFlushImmediate)
{
	BeginEvent("TransitionResource");

	// TODO: Try this with GetPlatformObject()
	DepthBuffer* depthBufferVK = (DepthBuffer*)depthBuffer;
	assert(depthBufferVK != nullptr);

	TextureBarrier barrier{
		.image				= depthBufferVK->GetImage(),
		.format				= FormatToVulkan(depthBuffer->GetFormat()),
		.imageAspect		= GetImageAspect(depthBuffer->GetFormat()),
		.beforeState		= depthBuffer->GetUsageState(),
		.afterState			= newState,
		.numMips			= depthBuffer->GetNumMips(),
		.mipLevel			= 0,
		.arraySizeOrDepth	= depthBuffer->GetArraySize(),
		.arraySlice			= 0,
		.bWholeTexture		= true
	};

	m_textureBarriers.push_back(barrier);

	depthBuffer->SetUsageState(newState);

	if (bFlushImmediate || GetPendingBarrierCount() >= 16)
	{
		FlushResourceBarriers();
	}

	EndEvent();
}


void CommandContextVK::TransitionResource(IGpuBuffer* gpuBuffer, ResourceState newState, bool bFlushImmediate)
{
	BeginEvent("TransitionResource");

	// TODO: Try this with GetPlatformObject()
	GpuBuffer* gpuBufferVK = (GpuBuffer*)gpuBuffer;
	assert(gpuBufferVK != nullptr);

	BufferBarrier barrier{
		.buffer			= gpuBufferVK->GetBuffer(),
		.beforeState	= gpuBuffer->GetUsageState(),
		.afterState		= newState,
		.size			= gpuBuffer->GetBufferSize()
	};

	m_bufferBarriers.push_back(barrier);

	gpuBuffer->SetUsageState(newState);

	if (bFlushImmediate || GetPendingBarrierCount() >= 16)
	{
		FlushResourceBarriers();
	}

	EndEvent();
}


void CommandContextVK::TransitionResource(ITexture* texture, ResourceState newState, bool bFlushImmediate)
{
	BeginEvent("TransitionResource");

	// TODO: Try this with GetPlatformObject()
	Texture* textureVK = (Texture*)texture;
	assert(textureVK != nullptr);

	const bool isVolume = texture->GetDimension() == TextureDimension::Texture3D;
	const uint32_t arraySizeOrDepth = isVolume ? 1 : (texture->GetArraySize() * texture->GetNumFaces());

	TextureBarrier barrier{
		.image				= textureVK->GetImage(),
		.format				= FormatToVulkan(texture->GetFormat()),
		.imageAspect		= GetImageAspect(texture->GetFormat()),
		.beforeState		= texture->GetUsageState(),
		.afterState			= newState,
		.numMips			= texture->GetNumMips(),
		.mipLevel			= 0,
		.arraySizeOrDepth	= arraySizeOrDepth,
		.arraySlice			= 0,
		.bWholeTexture		= true
	};

	m_textureBarriers.push_back(barrier);

	texture->SetUsageState(newState);

	if (bFlushImmediate || GetPendingBarrierCount() >= 16)
	{
		FlushResourceBarriers();
	}

	EndEvent();
}


void CommandContextVK::InsertUAVBarrier(const IColorBuffer* colorBuffer, bool bFlushImmediate)
{
	// TODO: Try this with GetPlatformObject()
	const ColorBuffer* colorBufferVK = (const ColorBuffer*)colorBuffer;
	assert(colorBufferVK != nullptr);

	TextureBarrier barrier{
		.image				= colorBufferVK->GetImage(),
		.format				= FormatToVulkan(colorBuffer->GetFormat()),
		.imageAspect		= GetImageAspect(colorBuffer->GetFormat()),
		.beforeState		= colorBuffer->GetUsageState(),
		.afterState			= colorBuffer->GetUsageState(),
		.numMips			= colorBuffer->GetNumMips(),
		.mipLevel			= 0,
		.arraySizeOrDepth	= colorBuffer->GetArraySize(),
		.arraySlice			= 0,
		.bWholeTexture		= true,
		.bUAVBarrier		= true
	};

	m_textureBarriers.push_back(barrier);

	if (bFlushImmediate || GetPendingBarrierCount() >= 16)
	{
		FlushResourceBarriers();
	}
}


void CommandContextVK::InsertUAVBarrier(const IGpuBuffer* gpuBuffer, bool bFlushImmediate)
{
	// TODO: Try this with GetPlatformObject()
	const GpuBuffer* gpuBufferVK = (const GpuBuffer*)gpuBuffer;
	assert(gpuBufferVK != nullptr);

	BufferBarrier barrier{
		.buffer			= gpuBufferVK->GetBuffer(),
		.beforeState	= gpuBuffer->GetUsageState(),
		.afterState		= gpuBuffer->GetUsageState(),
		.size			= gpuBuffer->GetBufferSize(),
		.bUAVBarrier	= true
	};

	m_bufferBarriers.push_back(barrier);

	if (bFlushImmediate || GetPendingBarrierCount() >= 16)
	{
		FlushResourceBarriers();
	}
}


void CommandContextVK::FlushResourceBarriers()
{
	BeginEvent("FlushResourceBarriers");

	for (const auto& barrier : m_textureBarriers)
	{
		// TODO: use named initializers
		VkImageSubresourceRange subresourceRange{};
		subresourceRange.aspectMask = barrier.imageAspect;
		subresourceRange.baseArrayLayer = barrier.arraySlice;
		subresourceRange.baseMipLevel = barrier.mipLevel;
		subresourceRange.layerCount = barrier.bWholeTexture ? VK_REMAINING_ARRAY_LAYERS : barrier.arraySizeOrDepth;
		subresourceRange.levelCount = barrier.bWholeTexture ? VK_REMAINING_MIP_LEVELS : barrier.numMips;

		// TODO: use named initializers
		VkImageMemoryBarrier2 vkBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		vkBarrier.srcAccessMask = barrier.bUAVBarrier ? VK_ACCESS_2_SHADER_WRITE_BIT : GetAccessMask(barrier.beforeState);
		vkBarrier.dstAccessMask = barrier.bUAVBarrier ? VK_ACCESS_2_SHADER_READ_BIT : GetAccessMask(barrier.afterState);
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
		// TODO: use named initializers
		VkBufferMemoryBarrier2 vkBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
		vkBarrier.srcAccessMask = barrier.bUAVBarrier ? VK_ACCESS_2_SHADER_WRITE_BIT : GetAccessMask(barrier.beforeState);
		vkBarrier.dstAccessMask = barrier.bUAVBarrier ? VK_ACCESS_2_SHADER_READ_BIT : GetAccessMask(barrier.afterState);
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

	EndEvent();
}


DynAlloc CommandContextVK::ReserveUploadMemory(size_t sizeInBytes)
{
	return m_cpuLinearAllocator.Allocate(sizeInBytes);
}


void CommandContextVK::ClearUAV(IGpuBuffer* gpuBuffer)
{
	// TODO: Try this with GetPlatformObject()
	GpuBuffer* gpuBufferVK = (GpuBuffer*)gpuBuffer;
	assert(gpuBufferVK != nullptr);

	uint32_t data = 0;
	vkCmdFillBuffer(m_commandBuffer, gpuBufferVK->GetBuffer(), 0, VK_WHOLE_SIZE, data);
}


void CommandContextVK::ClearColor(IColorBuffer* colorBuffer)
{
	ClearColor(colorBuffer, colorBuffer->GetClearColor());
}


void CommandContextVK::ClearColor(IColorBuffer* colorBuffer, Color clearColor)
{
	ResourceState oldState = colorBuffer->GetUsageState();

	TransitionResource(colorBuffer, ResourceState::CopyDest, true);

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

	// TODO: Try this with GetPlatformObject()
	ColorBuffer* colorBufferVK = (ColorBuffer*)colorBuffer;
	assert(colorBufferVK != nullptr);

	vkCmdClearColorImage(m_commandBuffer, colorBufferVK->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &colVal, 1, &range);

	TransitionResource(colorBuffer, oldState, true);
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

	TransitionResource(depthBuffer, ResourceState::CopyDest, true);

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

	// TODO: Try this with GetPlatformObject()
	DepthBuffer* depthBufferVK = (DepthBuffer*)depthBuffer;
	assert(depthBufferVK != nullptr);

	vkCmdClearDepthStencilImage(m_commandBuffer, depthBufferVK->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &depthVal, 1, &range);

	TransitionResource(depthBuffer, oldState, true);
}


void CommandContextVK::BeginRendering(const IColorBuffer* colorBuffer)
{
	ResetRenderTargets();

	// TODO: Try this with GetPlatformObject()
	const ColorBuffer* colorBufferVK = (const ColorBuffer*)colorBuffer;
	assert(colorBufferVK != nullptr);

	m_rtvs[0] = GetRenderingAttachmentInfo(*colorBufferVK);
	m_numRtvs = 1;
	m_rtvFormats[0] = FormatToVulkan(colorBuffer->GetFormat());

	SetRenderingArea(*colorBufferVK);

	BeginRenderingBlock();
}


void CommandContextVK::BeginRendering(const IColorBuffer* colorBuffer, const IDepthBuffer* depthBuffer, DepthStencilAspect depthStencilAspect)
{
	ResetRenderTargets();

	// TODO: Try this with GetPlatformObject()
	const ColorBuffer* colorBufferVK = (const ColorBuffer*)colorBuffer;
	assert(colorBufferVK != nullptr);

	const DepthBuffer* depthBufferVK = (const DepthBuffer*)depthBuffer;
	assert(depthBufferVK != nullptr);

	m_rtvs[0] = GetRenderingAttachmentInfo(*colorBufferVK);
	m_numRtvs = 1;
	m_rtvFormats[0] = FormatToVulkan(colorBuffer->GetFormat());

	m_dsv = GetRenderingAttachmentInfo(*depthBufferVK, depthStencilAspect);
	m_hasDsv = true;
	m_dsvFormat = FormatToVulkan(depthBuffer->GetFormat());

	SetRenderingArea(*colorBufferVK);
	SetRenderingArea(*depthBufferVK);

	BeginRenderingBlock();
}


void CommandContextVK::BeginRendering(const IDepthBuffer* depthBuffer, DepthStencilAspect depthStencilAspect)
{
	ResetRenderTargets();

	// TODO: Try this with GetPlatformObject()
	const DepthBuffer* depthBufferVK = (const DepthBuffer*)depthBuffer;
	assert(depthBufferVK != nullptr);

	m_dsv = GetRenderingAttachmentInfo(*depthBufferVK, depthStencilAspect);
	m_hasDsv = true;
	m_dsvFormat = FormatToVulkan(depthBuffer->GetFormat());

	SetRenderingArea(*depthBufferVK);

	BeginRenderingBlock();
}


void CommandContextVK::BeginRendering(std::span<const IColorBuffer*> colorBuffers)
{
	ResetRenderTargets();
	assert(colorBuffers.size() <= 8);

	uint32_t i = 0;
	for (const IColorBuffer* colorBuffer : colorBuffers)
	{
		// TODO: Try this with GetPlatformObject()
		const ColorBuffer* colorBufferVK = (const ColorBuffer*)colorBuffer;
		assert(colorBufferVK != nullptr);

		m_rtvs[i] = GetRenderingAttachmentInfo(*colorBufferVK);
		m_rtvFormats[i] = FormatToVulkan(colorBuffer->GetFormat());

		SetRenderingArea(*colorBufferVK);

		++i;
	}
	m_numRtvs = (uint32_t)colorBuffers.size();

	BeginRenderingBlock();
}


void CommandContextVK::BeginRendering(std::span<const IColorBuffer*> colorBuffers, const IDepthBuffer* depthBuffer, DepthStencilAspect depthStencilAspect)
{
	ResetRenderTargets();
	assert(colorBuffers.size() <= 8);

	uint32_t i = 0;
	for (const IColorBuffer* colorBuffer : colorBuffers)
	{
		// TODO: Try this with GetPlatformObject()
		const ColorBuffer* colorBufferVK = (const ColorBuffer*)colorBuffer;
		assert(colorBufferVK != nullptr);

		m_rtvs[i] = GetRenderingAttachmentInfo(*colorBufferVK);
		m_rtvFormats[i] = FormatToVulkan(colorBuffer->GetFormat());

		SetRenderingArea(*colorBufferVK);

		++i;
	}
	m_numRtvs = (uint32_t)colorBuffers.size();

	// TODO: Try this with GetPlatformObject()
	const DepthBuffer* depthBufferVK = (const DepthBuffer*)depthBuffer;
	assert(depthBufferVK != nullptr);

	m_dsv = GetRenderingAttachmentInfo(*depthBufferVK, depthStencilAspect);
	m_hasDsv = true;
	m_dsvFormat = FormatToVulkan(depthBuffer->GetFormat());

	SetRenderingArea(*depthBufferVK);

	BeginRenderingBlock();
}


void CommandContextVK::EndRendering()
{
	assert(m_isRendering);

	vkCmdEndRendering(m_commandBuffer);

	m_isRendering = false;
}


void CommandContextVK::BeginQuery(const IQueryHeap* queryHeap, uint32_t heapIndex)
{
	const QueryHeap* queryHeapVK = (const QueryHeap*)queryHeap;
	assert(queryHeapVK != nullptr);

	vkCmdBeginQuery(m_commandBuffer, queryHeapVK->GetQueryPool(), heapIndex, VK_FLAGS_NONE);
}


void CommandContextVK::EndQuery(const IQueryHeap* queryHeap, uint32_t heapIndex)
{
	const QueryHeap* queryHeapVK = (const QueryHeap*)queryHeap;
	assert(queryHeapVK != nullptr);

	vkCmdEndQuery(m_commandBuffer, queryHeapVK->GetQueryPool(), heapIndex);
}


void CommandContextVK::ResolveQueries(const IQueryHeap* queryHeap, uint32_t startIndex, uint32_t numQueries, const IGpuBuffer* destBuffer, uint64_t destBufferOffset)
{
	const QueryHeap* queryHeapVK = (const QueryHeap*)queryHeap;
	assert(queryHeapVK != nullptr);

	const GpuBuffer* destBufferVK = (const GpuBuffer*)destBuffer;
	assert(destBufferVK != nullptr);

	assert(!m_isRendering);

	size_t stride = sizeof(uint64_t);
	if (queryHeap->GetType() == QueryHeapType::PipelineStats)
	{
		stride = sizeof(PipelineStatistics);
	}

	vkCmdCopyQueryPoolResults(
		m_commandBuffer,
		queryHeapVK->GetQueryPool(),
		startIndex,
		numQueries,
		destBufferVK->GetBuffer(),
		destBufferOffset,
		stride,
		VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
}


void CommandContextVK::ResetQueries(const IQueryHeap* queryHeap, uint32_t startIndex, uint32_t numQueries)
{
	const QueryHeap* queryHeapVK = (const QueryHeap*)queryHeap;
	assert(queryHeapVK != nullptr);

	assert(!m_isRendering);

	vkCmdResetQueryPool(m_commandBuffer, queryHeapVK->GetQueryPool(), startIndex, numQueries);
}


void CommandContextVK::SetRootSignature(CommandListType type, const IRootSignature* rootSignature)
{
	assert(type == CommandListType::Graphics || type == CommandListType::Compute);

	// TODO: Try this with GetPlatformObject()
	const RootSignature* rootSignatureVK = (const RootSignature*)rootSignature;
	assert(rootSignatureVK != nullptr);

	VkPipelineLayout pipelineLayout = rootSignatureVK->GetPipelineLayout();

	if (type == CommandListType::Graphics)
	{
		m_computePipelineLayout = VK_NULL_HANDLE;
		m_computeRootSignature = nullptr;
		m_isComputeRootSignatureParsed = false;
		m_hasDirtyComputeDescriptors = false;

		m_graphicsPipelineLayout = pipelineLayout;

		if (m_graphicsRootSignature != rootSignature)
		{
			m_graphicsRootSignature = rootSignature;
			m_isGraphicsRootSignatureParsed = false;
			m_hasDirtyGraphicsDescriptors = false;
		}

		// Bind static samplers, if needed
#if USE_DESCRIPTOR_BUFFERS
		if (rootSignatureVK->HasStaticSamplers())
		{
			vkCmdBindDescriptorBufferEmbeddedSamplersEXT(
				m_commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_graphicsPipelineLayout,
				rootSignatureVK->GetStaticSamplerDescriptorSetIndex());
		}
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
		VkDescriptorSet staticSamplerDescriptorSet = rootSignatureVK->GetStaticSamplerDescriptorSet();
		if (staticSamplerDescriptorSet != VK_NULL_HANDLE)
		{
			vkCmdBindDescriptorSets(
				m_commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_graphicsPipelineLayout,
				rootSignatureVK->GetStaticSamplerDescriptorSetIndex(),
				1,
				&staticSamplerDescriptorSet,
				0,
				nullptr);
		}
#endif // USE_LEGACY_DESCRIPTOR_SETS
	}
	else
	{
		m_graphicsPipelineLayout = VK_NULL_HANDLE;
		m_graphicsRootSignature = nullptr;
		m_isGraphicsRootSignatureParsed = false;
		m_hasDirtyGraphicsDescriptors = false;

		m_computePipelineLayout = pipelineLayout;

		if (m_computeRootSignature != rootSignature)
		{
			m_computeRootSignature = rootSignature;
			m_isComputeRootSignatureParsed = false;
			m_hasDirtyComputeDescriptors = false;
		}

		// Bind static samplers, if needed
#if USE_DESCRIPTOR_BUFFERS
		if (rootSignatureVK->HasStaticSamplers())
		{
			vkCmdBindDescriptorBufferEmbeddedSamplersEXT(
				m_commandBuffer,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				m_computePipelineLayout,
				rootSignatureVK->GetStaticSamplerDescriptorSetIndex());
		}
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
		VkDescriptorSet staticSamplerDescriptorSet = rootSignatureVK->GetStaticSamplerDescriptorSet();
		if (staticSamplerDescriptorSet != VK_NULL_HANDLE)
		{
			vkCmdBindDescriptorSets(
				m_commandBuffer,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				m_computePipelineLayout,
				rootSignatureVK->GetStaticSamplerDescriptorSetIndex(),
				1,
				&staticSamplerDescriptorSet,
				0,
				nullptr);
		}
#endif // USE_LEGACY_DESCRIPTOR_SETS
	}

	const uint32_t numRootParameters = rootSignature->GetNumRootParameters();
	for (uint32_t i = 0; i < numRootParameters; ++i)
	{
		const auto& rootParameter = rootSignature->GetRootParameter(i);
		// TODO: Store this in the RootSignatureManager so we don't have to do this conversion every frame.
		m_shaderStages[i] = ShaderStageToVulkan(rootParameter.shaderVisibility);
	}
}


void CommandContextVK::SetGraphicsPipeline(const IGraphicsPipeline* graphicsPipeline)
{
	m_computePipelineLayout = VK_NULL_HANDLE;

	// TODO: Try this with GetPlatformObject()
	const GraphicsPipeline* graphicsPipelineVK = (const GraphicsPipeline*)graphicsPipeline;
	assert(graphicsPipelineVK != nullptr);

	VkPipeline vkPipeline = graphicsPipelineVK->GetPipelineState();

	if (vkPipeline != m_graphicsPipeline)
	{
		m_graphicsPipeline = vkPipeline;
		vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);
	}

	SetPrimitiveTopology(graphicsPipeline->GetPrimitiveTopology());
}


void CommandContextVK::SetComputePipeline(const IComputePipeline* computePipeline)
{
	m_graphicsPipelineLayout = VK_NULL_HANDLE;

	// TODO: Try this with GetPlatformObject()
	const ComputePipeline* computePipelineVK = (const ComputePipeline*)computePipeline;
	assert(computePipelineVK != nullptr);

	VkPipeline vkPipeline = computePipelineVK->GetPipelineState();

	if (vkPipeline != m_computePipeline)
	{
		m_computePipeline = vkPipeline;
		vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline);
	}
}


void CommandContextVK::SetMeshletPipeline(const IMeshletPipeline* meshletPipeline)
{
	m_computePipelineLayout = VK_NULL_HANDLE;

	// TODO: Try this with GetPlatformObject()
	const MeshletPipeline* meshletPipelineVK = (const MeshletPipeline*)meshletPipeline;
	assert(meshletPipelineVK != nullptr);

	VkPipeline vkPipeline = meshletPipelineVK->GetPipelineState();

	if (vkPipeline != m_graphicsPipeline)
	{
		m_graphicsPipeline = vkPipeline;
		vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);
	}
}


void CommandContextVK::SetViewport(float x, float y, float w, float h, float minDepth, float maxDepth)
{
	VkViewport viewport{
		.x			= x,
		.y			= h,
		.width		= w,
		.height		= -h,
		.minDepth	= minDepth,
		.maxDepth	= maxDepth
	};
	
	vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
}


void CommandContextVK::SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
	VkRect2D scissor{};
	scissor.extent.width = right - left;
	scissor.extent.height = bottom - top;
	scissor.offset.x = static_cast<int32_t>(left);
	scissor.offset.y = static_cast<int32_t>(top);
	vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
}


void CommandContextVK::SetStencilRef(uint32_t stencilRef)
{
	vkCmdSetStencilReference(m_commandBuffer, VK_STENCIL_FRONT_AND_BACK, stencilRef);
}


void CommandContextVK::SetBlendFactor(Color blendFactor)
{
	vkCmdSetBlendConstants(m_commandBuffer, blendFactor.GetPtr());
}


void CommandContextVK::SetPrimitiveTopology(PrimitiveTopology topology)
{
	const auto vkTopology = PrimitiveTopologyToVulkan(topology);
	if (m_primitiveTopology != vkTopology)
	{
		vkCmdSetPrimitiveTopology(m_commandBuffer, vkTopology);
		m_primitiveTopology = vkTopology;
	}
}


void CommandContextVK::SetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
	vkCmdSetDepthBias(m_commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}


void CommandContextVK::SetConstantArray(CommandListType type, uint32_t rootIndex, uint32_t numConstants, const void* constants, uint32_t offset)
{
	vkCmdPushConstants(
		m_commandBuffer,
		GetPipelineLayout(type),
		m_shaderStages[rootIndex],
		offset * sizeof(DWORD),
		numConstants * sizeof(DWORD),
		constants);
}


void CommandContextVK::SetConstant(CommandListType type, uint32_t rootIndex, uint32_t offset, DWParam val)
{
	uint32_t v = get<uint32_t>(val.value);
	vkCmdPushConstants(
		m_commandBuffer,
		GetPipelineLayout(type),
		m_shaderStages[rootIndex],
		offset * sizeof(uint32_t),
		sizeof(uint32_t),
		&v);
}


void CommandContextVK::SetConstants(CommandListType type, uint32_t rootIndex, DWParam x)
{
	uint32_t v = get<uint32_t>(x.value);
	vkCmdPushConstants(
		m_commandBuffer,
		GetPipelineLayout(type),
		m_shaderStages[rootIndex],
		0,
		sizeof(uint32_t),
		&v);
}


void CommandContextVK::SetConstants(CommandListType type, uint32_t rootIndex, DWParam x, DWParam y)
{
	uint32_t val[] = { get<uint32_t>(x.value), get<uint32_t>(y.value) };
	vkCmdPushConstants(
		m_commandBuffer,
		GetPipelineLayout(type),
		m_shaderStages[rootIndex],
		0,
		2 * sizeof(uint32_t),
		&val);
}


void CommandContextVK::SetConstants(CommandListType type, uint32_t rootIndex, DWParam x, DWParam y, DWParam z)
{
	uint32_t val[] = { get<uint32_t>(x.value), get<uint32_t>(y.value), get<uint32_t>(z.value) };
	vkCmdPushConstants(
		m_commandBuffer,
		GetPipelineLayout(type),
		m_shaderStages[rootIndex],
		0,
		3 * sizeof(uint32_t),
		&val);
}


void CommandContextVK::SetConstants(CommandListType type, uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w)
{
	uint32_t val[] = { get<uint32_t>(x.value), get<uint32_t>(y.value), get<uint32_t>(z.value), get<uint32_t>(w.value) };
	vkCmdPushConstants(
		m_commandBuffer,
		GetPipelineLayout(type),
		m_shaderStages[rootIndex],
		0,
		4 * sizeof(uint32_t),
		&val);
}


void CommandContextVK::SetRootCBV(CommandListType type, uint32_t rootIndex, const IGpuBuffer* gpuBuffer, size_t offsetInBytes)
{
	const GpuBuffer* gpuBufferVK = (const GpuBuffer*)gpuBuffer;
	assert(gpuBufferVK != nullptr);

	const RootSignature* rootSignature = GetRootSignature(type);

	FlushResourceBarriers();

	VkDescriptorBufferInfo info{
		.buffer		= gpuBufferVK->GetBuffer(),
		.offset		= offsetInBytes,
		.range		= VK_WHOLE_SIZE
	};

	const uint32_t binding = rootSignature->GetPushDescriptorBinding(rootIndex);
	assert(binding != (uint32_t)-1);

	VkWriteDescriptorSet writeDescriptor{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstBinding			= binding + GetRegisterShift(RootParameterType::RootCBV),
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo		= &info
	};

	VkPipelineBindPoint bindPoint = type == CommandListType::Graphics ?
		VK_PIPELINE_BIND_POINT_GRAPHICS :
		VK_PIPELINE_BIND_POINT_COMPUTE;

	const uint32_t pushDescriptorSetIndex = rootSignature->GetPushDescriptorSetIndex();
	assert(pushDescriptorSetIndex != (uint32_t)-1);

	vkCmdPushDescriptorSet(m_commandBuffer, bindPoint, GetPipelineLayout(type), pushDescriptorSetIndex, 1, &writeDescriptor);
}


void CommandContextVK::SetRootSRV(CommandListType type, uint32_t rootIndex, const IGpuBuffer* gpuBuffer, size_t offsetInBytes)
{
	const GpuBuffer* gpuBufferVK = (const GpuBuffer*)gpuBuffer;
	assert(gpuBufferVK != nullptr);

	const RootSignature* rootSignature = GetRootSignature(type);

	FlushResourceBarriers();

	VkDescriptorBufferInfo info{
		.buffer		= gpuBufferVK->GetBuffer(),
		.offset		= offsetInBytes,
		.range		= VK_WHOLE_SIZE
	};

	const uint32_t binding = rootSignature->GetPushDescriptorBinding(rootIndex);
	assert(binding != (uint32_t)-1);

	VkWriteDescriptorSet writeDescriptor{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstBinding			= binding + GetRegisterShift(RootParameterType::RootSRV),
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pBufferInfo		= &info
	};

	VkPipelineBindPoint bindPoint = type == CommandListType::Graphics ?
		VK_PIPELINE_BIND_POINT_GRAPHICS :
		VK_PIPELINE_BIND_POINT_COMPUTE;

	const uint32_t pushDescriptorSetIndex = rootSignature->GetPushDescriptorSetIndex();
	assert(pushDescriptorSetIndex != (uint32_t)-1);

	vkCmdPushDescriptorSet(m_commandBuffer, bindPoint, GetPipelineLayout(type), pushDescriptorSetIndex, 1, &writeDescriptor);
}


void CommandContextVK::SetRootUAV(CommandListType type, uint32_t rootIndex, const IGpuBuffer* gpuBuffer, size_t offsetInBytes)
{
	const GpuBuffer* gpuBufferVK = (const GpuBuffer*)gpuBuffer;
	assert(gpuBufferVK != nullptr);

	const RootSignature* rootSignature = GetRootSignature(type);

	FlushResourceBarriers();

	VkDescriptorBufferInfo info{
		.buffer		= gpuBufferVK->GetBuffer(),
		.offset		= offsetInBytes,
		.range		= VK_WHOLE_SIZE
	};

	const uint32_t binding = rootSignature->GetPushDescriptorBinding(rootIndex);
	assert(binding != (uint32_t)-1);

	VkWriteDescriptorSet writeDescriptor{
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstBinding			= binding + GetRegisterShift(RootParameterType::RootUAV),
		.dstArrayElement	= 0,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pBufferInfo		= &info
	};

	VkPipelineBindPoint bindPoint = type == CommandListType::Graphics ?
		VK_PIPELINE_BIND_POINT_GRAPHICS :
		VK_PIPELINE_BIND_POINT_COMPUTE;

	const uint32_t pushDescriptorSetIndex = rootSignature->GetPushDescriptorSetIndex();
	assert(pushDescriptorSetIndex != (uint32_t)-1);

	vkCmdPushDescriptorSet(m_commandBuffer, bindPoint, GetPipelineLayout(type), pushDescriptorSetIndex, 1, &writeDescriptor);
}


void CommandContextVK::SetDescriptors(CommandListType type, uint32_t rootIndex, IDescriptorSet* descriptorSet)
{
	SetDescriptors_Internal(type, rootIndex, descriptorSet);
}


void CommandContextVK::SetResources(CommandListType type, ResourceSet& resourceSet)
{
	BeginEvent("SetResources");

	const uint32_t numDescriptorSets = resourceSet.GetNumDescriptorSets();
	for (uint32_t i = 0; i < numDescriptorSets; ++i)
	{
		// Skip null entries, which are for root constants
		if (resourceSet[i] == nullptr)
		{
			continue;
		}

		SetDescriptors_Internal(type, i, resourceSet[i].get());
	}

	EndEvent();
}


void CommandContextVK::SetSRV(CommandListType type, uint32_t rootIndex, uint32_t srvRegister, const IColorBuffer* colorBuffer)
{
	ParseRootSignature(type);

	const bool graphicsPipe = type == CommandListType::Graphics;
	
#if USE_DESCRIPTOR_BUFFERS
	m_dynamicResourceDescriptorBuffer.SetSRV(type, rootIndex, srvRegister, 0, colorBuffer);
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	VkImageView imageView = ((const Descriptor*)colorBuffer->GetSrvDescriptor())->GetImageView();

	VkDescriptorImageInfo info{
		.imageView		= imageView,
		.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	m_dynamicDescriptorHeap->SetDescriptorImageInfo(rootIndex, srvRegister, info, graphicsPipe);
#endif // USE_LEGACY_DESCRIPTOR_SETS

	MarkDescriptorsDirty(type);
}


void CommandContextVK::SetSRV(CommandListType type, uint32_t rootIndex, uint32_t srvRegister, const IDepthBuffer* depthBuffer, bool depthSrv)
{
	ParseRootSignature(type);

	const bool graphicsPipe = type == CommandListType::Graphics;

#if USE_DESCRIPTOR_BUFFERS
	m_dynamicResourceDescriptorBuffer.SetSRV(type, rootIndex, srvRegister, 0, depthBuffer, depthSrv);
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	VkImageView imageView = ((const Descriptor*)colorBuffer->GetSrvDescriptor(depthSrv))->GetImageView();

	VkDescriptorImageInfo info{
		.imageView		= imageView,
		.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	m_dynamicDescriptorHeap->SetDescriptorImageInfo(rootIndex, srvRegister, info, graphicsPipe);
#endif // USE_LEGACY_DESCRIPTOR_SETS

	MarkDescriptorsDirty(type);
}


void CommandContextVK::SetSRV(CommandListType type, uint32_t rootIndex, uint32_t srvRegister, const IGpuBuffer* gpuBuffer)
{
	ParseRootSignature(type);

	const bool graphicsPipe = type == CommandListType::Graphics;

#if USE_DESCRIPTOR_BUFFERS
	m_dynamicResourceDescriptorBuffer.SetSRV(type, rootIndex, srvRegister, 0, gpuBuffer);
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	if (gpuBuffer->GetResourceType() == ResourceType::TypedBuffer)
	{
		m_dynamicDescriptorHeap->SetDescriptorBufferView(rootIndex, srvRegister, ((const Descriptor*)gpuBuffer->GetSrvDescriptor())->GetBufferView(), graphicsPipe);
	}
	else
	{
		VkDescriptorBufferInfo info{
			.buffer		= gpuBufferVK->GetBuffer(),
			.offset		= 0,
			.range		= VK_WHOLE_SIZE
		};
		m_dynamicDescriptorHeap->SetDescriptorBufferInfo(rootIndex, srvRegister, info, graphicsPipe);
	}
#endif // USE_LEGACY_DESCRIPTOR_SETS

	MarkDescriptorsDirty(type);
}


void CommandContextVK::SetSRV(CommandListType type, uint32_t rootIndex, uint32_t srvRegister, const ITexture* texture)
{
	ParseRootSignature(type);

	const bool graphicsPipe = type == CommandListType::Graphics;
	
#if USE_DESCRIPTOR_BUFFERS
	m_dynamicResourceDescriptorBuffer.SetSRV(type, rootIndex, srvRegister, 0, texture);
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	VkImageView imageView = ((const Descriptor*)texture->GetDescriptor())->GetImageView();

	VkDescriptorImageInfo info{
		.imageView		= imageView,
		.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	m_dynamicDescriptorHeap->SetDescriptorImageInfo(rootIndex, offset, info, graphicsPipe);
#endif // USE_LEGACY_DESCRIPTOR_SETS

	MarkDescriptorsDirty(type);
}


void CommandContextVK::SetUAV(CommandListType type, uint32_t rootIndex, uint32_t uavRegister, const IColorBuffer* colorBuffer)
{
	ParseRootSignature(type);

	const bool graphicsPipe = type == CommandListType::Graphics;

#if USE_DESCRIPTOR_BUFFERS
	m_dynamicResourceDescriptorBuffer.SetUAV(type, rootIndex, uavRegister, 0, colorBuffer);
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	VkDescriptorImageInfo info{
		.imageView		= ((const Descriptor*)colorBuffer->GetSrvDescriptor())->GetImageView(),
		.imageLayout	= VK_IMAGE_LAYOUT_GENERAL
	};

	m_dynamicDescriptorHeap->SetDescriptorImageInfo(rootIndex, offset, info, graphicsPipe);
#endif // USE_LEGACY_DESCRIPTOR_SETS

	MarkDescriptorsDirty(type);
}


void CommandContextVK::SetUAV(CommandListType type, uint32_t rootIndex, uint32_t uavRegister, const IDepthBuffer* depthBuffer)
{
	assert(false);

	const bool graphicsPipe = type == CommandListType::Graphics;
}


void CommandContextVK::SetUAV(CommandListType type, uint32_t rootIndex, uint32_t uavRegister, const IGpuBuffer* gpuBuffer)
{
	ParseRootSignature(type);

#if USE_DESCRIPTOR_BUFFERS
	m_dynamicResourceDescriptorBuffer.SetUAV(type, rootIndex, uavRegister, 0, gpuBuffer);
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	const bool graphicsPipe = type == CommandListType::Graphics;

	if (gpuBuffer->GetResourceType() == ResourceType::TypedBuffer)
	{
		m_dynamicDescriptorHeap->SetDescriptorBufferView(rootIndex, uavRegister, ((const Descriptor*)gpuBuffer->GetUavDescriptor())->GetBufferView(), graphicsPipe);
	}
	else
	{
		VkDescriptorBufferInfo info{
			.buffer		= ((const Descriptor*)gpuBuffer->GetUavDescriptor())->GetBuffer(),
			.offset		= 0,
			.range		= VK_WHOLE_SIZE
		};
		m_dynamicDescriptorHeap->SetDescriptorBufferInfo(rootIndex, uavRegister, info, graphicsPipe);
	}
#endif // USE_LEGACY_DESCRIPTOR_SETS

	MarkDescriptorsDirty(type);
}


void CommandContextVK::SetCBV(CommandListType type, uint32_t rootIndex, uint32_t cbvRegister, const IGpuBuffer* gpuBuffer)
{
	ParseRootSignature(type);

#if USE_DESCRIPTOR_BUFFERS
	m_dynamicResourceDescriptorBuffer.SetCBV(type, rootIndex, cbvRegister, 0, gpuBuffer);
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	const bool graphicsPipe = type == CommandListType::Graphics;

	VkDescriptorBufferInfo info{
		.buffer		= ((const Descriptor*)gpuBuffer->GetUavDescriptor())->GetBuffer(),
		.offset		= 0,
		.range		= VK_WHOLE_SIZE
	};

	m_dynamicDescriptorHeap->SetDescriptorBufferInfo(rootIndex, offset, info, graphicsPipe);
#endif USE_LEGACY_DESCRIPTOR_SETS

	MarkDescriptorsDirty(type);
}


void CommandContextVK::SetSampler(CommandListType type, uint32_t rootIndex, uint32_t samplerRegister, const ISampler* sampler)
{
	// TODO
	assert(false);
}


void CommandContextVK::SetIndexBuffer(const IGpuBuffer* gpuBuffer)
{
	// TODO: Try this with GetPlatformObject()
	const GpuBuffer* gpuBufferVK = (const GpuBuffer*)gpuBuffer;
	assert(gpuBufferVK != nullptr);

	const bool is16Bit = gpuBuffer->GetElementSize() == sizeof(uint16_t);
	const VkIndexType indexType = is16Bit ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
	vkCmdBindIndexBuffer(m_commandBuffer, gpuBufferVK->GetBuffer(), 0, indexType);
}


void CommandContextVK::SetVertexBuffer(uint32_t slot, const IGpuBuffer* gpuBuffer)
{
	// TODO: Try this with GetPlatformObject()
	const GpuBuffer* gpuBufferVK = (const GpuBuffer*)gpuBuffer;
	assert(gpuBufferVK != nullptr);

	VkDeviceSize offsets[1] = { 0 };
	VkBuffer buffers[1] = { gpuBufferVK->GetBuffer() };
	vkCmdBindVertexBuffers(m_commandBuffer, slot, 1, buffers, offsets);
}


void CommandContextVK::SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexStride, DynAlloc dynAlloc)
{
	VkDeviceSize offsets[1] = { dynAlloc.offset };
	VkBuffer buffers[1] = { reinterpret_cast<VkBuffer>(dynAlloc.resource) };
	vkCmdBindVertexBuffers(m_commandBuffer, slot, 1, buffers, offsets);
}


void CommandContextVK::SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexStride, const void* data)
{
	const size_t sizeInBytes = numVertices * vertexStride;

	DynAlloc dynAlloc = ReserveUploadMemory(sizeInBytes);

	memcpy(dynAlloc.dataPtr, data, sizeInBytes);

	VkDeviceSize offsets[1] = { 0 };
	VkBuffer buffers[1] = { reinterpret_cast<VkBuffer>(dynAlloc.gpuAddress) };
	vkCmdBindVertexBuffers(m_commandBuffer, slot, 1, buffers, offsets);
}


void CommandContextVK::SetDynamicIndexBuffer(uint32_t indexCount, bool indexSize16Bit, DynAlloc dynAlloc)
{
	const VkIndexType indexType = indexSize16Bit ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
	VkDeviceSize offset{ dynAlloc.offset };
	vkCmdBindIndexBuffer(m_commandBuffer, reinterpret_cast<VkBuffer>(dynAlloc.resource), offset, indexType);
}


void CommandContextVK::SetDynamicIndexBuffer(uint32_t indexCount, bool indexSize16Bit, const void* data)
{
	const size_t elementSize = indexSize16Bit ? sizeof(uint16_t) : sizeof(uint32_t);
	const size_t sizeInBytes = indexCount * elementSize;

	DynAlloc dynAlloc = ReserveUploadMemory(sizeInBytes);

	memcpy(dynAlloc.dataPtr, data, sizeInBytes);

	const VkIndexType indexType = indexSize16Bit ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
	VkDeviceSize offset{ 0 };
	vkCmdBindIndexBuffer(m_commandBuffer, reinterpret_cast<VkBuffer>(dynAlloc.gpuAddress), offset, indexType);
}


void CommandContextVK::DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
	uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
	FlushResourceBarriers();
	
	if (HasDirtyDescriptors(CommandListType::Graphics))
	{
		const bool graphicsPipe = true;
		UpdateAndBindDynamicDescriptors(graphicsPipe);
		ClearDirtyDescriptors(CommandListType::Graphics);
	}

	vkCmdDraw(m_commandBuffer, vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}


void CommandContextVK::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
	int32_t baseVertexLocation, uint32_t startInstanceLocation)
{
	FlushResourceBarriers();

	if (HasDirtyDescriptors(CommandListType::Graphics))
	{
		const bool graphicsPipe = true;
		UpdateAndBindDynamicDescriptors(graphicsPipe);
		ClearDirtyDescriptors(CommandListType::Graphics);
	}

	vkCmdDrawIndexed(m_commandBuffer, indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}


void CommandContextVK::DrawIndirect(const IGpuBuffer* argumentBuffer, uint64_t argumentBufferOffset)
{
	assert(argumentBuffer->GetResourceType() == ResourceType::IndirectArgsBuffer);
	assert(argumentBuffer->GetElementSize() == sizeof(VkDrawIndirectCommand));

	const GpuBuffer* argumentBufferVK = (const GpuBuffer*)argumentBuffer;
	assert(argumentBufferVK != nullptr);

	FlushResourceBarriers();

	if (HasDirtyDescriptors(CommandListType::Graphics))
	{
		const bool graphicsPipe = true;
		UpdateAndBindDynamicDescriptors(graphicsPipe);
		ClearDirtyDescriptors(CommandListType::Graphics);
	}

	// TODO: support multi-draw indirect	
	vkCmdDrawIndexedIndirect(
		m_commandBuffer, 
		argumentBufferVK->GetBuffer(), 
		argumentBufferOffset, 
		1, 
		(uint32_t)argumentBuffer->GetElementSize());
}


void CommandContextVK::DrawIndexedIndirect(const IGpuBuffer* argumentBuffer, uint64_t argumentBufferOffset)
{
	assert(argumentBuffer->GetResourceType() == ResourceType::IndirectArgsBuffer);
	assert(argumentBuffer->GetElementSize() == sizeof(VkDrawIndexedIndirectCommand));

	const GpuBuffer* argumentBufferVK = (const GpuBuffer*)argumentBuffer;
	assert(argumentBufferVK != nullptr);

	FlushResourceBarriers();

	if (HasDirtyDescriptors(CommandListType::Graphics))
	{
		const bool graphicsPipe = true;
		UpdateAndBindDynamicDescriptors(graphicsPipe);
		ClearDirtyDescriptors(CommandListType::Graphics);
	}

	// TODO: support multi-draw indirect
	vkCmdDrawIndexedIndirect(
		m_commandBuffer,
		argumentBufferVK->GetBuffer(),
		argumentBufferOffset,
		1,
		(uint32_t)argumentBuffer->GetElementSize());
}


void CommandContextVK::DispatchMesh(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
	FlushResourceBarriers();

	if (HasDirtyDescriptors(CommandListType::Graphics))
	{
		const bool graphicsPipe = true;
		UpdateAndBindDynamicDescriptors(graphicsPipe);
		ClearDirtyDescriptors(CommandListType::Graphics);
	}

	vkCmdDrawMeshTasksEXT(m_commandBuffer, groupCountX, groupCountY, groupCountZ);
}


void CommandContextVK::Resolve(const IColorBuffer* srcBuffer, const IColorBuffer* destBuffer, Format format)
{
	const ColorBuffer* srcBufferVK = (const ColorBuffer*)srcBuffer;
	assert(srcBufferVK != nullptr);

	const ColorBuffer* destBufferVK = (const ColorBuffer*)destBuffer;
	assert(destBufferVK != nullptr);

	FlushResourceBarriers();

	VkImageResolve resolve{
		.srcSubresource = {
			.aspectMask			= VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel			= 0,
			.baseArrayLayer		= 0,
			.layerCount			= srcBuffer->GetArraySize()
			},
		.srcOffset		= { .x = 0, .y = 0, .z = 0 },
		.dstSubresource = {
			.aspectMask			= VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel			= 0,
			.baseArrayLayer		= 0,
			.layerCount			= destBuffer->GetArraySize()
			},
		.dstOffset		= { .x = 0, .y = 0, .z = 0 },
		.extent			= { .width = (uint32_t)destBuffer->GetWidth(), .height = destBuffer->GetHeight(), .depth = 1 }
	};

	vkCmdResolveImage(
		m_commandBuffer,
		srcBufferVK->GetImage(),
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		destBufferVK->GetImage(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&resolve);
}


void CommandContextVK::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
	FlushResourceBarriers();

	if (HasDirtyDescriptors(CommandListType::Compute))
	{
		const bool graphicsPipe = false;
		UpdateAndBindDynamicDescriptors(graphicsPipe);
		ClearDirtyDescriptors(CommandListType::Compute);
	}
	
	vkCmdDispatch(m_commandBuffer, groupCountX, groupCountY, groupCountZ);
}


void CommandContextVK::Dispatch1D(uint32_t threadCountX, uint32_t groupSizeX)
{
	Dispatch(Math::DivideByMultiple(threadCountX, groupSizeX), 1, 1);
}


void CommandContextVK::Dispatch2D(uint32_t threadCountX, uint32_t threadCountY, uint32_t groupSizeX, uint32_t groupSizeY)
{
	Dispatch(
		Math::DivideByMultiple(threadCountX, groupSizeX),
		Math::DivideByMultiple(threadCountY, groupSizeY), 1);
}


void CommandContextVK::Dispatch3D(uint32_t threadCountX, uint32_t threadCountY, uint32_t threadCountZ, uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ)
{
	Dispatch(
		Math::DivideByMultiple(threadCountX, groupSizeX),
		Math::DivideByMultiple(threadCountY, groupSizeY),
		Math::DivideByMultiple(threadCountZ, groupSizeZ));
}


void CommandContextVK::DispatchIndirect(const IGpuBuffer* argumentBuffer, uint64_t argumentBufferOffset)
{
	assert(argumentBuffer->GetResourceType() == ResourceType::IndirectArgsBuffer);
	assert(argumentBuffer->GetElementSize() == sizeof(VkDispatchIndirectCommand));

	const GpuBuffer* argumentBufferVK = (const GpuBuffer*)argumentBuffer;
	assert(argumentBufferVK != nullptr);

	FlushResourceBarriers();

	if (HasDirtyDescriptors(CommandListType::Compute))
	{
		const bool graphicsPipe = false;
		UpdateAndBindDynamicDescriptors(graphicsPipe);
		ClearDirtyDescriptors(CommandListType::Compute);
	}

	vkCmdDispatchIndirect(m_commandBuffer, argumentBufferVK->GetBuffer(), argumentBufferOffset);
}


#if USE_DESCRIPTOR_BUFFERS
void CommandContextVK::SetDescriptorBuffer(DescriptorBufferType bufferType, VkBuffer descriptorBuffer)
{
	if (m_currentDescriptorBuffers[(uint32_t)bufferType] != descriptorBuffer)
	{
		m_currentDescriptorBuffers[(uint32_t)bufferType] = descriptorBuffer;
		BindDescriptorBuffers();
	}
}


void CommandContextVK::SetDescriptorBuffers(std::span<DescriptorBufferType> bufferTypes, std::span<VkBuffer> buffers)
{
	bool anyChanged = false;

	assert(bufferTypes.size() == buffers.size());

	for (size_t i = 0; i < buffers.size(); ++i)
	{
		const uint32_t index = (uint32_t)bufferTypes[i];

		if (m_currentDescriptorBuffers[index] != buffers[i])
		{
			m_currentDescriptorBuffers[index] = buffers[i];
			anyChanged = true;
		}
	}

	if (anyChanged)
	{
		BindDescriptorBuffers();
	}
}
#endif // USE_DESCRIPTOR_BUFFERS


void CommandContextVK::InitializeBuffer_Internal(IGpuBuffer* destBuffer, const void* bufferData, size_t numBytes, size_t offset)
{
	DynAlloc dynAlloc = ReserveUploadMemory(numBytes);

	if (Math::IsAligned(dynAlloc.dataPtr, 16) && Math::IsAligned(bufferData, 16))
	{
		SIMDMemCopy(dynAlloc.dataPtr, bufferData, Math::DivideByMultiple(numBytes, 16));
	}
	else
	{
		memcpy(dynAlloc.dataPtr, bufferData, numBytes);
	}

	// Copy from the upload buffer to the destination buffer
	TransitionResource(destBuffer, ResourceState::CopyDest, true);

	// TODO: Try this with GetPlatformObject()
	GpuBuffer* destBufferVK = (GpuBuffer*)destBuffer;
	assert(destBufferVK != nullptr);

	VkBufferCopy copyRegion{ .size = numBytes };
	vkCmdCopyBuffer(m_commandBuffer, reinterpret_cast<VkBuffer>(dynAlloc.resource), destBufferVK->GetBuffer(), 1, &copyRegion);

	TransitionResource(destBuffer, ResourceState::GenericRead, true);
}


void CommandContextVK::InitializeTexture_Internal(ITexture* destTexture, const TextureInitializer& texInit)
{
	const bool isTexture3D = texInit.dimension == TextureDimension::Texture3D;
	const uint32_t effectiveArraySize = isTexture3D ? 1 : texInit.arraySizeOrDepth;
	const uint32_t numSubResources = effectiveArraySize * texInit.numMips;
	vector<VkBufferImageCopy> bufferCopyRegions(numSubResources);
	
	for (uint32_t i = 0; i < numSubResources; ++i)
	{
		bufferCopyRegions[i] = TextureSubResourceDataToVulkan(texInit.subResourceData[i], VK_IMAGE_ASPECT_COLOR_BIT);
	}

	DynAlloc dynAlloc = ReserveUploadMemory(texInit.totalBytes);
	SIMDMemCopy(dynAlloc.dataPtr, texInit.baseData, Math::DivideByMultiple(texInit.totalBytes, 16));

	// TODO: Try this with GetPlatformObject()
	Texture* textureVK = (Texture*)destTexture;
	assert(textureVK != nullptr);

	// Copy from the upload buffer to the destination texture
	TransitionResource(destTexture, ResourceState::CopyDest, true);

	vkCmdCopyBufferToImage(
		m_commandBuffer,
		reinterpret_cast<VkBuffer>(dynAlloc.resource),
		textureVK->GetImage(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		numSubResources,
		bufferCopyRegions.data());

	TransitionResource(destTexture, ResourceState::PixelShaderResource, true);
}


void CommandContextVK::SetDescriptors_Internal(CommandListType type, uint32_t rootIndex, IDescriptorSet* descriptorSet)
{
	assert(type == CommandListType::Graphics || type == CommandListType::Compute);

	// TODO: Try this with GetPlatformObject()
	DescriptorSet* descriptorSetVK = (DescriptorSet*)descriptorSet;
	assert(descriptorSetVK != nullptr);

#if USE_DESCRIPTOR_BUFFERS
	BindUserDescriptorBuffers();

	// TODO: This refers to the Resource buffer.  Shouldn't hardcode index 0.
	uint32_t bufferIndex = 0;
	VkDeviceSize offset = descriptorSetVK->GetDescriptorBufferOffset();

	vkCmdSetDescriptorBufferOffsetsEXT(
		m_commandBuffer,
		(type == CommandListType::Graphics) ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE,
		(type == CommandListType::Graphics) ? m_graphicsPipelineLayout : m_computePipelineLayout,
		rootIndex,
		1,
		&bufferIndex,
		&offset
	);
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	if (!descriptorSetVK->HasDescriptors())
	{
		return;
	}

	VkDescriptorSet vkDescriptorSet = descriptorSetVK->GetDescriptorSet();
	if (vkDescriptorSet == VK_NULL_HANDLE)
	{
		return;
	}

	vkCmdBindDescriptorSets(
		m_commandBuffer,
		(type == CommandListType::Graphics) ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE,
		(type == CommandListType::Graphics) ? m_graphicsPipelineLayout : m_computePipelineLayout,
		rootIndex,
		1,
		&vkDescriptorSet,
		0,
		nullptr);
#endif // USE_LEGACY_DESCRIPTOR_SETS
}


#if USE_DESCRIPTOR_BUFFERS
void CommandContextVK::BindUserDescriptorBuffers()
{
	DescriptorBufferType types[] = {
		DescriptorBufferType::Resource,
		DescriptorBufferType::Sampler
	};

	VkBuffer buffers[] = {
		g_userDescriptorBufferAllocator[(uint32_t)DescriptorBufferType::Resource].GetBuffer(),
		g_userDescriptorBufferAllocator[(uint32_t)DescriptorBufferType::Sampler].GetBuffer()
	};

	SetDescriptorBuffers(types, buffers);
}


void CommandContextVK::BindDescriptorBuffers()
{
	VkDescriptorBufferBindingInfoEXT bindingInfos[2];
	uint32_t numBuffers = 0;
	uint32_t startBuffer = ~0u;
	bool firstBuffer = true;

	for (uint32_t i = 0; i < 2; ++i)
	{
		if (m_currentDescriptorBuffers[i] != VK_NULL_HANDLE)
		{
			bindingInfos[i].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			bindingInfos[i].pNext = nullptr;
			bindingInfos[i].address = GetBufferDeviceAddress(m_device->Get(), m_currentDescriptorBuffers[i]);
			bindingInfos[i].usage = i == 0 ?
				VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT :
				VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;
			if (firstBuffer)
			{
				startBuffer = i;
				firstBuffer = false;
			}
			++numBuffers;
		}
	}

	if (numBuffers > 0)
	{
		vkCmdBindDescriptorBuffersEXT(m_commandBuffer, numBuffers, &bindingInfos[startBuffer]);
	}
}
#endif // USE_DESCRIPTOR_BUFFERS


void CommandContextVK::UpdateAndBindDynamicDescriptors(bool graphicsPipe)
{
#if USE_DESCRIPTOR_BUFFERS
	m_dynamicResourceDescriptorBuffer.UpdateAndBindDescriptorSets(m_commandBuffer, graphicsPipe);
	m_dynamicSamplerDescriptorBuffer.UpdateAndBindDescriptorSets(m_commandBuffer, graphicsPipe);
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	m_dynamicDescriptorHeap->UpdateAndBindDescriptorSets(m_commandBuffer, graphicsPipe);
#endif // USE_LEGACY_DESCRIPTOR_SETS
}


void CommandContextVK::SetRenderingArea(const ColorBuffer& colorBuffer)
{
	m_renderingArea.offset.x = 0;
	m_renderingArea.offset.y = 0;

	if (m_renderingArea.extent.width != 0 || m_renderingArea.extent.height != 0)
	{
		// Validate dimensions
		assert(m_renderingArea.extent.width == (uint32_t)colorBuffer.GetWidth());
		assert(m_renderingArea.extent.height == colorBuffer.GetHeight());
	}

	m_renderingArea.extent.width = (uint32_t)colorBuffer.GetWidth();
	m_renderingArea.extent.height = colorBuffer.GetHeight();
}


void CommandContextVK::SetRenderingArea(const DepthBuffer& depthBuffer)
{
	m_renderingArea.offset.x = 0;
	m_renderingArea.offset.y = 0;

	if (m_renderingArea.extent.width != 0 || m_renderingArea.extent.height != 0)
	{
		// Validate dimensions
		assert(m_renderingArea.extent.width == (uint32_t)depthBuffer.GetWidth());
		assert(m_renderingArea.extent.height == depthBuffer.GetHeight());
	}

	m_renderingArea.extent.width = (uint32_t)depthBuffer.GetWidth();
	m_renderingArea.extent.height = depthBuffer.GetHeight();
}


void CommandContextVK::BeginRenderingBlock()
{
	assert(!m_isRendering);

	FlushResourceBarriers();

	VkRenderingInfo renderingInfo{
		.sType					= VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea				= m_renderingArea,
		.layerCount				= 1,
		.viewMask				= 0,
		.colorAttachmentCount	= m_numRtvs,
		.pColorAttachments		= m_numRtvs > 0 ? m_rtvs.data() : nullptr,
		.pDepthAttachment		= m_hasDsv ? &m_dsv : nullptr,
		.pStencilAttachment		= (m_hasDsv && IsStencilFormat(m_dsvFormat)) ? &m_dsv : nullptr
	};

	vkCmdBeginRendering(m_commandBuffer, &renderingInfo);

	m_isRendering = true;
}


void CommandContextVK::ResetRenderTargets()
{
	for (uint32_t i = 0; i < 8; ++i)
	{
		m_rtvFormats[i] = VK_FORMAT_UNDEFINED;
	}
	m_numRtvs = 0;
	m_dsvFormat = VK_FORMAT_UNDEFINED;
	m_hasDsv = false;

	// Reset the rendering area
	m_renderingArea.offset.x = -1;
	m_renderingArea.offset.y = -1;
	m_renderingArea.extent.width = 0;
	m_renderingArea.extent.height = 0;
}


void CommandContextVK::ParseRootSignature(CommandListType type)
{
	const RootSignature* rootSignatureVK = nullptr;
	bool graphicsPipe = true;
	bool shouldParse = false;

	if (type == CommandListType::Graphics)
	{
		if (!m_isGraphicsRootSignatureParsed && (m_graphicsRootSignature != nullptr))
		{
			rootSignatureVK = (const RootSignature*)m_graphicsRootSignature;
			shouldParse = true;

			m_isGraphicsRootSignatureParsed = true;
		}
	}
	else
	{
		if (!m_isComputeRootSignatureParsed && (m_computeRootSignature != nullptr))
		{
			rootSignatureVK = (const RootSignature*)m_computeRootSignature;
			graphicsPipe = false;
			shouldParse = true;

			m_isComputeRootSignatureParsed = true;
		}
	}

	if (shouldParse)
	{
#if USE_DESCRIPTOR_BUFFERS
		m_dynamicResourceDescriptorBuffer.ParseRootSignature(*rootSignatureVK, graphicsPipe);
		m_dynamicSamplerDescriptorBuffer.ParseRootSignature(*rootSignatureVK, graphicsPipe);
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
		m_dynamicDescriptorHeap->ParseRootSignature(*rootSignatureVK, graphicsPipe);
#endif // USE_LEGACY_DESCRIPTOR_SETS
	}
}


void CommandContextVK::MarkDescriptorsDirty(CommandListType type)
{
	if (type == CommandListType::Graphics)
	{
		m_hasDirtyGraphicsDescriptors = true;
	}
	else
	{
		m_hasDirtyComputeDescriptors = true;
	}
}


bool CommandContextVK::HasDirtyDescriptors(CommandListType type)
{
	if (type == CommandListType::Graphics)
	{
		return m_hasDirtyGraphicsDescriptors;
	}
	else
	{
		return m_hasDirtyComputeDescriptors;
	}
}


void CommandContextVK::ClearDirtyDescriptors(CommandListType type)
{
	if (type == CommandListType::Graphics)
	{
		m_hasDirtyGraphicsDescriptors = false;
	}
	else
	{
		m_hasDirtyComputeDescriptors = false;
	}
}


RootSignature* CommandContextVK::GetRootSignature(CommandListType type)
{
	if (type == CommandListType::Graphics)
	{
		return (RootSignature*)m_graphicsRootSignature;
	}
	else
	{
		return (RootSignature*)m_computeRootSignature;
	}
}

} // namespace Luna::VK