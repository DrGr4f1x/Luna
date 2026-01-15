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

#include "CommonStates.h"

#include "PipelineState.h"
#include "Sampler.h"


namespace Luna::CommonStates
{

const BlendStateDesc& BlendNoColorWrite()
{
	static BlendStateDesc blendStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		blendStateDesc.alphaToCoverageEnable = false;
		blendStateDesc.independentBlendEnable = false;
		for (auto& renderTargetBlend : blendStateDesc.renderTargetBlend)
		{
			renderTargetBlend.blendEnable = false;
			renderTargetBlend.srcBlend = Blend::SrcAlpha;
			renderTargetBlend.dstBlend = Blend::InvSrcAlpha;
			renderTargetBlend.blendOp = BlendOp::Add;
			renderTargetBlend.srcBlendAlpha = Blend::One;
			renderTargetBlend.dstBlendAlpha = Blend::InvSrcAlpha;
			renderTargetBlend.blendOpAlpha = BlendOp::Add;
			renderTargetBlend.writeMask = ColorWrite::None;
		}
		initialized = true;
	}

	return blendStateDesc;
}


const BlendStateDesc& BlendDisable()
{
	static BlendStateDesc blendStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		blendStateDesc.alphaToCoverageEnable = false;
		blendStateDesc.independentBlendEnable = false;
		for (auto& renderTargetBlend : blendStateDesc.renderTargetBlend)
		{
			renderTargetBlend.blendEnable = false;
			renderTargetBlend.srcBlend = Blend::SrcAlpha;
			renderTargetBlend.dstBlend = Blend::InvSrcAlpha;
			renderTargetBlend.blendOp = BlendOp::Add;
			renderTargetBlend.srcBlendAlpha = Blend::One;
			renderTargetBlend.dstBlendAlpha = Blend::InvSrcAlpha;
			renderTargetBlend.blendOpAlpha = BlendOp::Add;
			renderTargetBlend.writeMask = ColorWrite::All;
		}
		initialized = true;
	}

	return blendStateDesc;
}


const BlendStateDesc& BlendPreMultiplied()
{
	static BlendStateDesc blendStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		blendStateDesc.alphaToCoverageEnable = false;
		blendStateDesc.independentBlendEnable = false;
		for (auto& renderTargetBlend : blendStateDesc.renderTargetBlend)
		{
			renderTargetBlend.blendEnable = true;
			renderTargetBlend.srcBlend = Blend::One;
			renderTargetBlend.dstBlend = Blend::InvSrcAlpha;
			renderTargetBlend.blendOp = BlendOp::Add;
			renderTargetBlend.srcBlendAlpha = Blend::One;
			renderTargetBlend.dstBlendAlpha = Blend::InvSrcAlpha;
			renderTargetBlend.blendOpAlpha = BlendOp::Add;
			renderTargetBlend.writeMask = ColorWrite::All;
		}
		initialized = true;
	}

	return blendStateDesc;
}


const BlendStateDesc& BlendTraditional()
{
	static BlendStateDesc blendStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		blendStateDesc.alphaToCoverageEnable = false;
		blendStateDesc.independentBlendEnable = false;
		for (auto& renderTargetBlend : blendStateDesc.renderTargetBlend)
		{
			renderTargetBlend.blendEnable = true;
			renderTargetBlend.srcBlend = Blend::SrcAlpha;
			renderTargetBlend.dstBlend = Blend::InvSrcAlpha;
			renderTargetBlend.blendOp = BlendOp::Add;
			renderTargetBlend.srcBlendAlpha = Blend::One;
			renderTargetBlend.dstBlendAlpha = Blend::InvSrcAlpha;
			renderTargetBlend.blendOpAlpha = BlendOp::Add;
			renderTargetBlend.writeMask = ColorWrite::All;
		}
		initialized = true;
	}

	return blendStateDesc;
}


