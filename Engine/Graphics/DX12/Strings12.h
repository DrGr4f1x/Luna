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

#include "Flags12.h"


namespace Luna::DX12
{

static const std::string s_notSupported{ "Not supported" };

inline std::string D3DTypeToString(D3D12_COMMAND_LIST_SUPPORT_FLAGS commandListSupportFlags, bool bNumberOnly = false)
{
	return g_commandListSupportFlagsMap.BuildString(commandListSupportFlags);
}


inline std::string D3DTypeToString(D3D12_CONSERVATIVE_RASTERIZATION_TIER conservativeRasterizationTier, bool bNumberOnly = false)
{
	switch (conservativeRasterizationTier)
	{
	case D3D12_CONSERVATIVE_RASTERIZATION_TIER_1: return bNumberOnly ? "1" : "Tier 1"; break;
	case D3D12_CONSERVATIVE_RASTERIZATION_TIER_2: return bNumberOnly ? "2" : "Tier 2"; break;
	case D3D12_CONSERVATIVE_RASTERIZATION_TIER_3: return bNumberOnly ? "3" : "Tier 3"; break;
	default: return s_notSupported; break;
	}
}


inline std::string D3DTypeToString(D3D12_CROSS_NODE_SHARING_TIER crossNodeSharingTier, bool bNumberOnly = false)
{
	switch (crossNodeSharingTier)
	{
	case D3D12_CROSS_NODE_SHARING_TIER_1_EMULATED: return bNumberOnly ? "1 Emulated" : "Tier 1 Emulated"; break;
	case D3D12_CROSS_NODE_SHARING_TIER_1: return bNumberOnly ? "1" : "Tier 1"; break;
	case D3D12_CROSS_NODE_SHARING_TIER_2: return bNumberOnly ? "2" : "Tier 2"; break;
	case D3D12_CROSS_NODE_SHARING_TIER_3: return bNumberOnly ? "3" : "Tier 3";
	default: return s_notSupported; break;
	}
}


inline std::string D3DTypeToString(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, bool bNumberOnly = false)
{
	switch (descriptorHeapType)
	{
	case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:	return "Sampler"; break;
	case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:		return "RTV"; break;
	case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:		return "DSV"; break;
	default:									return "CbvSrvUav"; break;
	}
}


inline std::string D3DTypeToString(D3D_FEATURE_LEVEL featureLevel, bool bNumberOnly = false)
{
	switch (featureLevel)
	{
	case D3D_FEATURE_LEVEL_9_1:		return bNumberOnly ? "9.1" : "D3D_FEATURE_LEVEL_9_1"; break;
	case D3D_FEATURE_LEVEL_9_2:		return bNumberOnly ? "9.2" : "D3D_FEATURE_LEVEL_9_2"; break;
	case D3D_FEATURE_LEVEL_9_3:		return bNumberOnly ? "9.3" : "D3D_FEATURE_LEVEL_9_3"; break;
	case D3D_FEATURE_LEVEL_10_0:	return bNumberOnly ? "10.0" : "D3D_FEATURE_LEVEL_10_0"; break;
	case D3D_FEATURE_LEVEL_10_1:	return bNumberOnly ? "10.1" : "D3D_FEATURE_LEVEL_10_1"; break;
	case D3D_FEATURE_LEVEL_11_0:	return bNumberOnly ? "11.0" : "D3D_FEATURE_LEVEL_11_0"; break;
	case D3D_FEATURE_LEVEL_11_1:	return bNumberOnly ? "11.1" : "D3D_FEATURE_LEVEL_11_1"; break;
	case D3D_FEATURE_LEVEL_12_0:	return bNumberOnly ? "12.0" : "D3D_FEATURE_LEVEL_12_0"; break;
	case D3D_FEATURE_LEVEL_12_1:	return bNumberOnly ? "12.1" : "D3D_FEATURE_LEVEL_12_1"; break;
	case D3D_FEATURE_LEVEL_12_2:	return bNumberOnly ? "12.2" : "D3D_FEATURE_LEVEL_12_2"; break;
	default: return bNumberOnly ? "1.0 Core" : "D3D_FEATURE_LEVEL_1_0_CORE"; break;
	}
}


inline std::string D3DTypeToString(D3D12_MESH_SHADER_TIER meshShaderTier, bool bNumberOnly = false)
{
	return (meshShaderTier == D3D12_MESH_SHADER_TIER_1) ? (bNumberOnly ? "1" : "Tier 1") : s_notSupported;
}


inline std::string D3DTypeToString(D3D12_MESSAGE_CATEGORY messageCategory, bool bNumberOnly = false)
{
	switch (messageCategory)
	{
	case D3D12_MESSAGE_CATEGORY_APPLICATION_DEFINED: return "Application Defined"; break;
	case D3D12_MESSAGE_CATEGORY_MISCELLANEOUS: return "Miscellaneous"; break;
	case D3D12_MESSAGE_CATEGORY_INITIALIZATION: return "Initialization"; break;
	case D3D12_MESSAGE_CATEGORY_CLEANUP: return "Cleanup"; break;
	case D3D12_MESSAGE_CATEGORY_COMPILATION: return "Compilation"; break;
	case D3D12_MESSAGE_CATEGORY_STATE_CREATION: return "State Creation"; break;
	case D3D12_MESSAGE_CATEGORY_STATE_SETTING: return "State Setting"; break;
	case D3D12_MESSAGE_CATEGORY_STATE_GETTING: return "State Getting"; break;
	case D3D12_MESSAGE_CATEGORY_RESOURCE_MANIPULATION: return "Resource Manipulation"; break;
	case D3D12_MESSAGE_CATEGORY_EXECUTION: return "Execution"; break;
	case D3D12_MESSAGE_CATEGORY_SHADER: return "Shader"; break;
	default:
		return "Unknown";
		break;
	}
}


inline std::string D3DTypeToString(D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER programmableSamplePositionsTier, bool bNumberOnly = false)
{
	switch (programmableSamplePositionsTier)
	{
	case D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_1: return bNumberOnly ? "1" : "Tier 1"; break;
	case D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_2: return bNumberOnly ? "2" : "Tier 2"; break;
	default: return s_notSupported; break;
	}
}


inline std::string D3DTypeToString(D3D12_RAYTRACING_TIER raytracingTier, bool bNumberOnly = false)
{
	switch (raytracingTier)
	{
	case D3D12_RAYTRACING_TIER_1_0: return bNumberOnly ? "1.0" : "Tier 1.0"; break;
	case D3D12_RAYTRACING_TIER_1_1: return bNumberOnly ? "1.1" : "Tier 1.1"; break;
	default: return s_notSupported; break;
	}
}


inline std::string D3DTypeToString(D3D12_RENDER_PASS_TIER renderPassTier, bool bNumberOnly = false)
{
	switch (renderPassTier)
	{
	case D3D12_RENDER_PASS_TIER_1: return bNumberOnly ? "1" : "Tier 1"; break;
	case D3D12_RENDER_PASS_TIER_2: return bNumberOnly ? "2" : "Tier 2"; break;
	default: return bNumberOnly ? "0" : "Tier 0"; break;
	}
}


inline std::string D3DTypeToString(D3D12_RESOURCE_BINDING_TIER resourceBindingTier, bool bNumberOnly = false)
{
	switch (resourceBindingTier)
	{
	case D3D12_RESOURCE_BINDING_TIER_2: return bNumberOnly ? "2" : "Tier 2"; break;
	case D3D12_RESOURCE_BINDING_TIER_3: return bNumberOnly ? "3" : "Tier 3"; break;
	default: return bNumberOnly ? "1" : "Tier 1"; break;
	}
}


inline std::string D3DTypeToString(D3D12_RESOURCE_HEAP_TIER resourceHeapTier, bool bNumberOnly = false)
{
	return resourceHeapTier == D3D12_RESOURCE_HEAP_TIER_1 ? (bNumberOnly ? "1" : "Tier 1") : (bNumberOnly ? "2" : "Tier 2");
}


inline std::string D3DTypeToString(D3D12_SAMPLER_FEEDBACK_TIER samplerFeedbackTier, bool bNumberOnly = false)
{
	switch (samplerFeedbackTier)
	{
	case D3D12_SAMPLER_FEEDBACK_TIER_0_9: return bNumberOnly ? "0.9" : "Tier 0.9"; break;
	case D3D12_SAMPLER_FEEDBACK_TIER_1_0: return bNumberOnly ? "1.0" : "Tier 1.0"; break;
	default: return s_notSupported;
	}
}


inline std::string D3DTypeToString(D3D12_SHADER_MIN_PRECISION_SUPPORT shaderMinPrecisionSupport, bool bNumberOnly = false)
{
	if (shaderMinPrecisionSupport == D3D12_SHADER_MIN_PRECISION_SUPPORT_10_BIT)
	{
		return "10-bit";
	}
	else if (shaderMinPrecisionSupport == D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT)
	{
		return "16-bit";
	}
	else if (shaderMinPrecisionSupport == (D3D12_SHADER_MIN_PRECISION_SUPPORT_10_BIT | D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT))
	{
		return "10-bit, 16-bit";
	}
	else
	{
		return "None";
	}
}


inline std::string D3DTypeToString(D3D_SHADER_MODEL shaderModel, bool bNumberOnly = false)
{
	switch (shaderModel)
	{
	case D3D_SHADER_MODEL_6_0: return bNumberOnly ? "6.0" : "D3D_SHADER_MODEL_6_0"; break;
	case D3D_SHADER_MODEL_6_1: return bNumberOnly ? "6.1" : "D3D_SHADER_MODEL_6_1"; break;
	case D3D_SHADER_MODEL_6_2: return bNumberOnly ? "6.2" : "D3D_SHADER_MODEL_6_2"; break;
	case D3D_SHADER_MODEL_6_3: return bNumberOnly ? "6.3" : "D3D_SHADER_MODEL_6_3"; break;
	case D3D_SHADER_MODEL_6_4: return bNumberOnly ? "6.4" : "D3D_SHADER_MODEL_6_4"; break;
	case D3D_SHADER_MODEL_6_5: return bNumberOnly ? "6.5" : "D3D_SHADER_MODEL_6_5"; break;
	case D3D_SHADER_MODEL_6_6: return bNumberOnly ? "6.6" : "D3D_SHADER_MODEL_6_6"; break;
	case D3D_SHADER_MODEL_6_7: return bNumberOnly ? "6.7" : "D3D_SHADER_MODEL_6_7"; break;
	default: return bNumberOnly ? "5.1" : "D3D_SHADER_MODEL_5_1"; break;
	}
}


inline std::string D3DTypeToString(D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER sharedResourceCompatilityTier, bool bNumberOnly = false)
{
	switch (sharedResourceCompatilityTier)
	{
	case D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER_1: return bNumberOnly ? "1" : "Tier1"; break;
	case D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER_2: return bNumberOnly ? "2" : "Tier 2"; break;
	default: return bNumberOnly ? "0" : "Tier 0"; break;
	}
}


inline std::string D3DTypeToString(D3D12_TILED_RESOURCES_TIER tiledResourcesTier, bool bNumberOnly = false)
{
	switch (tiledResourcesTier)
	{
	case D3D12_TILED_RESOURCES_TIER_1: return bNumberOnly ? "1" : "Tier 1"; break;
	case D3D12_TILED_RESOURCES_TIER_2: return bNumberOnly ? "2" : "Tier 2"; break;
	case D3D12_TILED_RESOURCES_TIER_3: return bNumberOnly ? "3" : "Tier 3"; break;
	case D3D12_TILED_RESOURCES_TIER_4: return bNumberOnly ? "4" : "Tier 4"; break;
	default: return s_notSupported; break;
	}
}


inline std::string D3DTypeToString(D3D12_TRI_STATE triState, bool bNumberOnly = false)
{
	switch (triState)
	{
	case D3D12_TRI_STATE_FALSE: return "false"; break;
	case D3D12_TRI_STATE_TRUE: return "true"; break;
	default: return "unknown"; break;
	}
}


inline std::string D3DTypeToString(D3D12_VARIABLE_SHADING_RATE_TIER variableShadingRateTier, bool bNumberOnly = false)
{
	switch (variableShadingRateTier)
	{
	case D3D12_VARIABLE_SHADING_RATE_TIER_1: return bNumberOnly ? "1" : "Tier 1"; break;
	case D3D12_VARIABLE_SHADING_RATE_TIER_2: return bNumberOnly ? "2" : "Tier 2"; break;
	default: return s_notSupported;
	}
}


inline std::string D3DTypeToString(D3D12_VIEW_INSTANCING_TIER viewInstancingTier, bool bNumberOnly = false)
{
	switch (viewInstancingTier)
	{
	case D3D12_VIEW_INSTANCING_TIER_1: return bNumberOnly ? "1" : "Tier 1"; break;
	case D3D12_VIEW_INSTANCING_TIER_2: return bNumberOnly ? "2" : "Tier 2"; break;
	case D3D12_VIEW_INSTANCING_TIER_3: return bNumberOnly ? "3" : "Tier 3"; break;
	default: return s_notSupported;
	}
}


inline std::string D3DTypeToString(D3D12_WAVE_MMA_TIER waveMmaTier, bool bNumberOnly = false)
{
	return (waveMmaTier == D3D12_WAVE_MMA_TIER_1_0) ? (bNumberOnly ? "1.0" : "Tier 1.0") : s_notSupported;
}

} // namespace Luna::DX12


