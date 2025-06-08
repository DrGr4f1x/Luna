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

#include "Graphics\Formats.h"
#include "Graphics\Vulkan\VulkanApi.h"


namespace Luna::VK
{

VkFormat FormatToVulkan(Format engineFormat);

VkImageAspectFlags GetImageAspect(Format format);

inline bool IsDepthFormat(VkFormat format)
{
	return format == VK_FORMAT_D16_UNORM || 
		format == VK_FORMAT_D24_UNORM_S8_UINT || 
		format == VK_FORMAT_D32_SFLOAT || 
		format == VK_FORMAT_D32_SFLOAT_S8_UINT || 
		format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}


inline bool IsStencilFormat(VkFormat format)
{
	return format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}

} // namespace Luna::VK
