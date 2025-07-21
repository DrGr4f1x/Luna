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
#include "Graphics\Vulkan\LinearAllocatorVK.h"

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
	bool bUAVBarrier{ false };
};


struct BufferBarrier
{
	VkBuffer buffer{ VK_NULL_HANDLE };
	ResourceState beforeState{ ResourceState::Undefined };
	ResourceState afterState{ ResourceState::Undefined };
	size_t size{ 0 };
	bool bUAVBarrier{ false };
};


class __declspec(uuid("63C5358D-F31C-43DA-90DA-8676E272BE4A")) CommandContextVK final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ICommandContext>
{
public:
	explicit CommandContextVK(CVkDevice* device, CommandListType type)
		: m_device{ device }
		, m_commandListType{ type }
	{}
	~CommandContextVK() = default;

	void SetId(const std::string& id) override { m_id = id; }
	CommandListType GetType() const override { return m_commandListType; }

	// Debug events and markers
	void BeginEvent(const std::string& label) override;
	void EndEvent() override;
	void SetMarker(const std::string& label) override;

	void Reset() override;
	void Initialize() override;

	void BeginFrame() override;
	uint64_t Finish(bool bWaitForCompletion) override;

	void TransitionResource(IColorBuffer* colorBuffer, ResourceState newState, bool bFlushImmediate) override;
	void TransitionResource(IDepthBuffer* depthBuffer, ResourceState newSTate, bool bFlushImmediate) override;
	void TransitionResource(IGpuBuffer* gpuBuffer, ResourceState newState, bool bFlushImmediate) override;
	void TransitionResource(ITexture* texture, ResourceState newState, bool bFlushImmediate) override;
	void InsertUAVBarrier(const IColorBuffer* colorBuffer, bool bFlushImmediate) override;
	void InsertUAVBarrier(const IGpuBuffer* gpuBuffer, bool bFlushImmediate) override;
	void FlushResourceBarriers() override;

	DynAlloc ReserveUploadMemory(size_t sizeInBytes) override;

	void ClearUAV(IGpuBuffer* gpuBuffer) override;
	//void ClearUAV(IColorBuffer* colorBuffer) override;
	void ClearColor(IColorBuffer* colorBuffer) override;
	void ClearColor(IColorBuffer* colorBuffer, Color clearColor) override;
	void ClearDepth(IDepthBuffer* depthBuffer) override;
	void ClearStencil(IDepthBuffer* depthBuffer) override;
	void ClearDepthAndStencil(IDepthBuffer* depthBuffer) override;

	void BeginRendering(const IColorBuffer* colorBuffer) override;
	void BeginRendering(const IColorBuffer* colorBuffer, const IDepthBuffer* depthBuffer, DepthStencilAspect depthStencilAspect) override;
	void BeginRendering(const IDepthBuffer* depthBuffer, DepthStencilAspect depthStencilAspect) override;
	void BeginRendering(std::span<const IColorBuffer*> colorBuffers) override;
	void BeginRendering(std::span<const IColorBuffer*> colorBuffers, const IDepthBuffer* depthBuffer, DepthStencilAspect depthStencilAspect) override;
	void EndRendering() override;

	void BeginQuery(const IQueryHeap* queryHeap, uint32_t heapIndex) override;
	void EndQuery(const IQueryHeap* queryHeap, uint32_t heapIndex) override;
	void ResolveQueries(const IQueryHeap* queryHeap, uint32_t startIndex, uint32_t numQueries, const IGpuBuffer* destBuffer, uint64_t destBufferOffset) override;
	void ResetQueries(const IQueryHeap* queryHeap, uint32_t startIndex, uint32_t numQueries) override;

	void SetRootSignature(CommandListType type, const IRootSignature* rootSignature) override;
	void SetGraphicsPipeline(const IGraphicsPipeline* graphicsPipeline) override;
	void SetComputePipeline(const IComputePipeline* computePipeline) override;

