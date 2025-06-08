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


#include "Graphics\Vulkan\FlagsVK.h"
#include "Graphics\Vulkan\VulkanApi.h"


namespace Luna::VK
{

struct UUID
{
	uint32_t value1{ 0 };
	uint16_t value2{ 0 };
	uint16_t value3{ 0 };
	uint16_t value4{ 0 };
	uint64_t value5{ 0 };
};

inline UUID AsUUID(uint8_t uuid[VK_UUID_SIZE])
{
	UUID result{};
	result.value1 = ((uint32_t)uuid[0] << 24) | ((uint32_t)uuid[1] << 16) | ((uint32_t)uuid[2] << 8) | uuid[3];
	result.value2 = ((uint16_t)uuid[4] << 8) | uuid[5];
	result.value3 = ((uint16_t)uuid[6] << 8) | uuid[7];
	result.value4 = ((uint16_t)uuid[8] << 8) | uuid[9];
	result.value5 = ((uint64_t)uuid[10] << 40) | ((uint64_t)uuid[11] << 32) | ((uint64_t)uuid[12] << 24) | ((uint64_t)uuid[13] << 16) | ((uint64_t)uuid[14] << 8) | uuid[15];
	return result;
}


inline std::string VkTypeToString(UUID uuid)
{
	return std::format("{:08X}-{:04X}-{:04X}-{:04X}-{:012X}",
		uuid.value1, uuid.value2, uuid.value3, uuid.value4, uuid.value5);
}


struct LUID
{
	uint32_t value1{ 0 };
	uint32_t value2{ 0 };
};

inline LUID AsLUID(uint8_t luid[VK_LUID_SIZE])
{
	LUID result{};
	result.value1 = ((uint32_t)luid[0] << 24) | ((uint32_t)luid[1] << 16) | ((uint32_t)luid[2] << 8) | luid[3];
	result.value2 = ((uint32_t)luid[4] << 24) | ((uint32_t)luid[5] << 16) | ((uint32_t)luid[6] << 8) | luid[7];
	return result;
}


inline std::string VkTypeToString(LUID luid)
{
	return std::format("{:08X}-{:08X}", luid.value1, luid.value2);
}


inline std::string VkTypeToString(VkConformanceVersion conformanceVersion)
{
	return std::format("{}.{}.{}.{}", conformanceVersion.major, conformanceVersion.minor, conformanceVersion.subminor, conformanceVersion.patch);
}


inline std::string VkTypeToString(VkDriverId driverId)
{
	switch (driverId)
	{
	case VK_DRIVER_ID_AMD_PROPRIETARY: return "AMD Proprietary"; break;
	case VK_DRIVER_ID_AMD_OPEN_SOURCE: return "AMD Open Source"; break;
	case VK_DRIVER_ID_MESA_RADV: return "Mesa RADV"; break;
	case VK_DRIVER_ID_NVIDIA_PROPRIETARY: return "NVIDIA Proprietary"; break;
	case VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS: return "Intel Proprietary Windows"; break;
	case VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA: return "Intel Open Source Mesa"; break;
	case VK_DRIVER_ID_IMAGINATION_PROPRIETARY: return "Imagination Proprietary"; break;
	case VK_DRIVER_ID_QUALCOMM_PROPRIETARY: return "Qualcomm Proprietary"; break;
	case VK_DRIVER_ID_ARM_PROPRIETARY: return "ARM Proprietary"; break;
	case VK_DRIVER_ID_GOOGLE_SWIFTSHADER: return "Google SwiftShader"; break;
	case VK_DRIVER_ID_GGP_PROPRIETARY: return "GGP Proprietary"; break;
	case VK_DRIVER_ID_BROADCOM_PROPRIETARY: return "Broadcom Proprietary"; break;
	case VK_DRIVER_ID_MESA_LLVMPIPE: return "Mesa LLVMpipe"; break;
	case VK_DRIVER_ID_MOLTENVK: return "MoltenVK"; break;
	case VK_DRIVER_ID_COREAVI_PROPRIETARY: return "CoreAVI Proprietary"; break;
	case VK_DRIVER_ID_JUICE_PROPRIETARY: return "Juice Proprietary"; break;
	case VK_DRIVER_ID_VERISILICON_PROPRIETARY: return "VeriSilicon Proprietary"; break;
	case VK_DRIVER_ID_MESA_TURNIP: return "Mesa Turnip"; break;
	case VK_DRIVER_ID_MESA_V3DV: return "Mesa V3DV"; break;
	case VK_DRIVER_ID_MESA_PANVK: return "Mesa PanVK"; break;
	case VK_DRIVER_ID_SAMSUNG_PROPRIETARY: return "Samsung Proprietary"; break;
	case VK_DRIVER_ID_MESA_VENUS: return "Mesa Venus"; break;
	case VK_DRIVER_ID_MESA_DOZEN: return "Mesa Dozen"; break;
	case VK_DRIVER_ID_MESA_NVK: return "Mesa NVK"; break;
	case VK_DRIVER_ID_IMAGINATION_OPEN_SOURCE_MESA: return "Imagination Open Source Mesa"; break;
	default: return "Unknown"; break;
	}
}


inline std::string VkTypeToString(VkPhysicalDeviceType physicalDeviceType)
{
	switch (physicalDeviceType)
	{
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:	return "Integrated GPU"; break;
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:		return "Discrete GPU"; break;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:		return "Virtual GPU"; break;
	default:										return "Other"; break;
	}
}


inline std::string VkTypeToString(VkPointClippingBehavior pointClippingBehavior)
{
	switch (pointClippingBehavior)
	{
	case VK_POINT_CLIPPING_BEHAVIOR_USER_CLIP_PLANES_ONLY: return "User Clip Planes Only"; break;
	default: return "All Planes"; break;
	}
}


inline std::string VkTypeToString(VkResult result)
{
	switch (result)
	{
	case VK_SUCCESS:												return "VK_SUCCESS"; break;
	case VK_NOT_READY:												return "VK_NOT_READY"; break;
	case VK_TIMEOUT:												return "VK_TIMEOUT"; break;
	case VK_EVENT_SET:												return "VK_EVENT_SET"; break;
	case VK_EVENT_RESET:											return "VK_EVENT_RESET"; break;
	case VK_INCOMPLETE:												return "VK_INCOMPLETE"; break;
	case VK_ERROR_OUT_OF_HOST_MEMORY:								return "VK_ERROR_OUT_OF_HOST_MEMORY"; break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:								return "VK_ERROR_OUT_OF_DEVICE_MEMORY"; break;
	case VK_ERROR_INITIALIZATION_FAILED:							return "VK_ERROR_INITIALIZATION_FAILED"; break;
	case VK_ERROR_DEVICE_LOST:										return "VK_ERROR_DEVICE_LOST"; break;
	case VK_ERROR_MEMORY_MAP_FAILED:								return "VK_ERROR_MEMORY_MAP_FAILED"; break;
	case VK_ERROR_LAYER_NOT_PRESENT:								return "VK_ERROR_LAYER_NOT_PRESENT"; break;
	case VK_ERROR_EXTENSION_NOT_PRESENT:							return "VK_ERROR_EXTENSION_NOT_PRESENT"; break;
	case VK_ERROR_FEATURE_NOT_PRESENT:								return "VK_ERROR_FEATURE_NOT_PRESENT"; break;
	case VK_ERROR_INCOMPATIBLE_DRIVER:								return "VK_ERROR_INCOMPATIBLE_DRIVER"; break;
	case VK_ERROR_TOO_MANY_OBJECTS:									return "VK_ERROR_TOO_MANY_OBJECTS"; break;
	case VK_ERROR_FORMAT_NOT_SUPPORTED:								return "VK_ERROR_FORMAT_NOT_SUPPORTED"; break;
	case VK_ERROR_FRAGMENTED_POOL:									return "VK_ERROR_FRAGMENTED_POOL"; break;
	case VK_ERROR_UNKNOWN:											return "VK_ERROR_UNKNOWN"; break;
	case VK_ERROR_OUT_OF_POOL_MEMORY:								return "VK_ERROR_OUT_OF_POOL_MEMORY"; break;
	case VK_ERROR_INVALID_EXTERNAL_HANDLE:							return "VK_ERROR_INVALID_EXTERNAL_HANDLE"; break;
	case VK_ERROR_FRAGMENTATION:									return "VK_ERROR_FRAGMENTATION"; break;
	case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:					return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS"; break;
	case VK_ERROR_SURFACE_LOST_KHR:									return "VK_ERROR_SURFACE_LOST_KHR"; break;
	case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:							return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR"; break;
	case VK_SUBOPTIMAL_KHR:											return "VK_SUBOPTIMAL_KHR"; break;
	case VK_ERROR_OUT_OF_DATE_KHR:									return "VK_ERROR_OUT_OF_DATE_KHR"; break;
	case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:							return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR"; break;
	case VK_ERROR_VALIDATION_FAILED_EXT:							return "VK_ERROR_VALIDATION_FAILED_EXT"; break;
	case VK_ERROR_INVALID_SHADER_NV:								return "VK_ERROR_INVALID_SHADER_NV"; break;
	case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:		return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT"; break;
	case VK_ERROR_NOT_PERMITTED_EXT:								return "VK_ERROR_NOT_PERMITTED_EXT"; break;
	case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:				return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT"; break;
	case VK_THREAD_IDLE_KHR:										return "VK_THREAD_IDLE_KHR"; break;
	case VK_THREAD_DONE_KHR:										return "VK_THREAD_DONE_KHR"; break;
	case VK_OPERATION_DEFERRED_KHR:									return "VK_OPERATION_DEFERRED_KHR"; break;
	case VK_OPERATION_NOT_DEFERRED_KHR:								return "VK_OPERATION_NOT_DEFERRED_KHR"; break;
	case VK_PIPELINE_COMPILE_REQUIRED_EXT:							return "VK_PIPELINE_COMPILE_REQUIRED_EXT"; break;
	default:
		return "Unknown Error";
		break;
	}
}


inline std::string VkTypeToString(VkShaderFloatControlsIndependence shaderFloatControlsIndependence)
{
	switch (shaderFloatControlsIndependence)
	{
	case VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL: return "All"; break;
	case VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_NONE: return "None"; break;
	default: return "32-bit Only"; break;
	}
}


inline std::string VkQueueFlagsToString(VkQueueFlags queueFlags)
{
	return g_queueFlagsMap.BuildString(queueFlags, '|');
}


inline std::string VkResolveModeFlagsToString(VkResolveModeFlags resolveModeFlags)
{
	return g_resolveModeFlagsMap.BuildString(resolveModeFlags, '|');
}


inline std::string VkSampleCountFlagsToString(VkSampleCountFlags sampleCountFlags)
{
	return g_sampleCountFlagsMap.BuildString(sampleCountFlags);
}


inline std::string VkShaderStageFlagsToString(VkShaderStageFlags shaderStageFlags)
{
	if (shaderStageFlags == VK_SHADER_STAGE_ALL_GRAPHICS)
	{
		return "All Graphics";
	}
	else if (shaderStageFlags == VK_SHADER_STAGE_ALL)
	{
		return "All";
	}
	return g_shaderStageFlagsMap.BuildString(shaderStageFlags);
}


inline std::string VkSubgroupFeatureFlagsToString(VkSubgroupFeatureFlags subgroupFeatureFlags)
{
	return g_subgroupFeatureFlagsMap.BuildString(subgroupFeatureFlags);
}

} // namespace Luna::VK


#define DECLARE_STRING_FORMATTERS(VK_TYPE) \
template <> \
struct std::formatter<VK_TYPE> : public std::formatter<std::string> \
{ \
	auto format(VK_TYPE value, std::format_context& ctx) const \
	{ \
		auto str = Luna::VK::VkTypeToString(value); \
		return std::formatter<std::string>::format(str, ctx); \
	} \
}; \
inline std::ostream& operator<<(VK_TYPE type, std::ostream& os) { os << Luna::VK::VkTypeToString(type); return os; }


DECLARE_STRING_FORMATTERS(Luna::VK::UUID)
DECLARE_STRING_FORMATTERS(Luna::VK::LUID)
DECLARE_STRING_FORMATTERS(VkConformanceVersion)
DECLARE_STRING_FORMATTERS(VkDriverId)
DECLARE_STRING_FORMATTERS(VkPhysicalDeviceType)
DECLARE_STRING_FORMATTERS(VkPointClippingBehavior)
DECLARE_STRING_FORMATTERS(VkResult);
DECLARE_STRING_FORMATTERS(VkShaderFloatControlsIndependence)

#undef DECLARE_STRING_FORMATTERS