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

enum class Blend : uint8_t
{
	Zero,
	One,
	SrcColor,
	InvSrcColor,
	SrcAlpha,
	InvSrcAlpha,
	DstAlpha,
	InvDstAlpha,
	DstColor,
	InvDstColor,
	SrcAlphaSat,
	BlendFactor,
	InvBlendFactor,
	AlphaFactor,
	InvAlphaFactor,
	Src1Color,
	InvSrc1Color,
	Src1Alpha,
	InvSrc1Alpha
};


enum class BlendOp : uint8_t
{
	Add,
	Subtract,
	RevSubtract,
	Min,
	Max
};


enum class LogicOp : uint8_t
{
	Clear,
	Set,
	Copy,
	CopyInverted,
	Noop,
	Invert,
	And,
	Nand,
	Or,
	Nor,
	Xor,
	Equiv,
	AndReverse,
	OrReverse,
	OrInverted
};


enum class ColorWrite : uint8_t
{
	None	= 0,
	Red		= 1,
	Green	= 2,
	Blue	= 4,
	Alpha	= 8,
	All		= Red | Green | Blue | Alpha
};

template <> struct EnableBitmaskOperators<ColorWrite> { static const bool enable = true; };


enum class DepthWrite : uint8_t
{
	Zero	= 0,
	All		= 1
};


enum class CullMode : uint8_t
{
	None,
	Front,
	Back
};


enum class FillMode : uint8_t
{
	Wireframe,
	Solid
};


enum class ComparisonFunc : uint8_t
{
	None,
	Never,
	Less,
	Equal,
	LessEqual,
	Greater,
	NotEqual,
	GreaterEqual,
	Always
};


enum class StencilOp : uint8_t
{
	Keep,
	Zero,
	Replace,
	IncrSat,
	DecrSat,
	Invert,
	Incr,
	Decr
};


enum class IndexBufferStripCutValue : uint8_t
{
	Disabled,
	Value_0xFFFF,
	Value_0xFFFFFFFF
};


enum class PrimitiveTopology : uint8_t
{
	PointList,
	LineList,
	LineStrip,
	TriangleList,
	TriangleStrip,
	LineListWithAdjacency,
	LineStripWithAdjacency,
	TriangleListWithAdjacency,
	TriangleStripWithAdjacency,
	PatchList_1_ControlPoint,
	PatchList_2_ControlPoint,
	PatchList_3_ControlPoint,
	PatchList_4_ControlPoint,
	PatchList_5_ControlPoint,
	PatchList_6_ControlPoint,
	PatchList_7_ControlPoint,
	PatchList_8_ControlPoint,
	PatchList_9_ControlPoint,
	PatchList_10_ControlPoint,
	PatchList_11_ControlPoint,
	PatchList_12_ControlPoint,
	PatchList_13_ControlPoint,
	PatchList_14_ControlPoint,
	PatchList_15_ControlPoint,
	PatchList_16_ControlPoint,
	PatchList_17_ControlPoint,
	PatchList_18_ControlPoint,
	PatchList_19_ControlPoint,
	PatchList_20_ControlPoint,
	PatchList_21_ControlPoint,
	PatchList_22_ControlPoint,
	PatchList_23_ControlPoint,
	PatchList_24_ControlPoint,
	PatchList_25_ControlPoint,
	PatchList_26_ControlPoint,
	PatchList_27_ControlPoint,
	PatchList_28_ControlPoint,
	PatchList_29_ControlPoint,
	PatchList_30_ControlPoint,
	PatchList_31_ControlPoint,
	PatchList_32_ControlPoint
};


enum class ShaderStage : uint16_t
{
	None			= 0x0000,

	Compute			= 0x0020,

	Vertex			= 0x0001,
	Hull			= 0x0002,
	Domain			= 0x0004,
	Geometry		= 0x0008,
	Pixel			= 0x0010,
	Amplification	= 0x0040,
	Mesh			= 0x0080,
	AllGraphics		= 0x00FE,

	RayGeneration	= 0x0100,
	AnyHit			= 0x0200,
	ClosestHit		= 0x0400,
	Miss			= 0x0800,
	Intersection	= 0x1000,
	Callable		= 0x2000,
	AllRaytracing	= 0x3F00,

	All				= 0x3FFF
};

template <> struct EnableBitmaskOperators<ShaderStage> { static const bool enable = true; };


