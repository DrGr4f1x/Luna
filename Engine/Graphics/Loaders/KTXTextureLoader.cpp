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

#include "KTXTextureLoader.h"

#include "Graphics\Device.h"
#include "Graphics\Limits.h"
#include "Graphics\Texture.h"
#include "Graphics\Vulkan\VulkanCommon.h"

#include "ktx.h"
#include "ktxvulkan.h"


namespace Luna
{

TextureDimension GetTextureDimension(ktxTexture* texture)
{
	const uint64_t width = texture->baseWidth;
	const uint32_t height = texture->baseHeight;
	const uint32_t depth = texture->baseDepth;
	const uint32_t numArraySlices = texture->numLayers;
	const uint32_t numCubemapFaces = texture->numFaces;
	const bool isArray = texture->isArray;
	const bool isCubemap = texture->isCubemap;
	
	if (depth > 1)
	{
		return TextureDimension::Texture3D;
	}
	else if (isCubemap)
	{
		return isArray ? TextureDimension::TextureCube_Array : TextureDimension::TextureCube;
	}
	else if (height > 1)
	{ 
		return isArray ? TextureDimension::Texture2D_Array : TextureDimension::Texture2D;
	}
	else
	{
		return isArray ? TextureDimension::Texture1D_Array : TextureDimension::Texture1D;
	}
}


bool CreateKTXTextureFromMemory(IDevice* device, ITexture* texture, const std::string& textureName, std::byte* data, size_t dataSize, Format format, bool forceSrgb)
{
	assert(device != nullptr);
	assert(texture != nullptr);
	assert(data != nullptr);

	ktxTexture* kTexture = nullptr;
	auto result = ktxTexture_CreateFromMemory((const ktx_uint8_t*)data, dataSize, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &kTexture);
	if (result != KTX_SUCCESS)
	{
		LogWarning(LogKTX) << "Failed to create KTX texture from file " << textureName << std::endl;
		return false;
	}

	const uint64_t width = kTexture->baseWidth;
	const uint32_t height = kTexture->baseHeight;
	const uint32_t depth = kTexture->baseDepth;
	const uint32_t mipCount = kTexture->numLevels;
	const uint32_t arraySize = kTexture->numLayers;
	const uint32_t numCubemapFaces = kTexture->numFaces;
	const bool isArray = kTexture->isArray;
	const bool isCubemap = kTexture->isCubemap;

	TextureDimension dimension = GetTextureDimension(kTexture);

	switch (dimension)
	{
	case TextureDimension::Texture1D:
	case TextureDimension::Texture1D_Array:
		if ((arraySize > Limits::MaxTexture1DArrayElements()) ||
			(width > Limits::MaxTextureDimension1D()))
		{
			LogWarning(LogKTX) << "File " << textureName << " is a 1D texture, but has invalid dimensions" << std::endl;
			return false;
		}
		break;

	case TextureDimension::Texture2D:
	case TextureDimension::Texture2D_Array:
		if ((arraySize > Limits::MaxTexture2DArrayElements()) ||
			(width > Limits::MaxTextureDimension2D()) ||
			(height > Limits::MaxTextureDimension2D()))
		{
			LogWarning(LogKTX) << "File " << textureName << " is a 2D texture, but has invalid dimensions" << std::endl;
			return false;
		}
		break;

	case TextureDimension::TextureCube:
	case TextureDimension::TextureCube_Array:
		// This is the right bound because we set arraySize to (NumCubes*6) above
		if ((arraySize > Limits::MaxTexture2DArrayElements()) ||
			(width > Limits::MaxTextureDimensionCube()) ||
			(height > Limits::MaxTextureDimensionCube()))
		{
			LogWarning(LogKTX) << "File " << textureName << " is a cube-map texture, but has invalid dimensions" << std::endl;
			return false;
		}
		break;

	case TextureDimension::Texture3D:
		if ((arraySize > 1) ||
			(width > Limits::MaxTextureDimension3D()) ||
			(height > Limits::MaxTextureDimension3D()) ||
			(depth > Limits::MaxTextureDimension3D()))
		{
			LogWarning(LogKTX) << "File " << textureName << " is a 3D texture, but has invalid dimensions" << std::endl;
			return false;
		}
		break;

	default:
		LogWarning(LogKTX) << "File " << textureName << " has invalid dimension" << std::endl;
		return false;
	}

	if (format == Format::Unknown)
	{
		VkFormat ktxVulkanFormat = ktxTexture_GetVkFormat(kTexture);
		Format fileFormat = VK::VulkanToFormat(ktxVulkanFormat);
		if (fileFormat == Format::Unknown)
		{
			LogWarning(LogKTX) << "File " << textureName << " has unknown or unsupported format" << std::endl;
			return false;
		}
		format = fileFormat;
	}

	std::byte* bitData = (std::byte*)ktxTexture_GetData(kTexture);
	const size_t bitDataSize = ktxTexture_GetDataSize(kTexture);

	TextureInitializer texInit{
		.format = format,
		.dimension = dimension,
		.width = width,
		.height = height,
		.arraySizeOrDepth = isArray ? arraySize : depth,
		.numMips = static_cast<uint32_t>(mipCount),
		.baseData = bitData,
		.totalBytes = bitDataSize
	};

	const uint32_t numSubresources = arraySize * mipCount;
	texInit.subResourceData.reserve(numSubresources);
	for (uint32_t i = 0; i < numSubresources; ++i)
	{
		texInit.subResourceData.push_back(TextureSubresourceData{});
	}

	const size_t maxSize = 0;
	size_t skipMip = 0;
	bool dataOk = FillTextureInitializer(width, height, depth, mipCount, arraySize, format, maxSize, bitDataSize, bitData, skipMip, texInit);

	if (dataOk)
	{
		for (uint32_t i = 0; i < numSubresources; ++i)
		{
			std::byte* imageData = texInit.subResourceData[i].data;
			size_t height = texInit.subResourceData[i].height;
			size_t heightInBlocks = texInit.subResourceData[i].heightInBlocks;
			size_t rowPitch = texInit.subResourceData[i].rowPitch;

			size_t effectiveHeight = heightInBlocks > 0 ? heightInBlocks : height;
		}

		return device->InitializeTexture(texture, texInit);
	}

	return false;
}

} // namespace Luna