const BlendStateDesc& BlendAdditive()
{
	static BlendStateDesc blendStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		blendStateDesc.alphaToCoverageEnable = false;
		blendStateDesc.independentBlendEnable = false;
		for (auto& renderTargetBlend : blendStateDesc.renderTargetBlend)
		{
			renderTargetBlend.blendEnable = true;
			renderTargetBlend.srcBlend = Blend::One;
			renderTargetBlend.dstBlend = Blend::One;
			renderTargetBlend.blendOp = BlendOp::Add;
			renderTargetBlend.srcBlendAlpha = Blend::One;
			renderTargetBlend.dstBlendAlpha = Blend::InvSrcAlpha;
			renderTargetBlend.blendOpAlpha = BlendOp::Add;
			renderTargetBlend.writeMask = ColorWrite::All;
		}
		initialized = true;
	}

	return blendStateDesc;
}


const BlendStateDesc& BlendTraditionalAdditive()
{
	static BlendStateDesc blendStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		blendStateDesc.alphaToCoverageEnable = false;
		blendStateDesc.independentBlendEnable = false;
		for (auto& renderTargetBlend : blendStateDesc.renderTargetBlend)
		{
			renderTargetBlend.blendEnable = true;
			renderTargetBlend.srcBlend = Blend::SrcAlpha;
			renderTargetBlend.dstBlend = Blend::One;
			renderTargetBlend.blendOp = BlendOp::Add;
			renderTargetBlend.srcBlendAlpha = Blend::One;
			renderTargetBlend.dstBlendAlpha = Blend::InvSrcAlpha;
			renderTargetBlend.blendOpAlpha = BlendOp::Add;
			renderTargetBlend.writeMask = ColorWrite::All;
		}
		initialized = true;
	}

	return blendStateDesc;
}


const RasterizerStateDesc& RasterizerDefault()
{
	static RasterizerStateDesc rasterizerStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		rasterizerStateDesc.fillMode = FillMode::Solid;
		rasterizerStateDesc.cullMode = CullMode::Back;
		rasterizerStateDesc.frontCounterClockwise = true;
		rasterizerStateDesc.depthBias = 0;
		rasterizerStateDesc.depthBiasClamp = 0.0f;
		rasterizerStateDesc.slopeScaledDepthBias = 0.0f;
		rasterizerStateDesc.depthClipEnable = true;
		rasterizerStateDesc.multisampleEnable = false;
		rasterizerStateDesc.antialiasedLineEnable = false;
		rasterizerStateDesc.forcedSampleCount = 0;
		rasterizerStateDesc.conservativeRasterizationEnable = false;

		initialized = true;
	}

	return rasterizerStateDesc;
}


const RasterizerStateDesc& RasterizerDefaultCW()
{
	static RasterizerStateDesc rasterizerStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		rasterizerStateDesc.fillMode = FillMode::Solid;
		rasterizerStateDesc.cullMode = CullMode::Back;
		rasterizerStateDesc.frontCounterClockwise = false;
		rasterizerStateDesc.depthBias = 0;
		rasterizerStateDesc.depthBiasClamp = 0.0f;
		rasterizerStateDesc.slopeScaledDepthBias = 0.0f;
		rasterizerStateDesc.depthClipEnable = true;
		rasterizerStateDesc.multisampleEnable = false;
		rasterizerStateDesc.antialiasedLineEnable = false;
		rasterizerStateDesc.forcedSampleCount = 0;
		rasterizerStateDesc.conservativeRasterizationEnable = false;

		initialized = true;
	}

	return rasterizerStateDesc;
}


const RasterizerStateDesc& RasterizerTwoSided()
{
	static RasterizerStateDesc rasterizerStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		rasterizerStateDesc.fillMode = FillMode::Solid;
		rasterizerStateDesc.cullMode = CullMode::None;
		rasterizerStateDesc.frontCounterClockwise = true;
		rasterizerStateDesc.depthBias = 0;
		rasterizerStateDesc.depthBiasClamp = 0.0f;
		rasterizerStateDesc.slopeScaledDepthBias = 0.0f;
		rasterizerStateDesc.depthClipEnable = true;
		rasterizerStateDesc.multisampleEnable = false;
		rasterizerStateDesc.antialiasedLineEnable = false;
		rasterizerStateDesc.forcedSampleCount = 0;
		rasterizerStateDesc.conservativeRasterizationEnable = false;

		initialized = true;
	}

	return rasterizerStateDesc;
}