enum class ShaderType : uint8_t
{
	None = 0,
	Compute,
	Vertex,
	Hull,
	Domain,
	Geometry,
	Pixel,
	Amplification,
	Mesh,
	RayGeneration,
	AnyHit,
	ClosestHit,
	Miss,
	Intersection,
	Callable
};

inline ShaderStage ShaderTypeToShaderStage(ShaderType shaderType)
{
	switch (shaderType)
	{
	case ShaderType::Compute:			return ShaderStage::Compute;
	case ShaderType::Vertex:			return ShaderStage::Vertex;
	case ShaderType::Hull:				return ShaderStage::Hull;
	case ShaderType::Domain:			return ShaderStage::Domain;
	case ShaderType::Geometry:			return ShaderStage::Geometry;
	case ShaderType::Pixel:				return ShaderStage::Pixel;
	case ShaderType::Amplification:		return ShaderStage::Amplification;
	case ShaderType::Mesh:				return ShaderStage::Mesh;
	case ShaderType::RayGeneration:		return ShaderStage::RayGeneration;
	case ShaderType::AnyHit:			return ShaderStage::AnyHit;
	case ShaderType::ClosestHit:		return ShaderStage::ClosestHit;
	case ShaderType::Miss:				return ShaderStage::Miss;
	case ShaderType::Intersection:		return ShaderStage::Intersection;
	case ShaderType::Callable:			return ShaderStage::Callable;

	default: return ShaderStage::None;
	}
}


enum class DescriptorType : uint8_t
{
	None,
	ConstantBuffer,
	TextureSRV,
	TextureUAV,
	TypedBufferSRV,
	TypedBufferUAV,
	StructuredBufferSRV,
	StructuredBufferUAV,
	RawBufferSRV,
	RawBufferUAV,
	Sampler,
	RayTracingAccelStruct,
	PushConstants,
	SamplerFeedbackTextureUAV
};


enum class DescriptorRangeFlags : uint8_t
{
	None,
	AllowUpdateAfterSet,
	PartiallyBound,
	Array,
	VariableSizedArray
};

template <> struct EnableBitmaskOperators<DescriptorRangeFlags> { static const bool enable = true; };


enum class TextureFilter
{
	MinMagMipPoint,
	MinMagPointMipLinear,
	MinPointMagLinearMipPoint,
	MinPointMagMipLinear,
	MinLinearMagMipPoint,
	MinLinearMagPointMipLinear,
	MinMagLinearMipPoint,
	MinMagMipLinear,
	Anisotropic,

	ComparisonMinMagMipPoint,
	ComparisonMinMagPointMipLinear,
	ComparisonMinPointMagLinearMipPoint,
	ComparisonMinPointMagMipLinear,
	ComparisonMinLinearMagMipPoint,
	ComparisonMinLinearMagPointMipLinear,
	ComparisonMinMagLinearMipPoint,
	ComparisonMinMagMipLinear,
	ComparisonAnisotropic,

	MinimumMinMagMipPoint,
	MinimumMinMagPointMipLinear,
	MinimumMinPointMagLinearMipPoint,
	MinimumMinPointMagMipLinear,
	MinimumMinLinearMagMipPoint,
	MinimumMinLinearMagPointMipLinear,
	MinimumMinMagLinearMipPoint,
	MinimumMinMagMipLinear,
	MinimumAnisotropic,

	MaximumMinMagMipPoint,
	MaximumMinMagPointMipLinear,
	MaximumMinPointMagLinearMipPoint,
	MaximumMinPointMagMipLinear,
	MaximumMinLinearMagMipPoint,
	MaximumMinLinearMagPointMipLinear,
	MaximumMinMagLinearMipPoint,
	MaximumMinMagMipLinear,
	MaximumAnisotropic,

	Count
};


enum class TextureAddress : uint8_t
{
	Wrap,
	Mirror,
	Clamp,
	Border,
	MirrorOnce
};


enum class InputClassification : uint8_t
{
	PerVertexData,
	PerInstanceData
};


enum class CommandListType : uint8_t
{
	Direct,
	Bundle,
	Compute,
	Copy,

	Count
};


inline std::string EngineTypeToString(CommandListType commandListType)
{
	using enum CommandListType;

	switch (commandListType)
	{
	case Bundle:	return "Bundle"; break;
	case Compute:	return "Compute"; break;
	case Copy:		return "Copy"; break;
	default:		return "Direct"; break;
	}
}


