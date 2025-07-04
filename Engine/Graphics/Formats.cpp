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

#include "Formats.h"


namespace Luna
{

uint32_t BitsPerPixel(Format format)
{
	using enum Format;

	switch (format)
	{
	case RGBA32_Float:
	case RGBA32_SInt:
	case RGBA32_UInt:
		return 128;

	case RGB32_Float:
	case RGB32_SInt:
	case RGB32_UInt:
		return 96;

	case RGBA16_Float:
	case RGBA16_SInt:
	case RGBA16_SNorm:
	case RGBA16_UInt:
	case RGBA16_UNorm:
	case RG32_Float:
	case RG32_SInt:
	case RG32_UInt:
	case D32S8:
	case X32G8_UInt:
		return 64;

	case R10G10B10A2_UNorm:
	case R11G11B10_Float:
	case BGRA8_UNorm:
	case RGBA8_SInt:
	case RGBA8_SNorm:
	case RGBA8_UInt:
	case RGBA8_UNorm:
	case SRGBA8_UNorm:
	case SBGRA8_UNorm:
	case RG16_Float:
	case RG16_SInt:
	case RG16_SNorm:
	case RG16_UInt:
	case RG16_UNorm:
	case R32_Float:
	case R32_SInt:
	case R32_UInt:
	case D32:
	case D24S8:
	case X24G8_UInt:
		return 32;

	case RG8_SInt:
	case RG8_SNorm:
	case RG8_UInt:
	case RG8_UNorm:
	case R16_Float:
	case R16_SInt:
	case R16_SNorm:
	case R16_UInt:
	case R16_UNorm:
	case BGRA4_UNorm:
	case B5G5R5A1_UNorm:
	case B5G6R5_UNorm:
	case D16:
		return 16;

	case R8_SInt:
	case R8_SNorm:
	case R8_UInt:
	case R8_UNorm:
		return 8;

	case BC1_UNorm:
	case BC1_UNorm_Srgb:
	case BC4_UNorm:
	case BC4_SNorm:
		return 4;

	case BC2_UNorm:
	case BC2_UNorm_Srgb:
	case BC3_UNorm:
	case BC3_UNorm_Srgb:
	case BC5_UNorm:
	case BC5_SNorm:
	case BC6H_SFloat:
	case BC6H_UFloat:
	case BC7_UNorm:
	case BC7_UNorm_Srgb:
		return 8;

	default:
		assert(false);
		return 0;
	}
}


uint32_t BlockSize(Format format)
{
	using enum Format;

	switch (format)
	{
	case RGBA32_Float:
	case RGBA32_SInt:
	case RGBA32_UInt:
		return 16;

	case RGB32_Float:
	case RGB32_SInt:
	case RGB32_UInt:
		return 12;

	case RGBA16_Float:
	case RGBA16_SInt:
	case RGBA16_SNorm:
	case RGBA16_UInt:
	case RGBA16_UNorm:
	case RG32_Float:
	case RG32_SInt:
	case RG32_UInt:
		return 8;

	case D32S8:
	case X32G8_UInt:
		return 5;

	case R10G10B10A2_UNorm:
	case R11G11B10_Float:
	case BGRA8_UNorm:
	case SBGRA8_UNorm:
	case RGBA8_SInt:
	case RGBA8_SNorm:
	case RGBA8_UInt:
	case RGBA8_UNorm:
	case SRGBA8_UNorm:
	case RG16_Float:
	case RG16_SInt:
	case RG16_SNorm:
	case RG16_UInt:
	case RG16_UNorm:
	case R32_Float:
	case R32_SInt:
	case R32_UInt:
	case D32:
	case D24S8:
	case X24G8_UInt:
		return 4;

	case RG8_SInt:
	case RG8_SNorm:
	case RG8_UInt:
	case RG8_UNorm:
	case R16_Float:
	case R16_SInt:
	case R16_SNorm:
	case R16_UInt:
	case R16_UNorm:
	case BGRA4_UNorm:
	case B5G5R5A1_UNorm:
	case B5G6R5_UNorm:
	case D16:
		return 2;

	case R8_SInt:
	case R8_SNorm:
	case R8_UInt:
	case R8_UNorm:
		return 1;

	case BC1_UNorm:
	case BC1_UNorm_Srgb:
	case BC4_UNorm:
	case BC4_SNorm:
		return 8;

	case BC2_UNorm:
	case BC2_UNorm_Srgb:
	case BC3_UNorm:
	case BC3_UNorm_Srgb:
	case BC5_UNorm:
	case BC5_SNorm:
	case BC6H_SFloat:
	case BC6H_UFloat:
	case BC7_UNorm:
	case BC7_UNorm_Srgb:
		return 16;

	default:
		assert(false);
		return 0;
	}
}


void GetSurfaceInfo(
	size_t width, 
	size_t height, 
	Format format, 
	size_t* outNumBytes, 
	size_t* outRowBytes, 
	size_t* outNumRows, 
	size_t* outWidthInBlocks, 
	size_t* outHeightInBlocks)
{
	size_t numBytes = 0;
	size_t rowBytes = 0;
	size_t numRows = 0;
	size_t numBlocksWide = 0;
	size_t numBlocksHigh = 0;

	bool bc = false;
	size_t bpe = 0;

	switch (format)
	{
	case Format::BC1_UNorm:
	case Format::BC1_UNorm_Srgb:
	case Format::BC4_SNorm:
	case Format::BC4_UNorm:
		bc = true;
		bpe = 8;
		break;

	case Format::BC2_UNorm:
	case Format::BC2_UNorm_Srgb:
	case Format::BC3_UNorm:
	case Format::BC3_UNorm_Srgb:
	case Format::BC5_SNorm:
	case Format::BC5_UNorm:
	case Format::BC6H_SFloat:
	case Format::BC6H_UFloat:
	case Format::BC7_UNorm:
	case Format::BC7_UNorm_Srgb:
		bc = true;
		bpe = 16;
		break;
	}

	if (bc)
	{
		if (width > 0)
		{
			numBlocksWide = std::max<size_t>(1, (width + 3) / 4);
		}
		if (height > 0)
		{
			numBlocksHigh = std::max<size_t>(1, (height + 3) / 4);
		}
		rowBytes = numBlocksWide * bpe;
		numRows = numBlocksHigh;
		numBytes = rowBytes * numBlocksHigh;
	}
	else
	{
		size_t bpp = BitsPerPixel(format);
		rowBytes = (width * bpp + 7) / 8; // round up to nearest byte
		numRows = height;
		numBytes = rowBytes * height;
	}

	if (outNumBytes)
	{
		*outNumBytes = numBytes;
	}
	if (outRowBytes)
	{
		*outRowBytes = rowBytes;
	}
	if (outNumRows)
	{
		*outNumRows = numRows;
	}
	if (outWidthInBlocks)
	{
		*outWidthInBlocks = numBlocksWide;
	}
	if (outHeightInBlocks)
	{
		*outHeightInBlocks = numBlocksHigh;
	}
}

} // namespace Luna