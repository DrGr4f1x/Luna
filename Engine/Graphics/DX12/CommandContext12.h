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
#include "Graphics\DX12\DirectXCommon.h"
#include "Graphics\DX12\DynamicDescriptorHeap12.h"
#include "Graphics\DX12\LinearAllocator12.h"


using namespace Microsoft::WRL;


namespace Luna
{

// Forward declarations
class DescriptorSetHandleType;

} // namespace Luna


namespace Luna::DX12
{

class __declspec(uuid("D4B45425-3264-4D8E-8926-2AE73837C14C")) CommandContext12 final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ICommandContext>
{
public:
	explicit CommandContext12(CommandListType type);

	~CommandContext12();

	void SetId(const std::string& id) override { m_id = id; }
	CommandListType GetType() const override { return m_commandListType; }

	// Debug events and markers
	void BeginEvent(const std::string& label) override;
	void EndEvent() override;
	void SetMarker(const std::string& label) override;

	void Reset() override;
	void Initialize() override;

	void BeginFrame() override {}
	uint64_t Finish(bool bWaitForCompletion) override;

	void TransitionResource(ColorBufferPtr& colorBuffer, ResourceState newState, bool bFlushImmediate) override;
	void TransitionResource(DepthBufferPtr& depthBuffer, ResourceState newState, bool bFlushImmediate) override;
	void TransitionResource(GpuBufferPtr& gpuBuffer, ResourceState newState, bool bFlushImmediate) override;
	void TransitionResource(TexturePtr& texture, ResourceState newState, bool bFlushImmediate) override;
	void FlushResourceBarriers() override;

	DynAlloc ReserveUploadMemory(size_t sizeInBytes) override;

	// Graphics context
	void ClearUAV(GpuBufferPtr& gpuBuffer) override;
	//void ClearUAV(ColorBufferPtr& colorBuffer) override;
	void ClearColor(ColorBufferPtr& colorBuffer) override;
	void ClearColor(ColorBufferPtr& colorBuffer, Color clearColor) override;
	void ClearDepth(DepthBufferPtr& depthBuffer) override;
	void ClearStencil(DepthBufferPtr& depthBuffer) override;
	void ClearDepthAndStencil(DepthBufferPtr& depthBuffer) override;

	void BeginRendering(ColorBufferPtr& renderTarget) override;
	void BeginRendering(ColorBufferPtr& renderTarget, DepthBufferPtr& depthTarget, DepthStencilAspect depthStencilAspect) override;
	void BeginRendering(DepthBufferPtr& depthTarget, DepthStencilAspect depthStencilAspect) override;
	void BeginRendering(std::span<ColorBufferPtr>& renderTargets) override;
	void BeginRendering(std::span<ColorBufferPtr>& renderTargets, DepthBufferPtr& depthTarget, DepthStencilAspect depthStencilAspect) override;
	void EndRendering() override;

	void BeginOcclusionQuery(QueryHeapPtr& queryHeap, uint32_t heapIndex) override;
	void EndOcclusionQuery(QueryHeapPtr& queryHeap, uint32_t heapIndex) override;
	void ResolveOcclusionQueries(QueryHeapPtr& queryHeap, uint32_t startIndex, uint32_t numQueries, GpuBufferPtr& destBuffer, uint64_t destBufferOffset) override;
	void ResetOcclusionQueries(QueryHeapPtr& queryHeap, uint32_t startIndex, uint32_t numQueries) override;

	void SetRootSignature(CommandListType type, RootSignaturePtr& rootSignature) override;
	void SetGraphicsPipeline(GraphicsPipelineStatePtr& graphicsPipeline) override;
	void SetComputePipeline(ComputePipelineStatePtr& computePipeline) override;

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
	void SetConstantBuffer(CommandListType type, uint32_t rootIndex, GpuBufferPtr& gpuBuffer) override;
	void SetDescriptors(CommandListType type, uint32_t rootIndex, DescriptorSetPtr& descriptorSet) override;
	void SetResources(CommandListType type, ResourceSet& resourceSet) override;

