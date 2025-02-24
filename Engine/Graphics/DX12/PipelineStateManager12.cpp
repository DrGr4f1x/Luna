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

#include "PipelineStateManager12.h"

#include "FileSystem.h"

#include "RootSignatureManager12.h"
#include "Shader12.h"

using namespace std;


namespace Luna::DX12
{

PipelineStateManager* g_pipelineStateManager{ nullptr };


pair<string, bool> GetShaderFilenameWithExtension(const string& shaderFilename)
{
	auto fileSystem = GetFileSystem();

	string shaderFileWithExtension = shaderFilename;
	bool exists = false;

	// See if the filename already has an extension
	string extension = fileSystem->GetFileExtension(shaderFilename);
	if (!extension.empty())
	{
		exists = fileSystem->Exists(shaderFileWithExtension);
	}
	else
	{
		// Try .dxil extension first
		shaderFileWithExtension = shaderFilename + ".dxil";
		exists = fileSystem->Exists(shaderFileWithExtension);
		if (!exists)
		{
			// Try .dxbc next
			shaderFileWithExtension = shaderFilename + ".dxbc";
			exists = fileSystem->Exists(shaderFileWithExtension);
		}
	}

	return make_pair(shaderFileWithExtension, exists);
}


Shader* LoadShader(ShaderType type, const ShaderNameAndEntry& shaderNameAndEntry)
{
	auto [shaderFilenameWithExtension, exists] = GetShaderFilenameWithExtension(shaderNameAndEntry.shaderFile);

	if (!exists)
	{
		return nullptr;
	}

	ShaderDesc shaderDesc{
		.filenameWithExtension = shaderFilenameWithExtension,
		.entry = shaderNameAndEntry.entry,
		.type = type
	};

	return Shader::Load(shaderDesc);
}


PipelineStateManager::PipelineStateManager(ID3D12Device* device)
	: m_device{ device }
{
	assert(g_pipelineStateManager == nullptr);

	// Populate free list and data arrays
	for (uint32_t i = 0; i < MaxItems; ++i)
	{
		m_freeList.push(i);
		m_pipelines[i].reset();
		m_descs[i] = GraphicsPipelineDesc{};
	}

	g_pipelineStateManager = this;
}


PipelineStateManager::~PipelineStateManager()
{
	g_pipelineStateManager = nullptr;
}


PipelineStateHandle PipelineStateManager::CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_freeList.empty());

	uint32_t index = m_freeList.front();
	m_freeList.pop();

	m_descs[index] = pipelineDesc;

	m_pipelines[index] = FindOrCreateGraphicsPipelineState(pipelineDesc);

	return Create<PipelineStateHandleType>(index, this);
}


void PipelineStateManager::DestroyHandle(PipelineStateHandleType* handle)
{
	assert(handle != nullptr);

	std::lock_guard guard(m_allocationMutex);

	// TODO: queue this up to execute in one big batch, e.g. at the end of the frame

	uint32_t index = handle->GetIndex();
	m_descs[index] = GraphicsPipelineDesc{};
	m_pipelines[index].reset();

	m_freeList.push(index);
}


const GraphicsPipelineDesc& PipelineStateManager::GetDesc(PipelineStateHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_descs[index];
}


ID3D12PipelineState* PipelineStateManager::GetPipelineState(PipelineStateHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_pipelines[index].get();
}


