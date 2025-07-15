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

#include "STBTextureLoader.h"

#include "Graphics\Device.h"
#include "Graphics\Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "..\External\stb\stb_image.h"


namespace Luna
{

bool CreateSTBTextureFromMemory(IDevice* device, ITexture* texture, const std::string& textureName, std::byte* data, size_t dataSize, Format format, bool forceSrgb, bool retainData)
{
	bool is16Bit = stbi_is_16_bit_from_memory((const stbi_uc*)data, (int)dataSize);
	bool isHDR = stbi_is_hdr_from_memory((const stbi_uc*)data, (int)dataSize);

	int width = 0;
	int height = 0;
	int numComponents = 0;
	std::byte* imageData = nullptr;

	auto exitGuard = wil::scope_exit([&]()
		{
			if (imageData != nullptr)
			{
				stbi_image_free(imageData);
			}
		});

	Format fileFormat = Format::Unknown;

	if (isHDR)
	{
		imageData = (std::byte*)stbi_loadf_from_memory((const stbi_uc*)data, (int)dataSize, &width, &height, &numComponents, 4);

		switch (numComponents)
		{
		case 1: fileFormat = Format::R32_Float; break;
		case 2: fileFormat = Format::RG32_Float; break;
		case 3: fileFormat = Format::RGB32_Float; break;
		case 4: fileFormat = Format::RGBA32_Float; break;
		}
	}
	else if (is16Bit)
	{
		imageData = (std::byte*)stbi_load_16_from_memory((const stbi_uc*)data, (int)dataSize, &width, &height, &numComponents, 4);

		switch (numComponents)
		{
		case 1: fileFormat = Format::R16_UNorm; break;
		case 2: fileFormat = Format::RG16_UNorm; break;
		case 3:
		case 4: fileFormat = Format::RGBA16_UNorm; break;
		}
	}
	else
	{
		imageData = (std::byte*)stbi_load_from_memory((const stbi_uc*)data, (int)dataSize, &width, &height, &numComponents, 4);

		switch (numComponents)
		{
		case 1: fileFormat = Format::R8_UNorm; break;
		case 2: fileFormat = Format::RG8_UNorm; break;
		case 3:
		case 4: fileFormat = Format::RGBA8_UNorm; break;
		}
	}

	if (format == Format::Unknown)
	{
		format = fileFormat;
	}

	if (format == Format::Unknown)
	{
		LogWarning(LogSTB) << "File " << textureName << " has unknown format" << std::endl;
		return false;
	}

	if (imageData == nullptr)
	{
		LogWarning(LogSTB) << "Unable to load image data from file " << textureName << std::endl;
		return false;
	}

	TextureInitializer texInit{
		.format				= format,
		.dimension			= TextureDimension::Texture2D,
		.width				= (uint32_t)width,
		.height				= (uint32_t)height,
		.arraySizeOrDepth	= 1,
		.numMips			= 1,
		.baseData			= imageData,
		.totalBytes			= width * height * BitsPerPixel(format) / 8
	};

	size_t numBytes = 0;
	size_t rowBytes = 0;
	GetSurfaceInfo(width, height, format, &numBytes, &rowBytes, nullptr, nullptr, nullptr);

	TextureSubresourceData subResourceData{
		.data				= imageData,
		.rowPitch			= rowBytes,
		.slicePitch			= numBytes,
		.bufferOffset		= 0,
		.mipLevel			= 0,
		.baseArrayLayer		= 0,
		.layerCount			= 1,
		.width				= (uint32_t)width,
		.height				= (uint32_t)height,
		.depth				= 1
	};
	texInit.subResourceData.push_back(subResourceData);

	if (retainData)
	{
		texture->SetData(texInit.baseData, texInit.totalBytes);
	}

	return device->InitializeTexture(texture, texInit);
}

}