	void SetViewport(float x, float y, float w, float h, float minDepth = 0.0f, float maxDepth = 1.0f) override;
	void SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom) override;
	void SetStencilRef(uint32_t stencilRef) override;
	void SetBlendFactor(Color blendFactor) override;
	void SetPrimitiveTopology(PrimitiveTopology topology) override;

	void SetConstantArray(CommandListType type, uint32_t rootIndex, uint32_t numConstants, const void* constants, uint32_t offset) override;
	void SetConstant(CommandListType type, uint32_t rootIndex, uint32_t offset, DWParam val) override;
	void SetConstants(CommandListType type, uint32_t rootIndex, DWParam x) override;
	void SetConstants(CommandListType type, uint32_t rootIndex, DWParam x, DWParam y) override;
	void SetConstants(CommandListType type, uint32_t rootIndex, DWParam x, DWParam y, DWParam z) override;
	void SetConstants(CommandListType type, uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w) override;
	void SetConstantBuffer(CommandListType type, uint32_t rootIndex, const IGpuBuffer* gpuBuffer) override;
	void SetDescriptors(CommandListType type, uint32_t rootIndex, IDescriptorSet* descriptorSet) override;
	void SetResources(CommandListType type, ResourceSet& resourceSet) override;

	void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t offset, ColorBufferPtr& colorBuffer) override;
	void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t offset, DepthBufferPtr& depthBuffer, bool depthSrv) override;
	void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer) override;
	void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t offset, TexturePtr& texture) override;

	void SetUAV(CommandListType type, uint32_t rootIndex, uint32_t offset, ColorBufferPtr& colorBuffer) override;
	void SetUAV(CommandListType type, uint32_t rootIndex, uint32_t offset, DepthBufferPtr& depthBuffer) override;
	void SetUAV(CommandListType type, uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer) override;

	void SetCBV(CommandListType type, uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer) override;

	void SetIndexBuffer(const IGpuBuffer* gpuBuffer) override;
	void SetVertexBuffer(uint32_t slot, const IGpuBuffer* gpuBuffer) override;
	void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexStride, DynAlloc dynAlloc) override;
	void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexStride, const void* data) override;
	void SetDynamicIndexBuffer(uint32_t indexCount, bool indexSize16Bit, DynAlloc dynAlloc) override;
	void SetDynamicIndexBuffer(uint32_t indexCount, bool indexSize16Bit, const void* data) override;

	void DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
		uint32_t startVertexLocation, uint32_t startInstanceLocation) override;
	void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
		int32_t baseVertexLocation, uint32_t startInstanceLocation) override;

	void Resolve(const IColorBuffer* srcBuffer, const IColorBuffer* destBuffer, Format format) override;

	void Dispatch(uint32_t groupCountX = 1, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) override;
	void Dispatch1D(uint32_t threadCountX, uint32_t groupSizeX = 64) override;
	void Dispatch2D(uint32_t threadCountX, uint32_t threadCountY, uint32_t groupSizeX = 8, uint32_t groupSizeY = 8) override;
	void Dispatch3D(uint32_t threadCountX, uint32_t threadCountY, uint32_t threadCountZ, uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ) override;

private:
	void ClearDepthAndStencil_Internal(IDepthBuffer* depthBuffer, VkImageAspectFlags flags);
	void InitializeBuffer_Internal(IGpuBuffer* destBuffer, const void* bufferData, size_t numBytes, size_t offset) override;
	void InitializeTexture_Internal(ITexture* texture, const TextureInitializer& texInit) override;
	void SetDescriptors_Internal(CommandListType type, uint32_t rootIndex, IDescriptorSet* descriptorSet);

	void BindDescriptorHeaps() {}
	void SetRenderingArea(const ColorBuffer& colorBuffer);
	void SetRenderingArea(const DepthBuffer& depthBuffer);
	void BeginRenderingBlock();
	void ResetRenderTargets();

	void ParseRootSignature(CommandListType type);
	void MarkDescriptorsDirty(CommandListType type);
	bool HasDirtyDescriptors(CommandListType type);
	void ClearDirtyDescriptors(CommandListType type);

	size_t GetPendingBarrierCount() const noexcept { return m_textureBarriers.size() + m_bufferBarriers.size(); }

	VkPipelineLayout GetPipelineLayout(CommandListType type) const noexcept { return (type == CommandListType::Compute) ? m_computePipelineLayout : m_graphicsPipelineLayout; }

private:
	std::string m_id;
	wil::com_ptr<CVkDevice> m_device;
	CommandListType m_commandListType;

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
	VkPrimitiveTopology m_primitiveTopology{ VK_PRIMITIVE_TOPOLOGY_MAX_ENUM };

	const IRootSignature* m_graphicsRootSignature{ nullptr };
	const IRootSignature* m_computeRootSignature{ nullptr };
	bool m_isComputeRootSignatureParsed{ false };
	bool m_isGraphicsRootSignatureParsed{ false };
	bool m_hasDirtyGraphicsDescriptors{ false };
	bool m_hasDirtyComputeDescriptors{ false };

	LinearAllocator m_cpuLinearAllocator;
};

} // namespace Luna::VK