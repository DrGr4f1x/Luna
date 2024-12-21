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

#include "FormatsVK.h"


namespace Luna::VK
{

struct VkFormatMapping
{
	Luna::Format engineFormat;
	VkFormat vkFormat;
};

static const std::array<VkFormatMapping, (size_t)Luna::Format::Count> s_formatMap = { {
	{ Format::Unknown,				VK_FORMAT_UNDEFINED },

	{ Format::R8_UInt,				VK_FORMAT_R8_UINT },
	{ Format::R8_SInt,				VK_FORMAT_R8_SINT },
	{ Format::R8_UNorm,				VK_FORMAT_R8_UNORM },
	{ Format::R8_SNorm,				VK_FORMAT_R8_SNORM },
	{ Format::RG8_UInt,				VK_FORMAT_R8G8_UINT },
	{ Format::RG8_SInt,				VK_FORMAT_R8G8_SINT },
	{ Format::RG8_UNorm,			VK_FORMAT_R8G8_UNORM },
	{ Format::RG8_SNorm,			VK_FORMAT_R8G8_SNORM },
	{ Format::R16_UInt,				VK_FORMAT_R16_UINT },
	{ Format::R16_SInt,				VK_FORMAT_R16_SINT },
	{ Format::R16_UNorm,			VK_FORMAT_R16_UNORM },
	{ Format::R16_SNorm,			VK_FORMAT_R16_SNORM },
	{ Format::R16_Float,			VK_FORMAT_R16_SFLOAT },
	{ Format::BGRA4_UNorm,			VK_FORMAT_B4G4R4A4_UNORM_PACK16 },
	{ Format::B5G6R5_UNorm,			VK_FORMAT_B5G6R5_UNORM_PACK16 },
	{ Format::B5G5R5A1_UNorm,		VK_FORMAT_B5G5R5A1_UNORM_PACK16 },
	{ Format::RGBA8_UInt,			VK_FORMAT_R8G8B8A8_UINT },
	{ Format::RGBA8_SInt,			VK_FORMAT_R8G8B8A8_SINT },
	{ Format::RGBA8_UNorm,			VK_FORMAT_R8G8B8A8_UNORM },
	{ Format::RGBA8_SNorm,			VK_FORMAT_R8G8B8A8_SNORM },
	{ Format::BGRA8_UNorm,			VK_FORMAT_B8G8R8A8_UNORM },
	{ Format::SRGBA8_UNorm,			VK_FORMAT_R8G8B8A8_SRGB },
	{ Format::SBGRA8_UNorm,			VK_FORMAT_B8G8R8A8_SRGB },
	{ Format::R10G10B10A2_UNorm,	VK_FORMAT_A2B10G10R10_UNORM_PACK32 },
	{ Format::R11G11B10_Float,		VK_FORMAT_B10G11R11_UFLOAT_PACK32 },
	{ Format::RG16_UInt,			VK_FORMAT_R16G16_UINT },
	{ Format::RG16_SInt,			VK_FORMAT_R16G16_SINT },
	{ Format::RG16_UNorm,			VK_FORMAT_R16G16_UNORM },
	{ Format::RG16_SNorm,			VK_FORMAT_R16G16_SNORM },
	{ Format::RG16_Float,			VK_FORMAT_R16G16_SFLOAT },
	{ Format::R32_UInt,				VK_FORMAT_R32_UINT },
	{ Format::R32_SInt,				VK_FORMAT_R32_SINT },
	{ Format::R32_Float,			VK_FORMAT_R32_SFLOAT },
	{ Format::RGBA16_UInt,			VK_FORMAT_R16G16B16A16_UINT },
	{ Format::RGBA16_SInt,			VK_FORMAT_R16G16B16A16_SINT },
	{ Format::RGBA16_UNorm,			VK_FORMAT_R16G16B16A16_UNORM },
	{ Format::RGBA16_SNorm,			VK_FORMAT_R16G16B16A16_SNORM },
	{ Format::RGBA16_Float,			VK_FORMAT_R16G16B16A16_SFLOAT },
	{ Format::RG32_UInt,			VK_FORMAT_R32G32_UINT },
	{ Format::RG32_SInt,			VK_FORMAT_R32G32_SINT },
	{ Format::RG32_Float,			VK_FORMAT_R32G32_SFLOAT },
	{ Format::RGB32_UInt,			VK_FORMAT_R32G32B32_UINT },
	{ Format::RGB32_SInt,			VK_FORMAT_R32G32B32_SINT },
	{ Format::RGB32_Float,			VK_FORMAT_R32G32B32_SFLOAT },
	{ Format::RGBA32_UInt,			VK_FORMAT_R32G32B32A32_UINT },
	{ Format::RGBA32_SInt,			VK_FORMAT_R32G32B32A32_SINT },
	{ Format::RGBA32_Float,			VK_FORMAT_R32G32B32A32_SFLOAT },

	{ Format::D16,					VK_FORMAT_D16_UNORM },
	{ Format::D24S8,				VK_FORMAT_D24_UNORM_S8_UINT },
	{ Format::X24G8_UInt,			VK_FORMAT_D24_UNORM_S8_UINT },
	{ Format::D32,					VK_FORMAT_D32_SFLOAT },
	{ Format::D32S8,				VK_FORMAT_D32_SFLOAT_S8_UINT },
	{ Format::X32G8_UInt,			VK_FORMAT_D32_SFLOAT_S8_UINT },

	{ Format::BC1_UNorm,			VK_FORMAT_BC1_RGB_UNORM_BLOCK },
	{ Format::BC1_UNorm_Srgb,		VK_FORMAT_BC1_RGB_SRGB_BLOCK },
	{ Format::BC2_UNorm,			VK_FORMAT_BC2_UNORM_BLOCK },
	{ Format::BC2_UNorm_Srgb,		VK_FORMAT_BC2_SRGB_BLOCK },
	{ Format::BC3_UNorm,			VK_FORMAT_BC3_UNORM_BLOCK },
	{ Format::BC3_UNorm_Srgb,		VK_FORMAT_BC3_SRGB_BLOCK },
	{ Format::BC4_UNorm,			VK_FORMAT_BC4_UNORM_BLOCK },
	{ Format::BC4_SNorm,			VK_FORMAT_BC4_SNORM_BLOCK },
	{ Format::BC5_UNorm,			VK_FORMAT_BC5_UNORM_BLOCK },
	{ Format::BC5_SNorm,			VK_FORMAT_BC5_SNORM_BLOCK },
	{ Format::BC6H_UFloat,			VK_FORMAT_BC6H_UFLOAT_BLOCK },
	{ Format::BC6H_SFloat,			VK_FORMAT_BC6H_SFLOAT_BLOCK },
	{ Format::BC7_UNorm,			VK_FORMAT_BC7_UNORM_BLOCK },
	{ Format::BC7_UNorm_Srgb,		VK_FORMAT_BC7_SRGB_BLOCK },
} };


VkFormat FormatToVulkan(Format engineFormat)
{
	assert(engineFormat < Format::Count);
	assert(s_formatMap[(uint32_t)engineFormat].engineFormat == engineFormat);

	return s_formatMap[(uint32_t)engineFormat].vkFormat;
}


VkImageAspectFlags GetImageAspect(Format format)
{
	VkImageAspectFlags imageAspect{ 0 };

	if (IsColorFormat(format))
	{
		imageAspect |= VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (IsDepthFormat(format))
	{
		imageAspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	if (IsStencilFormat(format))
	{
		imageAspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	return imageAspect;
}

} // namespace Luna::VK