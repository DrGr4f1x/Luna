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

#include "DDSTextureLoader.h"

#include "Graphics\Device.h"
#include "Graphics\Limits.h"
#include "Graphics\Texture.h"
#include "Graphics\DX12\DirectXCommon.h"
#include "dds.h"


namespace Luna
{

//--------------------------------------------------------------------------------------
#define ISBITMASK( r,g,b,a ) ( ddpf.RBitMask == r && ddpf.GBitMask == g && ddpf.BBitMask == b && ddpf.ABitMask == a )


static DXGI_FORMAT DDSPixelFormatToDxgi(const DDS_PIXELFORMAT& ddpf)
{
	if (ddpf.flags & DDS_RGB)
	{
		// Note that sRGB formats are written using the "DX10" extended header

		switch (ddpf.RGBBitCount)
		{
		case 32:
			if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
			{
				return DXGI_FORMAT_R8G8B8A8_UNORM;
			}

			if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
			{
				return DXGI_FORMAT_B8G8R8A8_UNORM;
			}

			if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000))
			{
				return DXGI_FORMAT_B8G8R8X8_UNORM;
			}

			// No DXGI format maps to ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0x00000000) aka D3DFMT_X8B8G8R8

			// Note that many common DDS reader/writers (including D3DX) swap the
			// the RED/BLUE masks for 10:10:10:2 formats. We assumme
			// below that the 'backwards' header mask is being used since it is most
			// likely written by D3DX. The more robust solution is to use the 'DX10'
			// header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

			// For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
			if (ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
			{
				return DXGI_FORMAT_R10G10B10A2_UNORM;
			}

			// No DXGI format maps to ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) aka D3DFMT_A2R10G10B10

			if (ISBITMASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
			{
				return DXGI_FORMAT_R16G16_UNORM;
			}

			if (ISBITMASK(0xffffffff, 0x00000000, 0x00000000, 0x00000000))
			{
				// Only 32-bit color channel format in D3D9 was R32F
				return DXGI_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114
			}
			break;

		case 24:
			// No 24bpp DXGI formats aka D3DFMT_R8G8B8
			break;

		case 16:
			if (ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x8000))
			{
				return DXGI_FORMAT_B5G5R5A1_UNORM;
			}
			if (ISBITMASK(0xf800, 0x07e0, 0x001f, 0x0000))
			{
				return DXGI_FORMAT_B5G6R5_UNORM;
			}

			// No DXGI format maps to ISBITMASK(0x7c00,0x03e0,0x001f,0x0000) aka D3DFMT_X1R5G5B5

			if (ISBITMASK(0x0f00, 0x00f0, 0x000f, 0xf000))
			{
				return DXGI_FORMAT_B4G4R4A4_UNORM;
			}

			// No DXGI format maps to ISBITMASK(0x0f00,0x00f0,0x000f,0x0000) aka D3DFMT_X4R4G4B4

			// No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8, D3DFMT_A8P8, etc.
			break;
		}
	}
	else if (ddpf.flags & DDS_LUMINANCE)
	{
		if (8 == ddpf.RGBBitCount)
		{
			if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x00000000))
			{
				return DXGI_FORMAT_R8_UNORM; // D3DX10/11 writes this out as DX10 extension
			}

