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

#include "Graphics\Enums.h"
#include "Graphics\Formats.h"
#include "Graphics\InputLayout.h"


namespace Luna
{

// Forward declarations
class IRootSignature;
class GraphicsPSOData;


struct RenderTargetBlendDesc
{
	bool blendEnable{ false };
	bool logicOpEnable{ false };
	Blend srcBlend{ Blend::One };
	Blend dstBlend{ Blend::Zero };
	BlendOp blendOp{ BlendOp::Add };
	Blend srcBlendAlpha{ Blend::One };
	Blend dstBlendAlpha{ Blend::Zero };
	BlendOp blendOpAlpha{ BlendOp::Add };
	LogicOp logicOp{ LogicOp::Noop };
	ColorWrite writeMask{ ColorWrite::All };

	constexpr RenderTargetBlendDesc& SetBlendEnable(bool value) noexcept { blendEnable = value; return *this; }
	constexpr RenderTargetBlendDesc& SetLogicOpEnable(bool value) noexcept { logicOpEnable = value; return *this; }
	constexpr RenderTargetBlendDesc& SetSrcBlend(Blend value) noexcept { srcBlend = value; return *this; }
	constexpr RenderTargetBlendDesc& SetDstBlend(Blend value) noexcept { dstBlend = value; return *this; }
	constexpr RenderTargetBlendDesc& SetBlendOp(BlendOp value) noexcept { blendOp = value; return *this; }
	constexpr RenderTargetBlendDesc& SetSrcBlendAlpha(Blend value) noexcept { srcBlendAlpha = value; return *this; }
	constexpr RenderTargetBlendDesc& SetDstBlendAlpha(Blend value) noexcept { dstBlendAlpha = value; return *this; }
	constexpr RenderTargetBlendDesc& SetBlendOpAlpha(BlendOp value) noexcept { blendOpAlpha = value; return *this; }
	constexpr RenderTargetBlendDesc& SetLogicOp(LogicOp value) noexcept { logicOp = value; return *this; }
	constexpr RenderTargetBlendDesc& SetWriteMask(ColorWrite value) noexcept { writeMask = value; return *this; }
};


struct BlendStateDesc
{
	bool alphaToCoverageEnable{ false };
	bool independentBlendEnable{ false };
	RenderTargetBlendDesc renderTargetBlend[8];

	constexpr BlendStateDesc& SetAlphaToCoverageEnable(bool value) noexcept { alphaToCoverageEnable = value; return *this; }
	constexpr BlendStateDesc& SetIndependentBlendEnable(bool value) noexcept { independentBlendEnable = value; return *this; }
	BlendStateDesc& SetRenderTargetBlend(int index, const RenderTargetBlendDesc& desc)
	{
		assert(index >= 0 && index < 8);
		renderTargetBlend[index] = desc;
	}
};


struct RasterizerStateDesc
{
	CullMode cullMode{ CullMode::Back };
	FillMode fillMode{ FillMode::Solid };
	bool frontCounterClockwise{ false };
	int depthBias{ 0 };
	float slopeScaledDepthBias{ 0.0f };
	float depthBiasClamp{ 0.0f };
	bool depthClipEnable{ true };
	bool multisampleEnable{ false };
	bool antialiasedLineEnable{ false };
	uint32_t forcedSampleCount{ 0 };
	bool conservativeRasterizationEnable{ false };

