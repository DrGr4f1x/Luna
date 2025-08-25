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

struct GraphicsPipelineContext
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC stateDesc{};
	std::unique_ptr<const D3D12_INPUT_ELEMENT_DESC> inputElements;
	size_t hashCode{ 0 };
};


void FillGraphicsPipelineDesc(GraphicsPipelineContext& context, const GraphicsPipelineDesc& desc);
void FillGraphicsPipelineStateStreamDefault(CD3DX12_PIPELINE_STATE_STREAM& stateStream, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& stateDesc, const GraphicsPipelineDesc& desc);
void FillGraphicsPipelineStateStream1(CD3DX12_PIPELINE_STATE_STREAM1& stateStream, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& stateDesc, const GraphicsPipelineDesc& desc);
void FillGraphicsPipelineStateStream2(CD3DX12_PIPELINE_STATE_STREAM2& stateStream, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& stateDesc, const GraphicsPipelineDesc& desc);

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 606)
void FillGraphicsPipelineStateStream3(CD3DX12_PIPELINE_STATE_STREAM3& stateStream, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& stateDesc, const GraphicsPipelineDesc& desc);
#endif

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 608)
void FillGraphicsPipelineStateStream4(CD3DX12_PIPELINE_STATE_STREAM4& stateStream, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& stateDesc, const GraphicsPipelineDesc& desc);
#endif

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 610)
void FillGraphicsPipelineStateStream5(CD3DX12_PIPELINE_STATE_STREAM5& stateStream, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& stateDesc, const GraphicsPipelineDesc& desc);
#endif

} // namespace Luna::DX12