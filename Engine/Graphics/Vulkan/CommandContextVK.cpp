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

#include "ColorBufferVK.h"
#include "DepthBufferVK.h"
#include "DeviceVK.h"
#include "DeviceManagerVK.h"
#include "GpuBufferVK.h"
#include "QueueVK.h"
#include "RootSignatureVK.h"

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


VkRenderingAttachmentInfo GetRenderingAttachmentInfo(IColorBuffer* renderTarget)
{
	VkRenderingAttachmentInfo info{
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.pNext = nullptr,
		.imageView = renderTarget->GetNativeObject(NativeObjectType::VK_ImageView_RTV),
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.resolveMode = VK_RESOLVE_MODE_NONE,
		.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = GetColorClearValue(renderTarget->GetClearColor())
	};

	return info;
}


VkRenderingAttachmentInfo GetRenderingAttachmentInfo(IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect)
{
	VkImageView imageView{ VK_NULL_HANDLE };
	switch (depthStencilAspect)
	{
	case DepthStencilAspect::ReadWrite:
	case DepthStencilAspect::ReadOnly:
		imageView = depthTarget->GetNativeObject(NativeObjectType::VK_ImageView_DSV);
		break;

	case DepthStencilAspect::DepthReadOnly:
		imageView = depthTarget->GetNativeObject(NativeObjectType::VK_ImageView_DSV_Depth);
		break;

	case DepthStencilAspect::StencilReadOnly:
		imageView = depthTarget->GetNativeObject(NativeObjectType::VK_ImageView_DSV_Stencil);
		break;
	}

	VkRenderingAttachmentInfo info{
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.pNext = nullptr,
		.imageView = imageView,
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		.resolveMode = VK_RESOLVE_MODE_NONE,
		.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.clearValue = GetDepthStencilClearValue(depthTarget->GetClearDepth(), depthTarget->GetClearStencil())
	};

	return info;
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


void CommandContextVK::ClearUAV(IGpuBuffer* gpuBuffer)
{
	uint32_t data = 0;
	vkCmdFillBuffer(m_commandBuffer, gpuBuffer->GetNativeObject(NativeObjectType::VK_Buffer), 0, VK_WHOLE_SIZE, data);
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
	ResetRenderTargets();

	m_rtvs[0] = GetRenderingAttachmentInfo(renderTarget);
	m_numRtvs = 1;
	m_rtvFormats[0] = FormatToVulkan(renderTarget->GetFormat());

	BeginRenderingBlock();
}


void CommandContextVK::BeginRendering(IColorBuffer* renderTarget, IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect)
{
	ResetRenderTargets();

	m_rtvs[0] = GetRenderingAttachmentInfo(renderTarget);
	m_numRtvs = 1;
	m_rtvFormats[0] = FormatToVulkan(renderTarget->GetFormat());

	m_dsv = GetRenderingAttachmentInfo(depthTarget, depthStencilAspect);
	m_hasDsv = true;
	m_dsvFormat = FormatToVulkan(depthTarget->GetFormat());

	BeginRenderingBlock();
}


void CommandContextVK::BeginRendering(IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect)
{
	ResetRenderTargets();

	m_dsv = GetRenderingAttachmentInfo(depthTarget, depthStencilAspect);
	m_hasDsv = true;
	m_dsvFormat = FormatToVulkan(depthTarget->GetFormat());

	BeginRenderingBlock();
}


void CommandContextVK::BeginRendering(std::span<IColorBuffer*> renderTargets)
{
	ResetRenderTargets();
	assert(renderTargets.size() <= 8);

	uint32_t i = 0;
	for (IColorBuffer* renderTarget : renderTargets)
	{
		m_rtvs[i] = GetRenderingAttachmentInfo(renderTarget);
		m_rtvFormats[i] = FormatToVulkan(renderTarget->GetFormat());
		++i;
	}
	m_numRtvs = (uint32_t)renderTargets.size();

	BeginRenderingBlock();
}


void CommandContextVK::BeginRendering(std::span<IColorBuffer*> renderTargets, IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect)
{
	ResetRenderTargets();
	assert(renderTargets.size() <= 8);

	uint32_t i = 0;
	for (IColorBuffer* renderTarget : renderTargets)
	{
		m_rtvs[i] = GetRenderingAttachmentInfo(renderTarget);
		m_rtvFormats[i] = FormatToVulkan(renderTarget->GetFormat());
		++i;
	}
	m_numRtvs = (uint32_t)renderTargets.size();

	m_dsv = GetRenderingAttachmentInfo(depthTarget, depthStencilAspect);
	m_hasDsv = true;
	m_dsvFormat = FormatToVulkan(depthTarget->GetFormat());

	BeginRenderingBlock();
}


void CommandContextVK::EndRendering()
{
	assert(m_isRendering);

	vkCmdEndRendering(m_commandBuffer);

	m_isRendering = false;
}


void CommandContextVK::SetRootSignature(IRootSignature* rootSignature)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);

	if (m_type == CommandListType::Direct)
	{
		m_computePipelineLayout = VK_NULL_HANDLE;
		m_graphicsPipelineLayout = rootSignature->GetNativeObject(NativeObjectType::VK_PipelineLayout);
	}
	else
	{
		m_graphicsPipelineLayout = VK_NULL_HANDLE;
		m_computePipelineLayout = rootSignature->GetNativeObject(NativeObjectType::VK_PipelineLayout);
	}

	const uint32_t numRootParameters = rootSignature->GetNumRootParameters();
	for (uint32_t i = 0; i < numRootParameters; ++i)
	{
		const auto& rootParameter = rootSignature->GetRootParameter(i);
		m_shaderStages[i] = ShaderStageToVulkan(rootParameter.shaderVisibility);
	}
}


