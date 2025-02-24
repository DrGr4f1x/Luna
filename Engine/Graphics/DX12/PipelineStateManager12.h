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

#include "Graphics\PipelineState.h"
#include "Graphics\DX12\DirectXCommon.h"


namespace Luna::DX12
{

class PipelineStateManager : public IPipelineStateManager
{
	static const uint32_t MaxItems = (1 << 12);

public:
	explicit PipelineStateManager(ID3D12Device* device);
	~PipelineStateManager();

	// Create/Destroy pipeline state
	PipelineStateHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc) override;
	void DestroyHandle(PipelineStateHandleType* handle) override;

	// Platform agnostic getters
	const GraphicsPipelineDesc& GetDesc(PipelineStateHandleType* handle) const override;

	// Getters
	ID3D12PipelineState* GetPipelineState(PipelineStateHandleType* handle) const;

private:
	wil::com_ptr<ID3D12PipelineState> FindOrCreateGraphicsPipelineState(const GraphicsPipelineDesc& pipelineDesc);

private:
	wil::com_ptr<ID3D12Device> m_device;

	// Allocation mutex
	std::mutex m_allocationMutex;

	// Free list
	std::queue<uint32_t> m_freeList;

	// Hot data: ID3D12PipelineState
	std::array<wil::com_ptr<ID3D12PipelineState>, MaxItems> m_pipelines;

	// Cold data: GraphicsPipelineDesc
	std::array<GraphicsPipelineDesc, MaxItems> m_descs;

	// Pipeline state map
	std::mutex m_pipelineStateMutex;
	std::map<size_t, wil::com_ptr<ID3D12PipelineState>> m_pipelineStateMap;
};


PipelineStateManager* const GetD3D12PipelineStateManager();


} // namespace Luna::DX12