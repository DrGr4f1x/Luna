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

	void TransitionResource(ColorBuffer& colorBuffer, ResourceState newState, bool bFlushImmediate) override;
	void TransitionResource(DepthBuffer& depthBuffer, ResourceState newState, bool bFlushImmediate) override;
	void InsertUAVBarrier(ColorBuffer& colorBuffer, bool bFlushImmediate) override;
	void InsertUAVBarrier(DepthBuffer& depthBuffer, bool bFlushImmediate);
	void FlushResourceBarriers() override;

	// Graphics context
	void ClearColor(ColorBuffer& colorBuffer) override;
	void ClearColor(ColorBuffer& colorBuffer, Color clearColor) override;
	void ClearDepth(DepthBuffer& depthBuffer) override;
	void ClearStencil(DepthBuffer& depthBuffer) override;
	void ClearDepthAndStencil(DepthBuffer& depthBuffer) override;

private:
	void TransitionResource_Internal(GpuResource& gpuResource, ResourceState newState, bool bFlushImmediate);
	void InsertUAVBarrier_Internal(GpuResource& gpuResource, bool bFlushImmediate);
	void BindDescriptorHeaps();

private:
	std::string m_id;
	CommandListType m_type;

	ID3D12GraphicsCommandList* m_commandList{ nullptr };
	ID3D12CommandAllocator* m_currentAllocator{ nullptr };

	ID3D12RootSignature* m_curGraphicsRootSignature{ nullptr };
	ID3D12RootSignature* m_curComputeRootSignature{ nullptr };
	ID3D12PipelineState* m_curPipelineState{ nullptr };

	D3D12_RESOURCE_BARRIER m_resourceBarrierBuffer[16];
	uint32_t m_numBarriersToFlush{ 0 };

	bool m_bHasPendingDebugEvent{ false };
};


} // namespace Luna::DX12