void CommandContextVK::SetViewport(float x, float y, float w, float h, float minDepth, float maxDepth)
{
	VkViewport viewport{
		.x			= x,
		.y			= y,
		.width		= w,
		.height		= h,
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
	vkCmdSetPrimitiveTopology(m_commandBuffer, PrimitiveTopologyToVulkan(topology));
}


void CommandContextVK::SetConstantArray(uint32_t rootIndex, uint32_t numConstants, const void* constants, uint32_t offset)
{
	vkCmdPushConstants(
		m_commandBuffer,
		GetPipelineLayout(),
		m_shaderStages[rootIndex],
		offset * sizeof(DWORD),
		numConstants * sizeof(DWORD),
		constants);
}


void CommandContextVK::SetConstant(uint32_t rootIndex, uint32_t offset, DWParam val)
{
	uint32_t v = get<uint32_t>(val.value);
	vkCmdPushConstants(
		m_commandBuffer,
		GetPipelineLayout(),
		m_shaderStages[rootIndex],
		offset * sizeof(uint32_t),
		sizeof(uint32_t),
		&v);
}


void CommandContextVK::SetConstants(uint32_t rootIndex, DWParam x)
{
	uint32_t v = get<uint32_t>(x.value);
	vkCmdPushConstants(
		m_commandBuffer,
		GetPipelineLayout(),
		m_shaderStages[rootIndex],
		0,
		sizeof(uint32_t),
		&v);
}


void CommandContextVK::SetConstants(uint32_t rootIndex, DWParam x, DWParam y)
{
	uint32_t val[] = { get<uint32_t>(x.value), get<uint32_t>(y.value) };
	vkCmdPushConstants(
		m_commandBuffer,
		GetPipelineLayout(),
		m_shaderStages[rootIndex],
		0,
		2 * sizeof(uint32_t),
		&val);
}


void CommandContextVK::SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z)
{
	uint32_t val[] = { get<uint32_t>(x.value), get<uint32_t>(y.value), get<uint32_t>(z.value) };
	vkCmdPushConstants(
		m_commandBuffer,
		GetPipelineLayout(),
		m_shaderStages[rootIndex],
		0,
		3 * sizeof(uint32_t),
		&val);
}


void CommandContextVK::SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w)
{
	uint32_t val[] = { get<uint32_t>(x.value), get<uint32_t>(y.value), get<uint32_t>(z.value), get<uint32_t>(w.value) };
	vkCmdPushConstants(
		m_commandBuffer,
		GetPipelineLayout(),
		m_shaderStages[rootIndex],
		0,
		4 * sizeof(uint32_t),
		&val);
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


void CommandContextVK::BeginRenderingBlock()
{
	assert(!m_isRendering);

	VkRenderingInfo renderingInfo{
		.sType					= VK_STRUCTURE_TYPE_RENDERING_INFO,
		.pNext					= nullptr,
		.flags					= 0,
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
}

} // namespace Luna::VK