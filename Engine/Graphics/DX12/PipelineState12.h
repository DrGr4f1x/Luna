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

// Forward declarations
class Device;


class GraphicsPipeline : public IGraphicsPipeline
{ 
	friend class Device;

public:
	ID3D12PipelineState* GetPipelineState() const { return m_pipelineState.get(); }

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<ID3D12PipelineState> m_pipelineState;
};


class ComputePipeline : public IComputePipeline
{
	friend class Device;

public:ID3D12PipelineState* GetPipelineState() const { return m_pipelineState.get(); }

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<ID3D12PipelineState> m_pipelineState;
};

} // namespace Luna::DX12
