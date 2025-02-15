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
	explicit CommandContext12(CommandListType type)
		: m_type{ type }
	{}

	~CommandContext12();

	void SetId(const std::string& id) override { m_id = id; }
	CommandListType GetType() const override { return m_type; }

	// Debug events and markers
	void BeginEvent(const std::string & label) override;
	void EndEvent() override;
	void SetMarker(const std::string& label) override;

	void Reset() override;
	void Initialize() override;

	void BeginFrame() override {}
	uint64_t Finish(bool bWaitForCompletion) override;

	void TransitionResource(GpuBuffer& gpuBuffer, ResourceState newState, bool bFlushImmediate) override;
	void TransitionResource(IGpuResource* gpuResource, ResourceState newState, bool bFlushImmediate) override;
	void InsertUAVBarrier(IGpuResource* colorBuffer, bool bFlushImmediate) override;
	void FlushResourceBarriers() override;

	// Graphics context
	void ClearUAV(GpuBuffer& gpuBuffer) override;
	//void ClearUAV(IColorBuffer* colorBuffer) override;
	void ClearColor(IColorBuffer* colorBuffer) override;
	void ClearColor(IColorBuffer* colorBuffer, Color clearColor) override;
	void ClearDepth(IDepthBuffer* depthBuffer) override;
	void ClearStencil(IDepthBuffer* depthBuffer) override;
	void ClearDepthAndStencil(IDepthBuffer* depthBuffer) override;

	void BeginRendering(IColorBuffer* renderTarget) override;
	void BeginRendering(IColorBuffer* renderTarget, IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect) override;
	void BeginRendering(IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect) override;
	void BeginRendering(std::span<IColorBuffer*> renderTargets) override;
	void BeginRendering(std::span<IColorBuffer*> renderTargets, IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect) override;
	void EndRendering() override;

	void SetRootSignature(RootSignature& rootSignature) override;
	void SetGraphicsPipeline(GraphicsPipelineState& graphicsPipeline) override;
	

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
	void SetDescriptors(uint32_t rootIndex, DescriptorSet& descriptorSet) override;
	void SetResources(ResourceSet& resourceSet) override;

	void SetIndexBuffer(const GpuBuffer& gpuBuffer) override;
	void SetVertexBuffer(uint32_t slot, const GpuBuffer& gpuBuffer) override;

	void DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
		uint32_t startVertexLocation, uint32_t startInstanceLocation) override;
	void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
		int32_t baseVertexLocation, uint32_t startInstanceLocation) override;

protected:
	void TransitionResource_Internal(ID3D12Resource* resource, D3D12_RESOURCE_STATES oldState, D3D12_RESOURCE_STATES newState, bool bFlushImmediate);
	void InsertUAVBarrier_Internal(ID3D12Resource* resource, bool bFlushImmediate);
	void InitializeBuffer_Internal(GpuBuffer& destBuffer, const void* bufferData, size_t numBytes, size_t offset) override;
	void SetDescriptors_Internal(uint32_t rootIndex, DescriptorSetHandleType* descriptorSetHandle);

private:
	void BindDescriptorHeaps();
	void BindRenderTargets();
	void ResetRenderTargets();

private:
	std::string m_id;
	CommandListType m_type;

	ID3D12GraphicsCommandList* m_commandList{ nullptr };
	ID3D12CommandAllocator* m_currentAllocator{ nullptr };

	ID3D12RootSignature* m_graphicsRootSignature{ nullptr };
	ID3D12RootSignature* m_computeRootSignature{ nullptr };
	ID3D12PipelineState* m_graphicsPipelineState{ nullptr };
	ID3D12PipelineState* m_computePipelineState{ nullptr };

	D3D12_PRIMITIVE_TOPOLOGY m_primitiveTopology;

	D3D12_RESOURCE_BARRIER m_resourceBarrierBuffer[16];
	uint32_t m_numBarriersToFlush{ 0 };

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