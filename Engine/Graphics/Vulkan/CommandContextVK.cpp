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

#include "DescriptorSetManagerVK.h"
#include "DeviceManagerVK.h"
#include "QueueVK.h"
#include "ResourceManagerVK.h"
#include "RootSignatureManagerVK.h"
#include "VulkanUtil.h"

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
	auto handle = renderTarget.GetHandle();
	VkImageView imageView = GetVulkanResourceManager()->GetImageViewRtv(handle.get());

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
	auto resourceManager = GetVulkanResourceManager();
	auto handle = depthTarget.GetHandle();

	VkImageView imageView = resourceManager->GetImageViewDepth(handle.get(), depthStencilAspect);

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
		.storeOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.clearValue		= GetDepthStencilClearValue(depthTarget.GetClearDepth(), depthTarget.GetClearStencil())
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

	m_graphicsPipelineLayout = VK_NULL_HANDLE;
	m_computePipelineLayout = VK_NULL_HANDLE;
	m_graphicsPipeline = VK_NULL_HANDLE;
	m_computePipeline = VK_NULL_HANDLE;

	m_renderingArea.offset.x = -1;
	m_renderingArea.offset.y = -1;
	m_renderingArea.extent.width = 0;
	m_renderingArea.extent.height = 0;
}


// TODO - Move this logic into CommandManager::AllocateContext
void CommandContextVK::Initialize()
{
	assert(m_commandBuffer == VK_NULL_HANDLE);
	m_commandBuffer = GetVulkanDeviceManager()->GetQueue(m_type).RequestCommandBuffer();

	m_dynamicDescriptorHeap = make_unique<DefaultDynamicDescriptorHeap>(m_device.get());

	m_descriptorSetManager = GetVulkanDescriptorSetManager();
	m_resourceManager = GetVulkanResourceManager();
	m_rootSignatureManager = GetVulkanRootSignatureManager();
}


void CommandContextVK::BeginFrame()
{
	VkCommandBufferBeginInfo beginInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
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
	m_dynamicDescriptorHeap->CleanupUsedPools(fenceValue);

	if (bWaitForCompletion)
	{
		queue.WaitForFence(fenceValue);
	}

	return fenceValue;
}


void CommandContextVK::TransitionResource(ColorBuffer& colorBuffer, ResourceState newState, bool bFlushImmediate)
{
	auto handle = colorBuffer.GetHandle();

	TextureBarrier barrier{
		.image				= m_resourceManager->GetImage(handle.get()),
		.format				= FormatToVulkan(colorBuffer.GetFormat()),
		.imageAspect		= GetImageAspect(colorBuffer.GetFormat()),
		.beforeState		= colorBuffer.GetUsageState(),
		.afterState			= newState,
		.numMips			= colorBuffer.GetNumMips(),
		.mipLevel			= 0,
		.arraySizeOrDepth	= colorBuffer.GetArraySize(),
		.arraySlice			= 0,
		.bWholeTexture		= true
	};

	m_textureBarriers.push_back(barrier);

	colorBuffer.SetUsageState(newState);

	if (bFlushImmediate || GetPendingBarrierCount() >= 16)
	{
		FlushResourceBarriers();
	}
}


void CommandContextVK::TransitionResource(DepthBuffer& depthBuffer, ResourceState newState, bool bFlushImmediate)
{
	ResourceHandle handle = depthBuffer.GetHandle();

	TextureBarrier barrier{
		.image				= m_resourceManager->GetImage(handle.get()),
		.format				= FormatToVulkan(depthBuffer.GetFormat()),
		.imageAspect		= GetImageAspect(depthBuffer.GetFormat()),
		.beforeState		= depthBuffer.GetUsageState(),
		.afterState			= newState,
		.numMips			= depthBuffer.GetNumMips(),
		.mipLevel			= 0,
		.arraySizeOrDepth	= depthBuffer.GetArraySize(),
		.arraySlice			= 0,
		.bWholeTexture		= true
	};

	m_textureBarriers.push_back(barrier);

	depthBuffer.SetUsageState(newState);

	if (bFlushImmediate || GetPendingBarrierCount() >= 16)
	{
		FlushResourceBarriers();
	}
}


