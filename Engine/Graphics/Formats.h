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

namespace Luna
{

enum class Format : uint8_t
{
	Unknown,

	R8_UInt,
	R8_SInt,
	R8_UNorm,
	R8_SNorm,
	RG8_UInt,
	RG8_SInt,
	RG8_UNorm,
	RG8_SNorm,
	R16_UInt,
	R16_SInt,
	R16_UNorm,
	R16_SNorm,
	R16_Float,
	BGRA4_UNorm,
	B5G6R5_UNorm,
	B5G5R5A1_UNorm,
	RGBA8_UInt,
	RGBA8_SInt,
	RGBA8_UNorm,
	RGBA8_SNorm,
	BGRA8_UNorm,
	SRGBA8_UNorm,
	SBGRA8_UNorm,
	R10G10B10A2_UNorm,
	R11G11B10_Float,
	RG16_UInt,
	RG16_SInt,
	RG16_UNorm,
	RG16_SNorm,
	RG16_Float,
	R32_UInt,
	R32_SInt,
	R32_Float,
	RGBA16_UInt,
	RGBA16_SInt,
	RGBA16_UNorm,
	RGBA16_SNorm,
	RGBA16_Float,
	RG32_UInt,
	RG32_SInt,
	RG32_Float,
	RGB32_UInt,
	RGB32_SInt,
	RGB32_Float,
	RGBA32_UInt,
	RGBA32_SInt,
	RGBA32_Float,

	D16,
	D24S8,
	X24G8_UInt,
	D32,
	D32S8,
	X32G8_UInt,

	BC1_UNorm,
	BC1_UNorm_Srgb,
	BC2_UNorm,
	BC2_UNorm_Srgb,
	BC3_UNorm,
	BC3_UNorm_Srgb,
	BC4_UNorm,
	BC4_SNorm,
	BC5_UNorm,
	BC5_SNorm,
	BC6H_UFloat,
	BC6H_SFloat,
	BC7_UNorm,
	BC7_UNorm_Srgb,

	Count
};


uint32_t BitsPerPixel(Format format);

uint32_t BlockSize(Format format);


inline bool IsDepthFormat(Format format)
{
	using enum Format;

	return format == D16 || format == D24S8 || format == X24G8_UInt || format == D32 || format == D32S8 || format == X32G8_UInt;
}


inline bool IsStencilFormat(Format format)
{
	using enum Format;

	return format == D24S8 || format == X24G8_UInt || format == D32S8 || format == X32G8_UInt;
}


inline bool IsDepthStencilFormat(Format format)
{
	return IsDepthFormat(format) || IsStencilFormat(format);
}


inline bool IsColorFormat(Format format)
{
	return !IsDepthStencilFormat(format);
}


inline Format RemoveSrgb(Format format)
{
	switch (format)
	{
	case Format::SRGBA8_UNorm: return Format::RGBA8_UNorm;
	case Format::SBGRA8_UNorm: return Format::BGRA8_UNorm;
	case Format::BC1_UNorm_Srgb: return Format::BC1_UNorm;
	case Format::BC2_UNorm_Srgb: return Format::BC2_UNorm;
	case Format::BC7_UNorm_Srgb: return Format::BC7_UNorm;
	default:
		return format;
	}
}

void GetSurfaceInfo(size_t width, size_t height, Format format, size_t* outNumBytes, size_t* outRowBytes, size_t* outNumRows);

} // namespace Luna