const RasterizerStateDesc& RasterizerShadow()
{
	static RasterizerStateDesc rasterizerStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		rasterizerStateDesc.fillMode = FillMode::Solid;
		rasterizerStateDesc.cullMode = CullMode::Back;
		rasterizerStateDesc.frontCounterClockwise = true;
		rasterizerStateDesc.depthBias = -100;
		rasterizerStateDesc.depthBiasClamp = 0.0f;
		rasterizerStateDesc.slopeScaledDepthBias = -1.5f;
		rasterizerStateDesc.depthClipEnable = true;
		rasterizerStateDesc.multisampleEnable = false;
		rasterizerStateDesc.antialiasedLineEnable = false;
		rasterizerStateDesc.forcedSampleCount = 0;
		rasterizerStateDesc.conservativeRasterizationEnable = false;

		initialized = true;
	}

	return rasterizerStateDesc;
}


const RasterizerStateDesc& RasterizerShadowCW()
{
	static RasterizerStateDesc rasterizerStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		rasterizerStateDesc.fillMode = FillMode::Solid;
		rasterizerStateDesc.cullMode = CullMode::Back;
		rasterizerStateDesc.frontCounterClockwise = false;
		rasterizerStateDesc.depthBias = -100;
		rasterizerStateDesc.depthBiasClamp = 0.0f;
		rasterizerStateDesc.slopeScaledDepthBias = -1.5f;
		rasterizerStateDesc.depthClipEnable = true;
		rasterizerStateDesc.multisampleEnable = false;
		rasterizerStateDesc.antialiasedLineEnable = false;
		rasterizerStateDesc.forcedSampleCount = 0;
		rasterizerStateDesc.conservativeRasterizationEnable = false;

		initialized = true;
	}

	return rasterizerStateDesc;
}


const RasterizerStateDesc& RasterizerWireframe()
{
	static RasterizerStateDesc rasterizerStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		rasterizerStateDesc.fillMode = FillMode::Wireframe;
		rasterizerStateDesc.cullMode = CullMode::None;
		rasterizerStateDesc.frontCounterClockwise = true;
		rasterizerStateDesc.depthBias = 0;
		rasterizerStateDesc.depthBiasClamp = 0.0f;
		rasterizerStateDesc.slopeScaledDepthBias = 0.0f;
		rasterizerStateDesc.depthClipEnable = true;
		rasterizerStateDesc.multisampleEnable = false;
		rasterizerStateDesc.antialiasedLineEnable = false;
		rasterizerStateDesc.forcedSampleCount = 0;
		rasterizerStateDesc.conservativeRasterizationEnable = false;

		initialized = true;
	}

	return rasterizerStateDesc;
}


const DepthStencilStateDesc& CommonStates::DepthStateDisabled()
{
	static DepthStencilStateDesc depthStencilStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		depthStencilStateDesc.depthEnable = false;
		depthStencilStateDesc.depthWriteMask = DepthWrite::Zero;
		depthStencilStateDesc.depthFunc = ComparisonFunc::Always;
		depthStencilStateDesc.stencilEnable = false;
		depthStencilStateDesc.frontFace.stencilReadMask = 0xFF;
		depthStencilStateDesc.frontFace.stencilWriteMask = 0xFF;
		depthStencilStateDesc.frontFace.stencilFunc = ComparisonFunc::Always;
		depthStencilStateDesc.frontFace.stencilPassOp = StencilOp::Keep;
		depthStencilStateDesc.frontFace.stencilFailOp = StencilOp::Keep;
		depthStencilStateDesc.frontFace.stencilDepthFailOp = StencilOp::Keep;
		depthStencilStateDesc.backFace = depthStencilStateDesc.frontFace;

		initialized = true;
	}

	return depthStencilStateDesc;
}


