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

#include "Graphics\Vulkan\VulkanApi.h"

#include "Graphics\GraphicsCommon.h"

#include "Graphics\Vulkan\EnumsVK.h"
#include "Graphics\Vulkan\FormatsVK.h"
#include "Graphics\Vulkan\HashVK.h"
#include "Graphics\Vulkan\RefCountingImplVK.h"
#include "Graphics\Vulkan\StringsVK.h"
#include "Graphics\Vulkan\VersionVK.h"
#include "Graphics\Vulkan\VulkanUtil.h"


#define VK_SUCCEEDED(resexpr) VkResult res = resexpr; res == VK_SUCCESS
#define VK_FAILED(resexpr) VkResult res = resexpr; res != VK_SUCCESS

#define VK_FLAGS_NONE 0

#define VK_APPEND_PNEXT(desc) \
	*tail = &desc; \
	tail = &desc.pNext

namespace Luna::VK
{

inline const uint32_t g_requiredVulkanApiVersion = VK_API_VERSION_1_3;
inline LogCategory LogVulkan{ "LogVulkan" };


void SetDebugName(VkDevice device, VkInstance obj, const std::string& name);
void SetDebugName(VkDevice device, VkPhysicalDevice obj, const std::string& name);
void SetDebugName(VkDevice device, VkDevice obj, const std::string& name);
void SetDebugName(VkDevice device, VkQueue obj, const std::string& name);
void SetDebugName(VkDevice device, VkSemaphore obj, const std::string& name);
void SetDebugName(VkDevice device, VkCommandBuffer obj, const std::string& name);
void SetDebugName(VkDevice device, VkFence obj, const std::string& name);
void SetDebugName(VkDevice device, VkDeviceMemory obj, const std::string& name);
void SetDebugName(VkDevice device, VkBuffer obj, const std::string& name);
void SetDebugName(VkDevice device, VkImage obj, const std::string& name);
void SetDebugName(VkDevice device, VkEvent obj, const std::string& name);
void SetDebugName(VkDevice device, VkQueryPool obj, const std::string& name);
void SetDebugName(VkDevice device, VkBufferView obj, const std::string& name);
void SetDebugName(VkDevice device, VkImageView obj, const std::string& name);
void SetDebugName(VkDevice device, VkShaderModule obj, const std::string& name);
void SetDebugName(VkDevice device, VkPipelineCache obj, const std::string& name);
void SetDebugName(VkDevice device, VkPipelineLayout obj, const std::string& name);
void SetDebugName(VkDevice device, VkRenderPass obj, const std::string& name);
void SetDebugName(VkDevice device, VkPipeline obj, const std::string& name);
void SetDebugName(VkDevice device, VkDescriptorSetLayout obj, const std::string& name);
void SetDebugName(VkDevice device, VkSampler obj, const std::string& name);
void SetDebugName(VkDevice device, VkDescriptorPool obj, const std::string& name);
void SetDebugName(VkDevice device, VkDescriptorSet obj, const std::string& name);
void SetDebugName(VkDevice device, VkFramebuffer obj, const std::string& name);
void SetDebugName(VkDevice device, VkCommandPool obj, const std::string& name);
void SetDebugName(VkDevice device, VkSurfaceKHR obj, const std::string& name);
void SetDebugName(VkDevice device, VkSwapchainKHR obj, const std::string& name);
void SetDebugName(VkDevice device, VkDebugReportCallbackEXT obj, const std::string& name);
void SetDebugName(VkDevice device, VkDebugUtilsMessengerEXT obj, const std::string& name);

constexpr bool operator==(const VkImageSubresourceRange& a, const VkImageSubresourceRange& b)
{
	return a.aspectMask == b.aspectMask &&
		a.baseArrayLayer == b.baseArrayLayer &&
		a.baseMipLevel == b.baseMipLevel &&
		a.layerCount == b.layerCount &&
		a.levelCount == b.levelCount;
}


VkFormatProperties GetFormatProperties(VkPhysicalDevice physicalDevice, Format format);

} // namespace Luna::VK
