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
#include "Graphics\PixelBuffer.h"


namespace Luna
{

struct TextureDesc
{
	std::string name;
	ResourceType resourceType{ ResourceType::Texture2D };
	uint64_t width{ 0 };
	uint32_t height{ 0 };
	uint32_t arraySizeOrDepth{ 1 };
	uint32_t numMips{ 1 };
	Format format{ Format::Unknown };

	TextureDesc& SetName(const std::string& value) { name = value; return *this; }
	constexpr TextureDesc& SetResourceType(ResourceType value) { resourceType = value; return *this; }
	constexpr TextureDesc& SetWidth(uint64_t value) { width = value; return *this; }
	constexpr TextureDesc& SetHeight(uint32_t value) { height = value; return *this; }
	constexpr TextureDesc& SetArraySize(uint32_t value) { arraySizeOrDepth = value; return *this; }
	constexpr TextureDesc& SetDepth(uint32_t value) { arraySizeOrDepth = value; return *this; }
	constexpr TextureDesc& SetNumMips(uint32_t value) { numMips = value; return *this; }
	constexpr TextureDesc& SetFormat(Format value) { format = value; return *this; }
};


class Texture : public PixelBuffer
{
public:
	void Load(const std::string& filename, Format format = Format::Unknown, bool sRgb = false);
};

} // namespace Luna