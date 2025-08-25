//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Stdafx.h"

#include "PipelineStateUtil12.h"

#include "Graphics\DX12\RootSignature12.h"
#include "Graphics\DX12\Shader12.h"

using namespace std;


namespace Luna::DX12
{

inline bool IsDepthBiasEnabled(const D3D12_RASTERIZER_DESC& desc)
{
	return desc.DepthBias != 0 || desc.SlopeScaledDepthBias != 0.0f;
}


void FillBlendDesc(D3D12_BLEND_DESC& blendDesc, const BlendStateDesc& desc)
{
	blendDesc.AlphaToCoverageEnable = desc.alphaToCoverageEnable ? TRUE : FALSE;
	blendDesc.IndependentBlendEnable = desc.independentBlendEnable ? TRUE : FALSE;

	for (uint32_t i = 0; i < 8; ++i)
	{
		auto& rtDesc = blendDesc.RenderTarget[i];
		const auto& renderTargetBlend = desc.renderTargetBlend[i];

		rtDesc.BlendEnable = renderTargetBlend.blendEnable ? TRUE : FALSE;
		rtDesc.LogicOpEnable = renderTargetBlend.logicOpEnable ? TRUE : FALSE;
		rtDesc.SrcBlend = BlendToDX12(renderTargetBlend.srcBlend);
		rtDesc.DestBlend = BlendToDX12(renderTargetBlend.dstBlend);
		rtDesc.BlendOp = BlendOpToDX12(renderTargetBlend.blendOp);
		rtDesc.SrcBlendAlpha = BlendToDX12(renderTargetBlend.srcBlendAlpha);
		rtDesc.DestBlendAlpha = BlendToDX12(renderTargetBlend.dstBlendAlpha);
		rtDesc.BlendOpAlpha = BlendOpToDX12(renderTargetBlend.blendOpAlpha);
		rtDesc.LogicOp = LogicOpToDX12(renderTargetBlend.logicOp);
		rtDesc.RenderTargetWriteMask = ColorWriteToDX12(renderTargetBlend.writeMask);
	}
}


void FillRasterizerDesc(D3D12_RASTERIZER_DESC& rasterizerDesc, const RasterizerStateDesc& desc)
{
	rasterizerDesc.FillMode = FillModeToDX12(desc.fillMode);
	rasterizerDesc.CullMode = CullModeToDX12(desc.cullMode);
	rasterizerDesc.FrontCounterClockwise = desc.frontCounterClockwise ? TRUE : FALSE;
	rasterizerDesc.DepthBias = (INT)desc.depthBias;
	rasterizerDesc.DepthBiasClamp = desc.depthBiasClamp;
	rasterizerDesc.SlopeScaledDepthBias = desc.slopeScaledDepthBias;
	rasterizerDesc.DepthClipEnable = desc.depthClipEnable ? TRUE : FALSE;
	rasterizerDesc.MultisampleEnable = desc.multisampleEnable ? TRUE : FALSE;
	rasterizerDesc.AntialiasedLineEnable = desc.antialiasedLineEnable ? TRUE : FALSE;
	rasterizerDesc.ForcedSampleCount = desc.forcedSampleCount;
	rasterizerDesc.ConservativeRaster =
		desc.conservativeRasterizationEnable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}


void FillRasterizerDesc1(D3D12_RASTERIZER_DESC1& rasterizerDesc, const RasterizerStateDesc& desc)
{
	rasterizerDesc.FillMode = FillModeToDX12(desc.fillMode);
	rasterizerDesc.CullMode = CullModeToDX12(desc.cullMode);
	rasterizerDesc.FrontCounterClockwise = desc.frontCounterClockwise ? TRUE : FALSE;
	rasterizerDesc.DepthBias = desc.depthBias;
	rasterizerDesc.DepthBiasClamp = desc.depthBiasClamp;
	rasterizerDesc.SlopeScaledDepthBias = desc.slopeScaledDepthBias;
	rasterizerDesc.DepthClipEnable = desc.depthClipEnable ? TRUE : FALSE;
	rasterizerDesc.MultisampleEnable = desc.multisampleEnable ? TRUE : FALSE;
	rasterizerDesc.AntialiasedLineEnable = desc.antialiasedLineEnable ? TRUE : FALSE;
	rasterizerDesc.ForcedSampleCount = desc.forcedSampleCount;
	rasterizerDesc.ConservativeRaster =
		desc.conservativeRasterizationEnable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}


void FillRasterizerDesc2(D3D12_RASTERIZER_DESC2& rasterizerDesc, const RasterizerStateDesc& desc)
{
	rasterizerDesc.FillMode = FillModeToDX12(desc.fillMode);
	rasterizerDesc.CullMode = CullModeToDX12(desc.cullMode);
	rasterizerDesc.FrontCounterClockwise = desc.frontCounterClockwise ? TRUE : FALSE;
	rasterizerDesc.DepthBias = desc.depthBias;
	rasterizerDesc.DepthBiasClamp = desc.depthBiasClamp;
	rasterizerDesc.SlopeScaledDepthBias = desc.slopeScaledDepthBias;
	rasterizerDesc.DepthClipEnable = desc.depthClipEnable ? TRUE : FALSE;
	rasterizerDesc.LineRasterizationMode = 
		desc.antialiasedLineEnable ? D3D12_LINE_RASTERIZATION_MODE_ALPHA_ANTIALIASED : D3D12_LINE_RASTERIZATION_MODE_ALIASED;
	rasterizerDesc.ForcedSampleCount = desc.forcedSampleCount;
	rasterizerDesc.ConservativeRaster =
		desc.conservativeRasterizationEnable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}


void FillDepthStencilDesc(D3D12_DEPTH_STENCIL_DESC& depthStencilDesc, const DepthStencilStateDesc& desc)
{
	depthStencilDesc.DepthEnable = desc.depthEnable ? TRUE : FALSE;
	depthStencilDesc.DepthWriteMask = DepthWriteToDX12(desc.depthWriteMask);
	depthStencilDesc.DepthFunc = ComparisonFuncToDX12(desc.depthFunc);
	depthStencilDesc.StencilEnable = desc.stencilEnable ? TRUE : FALSE;
	depthStencilDesc.StencilReadMask = desc.frontFace.stencilReadMask;
	depthStencilDesc.StencilWriteMask = desc.frontFace.stencilWriteMask;
	depthStencilDesc.FrontFace.StencilFailOp = StencilOpToDX12(desc.frontFace.stencilFailOp);
	depthStencilDesc.FrontFace.StencilDepthFailOp = StencilOpToDX12(desc.frontFace.stencilDepthFailOp);
	depthStencilDesc.FrontFace.StencilPassOp = StencilOpToDX12(desc.frontFace.stencilPassOp);
	depthStencilDesc.FrontFace.StencilFunc = ComparisonFuncToDX12(desc.frontFace.stencilFunc);
	depthStencilDesc.BackFace.StencilFailOp = StencilOpToDX12(desc.backFace.stencilFailOp);
	depthStencilDesc.BackFace.StencilDepthFailOp = StencilOpToDX12(desc.backFace.stencilDepthFailOp);
	depthStencilDesc.BackFace.StencilPassOp = StencilOpToDX12(desc.backFace.stencilPassOp);
	depthStencilDesc.BackFace.StencilFunc = ComparisonFuncToDX12(desc.backFace.stencilFunc);
}


void FillDepthStencilDesc1(D3D12_DEPTH_STENCIL_DESC1& depthStencilDesc, const DepthStencilStateDesc& desc)
{
	depthStencilDesc.DepthEnable = desc.depthEnable ? TRUE : FALSE;
	depthStencilDesc.DepthWriteMask = DepthWriteToDX12(desc.depthWriteMask);
	depthStencilDesc.DepthFunc = ComparisonFuncToDX12(desc.depthFunc);
	depthStencilDesc.StencilEnable = desc.stencilEnable ? TRUE : FALSE;
	depthStencilDesc.StencilReadMask = desc.frontFace.stencilReadMask;
	depthStencilDesc.StencilWriteMask = desc.frontFace.stencilWriteMask;
	depthStencilDesc.FrontFace.StencilFailOp = StencilOpToDX12(desc.frontFace.stencilFailOp);
	depthStencilDesc.FrontFace.StencilDepthFailOp = StencilOpToDX12(desc.frontFace.stencilDepthFailOp);
	depthStencilDesc.FrontFace.StencilPassOp = StencilOpToDX12(desc.frontFace.stencilPassOp);
	depthStencilDesc.FrontFace.StencilFunc = ComparisonFuncToDX12(desc.frontFace.stencilFunc);
	depthStencilDesc.BackFace.StencilFailOp = StencilOpToDX12(desc.backFace.stencilFailOp);
	depthStencilDesc.BackFace.StencilDepthFailOp = StencilOpToDX12(desc.backFace.stencilDepthFailOp);
	depthStencilDesc.BackFace.StencilPassOp = StencilOpToDX12(desc.backFace.stencilPassOp);
	depthStencilDesc.BackFace.StencilFunc = ComparisonFuncToDX12(desc.backFace.stencilFunc);
	depthStencilDesc.DepthBoundsTestEnable = desc.depthBoundsTestEnable ? TRUE : FALSE;
}


void FillDepthStencilDesc2(D3D12_DEPTH_STENCIL_DESC2& depthStencilDesc, const DepthStencilStateDesc& desc)
{
	depthStencilDesc.DepthEnable = desc.depthEnable ? TRUE : FALSE;
	depthStencilDesc.DepthWriteMask = DepthWriteToDX12(desc.depthWriteMask);
	depthStencilDesc.DepthFunc = ComparisonFuncToDX12(desc.depthFunc);
	depthStencilDesc.StencilEnable = desc.stencilEnable ? TRUE : FALSE;
	depthStencilDesc.FrontFace.StencilFailOp = StencilOpToDX12(desc.frontFace.stencilFailOp);
	depthStencilDesc.FrontFace.StencilDepthFailOp = StencilOpToDX12(desc.frontFace.stencilDepthFailOp);
	depthStencilDesc.FrontFace.StencilPassOp = StencilOpToDX12(desc.frontFace.stencilPassOp);
	depthStencilDesc.FrontFace.StencilFunc = ComparisonFuncToDX12(desc.frontFace.stencilFunc);
	depthStencilDesc.FrontFace.StencilReadMask = desc.frontFace.stencilReadMask;
	depthStencilDesc.FrontFace.StencilWriteMask = desc.frontFace.stencilWriteMask;
	depthStencilDesc.BackFace.StencilFailOp = StencilOpToDX12(desc.backFace.stencilFailOp);
	depthStencilDesc.BackFace.StencilDepthFailOp = StencilOpToDX12(desc.backFace.stencilDepthFailOp);
	depthStencilDesc.BackFace.StencilPassOp = StencilOpToDX12(desc.backFace.stencilPassOp);
	depthStencilDesc.BackFace.StencilFunc = ComparisonFuncToDX12(desc.backFace.stencilFunc);
	depthStencilDesc.BackFace.StencilReadMask = desc.backFace.stencilReadMask;
	depthStencilDesc.BackFace.StencilWriteMask = desc.backFace.stencilWriteMask;
	depthStencilDesc.DepthBoundsTestEnable = desc.depthBoundsTestEnable ? TRUE : FALSE;
}


void FillGraphicsPipelineDesc(GraphicsPipelineContext& context, const GraphicsPipelineDesc& desc)
{
	context.stateDesc = D3D12_GRAPHICS_PIPELINE_STATE_DESC{};

	context.stateDesc.NodeMask = 1;
	context.stateDesc.SampleMask = desc.sampleMask;
	context.stateDesc.InputLayout.NumElements = 0;

	FillBlendDesc(context.stateDesc.BlendState, desc.blendState);
	FillRasterizerDesc(context.stateDesc.RasterizerState, desc.rasterizerState);
	FillDepthStencilDesc(context.stateDesc.DepthStencilState, desc.depthStencilState);
	
	if (IsDepthBiasEnabled(context.stateDesc.RasterizerState))
	{
		context.stateDesc.Flags |= D3D12_PIPELINE_STATE_FLAG_DYNAMIC_DEPTH_BIAS;
	}

	// Depth-stencil state
	const auto& depthStencilState = desc.depthStencilState;
	context.stateDesc.DepthStencilState.DepthEnable = depthStencilState.depthEnable ? TRUE : FALSE;
	context.stateDesc.DepthStencilState.DepthWriteMask = DepthWriteToDX12(depthStencilState.depthWriteMask);
	context.stateDesc.DepthStencilState.DepthFunc = ComparisonFuncToDX12(depthStencilState.depthFunc);
	context.stateDesc.DepthStencilState.StencilEnable = depthStencilState.stencilEnable ? TRUE : FALSE;
	context.stateDesc.DepthStencilState.FrontFace.StencilFailOp = StencilOpToDX12(depthStencilState.frontFace.stencilFailOp);
	context.stateDesc.DepthStencilState.FrontFace.StencilDepthFailOp = StencilOpToDX12(depthStencilState.frontFace.stencilDepthFailOp);
	context.stateDesc.DepthStencilState.FrontFace.StencilPassOp = StencilOpToDX12(depthStencilState.frontFace.stencilPassOp);
	context.stateDesc.DepthStencilState.FrontFace.StencilFunc = ComparisonFuncToDX12(depthStencilState.frontFace.stencilFunc);
	context.stateDesc.DepthStencilState.BackFace.StencilFailOp = StencilOpToDX12(depthStencilState.backFace.stencilFailOp);
	context.stateDesc.DepthStencilState.BackFace.StencilDepthFailOp = StencilOpToDX12(depthStencilState.backFace.stencilDepthFailOp);
	context.stateDesc.DepthStencilState.BackFace.StencilPassOp = StencilOpToDX12(depthStencilState.backFace.stencilPassOp);
	context.stateDesc.DepthStencilState.BackFace.StencilFunc = ComparisonFuncToDX12(depthStencilState.backFace.stencilFunc);

	// Primitive topology & primitive restart
	context.stateDesc.PrimitiveTopologyType = PrimitiveTopologyToPrimitiveTopologyTypeDX12(desc.topology);
	context.stateDesc.IBStripCutValue = IndexBufferStripCutValueToDX12(desc.indexBufferStripCut);

	// Render target formats
	const uint32_t numRtvs = (uint32_t)desc.rtvFormats.size();
	const uint32_t maxRenderTargets = 8;
	for (uint32_t i = 0; i < numRtvs; ++i)
	{
		context.stateDesc.RTVFormats[i] = FormatToDxgi(desc.rtvFormats[i]).rtvFormat;
	}
	for (uint32_t i = numRtvs; i < maxRenderTargets; ++i)
	{
		context.stateDesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	context.stateDesc.NumRenderTargets = numRtvs;
	context.stateDesc.DSVFormat = GetDSVFormat(FormatToDxgi(desc.dsvFormat).resourceFormat);
	context.stateDesc.SampleDesc.Count = desc.msaaCount;
	context.stateDesc.SampleDesc.Quality = 0; // TODO Rework this to enable quality levels in DX12

	// Input layout
	context.stateDesc.InputLayout.NumElements = (UINT)desc.vertexElements.size();

	if (context.stateDesc.InputLayout.NumElements > 0)
	{
		D3D12_INPUT_ELEMENT_DESC* newD3DElements = (D3D12_INPUT_ELEMENT_DESC*)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * context.stateDesc.InputLayout.NumElements);

		const auto& vertexElements = desc.vertexElements;

		for (uint32_t i = 0; i < context.stateDesc.InputLayout.NumElements; ++i)
		{
			newD3DElements[i].AlignedByteOffset = vertexElements[i].alignedByteOffset;
			newD3DElements[i].Format = FormatToDxgi(vertexElements[i].format).srvFormat;
			newD3DElements[i].InputSlot = vertexElements[i].inputSlot;
			newD3DElements[i].InputSlotClass = InputClassificationToDX12(vertexElements[i].inputClassification);
			newD3DElements[i].InstanceDataStepRate = vertexElements[i].instanceDataStepRate;
			newD3DElements[i].SemanticIndex = vertexElements[i].semanticIndex;
			newD3DElements[i].SemanticName = vertexElements[i].semanticName;
		}

		context.inputElements.reset((const D3D12_INPUT_ELEMENT_DESC*)newD3DElements);
	}

	// Shaders
	if (desc.vertexShader)
	{
		Shader* vertexShader = LoadShader(ShaderType::Vertex, desc.vertexShader);
		assert(vertexShader);
		context.stateDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader->GetByteCode(), vertexShader->GetByteCodeSize());
	}