	constexpr RasterizerStateDesc& SetCullMode(CullMode value) noexcept { cullMode = value; return *this; }
	constexpr RasterizerStateDesc& SetFillMode(FillMode value) noexcept { fillMode = value; return *this; }
	constexpr RasterizerStateDesc& SetFrontCounterClockwise(bool value) noexcept { frontCounterClockwise = value; return *this; }
	constexpr RasterizerStateDesc& SetDepthBias(int value) noexcept { depthBias = value; return *this; }
	constexpr RasterizerStateDesc& SetSlopeScaledDepthBias(float value) noexcept { slopeScaledDepthBias = value; return *this; }
	constexpr RasterizerStateDesc& SetDepthBiasClamp(float value) noexcept { depthBiasClamp = value; return *this; }
	constexpr RasterizerStateDesc& SetDepthClipEnable(bool value) noexcept { depthClipEnable = value; return *this; }
	constexpr RasterizerStateDesc& SetMultisampleEnable(bool value) noexcept { multisampleEnable = value; return *this; }
	constexpr RasterizerStateDesc& SetAntialiasedLineEnable(bool value) noexcept { antialiasedLineEnable = value; return *this; }
	constexpr RasterizerStateDesc& SetForcedSampleCount(uint32_t value) noexcept { forcedSampleCount = value; return *this; }
	constexpr RasterizerStateDesc& SetConservativeRasterizationEnable(bool value) noexcept { conservativeRasterizationEnable = value; return *this; }
};


struct StencilOpDesc
{
	StencilOp stencilFailOp{ StencilOp::Keep };
	StencilOp stencilDepthFailOp{ StencilOp::Keep };
	StencilOp stencilPassOp{ StencilOp::Keep };
	ComparisonFunc stencilFunc{ ComparisonFunc::Always };

	constexpr StencilOpDesc& SetStencilFailOp(StencilOp value) noexcept { stencilFailOp = value; return *this; }
	constexpr StencilOpDesc& SetStencilDepthFailOp(StencilOp value) noexcept { stencilDepthFailOp = value; return *this; }
	constexpr StencilOpDesc& SetStencilPassOp(StencilOp value) noexcept { stencilPassOp = value; return *this; }
	constexpr StencilOpDesc& SetStencilFunc(ComparisonFunc value) noexcept { stencilFunc = value; return *this; }
};


struct DepthStencilStateDesc
{
	bool depthEnable{ true };
	DepthWrite depthWriteMask{ DepthWrite::All };
	ComparisonFunc depthFunc{ ComparisonFunc::Less };
	bool stencilEnable{ false };
	uint8_t stencilReadMask{ 0xFF };
	uint8_t stencilWriteMask{ 0xFF };
	StencilOpDesc frontFace{};
	StencilOpDesc backFace{};

	constexpr DepthStencilStateDesc& SetDepthEnable(bool value) noexcept { depthEnable = value; return *this; }
	constexpr DepthStencilStateDesc& SetDepthWriteMask(DepthWrite value) noexcept { depthWriteMask = value; return *this; }
	constexpr DepthStencilStateDesc& SetDepthFunc(ComparisonFunc value) noexcept { depthFunc = value; return *this; }
	constexpr DepthStencilStateDesc& SetStencilEnable(bool value) noexcept { stencilEnable = value; return *this; }
	constexpr DepthStencilStateDesc& SetStencilReadMask(uint8_t value) noexcept { stencilReadMask = value; return *this; }
	constexpr DepthStencilStateDesc& SetStencilWriteMask(uint8_t value) noexcept { stencilWriteMask = value; return *this; }
	DepthStencilStateDesc& SetFrontFace(StencilOpDesc value) noexcept { frontFace = value; return *this; }
	DepthStencilStateDesc& SetBackFace(StencilOpDesc value) noexcept { backFace = value; return *this; }
};


struct ShaderNameAndEntry
{
	std::string shaderFile;
	std::string entry{ "main" };

	operator bool() const { return !shaderFile.empty(); }
};


struct GraphicsPipelineDesc
{
	std::string name;

	BlendStateDesc blendState;
	DepthStencilStateDesc depthStencilState;
	RasterizerStateDesc rasterizerState;

	uint32_t sampleMask{ 0xFFFFFFFF };
	std::vector<Format> rtvFormats;
	Format dsvFormat;
	uint32_t msaaCount{ 1 };
	bool sampleRateShading{ false };

	PrimitiveTopology topology;
	IndexBufferStripCutValue indexBufferStripCut{ IndexBufferStripCutValue::Disabled };

	ShaderNameAndEntry vertexShader;
	ShaderNameAndEntry pixelShader;
	ShaderNameAndEntry geometryShader;
	ShaderNameAndEntry hullShader;
	ShaderNameAndEntry domainShader;

	std::vector<VertexStreamDesc> vertexStreams;
	std::vector<VertexElementDesc> vertexElements;

	IRootSignature* rootSignature{ nullptr };

