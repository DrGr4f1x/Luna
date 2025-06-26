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
#include "Graphics\Vulkan\DynamicDescriptorHeapVK.h"

using namespace Microsoft::WRL;


namespace Luna::VK
{

// Forward declarations
class ColorBuffer;
class ComputeContext;
class DepthBuffer;
class GpuBuffer;
class GraphicsContext;
class IDynamicDescriptorHeap;


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
	VkBuffer buffer{ VK_NULL_HANDLE };
	ResourceState beforeState{ ResourceState::Undefined };
	ResourceState afterState{ ResourceState::Undefined };
	size_t size{ 0 };
};


class __declspec(uuid("63C5358D-F31C-43DA-90DA-8676E272BE4A")) CommandContextVK final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ICommandContext>
{
public:
	explicit CommandContextVK(CVkDevice* device, CommandListType type)
		: m_device{ device }
		, m_type{ type }
	{}
	~CommandContextVK() = default;

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

	void TransitionResource(ColorBufferPtr colorBuffer, ResourceState newState, bool bFlushImmediate) override;
	void TransitionResource(DepthBufferPtr depthBuffer, ResourceState newSTate, bool bFlushImmediate) override;
	void TransitionResource(GpuBufferPtr gpuBuffer, ResourceState newState, bool bFlushImmediate) override;
	void FlushResourceBarriers() override;

	void ClearUAV(GpuBufferPtr gpuBuffer) override;
	//void ClearUAV(ColorBufferPtr colorBuffer) override;
	void ClearColor(ColorBufferPtr colorBuffer) override;
	void ClearColor(ColorBufferPtr colorBuffer, Color clearColor) override;
	void ClearDepth(DepthBufferPtr depthBuffer) override;
	void ClearStencil(DepthBufferPtr depthBuffer) override;
	void ClearDepthAndStencil(DepthBufferPtr depthBuffer) override;

	void BeginRendering(IColorBuffer* colorBuffer) override;
	void BeginRendering(IColorBuffer* colorBuffer, IDepthBuffer* depthBuffer, DepthStencilAspect depthStencilAspect) override;
	void BeginRendering(IDepthBuffer* depthBuffer, DepthStencilAspect depthStencilAspect) override;
	void BeginRendering(std::span<IColorBuffer*> colorBuffers) override;
	void BeginRendering(std::span<IColorBuffer*> colorBuffers, IDepthBuffer* depthBuffer, DepthStencilAspect depthStencilAspect) override;
	void EndRendering() override;

	void SetRootSignature(IRootSignature* rootSignature) override;
	void SetGraphicsPipeline(IGraphicsPipelineState* graphicsPipeline) override;

	void SetViewport(float x, float y, float w, float h, float minDepth = 0.0f, float maxDepth = 1.0f) override;
	void SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom) override;
	void SetStencilRef(uint32_t stencilRef) override;
	void SetBlendFactor(Color blendFactor) override;
	void SetPrimitiveTopology(PrimitiveTopology topology) override;

	void SetConstantArray(uint32_t rootIndex, uint32_t numConstants, const void* constants, uint32_t offset) override;
	void SetConstant(uint32_t rootIndex, uint32_t offset, DWParam val) override;
	void SetConstants(uint32_t rootIndex, DWParam x) override;
	void SetConstants(uint32_t rootIndex, DWParam x, DWParam y) override;
	void SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z) override;
	void SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w) override;
	void SetConstantBuffer(uint32_t rootIndex, const IGpuBuffer* gpuBuffer) override;
	void SetDescriptors(uint32_t rootIndex, IDescriptorSet* descriptorSet) override;
	void SetResources(ResourceSet& resourceSet) override;

	void SetSRV(uint32_t rootIndex, uint32_t offset, const IColorBuffer* colorBuffer) override;
	void SetSRV(uint32_t rootIndex, uint32_t offset, const IDepthBuffer* depthBuffer, bool depthSrv) override;
	void SetSRV(uint32_t rootIndex, uint32_t offset, const IGpuBuffer* gpuBuffer) override;

	void SetUAV(uint32_t rootIndex, uint32_t offset, const IColorBuffer* colorBuffer) override;
	void SetUAV(uint32_t rootIndex, uint32_t offset, const IDepthBuffer* depthBuffer) override;
	void SetUAV(uint32_t rootIndex, uint32_t offset, const IGpuBuffer* gpuBuffer) override;

	void SetCBV(uint32_t rootIndex, uint32_t offset, const IGpuBuffer* gpuBuffer) override;

	void SetIndexBuffer(const IGpuBuffer* gpuBuffer) override;
	void SetVertexBuffer(uint32_t slot, const IGpuBuffer* gpuBuffer) override;

	void DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
		uint32_t startVertexLocation, uint32_t startInstanceLocation) override;
	void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
		int32_t baseVertexLocation, uint32_t startInstanceLocation) override;

private:
	void ClearDepthAndStencil_Internal(DepthBufferPtr depthBuffer, VkImageAspectFlags flags);
	void InitializeBuffer_Internal(GpuBufferPtr destBuffer, const void* bufferData, size_t numBytes, size_t offset) override;
	void SetDescriptors_Internal(uint32_t rootIndex, IDescriptorSet* descriptorSet);

	void BindDescriptorHeaps() {}
	void SetRenderingArea(const ColorBuffer& colorBuffer);
	void SetRenderingArea(const DepthBuffer& depthBuffer);
	void BeginRenderingBlock();
	void ResetRenderTargets();

	size_t GetPendingBarrierCount() const noexcept { return m_textureBarriers.size() + m_bufferBarriers.size(); }

	VkPipelineLayout GetPipelineLayout() const noexcept { return (m_type == CommandListType::Compute) ? m_computePipelineLayout : m_graphicsPipelineLayout; }

private:
	std::string m_id;
	wil::com_ptr<CVkDevice> m_device;
	CommandListType m_type;

	// Dynamic descriptor heap
	std::unique_ptr<IDynamicDescriptorHeap> m_dynamicDescriptorHeap;

	VkCommandBuffer m_commandBuffer{ VK_NULL_HANDLE };

	bool m_bInvertedViewport{ true };
	bool m_hasPendingDebugEvent{ false };
	bool m_isRendering{ false };

	// Resource barriers
	std::vector<TextureBarrier> m_textureBarriers;
	std::vector<BufferBarrier> m_bufferBarriers;
	std::vector<VkMemoryBarrier2> m_memoryBarriers;
	std::vector<VkBufferMemoryBarrier2> m_bufferMemoryBarriers;
	std::vector<VkImageMemoryBarrier2> m_imageMemoryBarriers;

	// Render target state
	std::array<VkRenderingAttachmentInfo, 8> m_rtvs;
	std::array<VkFormat, 8> m_rtvFormats;
	uint32_t m_numRtvs{ 0 };
	VkRenderingAttachmentInfo m_dsv;
	VkFormat m_dsvFormat{ VK_FORMAT_UNDEFINED };
	bool m_hasDsv{ false };
	VkRect2D m_renderingArea;

	// Pipeline state
	VkPipelineLayout m_graphicsPipelineLayout{ VK_NULL_HANDLE };
	VkPipelineLayout m_computePipelineLayout{ VK_NULL_HANDLE };
	VkPipeline m_graphicsPipeline{ VK_NULL_HANDLE };
	VkPipeline m_computePipeline{ VK_NULL_HANDLE };
	std::array<VkShaderStageFlags, MaxRootParameters> m_shaderStages;
};

} // namespace Luna::VK