const DepthStencilStateDesc& CommonStates::DepthStateReadWrite()
{
	static DepthStencilStateDesc depthStencilStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		depthStencilStateDesc.depthEnable = true;
		depthStencilStateDesc.depthWriteMask = DepthWrite::All;
		depthStencilStateDesc.depthFunc = ComparisonFunc::GreaterEqual;
		depthStencilStateDesc.stencilEnable = false;
		depthStencilStateDesc.frontFace.stencilReadMask = 0xFF;
		depthStencilStateDesc.frontFace.stencilWriteMask = 0xFF;
		depthStencilStateDesc.frontFace.stencilFunc = ComparisonFunc::Always;
		depthStencilStateDesc.frontFace.stencilPassOp = StencilOp::Keep;
		depthStencilStateDesc.frontFace.stencilFailOp = StencilOp::Keep;
		depthStencilStateDesc.frontFace.stencilDepthFailOp = StencilOp::Keep;
		depthStencilStateDesc.backFace = depthStencilStateDesc.frontFace;

		initialized = true;
	}

	return depthStencilStateDesc;
}


const DepthStencilStateDesc& CommonStates::DepthStateReadWriteReversed()
{
	static DepthStencilStateDesc depthStencilStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		depthStencilStateDesc.depthEnable = true;
		depthStencilStateDesc.depthWriteMask = DepthWrite::All;
		depthStencilStateDesc.depthFunc = ComparisonFunc::LessEqual;
		depthStencilStateDesc.stencilEnable = false;
		depthStencilStateDesc.frontFace.stencilReadMask = 0xFF;
		depthStencilStateDesc.frontFace.stencilWriteMask = 0xFF;
		depthStencilStateDesc.frontFace.stencilFunc = ComparisonFunc::Always;
		depthStencilStateDesc.frontFace.stencilPassOp = StencilOp::Keep;
		depthStencilStateDesc.frontFace.stencilFailOp = StencilOp::Keep;
		depthStencilStateDesc.frontFace.stencilDepthFailOp = StencilOp::Keep;
		depthStencilStateDesc.backFace = depthStencilStateDesc.frontFace;

		initialized = true;
	}

	return depthStencilStateDesc;
}


const DepthStencilStateDesc& CommonStates::DepthStateReadOnly()
{
	static DepthStencilStateDesc depthStencilStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		depthStencilStateDesc.depthEnable = true;
		depthStencilStateDesc.depthWriteMask = DepthWrite::Zero;
		depthStencilStateDesc.depthFunc = ComparisonFunc::GreaterEqual;
		depthStencilStateDesc.stencilEnable = false;
		depthStencilStateDesc.frontFace.stencilReadMask = 0xFF;
		depthStencilStateDesc.frontFace.stencilWriteMask = 0xFF;
		depthStencilStateDesc.frontFace.stencilFunc = ComparisonFunc::Always;
		depthStencilStateDesc.frontFace.stencilPassOp = StencilOp::Keep;
		depthStencilStateDesc.frontFace.stencilFailOp = StencilOp::Keep;
		depthStencilStateDesc.frontFace.stencilDepthFailOp = StencilOp::Keep;
		depthStencilStateDesc.backFace = depthStencilStateDesc.frontFace;

		initialized = true;
	}

	return depthStencilStateDesc;
}


const DepthStencilStateDesc& CommonStates::DepthStateReadOnlyReversed()
{
	static DepthStencilStateDesc depthStencilStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		depthStencilStateDesc.depthEnable = true;
		depthStencilStateDesc.depthWriteMask = DepthWrite::Zero;
		depthStencilStateDesc.depthFunc = ComparisonFunc::Less;
		depthStencilStateDesc.stencilEnable = false;
		depthStencilStateDesc.frontFace.stencilReadMask = 0xFF;
		depthStencilStateDesc.frontFace.stencilWriteMask = 0xFF;
		depthStencilStateDesc.frontFace.stencilFunc = ComparisonFunc::Always;
		depthStencilStateDesc.frontFace.stencilPassOp = StencilOp::Keep;
		depthStencilStateDesc.frontFace.stencilFailOp = StencilOp::Keep;
		depthStencilStateDesc.frontFace.stencilDepthFailOp = StencilOp::Keep;
		depthStencilStateDesc.backFace = depthStencilStateDesc.frontFace;

		initialized = true;
	}

	return depthStencilStateDesc;
}