enum class ResourceState : uint32_t
{
	Undefined					= 0,
	Common						= 0x00000001,
	ConstantBuffer				= 0x00000002,
	VertexBuffer				= 0x00000004,
	IndexBuffer					= 0x00000008,
	IndirectArgument			= 0x00000010,
	PixelShaderResource			= 0x00000020,
	NonPixelShaderResource		= 0x00000040,
	UnorderedAccess				= 0x00000080,
	RenderTarget				= 0x00000100,
	DepthWrite					= 0x00000200,
	DepthRead					= 0x00000400,
	StreamOut					= 0x00000800,
	CopyDest					= 0x00001000,
	CopySource					= 0x00002000,
	ResolveDest					= 0x00004000,
	ResolveSource				= 0x00008000,
	Present						= 0x00010000,
	AccelStructRead				= 0x00020000,
	AccelStructWrite			= 0x00040000,
	AccelStructBuildInput		= 0x00080000,
	AccelStructBuildBlas		= 0x00100000,
	ShadingRateSurface			= 0x00200000,
	OpacityMicromapWrite		= 0x00400000,
	OpacityMicromapBuildInput	= 0x00800000,
	Predication					= 0x01000000,
	GenericRead					= 0x02000000
};

template <> struct EnableBitmaskOperators<ResourceState> { static const bool enable = true; };


enum class QueryHeapType : uint8_t
{
	Occlusion,
	Timestamp,
	PipelineStats
};


enum class QueryType : uint8_t
{
	Occlusion,
	Timestamp,
	PipelineStats
};


enum class RootParameterType : uint8_t
{
	Unknown,
	RootConstants,
	RootCBV,
	RootSRV,
	RootUAV,
	Table
};


inline bool IsRootDescriptorType(RootParameterType type)
{
	using enum RootParameterType;
	return type == RootCBV || type == RootSRV || type == RootUAV;
}


enum class ResourceType : uint32_t
{
	Unknown				= 0x0000,
	Texture1D			= 0x0001,
	Texture1D_Array		= 0x0002,
	Texture2D			= 0x0004,
	Texture2D_Array		= 0x0008,
	Texture2DMS			= 0x0010,
	Texture2DMS_Array	= 0x0020,
	TextureCube			= 0x0040,
	TextureCube_Array	= 0x0080,
	Texture3D			= 0x0100,
	IndexBuffer			= 0x0200,
	VertexBuffer		= 0x0400,
	ConstantBuffer		= 0x0800,
	ByteAddressBuffer	= 0x1000,
	IndirectArgsBuffer	= 0x2000,
	StructuredBuffer	= 0x4000,
	TypedBuffer			= 0x8000,
	ReadbackBuffer		= 0x010000,

	Texture1D_Type		= Texture1D | Texture1D_Array,
	Texture2D_Type		= Texture2D | Texture2D_Array | Texture2DMS | Texture2DMS_Array,
	TextureCube_Type	= TextureCube | TextureCube_Array,
	TextureArray_Type	= Texture1D_Array | Texture2D_Array | Texture2DMS_Array | TextureCube_Array,

	Texture_Type		= Texture1D_Type | Texture2D_Type | TextureCube_Type | Texture3D,
	Buffer_Type			= IndexBuffer | VertexBuffer | ConstantBuffer | ByteAddressBuffer | IndirectArgsBuffer | StructuredBuffer |
							TypedBuffer | ReadbackBuffer,
	UnorderedAccess_Type = ByteAddressBuffer | IndirectArgsBuffer | StructuredBuffer | TypedBuffer,
};
template <> struct EnableBitmaskOperators<ResourceType> { static const bool enable = true; };


inline bool IsTextureType(ResourceType resourceType)
{
	return HasAnyFlag(resourceType, ResourceType::Texture_Type);
}


inline bool IsBufferType(ResourceType resourceType)
{
	return HasAnyFlag(resourceType, ResourceType::Buffer_Type);
}


inline bool IsUnorderedAccessType(ResourceType resourceType)
{
	return HasAnyFlag(resourceType, ResourceType::UnorderedAccess_Type);
}

enum class TextureDimension : uint8_t
{
	Unknown,
	Texture1D,
	Texture1D_Array,
	Texture2D,
	Texture2D_Array,
	Texture2DMS,
	Texture2DMS_Array,
	TextureCube,
	TextureCube_Array,
	Texture3D
};