wil::com_ptr<ID3D12PipelineState> PipelineStateManager::FindOrCreateGraphicsPipelineState(const GraphicsPipelineDesc& pipelineDesc)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12PipelineDesc{};
	d3d12PipelineDesc.NodeMask = 1;
	d3d12PipelineDesc.SampleMask = pipelineDesc.sampleMask;
	d3d12PipelineDesc.InputLayout.NumElements = 0;

	// Blend state
	d3d12PipelineDesc.BlendState.AlphaToCoverageEnable = pipelineDesc.blendState.alphaToCoverageEnable ? TRUE : FALSE;
	d3d12PipelineDesc.BlendState.IndependentBlendEnable = pipelineDesc.blendState.independentBlendEnable ? TRUE : FALSE;

	for (uint32_t i = 0; i < 8; ++i)
	{
		auto& rtDesc = d3d12PipelineDesc.BlendState.RenderTarget[i];
		const auto& renderTargetBlend = pipelineDesc.blendState.renderTargetBlend[i];

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

	// Rasterizer state
	const auto& rasterizerState = pipelineDesc.rasterizerState;
	d3d12PipelineDesc.RasterizerState.FillMode = FillModeToDX12(rasterizerState.fillMode);
	d3d12PipelineDesc.RasterizerState.CullMode = CullModeToDX12(rasterizerState.cullMode);
	d3d12PipelineDesc.RasterizerState.FrontCounterClockwise = rasterizerState.frontCounterClockwise ? TRUE : FALSE;
	d3d12PipelineDesc.RasterizerState.DepthBias = rasterizerState.depthBias;
	d3d12PipelineDesc.RasterizerState.DepthBiasClamp = rasterizerState.depthBiasClamp;
	d3d12PipelineDesc.RasterizerState.SlopeScaledDepthBias = rasterizerState.slopeScaledDepthBias;
	d3d12PipelineDesc.RasterizerState.DepthClipEnable = rasterizerState.depthClipEnable ? TRUE : FALSE;
	d3d12PipelineDesc.RasterizerState.MultisampleEnable = rasterizerState.multisampleEnable ? TRUE : FALSE;
	d3d12PipelineDesc.RasterizerState.AntialiasedLineEnable = rasterizerState.antialiasedLineEnable ? TRUE : FALSE;
	d3d12PipelineDesc.RasterizerState.ForcedSampleCount = rasterizerState.forcedSampleCount;
	d3d12PipelineDesc.RasterizerState.ConservativeRaster =
		rasterizerState.conservativeRasterizationEnable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// Depth-stencil state
	const auto& depthStencilState = pipelineDesc.depthStencilState;
	d3d12PipelineDesc.DepthStencilState.DepthEnable = depthStencilState.depthEnable ? TRUE : FALSE;
	d3d12PipelineDesc.DepthStencilState.DepthWriteMask = DepthWriteToDX12(depthStencilState.depthWriteMask);
	d3d12PipelineDesc.DepthStencilState.DepthFunc = ComparisonFuncToDX12(depthStencilState.depthFunc);
	d3d12PipelineDesc.DepthStencilState.StencilEnable = depthStencilState.stencilEnable ? TRUE : FALSE;
	d3d12PipelineDesc.DepthStencilState.StencilReadMask = depthStencilState.stencilReadMask;
	d3d12PipelineDesc.DepthStencilState.StencilWriteMask = depthStencilState.stencilWriteMask;
	d3d12PipelineDesc.DepthStencilState.FrontFace.StencilFailOp = StencilOpToDX12(depthStencilState.frontFace.stencilFailOp);
	d3d12PipelineDesc.DepthStencilState.FrontFace.StencilDepthFailOp = StencilOpToDX12(depthStencilState.frontFace.stencilDepthFailOp);
	d3d12PipelineDesc.DepthStencilState.FrontFace.StencilPassOp = StencilOpToDX12(depthStencilState.frontFace.stencilPassOp);
	d3d12PipelineDesc.DepthStencilState.FrontFace.StencilFunc = ComparisonFuncToDX12(depthStencilState.frontFace.stencilFunc);
	d3d12PipelineDesc.DepthStencilState.BackFace.StencilFailOp = StencilOpToDX12(depthStencilState.backFace.stencilFailOp);
	d3d12PipelineDesc.DepthStencilState.BackFace.StencilDepthFailOp = StencilOpToDX12(depthStencilState.backFace.stencilDepthFailOp);
	d3d12PipelineDesc.DepthStencilState.BackFace.StencilPassOp = StencilOpToDX12(depthStencilState.backFace.stencilPassOp);
	d3d12PipelineDesc.DepthStencilState.BackFace.StencilFunc = ComparisonFuncToDX12(depthStencilState.backFace.stencilFunc);

	// Primitive topology & primitive restart
	d3d12PipelineDesc.PrimitiveTopologyType = PrimitiveTopologyToPrimitiveTopologyTypeDX12(pipelineDesc.topology);
	d3d12PipelineDesc.IBStripCutValue = IndexBufferStripCutValueToDX12(pipelineDesc.indexBufferStripCut);

	// Render target formats
	const uint32_t numRtvs = (uint32_t)pipelineDesc.rtvFormats.size();
	const uint32_t maxRenderTargets = 8;
	for (uint32_t i = 0; i < numRtvs; ++i)
	{
		d3d12PipelineDesc.RTVFormats[i] = FormatToDxgi(pipelineDesc.rtvFormats[i]).rtvFormat;
	}
	for (uint32_t i = numRtvs; i < maxRenderTargets; ++i)
	{
		d3d12PipelineDesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	d3d12PipelineDesc.NumRenderTargets = numRtvs;
	d3d12PipelineDesc.DSVFormat = GetDSVFormat(FormatToDxgi(pipelineDesc.dsvFormat).resourceFormat);
	d3d12PipelineDesc.SampleDesc.Count = pipelineDesc.msaaCount;
	d3d12PipelineDesc.SampleDesc.Quality = 0; // TODO Rework this to enable quality levels in DX12

	// Input layout
	d3d12PipelineDesc.InputLayout.NumElements = (UINT)pipelineDesc.vertexElements.size();
	unique_ptr<const D3D12_INPUT_ELEMENT_DESC> d3dElements;

	if (d3d12PipelineDesc.InputLayout.NumElements > 0)
	{
		D3D12_INPUT_ELEMENT_DESC* newD3DElements = (D3D12_INPUT_ELEMENT_DESC*)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * d3d12PipelineDesc.InputLayout.NumElements);

		const auto& vertexElements = pipelineDesc.vertexElements;

		for (uint32_t i = 0; i < d3d12PipelineDesc.InputLayout.NumElements; ++i)
		{
			newD3DElements[i].AlignedByteOffset = vertexElements[i].alignedByteOffset;
			newD3DElements[i].Format = FormatToDxgi(vertexElements[i].format).srvFormat;
			newD3DElements[i].InputSlot = vertexElements[i].inputSlot;
			newD3DElements[i].InputSlotClass = InputClassificationToDX12(vertexElements[i].inputClassification);
			newD3DElements[i].InstanceDataStepRate = vertexElements[i].instanceDataStepRate;
			newD3DElements[i].SemanticIndex = vertexElements[i].semanticIndex;
			newD3DElements[i].SemanticName = vertexElements[i].semanticName;
		}

		d3dElements.reset((const D3D12_INPUT_ELEMENT_DESC*)newD3DElements);
	}

	// Shaders
	if (pipelineDesc.vertexShader)
	{
		Shader* vertexShader = LoadShader(ShaderType::Vertex, pipelineDesc.vertexShader);
		assert(vertexShader);
		d3d12PipelineDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader->GetByteCode(), vertexShader->GetByteCodeSize());
	}

	if (pipelineDesc.pixelShader)
	{
		Shader* pixelShader = LoadShader(ShaderType::Pixel, pipelineDesc.pixelShader);
		assert(pixelShader);
		d3d12PipelineDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader->GetByteCode(), pixelShader->GetByteCodeSize());
	}

	if (pipelineDesc.geometryShader)
	{
		Shader* geometryShader = LoadShader(ShaderType::Geometry, pipelineDesc.geometryShader);
		assert(geometryShader);
		d3d12PipelineDesc.GS = CD3DX12_SHADER_BYTECODE(geometryShader->GetByteCode(), geometryShader->GetByteCodeSize());
	}

	if (pipelineDesc.hullShader)
	{
		Shader* hullShader = LoadShader(ShaderType::Hull, pipelineDesc.hullShader);
		assert(hullShader);
		d3d12PipelineDesc.HS = CD3DX12_SHADER_BYTECODE(hullShader->GetByteCode(), hullShader->GetByteCodeSize());
	}

	if (pipelineDesc.domainShader)
	{
		Shader* domainShader = LoadShader(ShaderType::Domain, pipelineDesc.domainShader);
		assert(domainShader);
		d3d12PipelineDesc.DS = CD3DX12_SHADER_BYTECODE(domainShader->GetByteCode(), domainShader->GetByteCodeSize());
	}

	// Make sure the root signature is finalized first
	d3d12PipelineDesc.pRootSignature = GetD3D12RootSignatureManager()->GetRootSignature(pipelineDesc.rootSignature.get());
	assert(d3d12PipelineDesc.pRootSignature != nullptr);

	d3d12PipelineDesc.InputLayout.pInputElementDescs = nullptr;

	size_t hashCode = Utility::HashState(&d3d12PipelineDesc);
	hashCode = Utility::HashState(d3dElements.get(), d3d12PipelineDesc.InputLayout.NumElements, hashCode);

	d3d12PipelineDesc.InputLayout.pInputElementDescs = d3dElements.get();

	ID3D12PipelineState** ppPipelineState = nullptr;
	ID3D12PipelineState* pPipelineState;
	bool firstCompile = false;
	{
		lock_guard<mutex> CS(m_pipelineStateMutex);

		auto iter = m_pipelineStateMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_pipelineStateMap.end())
		{
			firstCompile = true;
			ppPipelineState = m_pipelineStateMap[hashCode].addressof();
		}
		else
		{
			ppPipelineState = iter->second.addressof();
		}
	}

	if (firstCompile)
	{
		HRESULT res = m_device->CreateGraphicsPipelineState(&d3d12PipelineDesc, IID_PPV_ARGS(&pPipelineState));
		ThrowIfFailed(res);

		SetDebugName(pPipelineState, pipelineDesc.name);

		m_pipelineStateMap[hashCode].attach(pPipelineState);
	}
	else
	{
		while (*ppPipelineState == nullptr)
		{
			this_thread::yield();
		}
		pPipelineState = *ppPipelineState;
	}

	return pPipelineState;
}


PipelineStateManager* const GetD3D12PipelineStateManager()
{
	return g_pipelineStateManager;
}

} // namespace Luna::DX12