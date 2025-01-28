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

using namespace Microsoft::WRL;


namespace Luna::DX12
{

struct GraphicsPSODescExt
{
	ID3D12PipelineState* pipelineState{ nullptr };
};


class __declspec(uuid("C902EA2D-C89A-48B1-A6C7-09388F259903")) IGraphicsPSOData : public IPlatformData
{
public:
	virtual ID3D12PipelineState* GetPipelineState() const noexcept = 0;
};


class __declspec(uuid("A36110A6-BE99-483B-9C30-B749AE3D5A59")) GraphicsPSOData final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IGraphicsPSOData, IPlatformData>>
	, NonCopyable
{
public:
	explicit GraphicsPSOData(const GraphicsPSODescExt& descExt);

	ID3D12PipelineState* GetPipelineState() const noexcept override { return m_pipelineState.get(); }

private:
	wil::com_ptr<ID3D12PipelineState> m_pipelineState;
};

} // namespace Luna::DX12