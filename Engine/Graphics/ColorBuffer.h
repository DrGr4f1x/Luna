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
#include "Graphics\PixelBuffer.h"

namespace Luna
{

struct ColorBufferDesc
{
	std::string name;
	ResourceType resourceType{ ResourceType::Texture2D };
	uint64_t width{ 0 };
	uint32_t height{ 0 };
	uint32_t arraySizeOrDepth{ 0 };
	uint32_t numMips{ 1 };
	uint32_t numSamples{ 1 };
	Format format{ Format::Unknown };
	uint32_t numFragments{ 1 };
	Color clearColor{ DirectX::Colors::Black };

	ColorBufferDesc& SetName(const std::string& value) { name = value; return *this; }
	constexpr ColorBufferDesc& SetResourceType(ResourceType value) noexcept { resourceType = value; return *this; }
	constexpr ColorBufferDesc& SetWidth(uint64_t value) noexcept { width = value; return *this; }
	constexpr ColorBufferDesc& SetHeight(uint32_t value) noexcept { height = value; return *this; }
	constexpr ColorBufferDesc& SetArraySize(uint32_t value) noexcept { arraySizeOrDepth = value; return *this; }
	constexpr ColorBufferDesc& SetDepth(uint32_t value) noexcept { arraySizeOrDepth = value; return *this; }
	constexpr ColorBufferDesc& SetNumMips(uint32_t value) noexcept { numMips = value; return *this; }
	constexpr ColorBufferDesc& SetNumSamples(uint32_t value) noexcept { numSamples = value; return *this; }
	constexpr ColorBufferDesc& SetFormat(Format value) noexcept { format = value; return *this; }
	constexpr ColorBufferDesc& SetNumFragments(uint32_t value) noexcept { numFragments = value; return *this; }
	ColorBufferDesc& SetClearColor(Color value) noexcept { clearColor = value; return *this; }
};


class __declspec(uuid("F49A3931-9E4F-4B90-8BFD-B912A13ED31E")) IColorBuffer : public IPixelBuffer
{
public:
	virtual ~IColorBuffer() = default;

	virtual Color GetClearColor() const noexcept = 0;
};
using ColorBufferHandle = Microsoft::WRL::ComPtr<IColorBuffer>;

} // namespace Luna