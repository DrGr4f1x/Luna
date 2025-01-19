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


namespace Luna
{

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
	float depthClampBias{ 0.0f };
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
	constexpr RasterizerStateDesc& SetDepthClampBias(float value) noexcept { depthClampBias = value; return *this; }
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

} // namespace