inline TextureDimension ResourceTypeToTextureDimension(ResourceType resourceType)
{
	switch (resourceType)
	{
	case ResourceType::Texture1D:			return TextureDimension::Texture1D;			break;
	case ResourceType::Texture1D_Array:		return TextureDimension::Texture1D_Array;	break;
	case ResourceType::Texture2D:			return TextureDimension::Texture2D;			break;
	case ResourceType::Texture2D_Array:		return TextureDimension::Texture2D_Array;	break;
	case ResourceType::Texture2DMS:			return TextureDimension::Texture2DMS;		break;
	case ResourceType::Texture2DMS_Array:	return TextureDimension::Texture2DMS_Array;	break;
	case ResourceType::TextureCube:			return TextureDimension::TextureCube;		break;
	case ResourceType::TextureCube_Array:	return TextureDimension::TextureCube_Array;	break;
	case ResourceType::Texture3D:			return TextureDimension::Texture3D;			break;
	default:								return TextureDimension::Unknown;			break;
	}
}


inline ResourceType TextureDimensionToResourceType(TextureDimension dimension)
{
	switch (dimension)
	{
	case TextureDimension::Texture1D:			return ResourceType::Texture1D;			break;
	case TextureDimension::Texture1D_Array:		return ResourceType::Texture1D_Array;	break;
	case TextureDimension::Texture2D:			return ResourceType::Texture2D;			break;
	case TextureDimension::Texture2D_Array:		return ResourceType::Texture2D_Array;	break;
	case TextureDimension::Texture2DMS:			return ResourceType::Texture2DMS;		break;
	case TextureDimension::Texture2DMS_Array:	return ResourceType::Texture2DMS_Array;	break;
	case TextureDimension::TextureCube:			return ResourceType::TextureCube;		break;
	case TextureDimension::TextureCube_Array:	return ResourceType::TextureCube_Array;	break;
	case TextureDimension::Texture3D:			return ResourceType::Texture3D;			break;
	default:									return ResourceType::Unknown;			break;
	}
}

enum class QueueType : uint8_t
{
	Graphics,
	Compute,
	Copy,

	Count
};


inline std::string EngineTypeToString(QueueType queueType)
{
	switch (queueType)
	{
	case QueueType::Compute:	return "Compute"; break;
	case QueueType::Copy:		return "Copy"; break;
	default:					return "Graphics"; break;
	}
}


enum class HardwareVendor
{
	Unknown,
	AMD,
	Intel,
	NVIDIA,
	Microsoft
};


inline std::string HardwareVendorToString(HardwareVendor hardwareVendor)
{
	using enum HardwareVendor;

	switch (hardwareVendor)
	{
	case AMD:		return "AMD"; break;
	case Intel:		return "Intel";	break;
	case NVIDIA:	return "NVIDIA"; break;
	case Microsoft: return "Microsoft"; break;
	default:		return "Unknown"; break;
	}
}


inline HardwareVendor VendorIdToHardwareVendor(uint32_t vendorId)
{
	using enum HardwareVendor;

	switch (vendorId)
	{
	case 0x10de:
		return NVIDIA;
		break;

	case 0x1002:
	case 0x1022:
		return AMD;
		break;

	case 0x163c:
	case 0x8086:
	case 0x8087:
		return Intel;
		break;

	case 0x1414:
		return Microsoft;
		break;

	default:
		return Unknown;
		break;
	}
}


inline std::string VendorIdToString(uint32_t vendorId)
{
	return HardwareVendorToString(VendorIdToHardwareVendor(vendorId));
}


enum class AdapterType : uint8_t
{
	Discrete,
	Integrated,
	Software,
	Other
};


inline std::string AdapterTypeToString(AdapterType adapterType)
{
	using enum AdapterType;

	switch (adapterType)
	{
	case Discrete:		return "Discrete"; break;
	case Integrated:	return "Integrated"; break;
	case Software:		return "Software"; break;
	default:			return "Other"; break;
	}
}


inline CommandListType QueueTypeToCommandListType(QueueType queueType)
{
	switch (queueType)
	{
	case QueueType::Compute:	return CommandListType::Compute; break;
	case QueueType::Copy:		return CommandListType::Copy; break;
	default:					return CommandListType::Direct; break;
	}
}


