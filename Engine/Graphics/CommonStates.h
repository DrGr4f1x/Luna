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

#include "Graphics\GraphicsCommon.h"


namespace Luna
{

struct BlendStateDesc;
struct DepthStencilStateDesc;
struct RasterizerStateDesc;
struct SamplerDesc;

namespace CommonStates
{

// Blend states
const BlendStateDesc& BlendNoColorWrite();			// XXX
const BlendStateDesc& BlendDisable();				// 1, 0
const BlendStateDesc& BlendPreMultiplied();			// 1, 1-SrcA
const BlendStateDesc& BlendTraditional();			// SrcA, 1-SrcA
const BlendStateDesc& BlendAdditive();				// 1, 1
const BlendStateDesc& BlendTraditionalAdditive();	// SrcA, 1

// Rasterizer states
const RasterizerStateDesc& RasterizerDefault();
const RasterizerStateDesc& RasterizerDefaultCW();
const RasterizerStateDesc& RasterizerTwoSided();
const RasterizerStateDesc& RasterizerShadow();
const RasterizerStateDesc& RasterizerShadowCW();
const RasterizerStateDesc& RasterizerWireframe();

// Depth stencil states
const DepthStencilStateDesc& DepthStateDisabled();
const DepthStencilStateDesc& DepthStateReadWrite();
const DepthStencilStateDesc& DepthStateReadWriteReversed();
const DepthStencilStateDesc& DepthStateReadOnly();
const DepthStencilStateDesc& DepthStateReadOnlyReversed();
const DepthStencilStateDesc& DepthStateTestEqual();

// Sampler states
const SamplerDesc& SamplerLinearWrap();
const SamplerDesc& SamplerAnisoWrap();
const SamplerDesc& SamplerShadow();
const SamplerDesc& SamplerLinearClamp();
const SamplerDesc& SamplerVolumeWrap();
const SamplerDesc& SamplerPointWrap();
const SamplerDesc& SamplerPointClamp();
const SamplerDesc& SamplerPointBorder(StaticBorderColor borderColor = StaticBorderColor::OpaqueBlack);
const SamplerDesc& SamplerLinearBorder(StaticBorderColor borderColor = StaticBorderColor::OpaqueBlack);
const SamplerDesc& SamplerLinearMirror();

} // namespace CommonStates

} // namespace Luna