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
#include "Graphics\DeviceCaps.h"
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

bool FillTextureInitializerKTX(GraphicsApi api, ktxTexture* texture, TextureInitializer& outTexInit)
{
	const uint64_t width = texture->baseWidth;
	const uint32_t height = texture->baseHeight;
	const uint32_t depth = texture->baseDepth;
	const uint32_t mipCount = texture->numLevels;
	const uint32_t arraySize = texture->numLayers;
	const uint32_t numCubemapFaces = texture->numFaces;
	const bool isArray = texture->isArray;
	const bool isCubemap = texture->isCubemap;
	const bool isCubemapArray = isArray && isCubemap;

	ktx_uint8_t* ktxTextureData = ktxTexture_GetData(texture);
	ktx_size_t ktxTextureSize = ktxTexture_GetDataSize(texture);

	outTexInit.width = std::max<uint64_t>(width, 1);
	outTexInit.height = std::max<uint32_t>(height, 1);
	outTexInit.arraySizeOrDepth = std::max<uint32_t>(depth, 1);
	if (isCubemap || isArray)
	{
		outTexInit.arraySizeOrDepth = arraySize;
		if (isCubemap)
			outTexInit.arraySizeOrDepth *= 6;
	}
	outTexInit.numMips = mipCount;
	outTexInit.baseData = (std::byte*)ktxTextureData;
	outTexInit.totalBytes = ktxTextureSize;

	const uint32_t numSubresources = outTexInit.arraySizeOrDepth * outTexInit.numMips;
	outTexInit.subResourceData.reserve(numSubresources);
	for (uint32_t i = 0; i < numSubresources; ++i)
	{
		outTexInit.subResourceData.push_back(TextureSubresourceData{});
	}

	const uint32_t numFaces = isCubemap ? 6 : 1;

	for (uint32_t face = 0; face < numFaces; ++face)
	{
		for (uint32_t slice = 0; slice < arraySize; ++slice)
		{
			for (uint32_t mipLevel = 0; mipLevel < mipCount; ++mipLevel)
			{
				ktx_size_t offset = 0;
				KTX_error_code ret = ktxTexture_GetImageOffset(texture, mipLevel, slice, face, &offset);
				assert(ret == KTX_SUCCESS);

				size_t w = std::max<size_t>(texture->baseWidth >> mipLevel, 1);
				size_t h = std::max<size_t>(texture->baseHeight >> mipLevel, 1);
				size_t numBytes = 0;
				size_t rowBytes = 0;
				GetSurfaceInfo(w, h, outTexInit.format, &numBytes, &rowBytes, nullptr, nullptr, nullptr);

				const uint32_t subIndex = outTexInit.GetSubresourceIndex(api, face, slice, mipLevel);
				auto& subresource = outTexInit.subResourceData[subIndex];
				subresource.data = (std::byte*)ktxTextureData + offset;
				subresource.rowPitch = rowBytes;
				subresource.slicePitch = numBytes;

				subresource.bufferOffset = offset;
				subresource.mipLevel = mipLevel;
				subresource.baseArrayLayer = isCubemap ? (6 * slice + face) : slice;
				subresource.layerCount = 1;
				subresource.width = (uint32_t)w;
				subresource.height = (uint32_t)h;
				subresource.depth = 1;
			}
		}
	}
	
	return true;
}


bool CreateKTXTextureFromMemory(IDevice* device, ITexture* texture, const std::string& textureName, std::byte* data, size_t dataSize, Format format, bool forceSrgb, bool retainData)
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

	const DeviceCaps& caps = device->GetDeviceCaps();

	switch (dimension)
	{
	case TextureDimension::Texture1D:
	case TextureDimension::Texture1D_Array:
		if ((arraySize > caps.dimensions.textureLayerMaxNum) ||
			(width > caps.dimensions.texture1DMaxDim))
		{
			LogWarning(LogKTX) << "File " << textureName << " is a 1D texture, but has invalid dimensions" << std::endl;
			return false;
		}
		break;

	case TextureDimension::Texture2D:
	case TextureDimension::Texture2D_Array:
		if ((arraySize > caps.dimensions.textureLayerMaxNum) ||
			(width > caps.dimensions.texture2DMaxDim) ||
			(height > caps.dimensions.texture2DMaxDim))
		{
			LogWarning(LogKTX) << "File " << textureName << " is a 2D texture, but has invalid dimensions" << std::endl;
			return false;
		}
		break;

	case TextureDimension::TextureCube:
	case TextureDimension::TextureCube_Array:
		// This is the right bound because we set arraySize to (NumCubes*6) above
		if ((arraySize > caps.dimensions.textureLayerMaxNum) ||
			(width > caps.dimensions.textureCubeMaxDim) ||
			(height > caps.dimensions.textureCubeMaxDim))
		{
			LogWarning(LogKTX) << "File " << textureName << " is a cube-map texture, but has invalid dimensions" << std::endl;
			return false;
		}
		break;

	case TextureDimension::Texture3D:
		if ((arraySize > 1) ||
			(width > caps.dimensions.texture3DMaxDim) ||
			(height > caps.dimensions.texture3DMaxDim) ||
			(depth > caps.dimensions.texture3DMaxDim))
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

	size_t effectiveArraySize = depth;

	if (isCubemap || isArray)
	{
		effectiveArraySize = arraySize;
		if (isCubemap)
			effectiveArraySize *= 6;
	}

	TextureInitializer texInit{ .format = format, .dimension = dimension, };
	const size_t maxSize = 0;
	size_t skipMip = 0;

	bool dataOk = FillTextureInitializerKTX(caps.api, kTexture, texInit);

	if (dataOk)
	{
		if (retainData)
		{
			texture->SetData(texInit.baseData, texInit.totalBytes);
		}

		return device->InitializeTexture(texture, texInit);
	}

	return false;
}

} // namespace Luna