	if (desc.pixelShader)
	{
		Shader* pixelShader = LoadShader(ShaderType::Pixel, desc.pixelShader);
		assert(pixelShader);
		context.stateDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader->GetByteCode(), pixelShader->GetByteCodeSize());
	}

	if (desc.geometryShader)
	{
		Shader* geometryShader = LoadShader(ShaderType::Geometry, desc.geometryShader);
		assert(geometryShader);
		context.stateDesc.GS = CD3DX12_SHADER_BYTECODE(geometryShader->GetByteCode(), geometryShader->GetByteCodeSize());
	}

	if (desc.hullShader)
	{
		Shader* hullShader = LoadShader(ShaderType::Hull, desc.hullShader);
		assert(hullShader);
		context.stateDesc.HS = CD3DX12_SHADER_BYTECODE(hullShader->GetByteCode(), hullShader->GetByteCodeSize());
	}

	if (desc.domainShader)
	{
		Shader* domainShader = LoadShader(ShaderType::Domain, desc.domainShader);
		assert(domainShader);
		context.stateDesc.DS = CD3DX12_SHADER_BYTECODE(domainShader->GetByteCode(), domainShader->GetByteCodeSize());
	}

	// Get the root signature from the desc
	auto rootSignature = (RootSignature*)desc.rootSignature.get();
	if (rootSignature)
	{
		context.stateDesc.pRootSignature = rootSignature->GetRootSignature();
	}
	assert(context.stateDesc.pRootSignature != nullptr);

	context.stateDesc.InputLayout.pInputElementDescs = nullptr;

	context.hashCode = Utility::HashState(&context.stateDesc);
	context.hashCode = Utility::HashState(context.inputElements.get(), context.stateDesc.InputLayout.NumElements, context.hashCode);

	context.stateDesc.InputLayout.pInputElementDescs = context.inputElements.get();
}


