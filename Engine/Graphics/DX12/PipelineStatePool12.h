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
#include "Graphics\ResourcePool.h"
#include "Graphics\DX12\DirectXCommon.h"


namespace Luna::DX12
{

struct PipelineStateData
{
	wil::com_ptr<ID3D12PipelineState> pipelineState;
};


class PipelineStateFactory
{
public:
	void SetDevice(ID3D12Device* device) { m_device = device; }
	PipelineStateData Create(const GraphicsPipelineDesc& pipelineDesc);

private:
	wil::com_ptr<ID3D12Device> m_device;

	// Pipeline state map
	std::mutex m_pipelineStateMutex;
	std::map<size_t, wil::com_ptr<ID3D12PipelineState>> m_pipelineStateMap;
};


class PipelineStatePool 
	: public IPipelineStatePool
	, public ResourcePool1<PipelineStateFactory, GraphicsPipelineDesc, PipelineStateData, 4096>
{
public:
	explicit PipelineStatePool(ID3D12Device* device);
	~PipelineStatePool();

	// Create/Destroy pipeline state
	PipelineStateHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc) override;
	void DestroyHandle(PipelineStateHandleType* handle) override;

	// Platform agnostic getters
	const GraphicsPipelineDesc& GetDesc(PipelineStateHandleType* handle) const override;

	// Getters
	ID3D12PipelineState* GetPipelineState(PipelineStateHandleType* handle) const;

private:
	wil::com_ptr<ID3D12Device> m_device;
};


PipelineStatePool* const GetD3D12PipelineStatePool();


} // namespace Luna::DX12