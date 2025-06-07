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


namespace Luna
{

// Forward declarations
class IResourceManager;

} // namespace Luna


namespace Luna::DX12
{

struct GraphicsPipelineStateRecord
{
	std::weak_ptr<ResourceHandleType> weakHandle;
	std::atomic<bool> isReady{ false };
};


class PipelineStateFactory : public PipelineStateFactoryBase
{
public:
	PipelineStateFactory(IResourceManager* owner, ID3D12Device* device);
	
	ResourceHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc);
	void Destroy(uint32_t index);

	const GraphicsPipelineDesc& GetDesc(uint32_t index) const;

	ID3D12PipelineState* GetGraphicsPipelineState(uint32_t index) const;

private:
	void ResetGraphicsPipelineState(uint32_t index)
	{
		m_graphicsPipelineStates[index].reset();
	}

	void ResetHash(uint32_t index)
	{
		m_hashList[index] = 0;
	}

private:
	IResourceManager* m_owner{ nullptr };
	wil::com_ptr<ID3D12Device> m_device;

	std::mutex m_mutex;

	std::queue<uint32_t> m_freeList;

	std::map<size_t, std::unique_ptr<GraphicsPipelineStateRecord>> m_hashToRecordMap;

	// Pipeline state objects
	std::array<wil::com_ptr<ID3D12PipelineState>, MaxResources> m_graphicsPipelineStates;

	// Hash keys
	std::array<size_t, MaxResources> m_hashList;
};

} // namespace Luna::DX12