const DepthStencilStateDesc& CommonStates::DepthStateTestEqual()
{
	static DepthStencilStateDesc depthStencilStateDesc{};
	static bool initialized = false;

	if (!initialized)
	{
		depthStencilStateDesc.depthEnable = true;
		depthStencilStateDesc.depthWriteMask = DepthWrite::Zero;
		depthStencilStateDesc.depthFunc = ComparisonFunc::Equal;
		depthStencilStateDesc.stencilEnable = false;
		depthStencilStateDesc.frontFace.stencilReadMask = 0xFF;
		depthStencilStateDesc.frontFace.stencilWriteMask = 0xFF;
		depthStencilStateDesc.frontFace.stencilFunc = ComparisonFunc::Always;
		depthStencilStateDesc.frontFace.stencilPassOp = StencilOp::Keep;
		depthStencilStateDesc.frontFace.stencilFailOp = StencilOp::Keep;
		depthStencilStateDesc.frontFace.stencilDepthFailOp = StencilOp::Keep;
		depthStencilStateDesc.backFace = depthStencilStateDesc.frontFace;

		initialized = true;
	}

	return depthStencilStateDesc;
}


const SamplerDesc& CommonStates::SamplerLinearWrap()
{
	static SamplerDesc desc{ .filter = TextureFilter::MinMagMipLinear };
	return desc;
}


const SamplerDesc& CommonStates::SamplerAnisoWrap()
{
	static SamplerDesc desc{ .maxAnisotropy = 8 };
	return desc;
}


const SamplerDesc& CommonStates::SamplerShadow()
{
	static SamplerDesc desc{ 
		.filter				= TextureFilter::ComparisonMinMagLinearMipPoint, 
		.addressU			= TextureAddress::Clamp,
		.addressV			= TextureAddress::Clamp,
		.addressW			= TextureAddress::Clamp,
		.comparisonFunc		= ComparisonFunc::GreaterEqual
	};

	return desc;
}


const SamplerDesc& CommonStates::SamplerLinearClamp()
{
	static SamplerDesc desc{ 
		.filter		= TextureFilter::MinMagMipLinear, 
		.addressU	= TextureAddress::Clamp,
		.addressV	= TextureAddress::Clamp,
		.addressW	= TextureAddress::Clamp
	};

	return desc;
}


const SamplerDesc& CommonStates::SamplerVolumeWrap()
{
	static SamplerDesc desc{ .filter = TextureFilter::MinMagMipPoint };
	return desc;
}


const SamplerDesc& CommonStates::SamplerPointWrap()
{
	static SamplerDesc desc{
		.filter = TextureFilter::MinMagMipPoint,
		.addressU = TextureAddress::Wrap,
		.addressV = TextureAddress::Wrap,
		.addressW = TextureAddress::Wrap
	};

	return desc;
}


const SamplerDesc& CommonStates::SamplerPointClamp()
{
	static SamplerDesc desc{ 
		.filter		= TextureFilter::MinMagMipPoint, 
		.addressU	= TextureAddress::Clamp,
		.addressV	= TextureAddress::Clamp,
		.addressW	= TextureAddress::Clamp
	};

	return desc;
}


const SamplerDesc& CommonStates::SamplerPointBorder(StaticBorderColor borderColor)
{
	static SamplerDesc desc{ 
		.filter				= TextureFilter::MinMagMipPoint, 
		.addressU			= TextureAddress::Clamp,
		.addressV			= TextureAddress::Clamp,
		.addressW			= TextureAddress::Clamp,
		.staticBorderColor	= borderColor
	};

	return desc;
}


const SamplerDesc& CommonStates::SamplerLinearBorder(StaticBorderColor borderColor)
{
	static SamplerDesc desc{ 
		.filter				= TextureFilter::MinMagMipLinear, 
		.addressU			= TextureAddress::Clamp,
		.addressV			= TextureAddress::Clamp,
		.addressW			= TextureAddress::Clamp,
		.staticBorderColor	= borderColor
	};
	
	return desc;
}


const SamplerDesc& CommonStates::SamplerLinearMirror()
{
	static SamplerDesc desc{ 
		.filter		= TextureFilter::MinMagMipLinear, 
		.addressU	= TextureAddress::Mirror,
		.addressV	= TextureAddress::Mirror,
		.addressW	= TextureAddress::Mirror
	};

	return desc;
}

} // namespace Luna::CommonStates