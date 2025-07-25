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

#include "Formats12.h"


namespace Luna::DX12
{

static const std::array<DxgiFormatMapping, (size_t)Luna::Format::Count> s_formatMap = { {
	{ Format::Unknown,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN,                  DXGI_FORMAT_UNKNOWN                },
	{ Format::R8_UInt,              DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_UINT,                  DXGI_FORMAT_R8_UINT                },
	{ Format::R8_SInt,              DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_SINT,                  DXGI_FORMAT_R8_SINT                },
	{ Format::R8_UNorm,             DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_UNORM,                 DXGI_FORMAT_R8_UNORM               },
	{ Format::R8_SNorm,             DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_SNORM,                 DXGI_FORMAT_R8_SNORM               },
	{ Format::RG8_UInt,             DXGI_FORMAT_R8G8_TYPELESS,          DXGI_FORMAT_R8G8_UINT,                DXGI_FORMAT_R8G8_UINT              },
	{ Format::RG8_SInt,             DXGI_FORMAT_R8G8_TYPELESS,          DXGI_FORMAT_R8G8_SINT,                DXGI_FORMAT_R8G8_SINT              },
	{ Format::RG8_UNorm,            DXGI_FORMAT_R8G8_TYPELESS,          DXGI_FORMAT_R8G8_UNORM,               DXGI_FORMAT_R8G8_UNORM             },
	{ Format::RG8_SNorm,            DXGI_FORMAT_R8G8_TYPELESS,          DXGI_FORMAT_R8G8_SNORM,               DXGI_FORMAT_R8G8_SNORM             },
	{ Format::R16_UInt,             DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_UINT,                 DXGI_FORMAT_R16_UINT               },
	{ Format::R16_SInt,             DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_SINT,                 DXGI_FORMAT_R16_SINT               },
	{ Format::R16_UNorm,            DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_UNORM,                DXGI_FORMAT_R16_UNORM              },
	{ Format::R16_SNorm,            DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_SNORM,                DXGI_FORMAT_R16_SNORM              },
	{ Format::R16_Float,            DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_FLOAT,                DXGI_FORMAT_R16_FLOAT              },
	{ Format::BGRA4_UNorm,          DXGI_FORMAT_B4G4R4A4_UNORM,         DXGI_FORMAT_B4G4R4A4_UNORM,           DXGI_FORMAT_B4G4R4A4_UNORM         },
	{ Format::B5G6R5_UNorm,         DXGI_FORMAT_B5G6R5_UNORM,           DXGI_FORMAT_B5G6R5_UNORM,             DXGI_FORMAT_B5G6R5_UNORM           },
	{ Format::B5G5R5A1_UNorm,       DXGI_FORMAT_B5G5R5A1_UNORM,         DXGI_FORMAT_B5G5R5A1_UNORM,           DXGI_FORMAT_B5G5R5A1_UNORM         },
	{ Format::RGBA8_UInt,           DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_UINT,            DXGI_FORMAT_R8G8B8A8_UINT          },
	{ Format::RGBA8_SInt,           DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_SINT,            DXGI_FORMAT_R8G8B8A8_SINT          },
	{ Format::RGBA8_UNorm,          DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_UNORM,           DXGI_FORMAT_R8G8B8A8_UNORM         },
	{ Format::RGBA8_SNorm,          DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_SNORM,           DXGI_FORMAT_R8G8B8A8_SNORM         },
	{ Format::BGRA8_UNorm,          DXGI_FORMAT_B8G8R8A8_TYPELESS,      DXGI_FORMAT_B8G8R8A8_UNORM,           DXGI_FORMAT_B8G8R8A8_UNORM         },
	{ Format::SRGBA8_UNorm,         DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB    },
	{ Format::SBGRA8_UNorm,         DXGI_FORMAT_B8G8R8A8_TYPELESS,      DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,      DXGI_FORMAT_B8G8R8A8_UNORM_SRGB    },
	{ Format::R10G10B10A2_UNorm,    DXGI_FORMAT_R10G10B10A2_TYPELESS,   DXGI_FORMAT_R10G10B10A2_UNORM,        DXGI_FORMAT_R10G10B10A2_UNORM      },
	{ Format::R11G11B10_Float,      DXGI_FORMAT_R11G11B10_FLOAT,        DXGI_FORMAT_R11G11B10_FLOAT,          DXGI_FORMAT_R11G11B10_FLOAT        },
	{ Format::RG16_UInt,            DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_UINT,              DXGI_FORMAT_R16G16_UINT            },
	{ Format::RG16_SInt,            DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_SINT,              DXGI_FORMAT_R16G16_SINT            },
	{ Format::RG16_UNorm,           DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_UNORM,             DXGI_FORMAT_R16G16_UNORM           },
	{ Format::RG16_SNorm,           DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_SNORM,             DXGI_FORMAT_R16G16_SNORM           },
	{ Format::RG16_Float,           DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_FLOAT,             DXGI_FORMAT_R16G16_FLOAT           },
	{ Format::R32_UInt,             DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_UINT,                 DXGI_FORMAT_R32_UINT               },
	{ Format::R32_SInt,             DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_SINT,                 DXGI_FORMAT_R32_SINT               },
	{ Format::R32_Float,            DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_FLOAT,                DXGI_FORMAT_R32_FLOAT              },
	{ Format::RGBA16_UInt,          DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_UINT,        DXGI_FORMAT_R16G16B16A16_UINT      },
	{ Format::RGBA16_SInt,          DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_SINT,        DXGI_FORMAT_R16G16B16A16_SINT      },
	{ Format::RGBA16_UNorm,         DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_UNORM,       DXGI_FORMAT_R16G16B16A16_UNORM     },
	{ Format::RGBA16_SNorm,         DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_SNORM,       DXGI_FORMAT_R16G16B16A16_SNORM     },
	{ Format::RGBA16_Float,         DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_FLOAT,       DXGI_FORMAT_R16G16B16A16_FLOAT     },
	{ Format::RG32_UInt,            DXGI_FORMAT_R32G32_TYPELESS,        DXGI_FORMAT_R32G32_UINT,              DXGI_FORMAT_R32G32_UINT            },
	{ Format::RG32_SInt,            DXGI_FORMAT_R32G32_TYPELESS,        DXGI_FORMAT_R32G32_SINT,              DXGI_FORMAT_R32G32_SINT            },
	{ Format::RG32_Float,           DXGI_FORMAT_R32G32_TYPELESS,        DXGI_FORMAT_R32G32_FLOAT,             DXGI_FORMAT_R32G32_FLOAT           },
	{ Format::RGB32_UInt,           DXGI_FORMAT_R32G32B32_TYPELESS,     DXGI_FORMAT_R32G32B32_UINT,           DXGI_FORMAT_R32G32B32_UINT         },
	{ Format::RGB32_SInt,           DXGI_FORMAT_R32G32B32_TYPELESS,     DXGI_FORMAT_R32G32B32_SINT,           DXGI_FORMAT_R32G32B32_SINT         },
	{ Format::RGB32_Float,          DXGI_FORMAT_R32G32B32_TYPELESS,     DXGI_FORMAT_R32G32B32_FLOAT,          DXGI_FORMAT_R32G32B32_FLOAT        },
	{ Format::RGBA32_UInt,          DXGI_FORMAT_R32G32B32A32_TYPELESS,  DXGI_FORMAT_R32G32B32A32_UINT,        DXGI_FORMAT_R32G32B32A32_UINT      },
	{ Format::RGBA32_SInt,          DXGI_FORMAT_R32G32B32A32_TYPELESS,  DXGI_FORMAT_R32G32B32A32_SINT,        DXGI_FORMAT_R32G32B32A32_SINT      },
	{ Format::RGBA32_Float,         DXGI_FORMAT_R32G32B32A32_TYPELESS,  DXGI_FORMAT_R32G32B32A32_FLOAT,       DXGI_FORMAT_R32G32B32A32_FLOAT     },

	{ Format::D16,                  DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_UNORM,                DXGI_FORMAT_D16_UNORM              },
	{ Format::D24S8,                DXGI_FORMAT_R24G8_TYPELESS,         DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    DXGI_FORMAT_D24_UNORM_S8_UINT      },
	{ Format::X24G8_UInt,           DXGI_FORMAT_R24G8_TYPELESS,         DXGI_FORMAT_X24_TYPELESS_G8_UINT,     DXGI_FORMAT_D24_UNORM_S8_UINT      },
	{ Format::D32,                  DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_FLOAT,                DXGI_FORMAT_D32_FLOAT              },
	{ Format::D32S8,                DXGI_FORMAT_R32G8X24_TYPELESS,      DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, DXGI_FORMAT_D32_FLOAT_S8X24_UINT   },
	{ Format::X32G8_UInt,           DXGI_FORMAT_R32G8X24_TYPELESS,      DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,  DXGI_FORMAT_D32_FLOAT_S8X24_UINT   },

	{ Format::BC1_UNorm,            DXGI_FORMAT_BC1_TYPELESS,           DXGI_FORMAT_BC1_UNORM,                DXGI_FORMAT_BC1_UNORM              },
	{ Format::BC1_UNorm_Srgb,       DXGI_FORMAT_BC1_TYPELESS,           DXGI_FORMAT_BC1_UNORM_SRGB,           DXGI_FORMAT_BC1_UNORM_SRGB         },
	{ Format::BC2_UNorm,            DXGI_FORMAT_BC2_TYPELESS,           DXGI_FORMAT_BC2_UNORM,                DXGI_FORMAT_BC2_UNORM              },
	{ Format::BC2_UNorm_Srgb,       DXGI_FORMAT_BC2_TYPELESS,           DXGI_FORMAT_BC2_UNORM_SRGB,           DXGI_FORMAT_BC2_UNORM_SRGB         },
	{ Format::BC3_UNorm,            DXGI_FORMAT_BC3_TYPELESS,           DXGI_FORMAT_BC3_UNORM,                DXGI_FORMAT_BC3_UNORM              },
	{ Format::BC3_UNorm_Srgb,       DXGI_FORMAT_BC3_TYPELESS,           DXGI_FORMAT_BC3_UNORM_SRGB,           DXGI_FORMAT_BC3_UNORM_SRGB         },
	{ Format::BC4_UNorm,            DXGI_FORMAT_BC4_TYPELESS,           DXGI_FORMAT_BC4_UNORM,                DXGI_FORMAT_BC4_UNORM              },
	{ Format::BC4_SNorm,            DXGI_FORMAT_BC4_TYPELESS,           DXGI_FORMAT_BC4_SNORM,                DXGI_FORMAT_BC4_SNORM              },
	{ Format::BC5_UNorm,            DXGI_FORMAT_BC5_TYPELESS,           DXGI_FORMAT_BC5_UNORM,                DXGI_FORMAT_BC5_UNORM              },
	{ Format::BC5_SNorm,            DXGI_FORMAT_BC5_TYPELESS,           DXGI_FORMAT_BC5_SNORM,                DXGI_FORMAT_BC5_SNORM              },
	{ Format::BC6H_UFloat,          DXGI_FORMAT_BC6H_TYPELESS,          DXGI_FORMAT_BC6H_UF16,                DXGI_FORMAT_BC6H_UF16              },
	{ Format::BC6H_SFloat,          DXGI_FORMAT_BC6H_TYPELESS,          DXGI_FORMAT_BC6H_SF16,                DXGI_FORMAT_BC6H_SF16              },
	{ Format::BC7_UNorm,            DXGI_FORMAT_BC7_TYPELESS,           DXGI_FORMAT_BC7_UNORM,                DXGI_FORMAT_BC7_UNORM              },
	{ Format::BC7_UNorm_Srgb,       DXGI_FORMAT_BC7_TYPELESS,           DXGI_FORMAT_BC7_UNORM_SRGB,           DXGI_FORMAT_BC7_UNORM_SRGB         },
} };


const DxgiFormatMapping& FormatToDxgi(Format engineFormat)
{
	assert(engineFormat < Format::Count);
	assert(s_formatMap[(uint32_t)engineFormat].engineFormat == engineFormat);

	return s_formatMap[(uint32_t)engineFormat];
}


Format DxgiToFormat(DXGI_FORMAT format)
{
	static bool initialized = false;
	static std::array<Format, DXGI_FORMAT_A4B4G4R4_UNORM + 1> remapTable;

	if (!initialized)
	{
		for (uint32_t i = 0; i < DXGI_FORMAT_A4B4G4R4_UNORM + 1; ++i)
		{
			remapTable[i] = Format::Unknown;
		}

		remapTable[DXGI_FORMAT_B4G4R4A4_UNORM] = Format::BGRA4_UNorm;
		remapTable[DXGI_FORMAT_B5G6R5_UNORM] = Format::B5G6R5_UNorm;
		remapTable[DXGI_FORMAT_B5G5R5A1_UNORM] = Format::B5G5R5A1_UNorm;
		remapTable[DXGI_FORMAT_B8G8R8A8_UNORM] = Format::BGRA8_UNorm;
		remapTable[DXGI_FORMAT_B8G8R8A8_UNORM_SRGB] = Format::SBGRA8_UNorm;
		remapTable[DXGI_FORMAT_R8_UNORM] = Format::R8_UNorm;
		remapTable[DXGI_FORMAT_R8_SNORM] = Format::R8_SNorm;
		remapTable[DXGI_FORMAT_R8_UINT] = Format::R8_UInt;
		remapTable[DXGI_FORMAT_R8_SINT] = Format::R8_SInt;
		remapTable[DXGI_FORMAT_R8G8_UNORM] = Format::RG8_UNorm;
		remapTable[DXGI_FORMAT_R8G8_SNORM] = Format::RG8_SNorm;
		remapTable[DXGI_FORMAT_R8G8_UINT] = Format::RG8_UInt;
		remapTable[DXGI_FORMAT_R8G8_SINT] = Format::RG8_SInt;
		remapTable[DXGI_FORMAT_R8G8B8A8_UNORM] = Format::RGBA8_UNorm;
		remapTable[DXGI_FORMAT_R8G8B8A8_SNORM] = Format::RGBA8_SNorm;
		remapTable[DXGI_FORMAT_R8G8B8A8_UINT] = Format::RGBA8_UInt;
		remapTable[DXGI_FORMAT_R8G8B8A8_SINT] = Format::RGBA8_SInt;
		remapTable[DXGI_FORMAT_R8G8B8A8_UNORM_SRGB] = Format::SRGBA8_UNorm;
		remapTable[DXGI_FORMAT_R16_UNORM] = Format::R16_UNorm;
		remapTable[DXGI_FORMAT_R16_SNORM] = Format::R16_SNorm;
		remapTable[DXGI_FORMAT_R16_UINT] = Format::R16_UInt;
		remapTable[DXGI_FORMAT_R16_SINT] = Format::R16_SInt;
		remapTable[DXGI_FORMAT_R16_FLOAT] = Format::R16_Float;
		remapTable[DXGI_FORMAT_R16G16_UNORM] = Format::RG16_UNorm;
		remapTable[DXGI_FORMAT_R16G16_SNORM] = Format::RG16_SNorm;
		remapTable[DXGI_FORMAT_R16G16_UINT] = Format::RG16_UInt;
		remapTable[DXGI_FORMAT_R16G16_SINT] = Format::RG16_SInt;
		remapTable[DXGI_FORMAT_R16G16_FLOAT] = Format::RG16_Float;
		remapTable[DXGI_FORMAT_R16G16B16A16_UNORM] = Format::RGBA16_UNorm;
		remapTable[DXGI_FORMAT_R16G16B16A16_SNORM] = Format::RGBA16_SNorm;
		remapTable[DXGI_FORMAT_R16G16B16A16_UINT] = Format::RGBA16_UInt;
		remapTable[DXGI_FORMAT_R16G16B16A16_SINT] = Format::RGBA16_SInt;
		remapTable[DXGI_FORMAT_R16G16B16A16_FLOAT] = Format::RGBA16_Float;
		remapTable[DXGI_FORMAT_R32_UINT] = Format::R32_UInt;
		remapTable[DXGI_FORMAT_R32_SINT] = Format::R32_SInt;
		remapTable[DXGI_FORMAT_R32_FLOAT] = Format::R32_Float;
		remapTable[DXGI_FORMAT_R32G32_UINT] = Format::RG32_UInt;
		remapTable[DXGI_FORMAT_R32G32_SINT] = Format::RG32_SInt;
		remapTable[DXGI_FORMAT_R32G32_FLOAT] = Format::RG32_Float;
		remapTable[DXGI_FORMAT_R32G32B32_UINT] = Format::RGB32_UInt;
		remapTable[DXGI_FORMAT_R32G32B32_SINT] = Format::RGB32_SInt;
		remapTable[DXGI_FORMAT_R32G32B32_FLOAT] = Format::RGB32_Float;
		remapTable[DXGI_FORMAT_R32G32B32A32_UINT] = Format::RGBA32_UInt;
		remapTable[DXGI_FORMAT_R32G32B32A32_SINT] = Format::RGBA32_SInt;
		remapTable[DXGI_FORMAT_R32G32B32A32_FLOAT] = Format::RGBA32_Float;
		remapTable[DXGI_FORMAT_R11G11B10_FLOAT] = Format::R11G11B10_Float;
		remapTable[DXGI_FORMAT_R10G10B10A2_UNORM] = Format::R10G10B10A2_UNorm;

		remapTable[DXGI_FORMAT_D16_UNORM] = Format::D16;
		remapTable[DXGI_FORMAT_D24_UNORM_S8_UINT] = Format::D24S8;
		remapTable[DXGI_FORMAT_X24_TYPELESS_G8_UINT] = Format::X24G8_UInt;
		remapTable[DXGI_FORMAT_D32_FLOAT] = Format::D32;
		remapTable[DXGI_FORMAT_D32_FLOAT_S8X24_UINT] = Format::D32S8;
		remapTable[DXGI_FORMAT_X32_TYPELESS_G8X24_UINT] = Format::X32G8_UInt;

		remapTable[DXGI_FORMAT_BC1_UNORM] = Format::BC1_UNorm;
		remapTable[DXGI_FORMAT_BC1_UNORM_SRGB] = Format::BC1_UNorm_Srgb;
		remapTable[DXGI_FORMAT_BC2_UNORM] = Format::BC2_UNorm;
		remapTable[DXGI_FORMAT_BC2_UNORM_SRGB] = Format::BC2_UNorm_Srgb;
		remapTable[DXGI_FORMAT_BC3_UNORM] = Format::BC3_UNorm;
		remapTable[DXGI_FORMAT_BC3_UNORM_SRGB] = Format::BC3_UNorm_Srgb;
		remapTable[DXGI_FORMAT_BC4_UNORM] = Format::BC4_UNorm;
		remapTable[DXGI_FORMAT_BC4_SNORM] = Format::BC4_SNorm;
		remapTable[DXGI_FORMAT_BC5_UNORM] = Format::BC5_UNorm;
		remapTable[DXGI_FORMAT_BC5_SNORM] = Format::BC5_SNorm;
		remapTable[DXGI_FORMAT_BC6H_SF16] = Format::BC6H_SFloat;
		remapTable[DXGI_FORMAT_BC6H_UF16] = Format::BC6H_UFloat;
		remapTable[DXGI_FORMAT_BC7_UNORM] = Format::BC7_UNorm;
		remapTable[DXGI_FORMAT_BC7_UNORM_SRGB] = Format::BC7_UNorm_Srgb;

		initialized = true;
	}

	if (format < remapTable.size())
	{
		return remapTable[format];
	}
	return Format::Unknown;
}


DXGI_FORMAT GetUAVFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
		break;

	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
		break;

	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return DXGI_FORMAT_B8G8R8X8_UNORM;
		break;

	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_R32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
		break;

#ifdef _DEBUG
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_D16_UNORM:
		assert_msg(false, "Requested a UAV format for a depth stencil format.");
#endif

	default:
		return format;
		break;
	}
}


DXGI_FORMAT GetDSVFormat(DXGI_FORMAT format)
{
	switch (format)
	{
		// 32-bit Z w/ Stencil
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

		// No Stencil
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
		return DXGI_FORMAT_D32_FLOAT;

		// 24-bit Z
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;

		// 16-bit Z w/o Stencil
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
		return DXGI_FORMAT_D16_UNORM;

	default:
		return format;
	}
}


DXGI_FORMAT GetDepthFormat(DXGI_FORMAT format)
{
	switch (format)
	{
		// 32-bit Z w/ Stencil
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

		// No Stencil
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;

		// 24-bit Z
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

		// 16-bit Z w/o Stencil
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
		return DXGI_FORMAT_R16_UNORM;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}


DXGI_FORMAT GetStencilFormat(DXGI_FORMAT format)
{
	switch (format)
	{
		// 32-bit Z w/ Stencil
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;

		// 24-bit Z
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_X24_TYPELESS_G8_UINT;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

} // namespace Luna::DX12