inline QueueType CommandListTypeToQueueType(CommandListType commandListType)
{
	switch (commandListType)
	{
	case CommandListType::Compute:	return QueueType::Compute; break;
	case CommandListType::Copy:		return QueueType::Copy; break;
	default:						return QueueType::Graphics; break;
	}
}


enum class GpuImageUsage
{
	Unknown				= 0x0000,
	RenderTarget		= 0x0001,
	DepthStencilTarget	= 0x0002,
	ShaderResource		= 0x0004,
	UnorderedAccess		= 0x0008,
	CopySource			= 0x0010,
	CopyDest			= 0x0020,

	ColorBuffer			= RenderTarget | ShaderResource | UnorderedAccess | CopyDest | CopySource,
	DepthBuffer			= DepthStencilTarget | ShaderResource | CopyDest | CopySource,
};
template <> struct EnableBitmaskOperators<GpuImageUsage> { static const bool enable = true; };


enum class ImageAspect
{
	Color		= 0x0001,
	Depth		= 0x0002,
	Stencil		= 0x0004,
};
template <> struct EnableBitmaskOperators<ImageAspect> { static const bool enable = true; };


enum class MemoryAccess
{
	Unknown			= 0,
	GpuRead			= 1 << 0,
	GpuWrite		= 1 << 1,
	CpuRead			= 1 << 2,
	CpuWrite		= 1 << 3,
	CpuMapped		= 1 << 4,

	GpuReadWrite	= GpuRead | GpuWrite
};
template <> struct EnableBitmaskOperators<MemoryAccess> { static const bool enable = true; };


enum class DepthStencilAspect
{
	ReadWrite,
	ReadOnly,
	DepthReadOnly,
	StencilReadOnly
};


enum class NativeObjectType : uint32_t
{
	// DX12
	DX12_Resource				= 0x00010001,
	DX12_RTV					= 0x00010002,
	DX12_SRV					= 0x00010003,
	DX12_UAV					= 0x00010004,
	DX12_GpuVirtualAddress		= 0x00010005,
	DX12_DSV					= 0x00010006,
	DX12_DSV_ReadOnly			= 0x00010007,
	DX12_DSV_DepthReadOnly		= 0x00010008,
	DX12_DSV_StencilReadOnly	= 0x00010009,
	DX12_SRV_Depth				= 0x0001000A,
	DX12_SRV_Stencil			= 0x0001000B,
	DX12_RootSignature			= 0x0001000C,
	DX12_CBV					= 0x0001000D,

	// Vulkan
	VK_Image					= 0x00020001,
	VK_Buffer					= 0x00020002,
	VK_FrameBuffer				= 0x00020003,
	VK_RenderPass				= 0x00020004,
	VK_PipelineLayout			= 0x00020005,
	VK_ImageView_RTV			= 0x00020006,
	VK_ImageView_SRV			= 0x00020007,
	VK_ImageView_DSV			= 0x00020008,
	VK_ImageView_DSV_Depth		= 0x00020009,
	VK_ImageView_DSV_Stencil	= 0x0002000A,
	VK_ImageInfo_SRV			= 0x0002000B,
	VK_ImageInfo_UAV			= 0x0002000C,
	VK_ImageInfo_SRV_Depth		= 0x0002000D,
	VK_ImageInfo_SRV_Stencil	= 0x0002000E,
	VK_BufferInfo				= 0x0002000F,
	VK_BufferView				= 0x00020010,
};


enum class StaticBorderColor : uint8_t
{
	TransparentBlack,
	OpaqueBlack,
	OpaqueWhite
};

} // namespace Luna


#define DECLARE_STRING_FORMATTERS(ENGINE_TYPE) \
template <> \
struct std::formatter<ENGINE_TYPE> : public std::formatter<std::string> \
{ \
	auto format(ENGINE_TYPE value, std::format_context& ctx) const \
	{ \
		auto str = Luna::EngineTypeToString(value); \
		return std::formatter<std::string>::format(str, ctx); \
	} \
}; \
inline std::ostream& operator<<(ENGINE_TYPE type, std::ostream& os) { os << Luna::EngineTypeToString(type); return os; }


DECLARE_STRING_FORMATTERS(Luna::CommandListType)
DECLARE_STRING_FORMATTERS(Luna::QueueType)


#undef DECLARE_STRING_FORMATTERS