	void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t offset, ColorBufferPtr& colorBuffer) override;
	void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t offset, DepthBufferPtr& depthBuffer, bool depthSrv) override;
	void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer) override;
	void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t offset, TexturePtr& texture) override;

	void SetUAV(CommandListType type, uint32_t rootIndex, uint32_t offset, ColorBufferPtr& colorBuffer) override;
	void SetUAV(CommandListType type, uint32_t rootIndex, uint32_t offset, DepthBufferPtr& depthBuffer) override;
	void SetUAV(CommandListType type, uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer) override;

	void SetCBV(CommandListType type, uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer) override;

	void SetIndexBuffer(GpuBufferPtr& gpuBuffer) override;
	void SetVertexBuffer(uint32_t slot, GpuBufferPtr& gpuBuffer) override;
	void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexStride, DynAlloc dynAlloc) override;
	void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexStride, const void* data) override;
	void SetDynamicIndexBuffer(uint32_t indexCount, bool indexSize16Bit, DynAlloc dynAlloc) override;
	void SetDynamicIndexBuffer(uint32_t indexCount, bool indexSize16Bit, const void* data) override;

	void DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
		uint32_t startVertexLocation, uint32_t startInstanceLocation) override;
	void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
		int32_t baseVertexLocation, uint32_t startInstanceLocation) override;

	// Compute context
	void Dispatch(uint32_t groupCountX = 1, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) override;
	void Dispatch1D(uint32_t threadCountX, uint32_t groupSizeX = 64) override;
	void Dispatch2D(uint32_t threadCountX, uint32_t threadCountY, uint32_t groupSizeX = 8, uint32_t groupSizeY = 8) override;
	void Dispatch3D(uint32_t threadCountX, uint32_t threadCountY, uint32_t threadCountZ, uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ) override;

	// Platform-specific functions
	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heapPtr);
	void SetDescriptorHeaps(uint32_t heapCount, D3D12_DESCRIPTOR_HEAP_TYPE types[], ID3D12DescriptorHeap* heapPtrs[]);

protected:
	void TransitionResource_Internal(ID3D12Resource* resource, D3D12_RESOURCE_STATES oldState, D3D12_RESOURCE_STATES newState, bool bFlushImmediate);
	void InsertUAVBarrier_Internal(ID3D12Resource* resource, bool bFlushImmediate);
	void InitializeBuffer_Internal(GpuBufferPtr& destBuffer, const void* bufferData, size_t numBytes, size_t offset) override;
	void InitializeTexture_Internal(TexturePtr& destTexture, const TextureInitializer& texInit) override;
	void SetDescriptors_Internal(CommandListType type, uint32_t rootIndex, DescriptorSetPtr& descriptorSet);
	void SetDynamicDescriptors_Internal(CommandListType type, uint32_t rootIndex, uint32_t offset, uint32_t numDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE handles[]);

private:
	void BindDescriptorHeaps();
	void BindRenderTargets();
	void ResetRenderTargets();

private:
	std::string m_id;
	CommandListType m_commandListType;

	ID3D12GraphicsCommandList* m_commandList{ nullptr };
	ID3D12CommandAllocator* m_currentAllocator{ nullptr };

	ID3D12RootSignature* m_graphicsRootSignature{ nullptr };
	ID3D12RootSignature* m_computeRootSignature{ nullptr };
	ID3D12PipelineState* m_graphicsPipelineState{ nullptr };
	ID3D12PipelineState* m_computePipelineState{ nullptr };

	DynamicDescriptorHeap m_dynamicViewDescriptorHeap;
	DynamicDescriptorHeap m_dynamicSamplerDescriptorHeap;

	LinearAllocator m_cpuLinearAllocator;

	D3D12_PRIMITIVE_TOPOLOGY m_primitiveTopology;

	D3D12_RESOURCE_BARRIER m_resourceBarrierBuffer[16];
	uint32_t m_numBarriersToFlush{ 0 };

	ID3D12DescriptorHeap* m_currentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	bool m_bHasPendingDebugEvent{ false };

	// Render target state
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 8> m_rtvs;
	D3D12_CPU_DESCRIPTOR_HANDLE m_dsv;
	uint32_t m_numRtvs{ 0 };
	bool m_hasDsv{ false };
	std::array<DXGI_FORMAT, 8> m_rtvFormats;
	DXGI_FORMAT m_dsvFormat{ DXGI_FORMAT_UNKNOWN };

	bool m_isRendering{ false };
};


} // namespace Luna::DX12