			// No DXGI format maps to ISBITMASK(0x0f,0x00,0x00,0xf0) aka D3DFMT_A4L4
		}

		if (16 == ddpf.RGBBitCount)
		{
			if (ISBITMASK(0x0000ffff, 0x00000000, 0x00000000, 0x00000000))
			{
				return DXGI_FORMAT_R16_UNORM; // D3DX10/11 writes this out as DX10 extension
			}
			if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
			{
				return DXGI_FORMAT_R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension
			}
		}
	}
	else if (ddpf.flags & DDS_ALPHA)
	{
		if (8 == ddpf.RGBBitCount)
		{
			return DXGI_FORMAT_A8_UNORM;
		}
	}
	else if (ddpf.flags & DDS_FOURCC)
	{
		if (MAKEFOURCC('D', 'X', 'T', '1') == ddpf.fourCC)
		{
			return DXGI_FORMAT_BC1_UNORM;
		}
		if (MAKEFOURCC('D', 'X', 'T', '3') == ddpf.fourCC)
		{
			return DXGI_FORMAT_BC2_UNORM;
		}
		if (MAKEFOURCC('D', 'X', 'T', '5') == ddpf.fourCC)
		{
			return DXGI_FORMAT_BC3_UNORM;
		}

		// While pre-mulitplied alpha isn't directly supported by the DXGI formats,
		// they are basically the same as these BC formats so they can be mapped
		if (MAKEFOURCC('D', 'X', 'T', '2') == ddpf.fourCC)
		{
			return DXGI_FORMAT_BC2_UNORM;
		}
		if (MAKEFOURCC('D', 'X', 'T', '4') == ddpf.fourCC)
		{
			return DXGI_FORMAT_BC3_UNORM;
		}

		if (MAKEFOURCC('A', 'T', 'I', '1') == ddpf.fourCC)
		{
			return DXGI_FORMAT_BC4_UNORM;
		}
		if (MAKEFOURCC('B', 'C', '4', 'U') == ddpf.fourCC)
		{
			return DXGI_FORMAT_BC4_UNORM;
		}
		if (MAKEFOURCC('B', 'C', '4', 'S') == ddpf.fourCC)
		{
			return DXGI_FORMAT_BC4_SNORM;
		}

		if (MAKEFOURCC('A', 'T', 'I', '2') == ddpf.fourCC)
		{
			return DXGI_FORMAT_BC5_UNORM;
		}
		if (MAKEFOURCC('B', 'C', '5', 'U') == ddpf.fourCC)
		{
			return DXGI_FORMAT_BC5_UNORM;
		}
		if (MAKEFOURCC('B', 'C', '5', 'S') == ddpf.fourCC)
		{
			return DXGI_FORMAT_BC5_SNORM;
		}

		// BC6H and BC7 are written using the "DX10" extended header

		if (MAKEFOURCC('R', 'G', 'B', 'G') == ddpf.fourCC)
		{
			return DXGI_FORMAT_R8G8_B8G8_UNORM;
		}
		if (MAKEFOURCC('G', 'R', 'G', 'B') == ddpf.fourCC)
		{
			return DXGI_FORMAT_G8R8_G8B8_UNORM;
		}

		if (MAKEFOURCC('Y', 'U', 'Y', '2') == ddpf.fourCC)
		{
			return DXGI_FORMAT_YUY2;
		}

		// Check for D3DFORMAT enums being set here
		switch (ddpf.fourCC)
		{
		case 36: // D3DFMT_A16B16G16R16
			return DXGI_FORMAT_R16G16B16A16_UNORM;

		case 110: // D3DFMT_Q16W16V16U16
			return DXGI_FORMAT_R16G16B16A16_SNORM;

		case 111: // D3DFMT_R16F
			return DXGI_FORMAT_R16_FLOAT;

		case 112: // D3DFMT_G16R16F
			return DXGI_FORMAT_R16G16_FLOAT;

		case 113: // D3DFMT_A16B16G16R16F
			return DXGI_FORMAT_R16G16B16A16_FLOAT;

		case 114: // D3DFMT_R32F
			return DXGI_FORMAT_R32_FLOAT;

		case 115: // D3DFMT_G32R32F
			return DXGI_FORMAT_R32G32_FLOAT;

		case 116: // D3DFMT_A32B32G32R32F
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		}
	}

	return DXGI_FORMAT_UNKNOWN;
}