	GraphicsPipelineDesc& SetName(const std::string& value) { name = value; return *this; }
	GraphicsPipelineDesc& SetBlendState(const BlendStateDesc& value) noexcept { blendState = value; return *this; }
	GraphicsPipelineDesc& SetDepthStencilState(const DepthStencilStateDesc& value) noexcept { depthStencilState = value; return *this; }
	GraphicsPipelineDesc& SetRasterizerState(const RasterizerStateDesc& value) noexcept { rasterizerState = value; return *this; }
	constexpr GraphicsPipelineDesc& SetSampleMask(uint32_t value) noexcept { sampleMask = value; return *this; }
	GraphicsPipelineDesc& SetRtvFormats(const std::vector<Format>& value) { rtvFormats = value; return *this; }
	constexpr GraphicsPipelineDesc& SetDsvFormat(Format value) noexcept { dsvFormat = value; return *this; }
	constexpr GraphicsPipelineDesc& SetMsaaCount(uint32_t value) noexcept { msaaCount = value; return *this; }
	constexpr GraphicsPipelineDesc& SetSampleRateShading(bool value) noexcept { sampleRateShading = value; return *this; }
	constexpr GraphicsPipelineDesc& SetTopology(PrimitiveTopology value) noexcept { topology = value; return *this; }
	constexpr GraphicsPipelineDesc& SetIndexBufferStripCut(IndexBufferStripCutValue value) noexcept { indexBufferStripCut = value; return *this; }
	GraphicsPipelineDesc& SetVertexShader(const std::string& value, const std::string& entry = "main") { vertexShader.shaderFile = value; vertexShader.entry = entry; return *this; }
	GraphicsPipelineDesc& SetPixelShader(const std::string& value, const std::string& entry = "main") { pixelShader.shaderFile = value; pixelShader.entry = entry; return *this; }
	GraphicsPipelineDesc& SetGeometryShader(const std::string& value, const std::string& entry = "main") { geometryShader.shaderFile = value; geometryShader.entry = entry; return *this; }
	GraphicsPipelineDesc& SetHullShader(const std::string& value, const std::string& entry = "main") { hullShader.shaderFile = value; hullShader.entry = entry; return *this; }
	GraphicsPipelineDesc& SetDomainShader(const std::string& value, const std::string& entry = "main") { domainShader.shaderFile = value; domainShader.entry = entry; return *this; }
	GraphicsPipelineDesc& SetVertexStreams(const std::vector<VertexStreamDesc>& value) { vertexStreams = value; return *this; }
	GraphicsPipelineDesc& SetVertexElements(const std::vector<VertexElementDesc>& value) { vertexElements = value; return *this; }
	constexpr GraphicsPipelineDesc& SetRootSignature(IRootSignature* value) { rootSignature = value; return *this; }
};


class IPipelineStatePool;


class __declspec(uuid("F2A84C37-1876-440B-AC18-4D712C906D83")) PipelineStateHandleType : public RefCounted<PipelineStateHandleType>
{
public:
	PipelineStateHandleType(uint32_t index, IPipelineStatePool* pool)
		: m_index{ index }
		, m_pool{ pool }
	{}

	~PipelineStateHandleType();

	uint32_t GetIndex() const { return m_index; }

private:
	uint32_t m_index{ 0 };
	IPipelineStatePool* m_pool{ nullptr };
};

using PipelineStateHandle = wil::com_ptr<PipelineStateHandleType>;


class IPipelineStatePool
{
public:
	// Create/Destroy pipeline state
	virtual PipelineStateHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc) = 0;
	virtual void DestroyHandle(PipelineStateHandleType* handle) = 0;

	// Platform agnostic getters
	virtual const GraphicsPipelineDesc& GetDesc(PipelineStateHandleType* handle) const = 0;
};


class GraphicsPipelineState
{
public:
	// TODO: get this from the PipelineStatePool
	PrimitiveTopology GetPrimitiveTopology() const;

	void Initialize(GraphicsPipelineDesc& pipelineDesc);

	PipelineStateHandle GetHandle() const { return m_handle; }

private:
	const GraphicsPipelineDesc& GetDesc() const;

private:
	PipelineStateHandle m_handle;
};

} // namespace