void CommandContextVK::TransitionResource(GpuBuffer& gpuBuffer, ResourceState newState, bool bFlushImmediate)
{
	auto handle = gpuBuffer.GetHandle();

	BufferBarrier barrier{
		.buffer			= m_resourceManager->GetBuffer(handle.get()),
		.beforeState	= gpuBuffer.GetUsageState(),
		.afterState		= newState,
		.size			= gpuBuffer.GetSize()
	};

	m_bufferBarriers.push_back(barrier);

	gpuBuffer.SetUsageState(newState);

	if (bFlushImmediate || GetPendingBarrierCount() >= 16)
	{
		FlushResourceBarriers();
	}
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


void CommandContextVK::ClearUAV(GpuBuffer& gpuBuffer)
{
	auto handle = gpuBuffer.GetHandle();

	uint32_t data = 0;
	vkCmdFillBuffer(m_commandBuffer, m_resourceManager->GetBuffer(handle.get()), 0, VK_WHOLE_SIZE, data);
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

	auto handle = colorBuffer.GetHandle();

	vkCmdClearColorImage(m_commandBuffer, m_resourceManager->GetImage(handle.get()), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &colVal, 1, &range);

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

	auto handle = depthBuffer.GetHandle();

	vkCmdClearDepthStencilImage(m_commandBuffer, m_resourceManager->GetImage(handle.get()), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &depthVal, 1, &range);

	TransitionResource(depthBuffer, oldState, false);
}


void CommandContextVK::BeginRendering(ColorBuffer& renderTarget)
{
	ResetRenderTargets();

	m_rtvs[0] = GetRenderingAttachmentInfo(renderTarget);
	m_numRtvs = 1;
	m_rtvFormats[0] = FormatToVulkan(renderTarget.GetFormat());

	SetRenderingArea(renderTarget);

	BeginRenderingBlock();
}


void CommandContextVK::BeginRendering(ColorBuffer& renderTarget, DepthBuffer& depthTarget, DepthStencilAspect depthStencilAspect)
{
	ResetRenderTargets();

	m_rtvs[0] = GetRenderingAttachmentInfo(renderTarget);
	m_numRtvs = 1;
	m_rtvFormats[0] = FormatToVulkan(renderTarget.GetFormat());

	m_dsv = GetRenderingAttachmentInfo(depthTarget, depthStencilAspect);
	m_hasDsv = true;
	m_dsvFormat = FormatToVulkan(depthTarget.GetFormat());

	SetRenderingArea(renderTarget);
	SetRenderingArea(depthTarget);

	BeginRenderingBlock();
}


void CommandContextVK::BeginRendering(DepthBuffer& depthTarget, DepthStencilAspect depthStencilAspect)
{
	ResetRenderTargets();

	m_dsv = GetRenderingAttachmentInfo(depthTarget, depthStencilAspect);
	m_hasDsv = true;
	m_dsvFormat = FormatToVulkan(depthTarget.GetFormat());

	SetRenderingArea(depthTarget);

	BeginRenderingBlock();
}


void CommandContextVK::BeginRendering(std::span<ColorBuffer> renderTargets)
{
	ResetRenderTargets();
	assert(renderTargets.size() <= 8);

	uint32_t i = 0;
	for (const ColorBuffer& renderTarget : renderTargets)
	{
		m_rtvs[i] = GetRenderingAttachmentInfo(renderTarget);
		m_rtvFormats[i] = FormatToVulkan(renderTarget.GetFormat());

		SetRenderingArea(renderTarget);

		++i;
	}
	m_numRtvs = (uint32_t)renderTargets.size();

	BeginRenderingBlock();
}


void CommandContextVK::BeginRendering(std::span<ColorBuffer> renderTargets, DepthBuffer& depthTarget, DepthStencilAspect depthStencilAspect)
{
	ResetRenderTargets();
	assert(renderTargets.size() <= 8);

	uint32_t i = 0;
	for (const ColorBuffer& renderTarget : renderTargets)
	{
		m_rtvs[i] = GetRenderingAttachmentInfo(renderTarget);
		m_rtvFormats[i] = FormatToVulkan(renderTarget.GetFormat());

		SetRenderingArea(renderTarget);

		++i;
	}
	m_numRtvs = (uint32_t)renderTargets.size();

	m_dsv = GetRenderingAttachmentInfo(depthTarget, depthStencilAspect);
	m_hasDsv = true;
	m_dsvFormat = FormatToVulkan(depthTarget.GetFormat());

	SetRenderingArea(depthTarget);

	BeginRenderingBlock();
}


void CommandContextVK::EndRendering()
{
	assert(m_isRendering);

	vkCmdEndRendering(m_commandBuffer);

	m_isRendering = false;
}


void CommandContextVK::SetRootSignature(RootSignature& rootSignature)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);

	VkPipelineLayout pipelineLayout = m_rootSignatureManager->GetPipelineLayout(rootSignature.GetHandle().get());

	if (m_type == CommandListType::Direct)
	{
		m_computePipelineLayout = VK_NULL_HANDLE;
		m_graphicsPipelineLayout = pipelineLayout;

		m_dynamicDescriptorHeap->ParseRootSignature(rootSignature, true);
	}
	else
	{
		m_graphicsPipelineLayout = VK_NULL_HANDLE;
		m_computePipelineLayout = pipelineLayout;

		m_dynamicDescriptorHeap->ParseRootSignature(rootSignature, false);
	}

	const uint32_t numRootParameters = rootSignature.GetNumRootParameters();
	for (uint32_t i = 0; i < numRootParameters; ++i)
	{
		const auto& rootParameter = rootSignature.GetRootParameter(i);
		// TODO: Store this in the RootSignatureManager so we don't have to do this conversion every frame.
		m_shaderStages[i] = ShaderStageToVulkan(rootParameter.shaderVisibility);
	}
}


