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

#include "Enums12.h"


namespace Luna::DX12
{

D3D12_BLEND BlendToDX12(Blend blend)
{
	using enum Blend;
	switch (blend)
	{
	case Zero:				return D3D12_BLEND_ZERO; break;
	case One:				return D3D12_BLEND_ONE; break;
	case SrcColor:			return D3D12_BLEND_SRC_COLOR; break;
	case InvSrcColor:		return D3D12_BLEND_INV_SRC_COLOR; break;
	case SrcAlpha:			return D3D12_BLEND_SRC_ALPHA; break;
	case InvSrcAlpha:		return D3D12_BLEND_INV_SRC_ALPHA; break;
	case DstAlpha:			return D3D12_BLEND_DEST_ALPHA; break;
	case InvDstAlpha:		return D3D12_BLEND_INV_DEST_ALPHA; break;
	case DstColor:			return D3D12_BLEND_DEST_COLOR; break;
	case InvDstColor:		return D3D12_BLEND_INV_DEST_COLOR; break;
	case SrcAlphaSat:		return D3D12_BLEND_SRC_ALPHA_SAT; break;
	case BlendFactor:		return D3D12_BLEND_BLEND_FACTOR; break;
	case InvBlendFactor:	return D3D12_BLEND_INV_BLEND_FACTOR; break;
	case AlphaFactor:		return D3D12_BLEND_ALPHA_FACTOR; break;
	case InvAlphaFactor:	return D3D12_BLEND_INV_ALPHA_FACTOR; break;
	case Src1Color:			return D3D12_BLEND_SRC1_COLOR; break;
	case InvSrc1Color:		return D3D12_BLEND_INV_SRC1_COLOR; break;
	case Src1Alpha:			return D3D12_BLEND_SRC1_ALPHA; break;
	case InvSrc1Alpha:		return D3D12_BLEND_INV_SRC1_ALPHA; break;
	default:
		assert(false);
		return D3D12_BLEND_ZERO;
		break;
	}
}


D3D12_BLEND_OP BlendOpToDX12(BlendOp blendOp)
{
	using enum BlendOp;
	switch (blendOp)
	{
	case Add:			return D3D12_BLEND_OP_ADD; break;
	case Subtract:		return D3D12_BLEND_OP_SUBTRACT; break;
	case RevSubtract:	return D3D12_BLEND_OP_REV_SUBTRACT; break;
	case Min:			return D3D12_BLEND_OP_MIN; break;
	case Max:			return D3D12_BLEND_OP_MAX; break;
	default:
		assert(false);
		return D3D12_BLEND_OP_ADD;
		break;
	}
}


D3D12_LOGIC_OP LogicOpToDX12(LogicOp logicOp)
{
	using enum LogicOp;
	switch (logicOp)
	{
	case Clear:			return D3D12_LOGIC_OP_CLEAR; break;
	case Set:			return D3D12_LOGIC_OP_SET; break;
	case Copy:			return D3D12_LOGIC_OP_COPY; break;
	case CopyInverted:	return D3D12_LOGIC_OP_COPY_INVERTED; break;
	case Noop:			return D3D12_LOGIC_OP_NOOP; break;
	case Invert:		return D3D12_LOGIC_OP_INVERT; break;
	case And:			return D3D12_LOGIC_OP_AND; break;
	case Nand:			return D3D12_LOGIC_OP_NAND; break;
	case Or:			return D3D12_LOGIC_OP_OR; break;
	case Nor:			return D3D12_LOGIC_OP_NOR; break;
	case Xor:			return D3D12_LOGIC_OP_XOR; break;
	case Equiv:			return D3D12_LOGIC_OP_EQUIV; break;
	case AndReverse:	return D3D12_LOGIC_OP_AND_REVERSE; break;
	case OrReverse:		return D3D12_LOGIC_OP_OR_REVERSE; break;
	case OrInverted:	return D3D12_LOGIC_OP_OR_INVERTED; break;
	default:
		assert(false);
		return D3D12_LOGIC_OP_NOOP;
		break;
	}
}

D3D12_COLOR_WRITE_ENABLE ColorWriteToDX12(ColorWrite colorWrite)
{
	using enum ColorWrite;

	uint32_t result = 0;
	result |= HasFlag(colorWrite, Red) ? D3D12_COLOR_WRITE_ENABLE_RED : 0;
	result |= HasFlag(colorWrite, Green) ? D3D12_COLOR_WRITE_ENABLE_GREEN : 0;
	result |= HasFlag(colorWrite, Blue) ? D3D12_COLOR_WRITE_ENABLE_BLUE : 0;
	result |= HasFlag(colorWrite, Alpha) ? D3D12_COLOR_WRITE_ENABLE_ALPHA : 0;

	return (D3D12_COLOR_WRITE_ENABLE)result;
}


D3D12_DEPTH_WRITE_MASK DepthWriteToDX12(DepthWrite depthWrite)
{
	using enum DepthWrite;

	switch (depthWrite)
	{
	case Zero:	return D3D12_DEPTH_WRITE_MASK_ZERO; break;
	case All:	return D3D12_DEPTH_WRITE_MASK_ALL; break;
	default:
		assert(false);
		return D3D12_DEPTH_WRITE_MASK_ALL;
		break;
	}
}


D3D12_CULL_MODE CullModeToDX12(CullMode cullMode)
{
	using enum CullMode;

	switch (cullMode)
	{
	case None:	return D3D12_CULL_MODE_NONE; break;
	case Front:	return D3D12_CULL_MODE_FRONT; break;
	case Back:	return D3D12_CULL_MODE_BACK; break;
	default:
		assert(false);
		return D3D12_CULL_MODE_BACK;
		break;
	}
}


D3D12_FILL_MODE FillModeToDX12(FillMode fillMode)
{
	using enum FillMode;

	switch (fillMode)
	{
	case Wireframe: return D3D12_FILL_MODE_WIREFRAME; break;
	case Solid:		return D3D12_FILL_MODE_SOLID; break;
	default:
		assert(false);
		return D3D12_FILL_MODE_SOLID;
		break;
	}
}


D3D12_COMPARISON_FUNC ComparisonFuncToDX12(ComparisonFunc comparisonFunc)
{
	using enum ComparisonFunc;

	switch (comparisonFunc)
	{
	case Never:			return D3D12_COMPARISON_FUNC_NEVER; break;
	case Less:			return D3D12_COMPARISON_FUNC_LESS; break;
	case LessEqual:		return D3D12_COMPARISON_FUNC_LESS_EQUAL; break;
	case Greater:		return D3D12_COMPARISON_FUNC_GREATER; break;
	case NotEqual:		return D3D12_COMPARISON_FUNC_NOT_EQUAL; break;
	case GreaterEqual:	return D3D12_COMPARISON_FUNC_GREATER_EQUAL; break;
	case Always:		return D3D12_COMPARISON_FUNC_ALWAYS; break;
	default:
		assert(false);
		return D3D12_COMPARISON_FUNC_ALWAYS;
		break;
	}
}


D3D12_STENCIL_OP StencilOpToDX12(StencilOp stencilOp)
{
	using enum StencilOp;

	switch (stencilOp)
	{
	case Keep:		return D3D12_STENCIL_OP_KEEP; break;
	case Zero:		return D3D12_STENCIL_OP_ZERO; break;
	case Replace:	return D3D12_STENCIL_OP_REPLACE; break;
	case IncrSat:	return D3D12_STENCIL_OP_INCR_SAT; break;
	case DecrSat:	return D3D12_STENCIL_OP_DECR_SAT; break;
	case Invert:	return D3D12_STENCIL_OP_INVERT; break;
	case Incr:		return D3D12_STENCIL_OP_INCR; break;
	case Decr:		return D3D12_STENCIL_OP_DECR; break;
	default:
		assert(false);
		return D3D12_STENCIL_OP_KEEP;
		break;

	}
}


D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IndexBufferStripCutValueToDX12(IndexBufferStripCutValue indexBufferStripCutValue)
{
	using enum IndexBufferStripCutValue;

	switch (indexBufferStripCutValue)
	{
	case Disabled:			return D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; break;
	case Value_0xFFFF:		return D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF; break;
	case Value_0xFFFFFFFF:	return D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF; break;
	default:
		assert(false);
		return D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		break;
	}
}


D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopologyToDX12(PrimitiveTopology primitiveTopology)
{
	using enum PrimitiveTopology;

	switch (primitiveTopology)
	{
	case PointList:						return D3D_PRIMITIVE_TOPOLOGY_POINTLIST; break;
	case LineList:						return D3D_PRIMITIVE_TOPOLOGY_LINELIST; break;
	case LineStrip:						return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP; break;
	case TriangleList:					return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
	case TriangleStrip:					return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
	case LineListWithAdjacency:			return D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ; break;
	case LineStripWithAdjacency:		return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ; break;
	case TriangleListWithAdjacency:		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ; break;
	case TriangleStripWithAdjacency:	return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ; break;
	case PatchList_1_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST; break;
	case PatchList_2_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST; break;
	case PatchList_3_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST; break;
	case PatchList_4_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST; break;
	case PatchList_5_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST; break;
	case PatchList_6_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST; break;
	case PatchList_7_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST; break;
	case PatchList_8_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST; break;
	case PatchList_9_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST; break;
	case PatchList_10_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST; break;
	case PatchList_11_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST; break;
	case PatchList_12_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST; break;
	case PatchList_13_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST; break;
	case PatchList_14_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST; break;
	case PatchList_15_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST; break;
	case PatchList_16_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST; break;
	case PatchList_17_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST; break;
	case PatchList_18_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST; break;
	case PatchList_19_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST; break;
	case PatchList_20_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST; break;
	case PatchList_21_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST; break;
	case PatchList_22_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST; break;
	case PatchList_23_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST; break;
	case PatchList_24_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST; break;
	case PatchList_25_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST; break;
	case PatchList_26_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST; break;
	case PatchList_27_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST; break;
	case PatchList_28_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST; break;
	case PatchList_29_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST; break;
	case PatchList_30_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST; break;
	case PatchList_31_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST; break;
	case PatchList_32_ControlPoint:		return D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST; break;
	default:
		assert(false);
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	}
}


D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyToPrimitiveTopologyTypeDX12(PrimitiveTopology primitiveTopology)
{
	using enum PrimitiveTopology;

	switch (primitiveTopology)
	{
	case PointList:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		break;

	case LineList:
	case LineStrip:
	case LineListWithAdjacency:
	case LineStripWithAdjacency:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;

	case TriangleList:
	case TriangleStrip:
	case TriangleListWithAdjacency:
	case TriangleStripWithAdjacency:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;

	case PatchList_1_ControlPoint:
	case PatchList_2_ControlPoint:
	case PatchList_3_ControlPoint:
	case PatchList_4_ControlPoint:
	case PatchList_5_ControlPoint:
	case PatchList_6_ControlPoint:
	case PatchList_7_ControlPoint:
	case PatchList_8_ControlPoint:
	case PatchList_9_ControlPoint:
	case PatchList_10_ControlPoint:
	case PatchList_11_ControlPoint:
	case PatchList_12_ControlPoint:
	case PatchList_13_ControlPoint:
	case PatchList_14_ControlPoint:
	case PatchList_15_ControlPoint:
	case PatchList_16_ControlPoint:
	case PatchList_17_ControlPoint:
	case PatchList_18_ControlPoint:
	case PatchList_19_ControlPoint:
	case PatchList_20_ControlPoint:
	case PatchList_21_ControlPoint:
	case PatchList_22_ControlPoint:
	case PatchList_23_ControlPoint:
	case PatchList_24_ControlPoint:
	case PatchList_25_ControlPoint:
	case PatchList_26_ControlPoint:
	case PatchList_27_ControlPoint:
	case PatchList_28_ControlPoint:
	case PatchList_29_ControlPoint:
	case PatchList_30_ControlPoint:
	case PatchList_31_ControlPoint:
	case PatchList_32_ControlPoint:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		break;

	default:
		assert(false);
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	}
}


D3D12_INPUT_CLASSIFICATION InputClassificationToDX12(InputClassification inputClassification)
{
	switch (inputClassification)
	{
	case InputClassification::PerInstanceData:
		return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;

	default:
		return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	}
}


D3D12_SHADER_VISIBILITY ShaderStageToDX12(ShaderStage shaderStage)
{
	using enum ShaderStage;

	switch (shaderStage)
	{
	case Vertex:		return D3D12_SHADER_VISIBILITY_VERTEX; break;
	case Hull:			return D3D12_SHADER_VISIBILITY_HULL; break;
	case Domain:		return D3D12_SHADER_VISIBILITY_DOMAIN; break;
	case Geometry:		return D3D12_SHADER_VISIBILITY_GEOMETRY; break;
	case Pixel:			return D3D12_SHADER_VISIBILITY_PIXEL; break;
	case Amplification:	return D3D12_SHADER_VISIBILITY_AMPLIFICATION; break;
	case Mesh:			return D3D12_SHADER_VISIBILITY_MESH; break;
	default:
		// Some combinations are unrepresentable in DX12
		return D3D12_SHADER_VISIBILITY_ALL;
		break;
	}
}


D3D12_DESCRIPTOR_RANGE_TYPE DescriptorTypeToDX12(DescriptorType type)
{
	using enum DescriptorType;

	switch (type)
	{
	case ConstantBuffer:
		return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		break;

	case Sampler:
		return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		break;

	case TextureSRV:
	case TypedBufferSRV:
	case StructuredBufferSRV:
	case RawBufferSRV:
	case RayTracingAccelStruct:
		return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		break;

	case TextureUAV:
	case TypedBufferUAV:
	case StructuredBufferUAV:
	case RawBufferUAV:
	case SamplerFeedbackTextureUAV:
		return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		break;

	default:
		assert(false);
		return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		break;
	}
}


D3D12_ROOT_SIGNATURE_FLAGS RootSignatureFlagsToDX12(RootSignatureFlags rootSignatureFlags)
{
	using enum RootSignatureFlags;

	uint32_t result = 0;
	result |= HasFlag(rootSignatureFlags, AllowInputAssemblerInputLayout) ? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT : 0;
	result |= HasFlag(rootSignatureFlags, DenyVertexShaderRootAccess) ? D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS : 0;
	result |= HasFlag(rootSignatureFlags, DenyHullShaderRootAccess) ? D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS : 0;
	result |= HasFlag(rootSignatureFlags, DenyDomainShaderRootAccess) ? D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS : 0;
	result |= HasFlag(rootSignatureFlags, DenyGeometryShaderRootAccess) ? D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS : 0;
	result |= HasFlag(rootSignatureFlags, DenyPixelShaderRootAccess) ? D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS : 0;
	result |= HasFlag(rootSignatureFlags, AllowStreamOutput) ? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT : 0;
	result |= HasFlag(rootSignatureFlags, DenyAmplificationShaderRootAccess) ? D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS : 0;
	result |= HasFlag(rootSignatureFlags, DenyMeshShaderRootAccess) ? D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS : 0;
	result |= HasFlag(rootSignatureFlags, CbvSrvUavHeapDirectlyIndexed) ? D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED : 0;
	result |= HasFlag(rootSignatureFlags, SamplerHeapDirectlyIndexed) ? D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED : 0;

	return (D3D12_ROOT_SIGNATURE_FLAGS)result;
}


D3D12_ROOT_PARAMETER_TYPE RootParameterTypeToDX12(RootParameterType rootParameterType)
{
	switch (rootParameterType)
	{
	case RootParameterType::RootConstants:	return D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS; break;
	case RootParameterType::RootCBV:		return D3D12_ROOT_PARAMETER_TYPE_CBV; break;
	case RootParameterType::RootSRV:		return D3D12_ROOT_PARAMETER_TYPE_SRV; break;
	case RootParameterType::RootUAV:		return D3D12_ROOT_PARAMETER_TYPE_UAV; break;
	default:								return D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; break;
	}
}


D3D12_FILTER TextureFilterToDX12(TextureFilter textureFilter)
{
	using enum TextureFilter;

	switch (textureFilter)
	{
	case MinMagMipPoint:						return D3D12_FILTER_MIN_MAG_MIP_POINT; break;
	case MinMagPointMipLinear:					return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR; break;
	case MinPointMagLinearMipPoint:				return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT; break;
	case MinPointMagMipLinear:					return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR; break;
	case MinLinearMagMipPoint:					return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT; break;
	case MinLinearMagPointMipLinear:			return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR; break;
	case MinMagLinearMipPoint:					return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT; break;
	case MinMagMipLinear:						return D3D12_FILTER_MIN_MAG_MIP_LINEAR; break;
	case Anisotropic:							return D3D12_FILTER_ANISOTROPIC; break;

	case ComparisonMinMagMipPoint:				return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT; break;
	case ComparisonMinMagPointMipLinear:		return D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR; break;
	case ComparisonMinPointMagLinearMipPoint:	return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT; break;
	case ComparisonMinPointMagMipLinear:		return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR; break;
	case ComparisonMinLinearMagMipPoint:		return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT; break;
	case ComparisonMinLinearMagPointMipLinear:	return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR; break;
	case ComparisonMinMagLinearMipPoint:		return D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; break;
	case ComparisonMinMagMipLinear:				return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR; break;
	case ComparisonAnisotropic:					return D3D12_FILTER_COMPARISON_ANISOTROPIC; break;

	case MinimumMinMagMipPoint:					return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT; break;
	case MinimumMinMagPointMipLinear:			return D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR; break;
	case MinimumMinPointMagLinearMipPoint:		return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT; break;
	case MinimumMinPointMagMipLinear:			return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR; break;
	case MinimumMinLinearMagMipPoint:			return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT; break;
	case MinimumMinLinearMagPointMipLinear:		return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR; break;
	case MinimumMinMagLinearMipPoint:			return D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT; break;
	case MinimumMinMagMipLinear:				return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR; break;
	case MinimumAnisotropic:					return D3D12_FILTER_MINIMUM_ANISOTROPIC; break;

	case MaximumMinMagMipPoint:					return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT; break;
	case MaximumMinMagPointMipLinear:			return D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR; break;
	case MaximumMinPointMagLinearMipPoint:		return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT; break;
	case MaximumMinPointMagMipLinear:			return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR; break;
	case MaximumMinLinearMagMipPoint:			return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT; break;
	case MaximumMinLinearMagPointMipLinear:		return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR; break;
	case MaximumMinMagLinearMipPoint:			return D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT; break;
	case MaximumMinMagMipLinear:				return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR; break;
	case MaximumAnisotropic:					return D3D12_FILTER_MAXIMUM_ANISOTROPIC; break;

	default:
		assert(false);
		return D3D12_FILTER_MIN_MAG_MIP_POINT;
		break;
	}
}


D3D12_TEXTURE_ADDRESS_MODE TextureAddressToDX12(TextureAddress textureAddress)
{
	using enum TextureAddress;

	switch (textureAddress)
	{
	case Wrap:			return D3D12_TEXTURE_ADDRESS_MODE_WRAP; break;
	case Mirror:		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR; break;
	case Clamp:			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP; break;
	case Border:		return D3D12_TEXTURE_ADDRESS_MODE_BORDER; break;
	case MirrorOnce:	return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE; break;
	default:
		assert(false);
		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		break;
	}
}


D3D12_COMMAND_LIST_TYPE CommandListTypeToDX12(CommandListType commandListType)
{
	using enum CommandListType;

	switch (commandListType)
	{
	case Direct:	return D3D12_COMMAND_LIST_TYPE_DIRECT; break;
	case Bundle:	return D3D12_COMMAND_LIST_TYPE_BUNDLE; break;
	case Compute:	return D3D12_COMMAND_LIST_TYPE_COMPUTE; break;
	case Copy:		return D3D12_COMMAND_LIST_TYPE_COPY; break;
	default:
		assert(false);
		return D3D12_COMMAND_LIST_TYPE_DIRECT;
		break;
	}
}


D3D12_RESOURCE_STATES ResourceStateToDX12(ResourceState resourceState)
{
	using enum ResourceState;

	uint32_t result = 0;
	result |= HasFlag(resourceState, Common) ? D3D12_RESOURCE_STATE_COMMON : 0;
	result |= HasAnyFlag(resourceState, VertexBuffer | ConstantBuffer) ? D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER : 0;
	result |= HasFlag(resourceState, IndexBuffer) ? D3D12_RESOURCE_STATE_INDEX_BUFFER : 0;
	result |= HasFlag(resourceState, RenderTarget) ? D3D12_RESOURCE_STATE_RENDER_TARGET : 0;
	result |= HasFlag(resourceState, UnorderedAccess) ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : 0;
	result |= HasFlag(resourceState, ResourceState::DepthWrite) ? D3D12_RESOURCE_STATE_DEPTH_WRITE : 0;
	result |= HasFlag(resourceState, DepthRead) ? D3D12_RESOURCE_STATE_DEPTH_READ : 0;
	result |= HasFlag(resourceState, ShaderResource) ? result |= (D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) : 0;
	result |= HasFlag(resourceState, StreamOut) ? D3D12_RESOURCE_STATE_STREAM_OUT : 0;
	result |= HasFlag(resourceState, IndirectArgument) ? D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT : 0;
	result |= HasFlag(resourceState, CopyDest) ? D3D12_RESOURCE_STATE_COPY_DEST : 0;
	result |= HasFlag(resourceState, CopySource) ? D3D12_RESOURCE_STATE_COPY_SOURCE : 0;
	result |= HasFlag(resourceState, ResolveDest) ? D3D12_RESOURCE_STATE_RESOLVE_DEST : 0;
	result |= HasFlag(resourceState, ResolveSource) ? D3D12_RESOURCE_STATE_RESOLVE_SOURCE : 0;
	result |= HasFlag(resourceState, Present) ? D3D12_RESOURCE_STATE_PRESENT : 0;
	result |= HasFlag(resourceState, Predication) ? D3D12_RESOURCE_STATE_PREDICATION : 0;
	result |= HasFlag(resourceState, AccelStructRead) ? D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE : 0;
	result |= HasFlag(resourceState, AccelStructWrite) ? D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE : 0;
	result |= HasFlag(resourceState, AccelStructBuildInput) ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE : 0;
	result |= HasFlag(resourceState, AccelStructBuildBlas) ? D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE : 0;
	result |= HasFlag(resourceState, ShadingRateSurface) ? D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE : 0;
	result |= HasFlag(resourceState, OpacityMicromapBuildInput) ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE : 0;
	result |= HasFlag(resourceState, OpacityMicromapWrite) ? D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE : 0;
	result |= HasFlag(resourceState, GenericRead) ? D3D12_RESOURCE_STATE_GENERIC_READ : 0;

	return (D3D12_RESOURCE_STATES)result;
}


D3D12_QUERY_HEAP_TYPE QueryTypeToDX12(QueryType queryHeapType)
{
	using enum QueryType;

	switch (queryHeapType)
	{
	case Occlusion:			return D3D12_QUERY_HEAP_TYPE_OCCLUSION; break;
	case Timestamp:			return D3D12_QUERY_HEAP_TYPE_TIMESTAMP; break;
	case PipelineStats:		return D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS1; break;
	default:
		assert(false);
		return D3D12_QUERY_HEAP_TYPE_OCCLUSION;
		break;
	}
}


D3D12_UAV_DIMENSION GetUAVDimension(ResourceType resourceType)
{
	using enum ResourceType;

	switch (resourceType)
	{
	case Texture1D:
		return D3D12_UAV_DIMENSION_TEXTURE1D;
		break;

	case Texture1D_Array:
		return D3D12_UAV_DIMENSION_TEXTURE1D;
		break;

	case Texture2D:
	case Texture2DMS:
		return D3D12_UAV_DIMENSION_TEXTURE2D;
		break;

	case Texture2D_Array:
	case Texture2DMS_Array:
	case TextureCube:
		return D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		break;

	case IndexBuffer:
	case VertexBuffer:
	case ConstantBuffer:
	case ByteAddressBuffer:
	case IndirectArgsBuffer:
	case StructuredBuffer:
	case TypedBuffer:
	case ReadbackBuffer:
		return D3D12_UAV_DIMENSION_BUFFER;
		break;

	default:
		assert(false);
		return D3D12_UAV_DIMENSION_UNKNOWN;
		break;
	}
}


D3D12_SRV_DIMENSION GetSRVDimension(ResourceType resourceType)
{
	using enum ResourceType;

	switch (resourceType)
	{
	case Texture1D:				return D3D12_SRV_DIMENSION_TEXTURE1D; break;
	case Texture1D_Array:		return D3D12_SRV_DIMENSION_TEXTURE1DARRAY; break;
	case Texture2D:				return D3D12_SRV_DIMENSION_TEXTURE2D; break;
	case Texture2D_Array:		return D3D12_SRV_DIMENSION_TEXTURE2DARRAY; break;
	case Texture2DMS:			return D3D12_SRV_DIMENSION_TEXTURE2DMS; break;
	case Texture2DMS_Array:		return D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY; break;
	case Texture3D:				return D3D12_SRV_DIMENSION_TEXTURE3D; break;
	case TextureCube:			return D3D12_SRV_DIMENSION_TEXTURECUBE; break;
	case TextureCube_Array:		return D3D12_SRV_DIMENSION_TEXTURECUBEARRAY; break;

	case IndexBuffer:
	case VertexBuffer:
	case ConstantBuffer:
	case ByteAddressBuffer:
	case IndirectArgsBuffer:
	case StructuredBuffer:
	case TypedBuffer:
	case ReadbackBuffer:
		return D3D12_SRV_DIMENSION_BUFFER;
		break;

	default:
		assert(false);
		return D3D12_SRV_DIMENSION_UNKNOWN;
		break;
	}
}


D3D12_RESOURCE_DIMENSION GetResourceDimension(ResourceType resourceType)
{
	using enum ResourceType;

	switch (resourceType)
	{
	case Texture1D:
	case Texture1D_Array:
		return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		break;

	case Texture2D:
	case Texture2D_Array:
	case Texture2DMS:
	case Texture2DMS_Array:
	case TextureCube:
	case TextureCube_Array:
		return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		break;

	case Texture3D:
		return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		break;

	case IndexBuffer:
	case VertexBuffer:
	case ConstantBuffer:
	case ByteAddressBuffer:
	case IndirectArgsBuffer:
	case StructuredBuffer:
	case TypedBuffer:
	case ReadbackBuffer:
		return D3D12_RESOURCE_DIMENSION_BUFFER;
		break;

	default:
		assert(false);
		return D3D12_RESOURCE_DIMENSION_UNKNOWN;
	}
}


D3D12_HEAP_TYPE GetHeapType(MemoryAccess memoryAccess)
{
	if (HasFlag(memoryAccess, MemoryAccess::CpuWrite))
	{
		return D3D12_HEAP_TYPE_UPLOAD;
	}
	else if (HasFlag(memoryAccess, MemoryAccess::CpuWrite))
	{
		return D3D12_HEAP_TYPE_READBACK;
	}
	return D3D12_HEAP_TYPE_DEFAULT;
}

} // namespace Luna::DX12