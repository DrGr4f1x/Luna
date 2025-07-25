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

struct SamplerDesc
{
	TextureFilter filter{ TextureFilter::Anisotropic };
	TextureAddress addressU{ TextureAddress::Wrap };
	TextureAddress addressV{ TextureAddress::Wrap };
	TextureAddress addressW{ TextureAddress::Wrap };
	float mipLODBias{ 0.0f };
	uint32_t maxAnisotropy{ 16 };
	ComparisonFunc comparisonFunc{ ComparisonFunc::None };
	Color borderColor{ DirectX::Colors::Black };
	StaticBorderColor staticBorderColor{ StaticBorderColor::OpaqueWhite };
	float minLOD{ 0.0f };
	float maxLOD{ FLT_MAX };

	constexpr SamplerDesc& SetFilter(TextureFilter value) noexcept { filter = value; return *this; }
	constexpr SamplerDesc& SetAddressU(TextureAddress value) noexcept { addressU = value; return *this; }
	constexpr SamplerDesc& SetAddressV(TextureAddress value) noexcept { addressV = value; return *this; }
	constexpr SamplerDesc& SetAddressW(TextureAddress value) noexcept { addressW = value; return *this; }
	constexpr SamplerDesc& SetMipLODBias(float value) noexcept { mipLODBias = value; return *this; }
	constexpr SamplerDesc& SetMaxAnisotropy(uint32_t value) noexcept { maxAnisotropy = value; return *this; }
	constexpr SamplerDesc& SetComparisonFunc(ComparisonFunc value) noexcept { comparisonFunc = value; return *this; }
	SamplerDesc& SetBorderColor(Color value) noexcept { borderColor = value; return *this; }
	constexpr SamplerDesc& SetStaticBorderColor(StaticBorderColor value) noexcept { staticBorderColor = value; return *this; }
	constexpr SamplerDesc& SetMinLOD(float value) noexcept { minLOD = value; return *this; }
	constexpr SamplerDesc& SetMaxLOD(float value) noexcept { maxLOD = value; return *this; }
};


class ISampler
{
public:
	virtual ~ISampler() = default;
};

using SamplerPtr = std::shared_ptr<ISampler>;

} // namespace Luna