#define DECLARE_STRING_FORMATTERS(D3D_TYPE) \
template <> \
struct std::formatter<D3D_TYPE> : public std::formatter<std::string> \
{ \
	auto format(D3D_TYPE value, std::format_context& ctx) const \
	{ \
		auto str = Luna::DX12::D3DTypeToString(value); \
		return std::formatter<std::string>::format(str, ctx); \
	} \
}; \
inline std::ostream& operator<<(D3D_TYPE type, std::ostream& os) { os << Luna::DX12::D3DTypeToString(type); return os; }

DECLARE_STRING_FORMATTERS(D3D12_COMMAND_LIST_SUPPORT_FLAGS)
DECLARE_STRING_FORMATTERS(D3D12_CONSERVATIVE_RASTERIZATION_TIER)
DECLARE_STRING_FORMATTERS(D3D12_CROSS_NODE_SHARING_TIER)
DECLARE_STRING_FORMATTERS(D3D12_DESCRIPTOR_HEAP_TYPE)
DECLARE_STRING_FORMATTERS(D3D_FEATURE_LEVEL)
DECLARE_STRING_FORMATTERS(D3D12_MESH_SHADER_TIER)
DECLARE_STRING_FORMATTERS(D3D12_MESSAGE_CATEGORY);
DECLARE_STRING_FORMATTERS(D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER)
DECLARE_STRING_FORMATTERS(D3D12_RAYTRACING_TIER)
DECLARE_STRING_FORMATTERS(D3D12_RENDER_PASS_TIER)
DECLARE_STRING_FORMATTERS(D3D12_RESOURCE_BINDING_TIER)
DECLARE_STRING_FORMATTERS(D3D12_RESOURCE_HEAP_TIER)
DECLARE_STRING_FORMATTERS(D3D12_SAMPLER_FEEDBACK_TIER)
DECLARE_STRING_FORMATTERS(D3D12_SHADER_MIN_PRECISION_SUPPORT)
DECLARE_STRING_FORMATTERS(D3D_SHADER_MODEL)
DECLARE_STRING_FORMATTERS(D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER)
DECLARE_STRING_FORMATTERS(D3D12_TILED_RESOURCES_TIER)
DECLARE_STRING_FORMATTERS(D3D12_TRI_STATE)
DECLARE_STRING_FORMATTERS(D3D12_VARIABLE_SHADING_RATE_TIER)
DECLARE_STRING_FORMATTERS(D3D12_VIEW_INSTANCING_TIER)
DECLARE_STRING_FORMATTERS(D3D12_WAVE_MMA_TIER)

#undef DECLARE_STRING_FORMATTERS