void FillGraphicsPipelineStateStreamDefault(CD3DX12_PIPELINE_STATE_STREAM& stateStream, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& stateDesc, const GraphicsPipelineDesc& desc)
{
	stateStream = CD3DX12_PIPELINE_STATE_STREAM(stateDesc);
}


void FillGraphicsPipelineStateStream1(CD3DX12_PIPELINE_STATE_STREAM1& stateStream, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& stateDesc, const GraphicsPipelineDesc& desc)
{
	stateStream = CD3DX12_PIPELINE_STATE_STREAM1(stateDesc);
	FillDepthStencilDesc1(stateStream.DepthStencilState, desc.depthStencilState);
}


void FillGraphicsPipelineStateStream2(CD3DX12_PIPELINE_STATE_STREAM2& stateStream, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& stateDesc, const GraphicsPipelineDesc& desc)
{
	stateStream = CD3DX12_PIPELINE_STATE_STREAM2(stateDesc);
	FillDepthStencilDesc1(stateStream.DepthStencilState, desc.depthStencilState);
	// TODO: Support view instancing
}


#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 606)
void FillGraphicsPipelineStateStream3(CD3DX12_PIPELINE_STATE_STREAM3& stateStream, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& stateDesc, const GraphicsPipelineDesc& desc)
{
	stateStream = CD3DX12_PIPELINE_STATE_STREAM3(stateDesc);
	FillDepthStencilDesc2(stateStream.DepthStencilState, desc.depthStencilState);
	// TODO: Support view instancing
}
#endif


#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 608)
void FillGraphicsPipelineStateStream4(CD3DX12_PIPELINE_STATE_STREAM4& stateStream, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& stateDesc, const GraphicsPipelineDesc& desc)
{
	stateStream = CD3DX12_PIPELINE_STATE_STREAM4(stateDesc);
	FillRasterizerDesc1(stateStream.RasterizerState, desc.rasterizerState);
	FillDepthStencilDesc2(stateStream.DepthStencilState, desc.depthStencilState);
	// TODO: Support view instancing
}
#endif


#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 610)
void FillGraphicsPipelineStateStream5(CD3DX12_PIPELINE_STATE_STREAM5& stateStream, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& stateDesc, const GraphicsPipelineDesc& desc)
{
	stateStream = CD3DX12_PIPELINE_STATE_STREAM5(stateDesc);
	FillRasterizerDesc2(stateStream.RasterizerState, desc.rasterizerState);
	FillDepthStencilDesc2(stateStream.DepthStencilState, desc.depthStencilState);
	// TODO: Support view instancing
}
#endif

} // namespace Luna::DX12