bool CreateTextureFromDDS(
	IDevice* device, 
	ITexture* texture, 
	const std::string& textureName, 
	const DDS_HEADER* header, 
	std::byte* bitData, 
	size_t bitDataSize,
	size_t maxSize,
	Format format, 
	bool forceSrgb,
	bool retainData)
{
	uint32_t width = header->width;
	uint32_t height = header->height;
	uint32_t depth = header->depth;
	uint32_t arraySize = 1;
	Format fileFormat = Format::Unknown;
	bool isCubeMap = false;
	TextureDimension dimension = TextureDimension::Unknown;

	size_t mipCount = header->mipMapCount;
	if (mipCount == 0)
	{ 
		mipCount = 1;
	}

	if ((header->ddspf.flags & DDS_FOURCC) && (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC))
	{
		auto d3d10ext = reinterpret_cast<const DDS_HEADER_DXT10*>((const char*)header + sizeof(DDS_HEADER));

		arraySize = d3d10ext->arraySize;
		bool isArray = arraySize > 1;

		if (arraySize == 0)
		{
			LogWarning(LogDDS) << "File " << textureName << " has invalid array size 0" << std::endl;
			return false;
		}

		fileFormat = DX12::DxgiToFormat(d3d10ext->dxgiFormat);
		if (fileFormat == Format::Unknown)
		{
			LogWarning(LogDDS) << "File " << textureName << " has unsupported DXGI format " << d3d10ext->dxgiFormat << std::endl;
			return false;
		}

		if (format == Format::Unknown)
		{
			format = fileFormat;
		}

		switch (d3d10ext->resourceDimension)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			// D3DX writes 1D textures with a fixed Height of 1
			if ((header->flags & DDS_HEIGHT) && height != 1)
			{
				LogWarning(LogDDS) << "File " << textureName << " is 1D, but has invalid height " << height << " (must be 1)" << std::endl;
				return false;
			}
			height = depth = 1;
			dimension = isArray ? TextureDimension::Texture1D_Array : TextureDimension::Texture1D;
			break;

		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			if (d3d10ext->miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE)
			{
				arraySize *= 6;
				isCubeMap = true;

				dimension = isArray ? TextureDimension::TextureCube_Array : TextureDimension::TextureCube;
			}
			else
			{
				dimension = isArray ? TextureDimension::Texture2D_Array : TextureDimension::Texture2D;
			}
			depth = 1;
			break;

		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			if (!(header->flags & DDS_HEADER_FLAGS_VOLUME))
			{
				LogWarning(LogDDS) << "File " << textureName << " is a 3D texture, but is missing the DDS_HEADER_FLAGS_VOLUME flag" << std::endl;
				return false;
			}

			if (arraySize > 1)
			{
				LogWarning(LogDDS) << "File " << textureName << " is a 3D texture, but has array size " << arraySize << ", which is not supported" << std::endl;
				return false;
			}

			dimension = TextureDimension::Texture3D;
			break;

		default:
			LogWarning(LogDDS) << "File " << textureName << " has unknown dimensions" << std::endl;
			return false;
		}
	}
	else
	{
		DXGI_FORMAT dxgiFormat = DDSPixelFormatToDxgi(header->ddspf);
		fileFormat = DX12::DxgiToFormat(dxgiFormat);

		if (fileFormat == Format::Unknown)
		{
			LogWarning(LogDDS) << "File " << textureName << " has unsupported DXGI format " << dxgiFormat << std::endl;
			return false;
		}

		if (format == Format::Unknown)
		{
			format = fileFormat;
		}

		if (header->flags & DDS_HEADER_FLAGS_VOLUME)
		{
			dimension = TextureDimension::Texture3D;
		}
		else
		{
			if (header->caps2 & DDS_CUBEMAP)
			{
				// We require all six faces to be defined
				if ((header->caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
				{
					return false;
				}

				arraySize = 6;
				isCubeMap = true;

				dimension = TextureDimension::TextureCube;
			}
			else
			{
				dimension = TextureDimension::Texture2D;
			}

			depth = 1;

			// Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
		}

		assert(BitsPerPixel(format) != 0);
	}

	// Bound sizes (for security purposes we don't trust DDS file metadata larger than the D3D 11.x hardware requirements)
	if (mipCount > D3D12_REQ_MIP_LEVELS)
	{
		LogWarning(LogDDS) << "File " << textureName << " has invalid mip count " << mipCount << std::endl;
		return false;
	}

	switch (dimension)
	{
	case TextureDimension::Texture1D:
	case TextureDimension::Texture1D_Array:
		if ((arraySize > Limits::MaxTexture1DArrayElements()) ||
			(width > Limits::MaxTextureDimension1D()))
		{
			LogWarning(LogDDS) << "File " << textureName << " is a 1D texture, but has invalid dimensions" <<  std::endl;
			return false;
		}
		break;

	case TextureDimension::Texture2D:
	case TextureDimension::Texture2D_Array:
		if ((arraySize > Limits::MaxTexture2DArrayElements()) ||
			(width > Limits::MaxTextureDimension2D()) ||
			(height > Limits::MaxTextureDimension2D()))
		{
			LogWarning(LogDDS) << "File " << textureName << " is a 2D texture, but has invalid dimensions" << std::endl;
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
			LogWarning(LogDDS) << "File " << textureName << " is a cube-map texture, but has invalid dimensions" << std::endl;
			return false;
		}
		break;

	case TextureDimension::Texture3D:
		if ((arraySize > 1) ||
			(width > Limits::MaxTextureDimension3D()) ||
			(height > Limits::MaxTextureDimension3D()) ||
			(depth > Limits::MaxTextureDimension3D()))
		{
			LogWarning(LogDDS) << "File " << textureName << " is a 3D texture, but has invalid dimensions" << std::endl;
			return false;
		}
		break;

	default:
		LogWarning(LogDDS) << "File " << textureName << " has invalid dimension" << std::endl;
		return false;
	}

	// Fill texture initializer data
	TextureInitializer texInit{
		.format = format,
		.dimension = dimension,
		.width = width,
		.height = height,
		.arraySizeOrDepth = arraySize > 1 ? arraySize : depth,
		.numMips = static_cast<uint32_t>(mipCount)
	};
	const uint32_t numSubresources = static_cast<uint32_t>(mipCount) * arraySize;
	texInit.subResourceData.reserve(numSubresources);
	for (uint32_t i = 0; i < numSubresources; ++i)
	{
		texInit.subResourceData.push_back(TextureSubresourceData{});
	}

	size_t skipMip = 0;
	bool dataOk = FillTextureInitializer(width, height, depth, mipCount, arraySize, format, maxSize, bitDataSize, bitData, skipMip, texInit);

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


bool CreateDDSTextureFromMemory(IDevice* device, ITexture* texture, const std::string& textureName, std::byte* data, size_t dataSize, Format format, bool forceSrgb, bool retainData)
{
	assert(device != nullptr);
	assert(texture != nullptr);
	assert(data != nullptr);

	// Validate DDS file in memory
	if (dataSize < (sizeof(uint32_t) + sizeof(DDS_HEADER)))
	{
		LogWarning(LogDDS) << "File " << textureName << " is smaller than the minimum size of a DDS texture." << std::endl;
		return false;
	}

	// Validate the DDS magic number
	uint32_t dwMagicNumber = *(const uint32_t*)(data);
	if (dwMagicNumber != DDS_MAGIC)
	{
		LogWarning(LogDDS) << "File " << textureName << " does not have a valid DDS magic number." << std::endl;
		return false;
	}

	auto header = reinterpret_cast<const DDS_HEADER*>(data + sizeof(uint32_t));

	// Verify header to validate DDS file
	if (header->size != sizeof(DDS_HEADER) || header->ddspf.size != sizeof(DDS_PIXELFORMAT))
	{
		LogWarning(LogDDS) << "File " << textureName << " has invalid DDS header size." << std::endl;
		return false;
	}

	size_t offset = sizeof(DDS_HEADER) + sizeof(uint32_t);

	// Check for extensions
	if (header->ddspf.flags & DDS_FOURCC)
	{
		if (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC)
		{
			offset += sizeof(DDS_HEADER_DXT10);
		}
	}

	// Must be long enough for all headers and magic value
	if (dataSize < offset)
	{
		LogWarning(LogDDS) << "File " << textureName << " does not contain enough bytes to hold the DDS header and magic number." << std::endl;
		return false;
	}

	const size_t maxSize = 0;
	return CreateTextureFromDDS(device, texture, textureName, header, data + offset, dataSize - offset, maxSize, format, forceSrgb, retainData);
}

} // namespace Luna