void CommandContextVK::SetGraphicsPipeline(GraphicsPipelineState& graphicsPipeline)
{
	m_computePipelineLayout = VK_NULL_HANDLE;

	VkPipeline vkPipeline = m_resourceManager->GetGraphicsPipeline(graphicsPipeline.GetHandle().get());

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


void CommandContextVK::SetConstantBuffer(uint32_t rootIndex, const GpuBuffer& gpuBuffer)
{
	auto handle = gpuBuffer.GetHandle();
	m_dynamicDescriptorHeap->SetDescriptorBufferInfo(rootIndex, 0, m_resourceManager->GetBufferInfo(handle.get()), true);
}


void CommandContextVK::SetDescriptors(uint32_t rootIndex, DescriptorSet& descriptorSet)
{
	SetDescriptors_Internal(rootIndex, descriptorSet.GetHandle().get());
}


void CommandContextVK::SetResources(ResourceSet& resourceSet)
{
	const uint32_t numDescriptorSets = resourceSet.GetNumDescriptorSets();
	for (uint32_t i = 0; i < numDescriptorSets; ++i)
	{
		SetDescriptors_Internal(i, resourceSet[i].GetHandle().get());
	}
}


void CommandContextVK::SetSRV(uint32_t rootIndex, uint32_t offset, const ColorBuffer& colorBuffer)
{
	auto handle = colorBuffer.GetHandle();
	m_dynamicDescriptorHeap->SetDescriptorImageInfo(rootIndex, offset, m_resourceManager->GetImageInfoSrv(handle.get()), true);
}


void CommandContextVK::SetSRV(uint32_t rootIndex, uint32_t offset, const DepthBuffer& depthBuffer, bool depthSrv)
{
	auto handle = depthBuffer.GetHandle();
	m_dynamicDescriptorHeap->SetDescriptorImageInfo(rootIndex, offset, m_resourceManager->GetImageInfoDepth(handle.get(), depthSrv), true);
}


void CommandContextVK::SetSRV(uint32_t rootIndex, uint32_t offset, const GpuBuffer& gpuBuffer)
{
	auto handle = gpuBuffer.GetHandle();
	if (gpuBuffer.GetResourceType() == ResourceType::TypedBuffer)
	{
		m_dynamicDescriptorHeap->SetDescriptorBufferView(rootIndex, offset, m_resourceManager->GetBufferView(handle.get()), true);
	}
	else
	{
		m_dynamicDescriptorHeap->SetDescriptorBufferInfo(rootIndex, offset, m_resourceManager->GetBufferInfo(handle.get()), true);
	}
}


void CommandContextVK::SetUAV(uint32_t rootIndex, uint32_t offset, const ColorBuffer& colorBuffer)
{
	auto handle = colorBuffer.GetHandle();
	m_dynamicDescriptorHeap->SetDescriptorImageInfo(rootIndex, offset, m_resourceManager->GetImageInfoUav(handle.get()), true);
}


void CommandContextVK::SetUAV(uint32_t rootIndex, uint32_t offset, const DepthBuffer& depthBuffer)
{
	assert(false);
}


void CommandContextVK::SetUAV(uint32_t rootIndex, uint32_t offset, const GpuBuffer& gpuBuffer)
{
	auto handle = gpuBuffer.GetHandle();
	if (gpuBuffer.GetResourceType() == ResourceType::TypedBuffer)
	{
		m_dynamicDescriptorHeap->SetDescriptorBufferView(rootIndex, offset, m_resourceManager->GetBufferView(handle.get()), true);
	}
	else
	{
		m_dynamicDescriptorHeap->SetDescriptorBufferInfo(rootIndex, offset, m_resourceManager->GetBufferInfo(handle.get()), true);
	}
}


void CommandContextVK::SetCBV(uint32_t rootIndex, uint32_t offset, const GpuBuffer& gpuBuffer)
{
	auto handle = gpuBuffer.GetHandle();
	m_dynamicDescriptorHeap->SetDescriptorBufferInfo(rootIndex, offset, m_resourceManager->GetBufferInfo(handle.get()), true);
}


void CommandContextVK::SetIndexBuffer(const GpuBuffer& gpuBuffer)
{
	auto handle = gpuBuffer.GetHandle();

	const bool is16Bit = gpuBuffer.GetElementSize() == sizeof(uint16_t);
	const VkIndexType indexType = is16Bit ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
	vkCmdBindIndexBuffer(m_commandBuffer, m_resourceManager->GetBuffer(handle.get()), 0, indexType);
}


void CommandContextVK::SetVertexBuffer(uint32_t slot, const GpuBuffer& gpuBuffer)
{
	auto handle = gpuBuffer.GetHandle();

	VkDeviceSize offsets[1] = { 0 };
	VkBuffer buffers[1] = { m_resourceManager->GetBuffer(handle.get()) };
	vkCmdBindVertexBuffers(m_commandBuffer, slot, 1, buffers, offsets);
}


void CommandContextVK::DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
	uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
	FlushResourceBarriers();
	m_dynamicDescriptorHeap->UpdateAndBindDescriptorSets(m_commandBuffer, m_type == CommandListType::Direct);
	vkCmdDraw(m_commandBuffer, vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}


void CommandContextVK::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
	int32_t baseVertexLocation, uint32_t startInstanceLocation)
{
	FlushResourceBarriers();
	m_dynamicDescriptorHeap->UpdateAndBindDescriptorSets(m_commandBuffer, m_type == CommandListType::Direct);
	vkCmdDrawIndexed(m_commandBuffer, indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}


void CommandContextVK::InitializeBuffer_Internal(GpuBuffer& destBuffer, const void* bufferData, size_t numBytes, size_t offset)
{
	auto deviceManager = GetVulkanDeviceManager();
	auto stagingBuffer = CreateStagingBuffer(deviceManager->GetDevice(), deviceManager->GetAllocator(), bufferData, numBytes);

	// Copy from the upload buffer to the destination buffer
	TransitionResource(destBuffer, ResourceState::CopyDest, true);

	auto handle = destBuffer.GetHandle();

	VkBufferCopy copyRegion{ .size = numBytes };
	vkCmdCopyBuffer(m_commandBuffer, *stagingBuffer, m_resourceManager->GetBuffer(handle.get()), 1, &copyRegion);

	TransitionResource(destBuffer, ResourceState::GenericRead, true);

	GetVulkanDeviceManager()->ReleaseBuffer(stagingBuffer.get());
}


void CommandContextVK::SetDescriptors_Internal(uint32_t rootIndex, DescriptorSetHandleType* descriptorSetHandle)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);

	if (!m_descriptorSetManager->HasDescriptors(descriptorSetHandle))
		return;

	m_descriptorSetManager->UpdateGpuDescriptors(descriptorSetHandle);

	VkDescriptorSet vkDescriptorSet = m_descriptorSetManager->GetDescriptorSet(descriptorSetHandle);
	if (vkDescriptorSet == VK_NULL_HANDLE)
		return;

	const uint32_t dynamicOffset = m_descriptorSetManager->GetDynamicOffset(descriptorSetHandle);
	const bool isDynamicBuffer = m_descriptorSetManager->IsDynamicBuffer(descriptorSetHandle);

	vkCmdBindDescriptorSets(
		m_commandBuffer,
		(m_type == CommandListType::Direct) ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE,
		(m_type == CommandListType::Direct) ? m_graphicsPipelineLayout : m_computePipelineLayout,
		rootIndex,
		1,
		&vkDescriptorSet,
		isDynamicBuffer ? 1 : 0,
		isDynamicBuffer ? &dynamicOffset : nullptr);
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

} // namespace Luna::VK