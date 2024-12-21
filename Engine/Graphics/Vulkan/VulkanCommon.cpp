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

#include "Stdafx.h"

#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#include "VulkanCommon.h"

using namespace std;


namespace Luna::VK
{
#if ENABLE_VULKAN_DEBUG_MARKERS

void SetDebugNameImpl(VkDevice device, uint64_t obj, VkObjectType objType, const string& name)
{
	VkDebugUtilsObjectNameInfoEXT nameInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
	nameInfo.objectType = objType;
	nameInfo.objectHandle = obj;
	nameInfo.pObjectName = name.c_str();
	vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
}


void SetDebugName(VkDevice device, VkInstance obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_INSTANCE, name);
}


void SetDebugName(VkDevice device, VkPhysicalDevice obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_PHYSICAL_DEVICE, name);
}


void SetDebugName(VkDevice device, VkDevice obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_DEVICE, name);
}


void SetDebugName(VkDevice device, VkQueue obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_QUEUE, name);
}


void SetDebugName(VkDevice device, VkSemaphore obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_SEMAPHORE, name);
}


void SetDebugName(VkDevice device, VkCommandBuffer obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_COMMAND_BUFFER, name);
}


void SetDebugName(VkDevice device, VkFence obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_FENCE, name);
}


void SetDebugName(VkDevice device, VkDeviceMemory obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_DEVICE_MEMORY, name);
}


void SetDebugName(VkDevice device, VkBuffer obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_BUFFER, name);
}


void SetDebugName(VkDevice device, VkImage obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_IMAGE, name);
}


void SetDebugName(VkDevice device, VkEvent obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_EVENT, name);
}


void SetDebugName(VkDevice device, VkQueryPool obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_QUERY_POOL, name);
}


void SetDebugName(VkDevice device, VkBufferView obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_BUFFER_VIEW, name);
}


void SetDebugName(VkDevice device, VkImageView obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_IMAGE_VIEW, name);
}


void SetDebugName(VkDevice device, VkShaderModule obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_SHADER_MODULE, name);
}


void SetDebugName(VkDevice device, VkPipelineCache obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_PIPELINE_CACHE, name);
}


void SetDebugName(VkDevice device, VkPipelineLayout obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_PIPELINE_LAYOUT, name);
}


void SetDebugName(VkDevice device, VkRenderPass obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_RENDER_PASS, name);
}


void SetDebugName(VkDevice device, VkPipeline obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_PIPELINE, name);
}


void SetDebugName(VkDevice device, VkDescriptorSetLayout obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, name);
}


void SetDebugName(VkDevice device, VkSampler obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_SAMPLER, name);
}


void SetDebugName(VkDevice device, VkDescriptorPool obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_DESCRIPTOR_POOL, name);
}


void SetDebugName(VkDevice device, VkDescriptorSet obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_DESCRIPTOR_SET, name);
}


void SetDebugName(VkDevice device, VkFramebuffer obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_FRAMEBUFFER, name);
}


void SetDebugName(VkDevice device, VkCommandPool obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_COMMAND_POOL, name);
}


void SetDebugName(VkDevice device, VkSurfaceKHR obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_SURFACE_KHR, name);
}


void SetDebugName(VkDevice device, VkSwapchainKHR obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_SWAPCHAIN_KHR, name);
}


void SetDebugName(VkDevice device, VkDebugReportCallbackEXT obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT, name);
}


void SetDebugName(VkDevice device, VkDebugUtilsMessengerEXT obj, const string& name)
{
	SetDebugNameImpl(device, reinterpret_cast<uint64_t>(obj), VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT, name);
}

#else // ENABLE_VULKAN_DEBUG_MARKERS

void SetDebugName(VkDevice device, VkInstance obj, const string& name) {}
void SetDebugName(VkDevice device, VkPhysicalDevice obj, const string& name) {}
void SetDebugName(VkDevice device, VkDevice obj, const string& name) {}
void SetDebugName(VkDevice device, VkQueue obj, const string& name) {}
void SetDebugName(VkDevice device, VkSemaphore obj, const string& name) {}
void SetDebugName(VkDevice device, VkCommandBuffer obj, const string& name) {}
void SetDebugName(VkDevice device, VkFence obj, const string& name) {}
void SetDebugName(VkDevice device, VkDeviceMemory obj, const string& name) {}
void SetDebugName(VkDevice device, VkBuffer obj, const string& name) {}
void SetDebugName(VkDevice device, VkImage obj, const string& name) {}
void SetDebugName(VkDevice device, VkEvent obj, const string& name) {}
void SetDebugName(VkDevice device, VkQueryPool obj, const string& name) {}
void SetDebugName(VkDevice device, VkBufferView obj, const string& name) {}
void SetDebugName(VkDevice device, VkImageView obj, const string& name) {}
void SetDebugName(VkDevice device, VkShaderModule obj, const string& name) {}
void SetDebugName(VkDevice device, VkPipelineCache obj, const string& name) {}
void SetDebugName(VkDevice device, VkPipelineLayout obj, const string& name) {}
void SetDebugName(VkDevice device, VkRenderPass obj, const string& name) {}
void SetDebugName(VkDevice device, VkPipeline obj, const string& name) {}
void SetDebugName(VkDevice device, VkDescriptorSetLayout obj, const string& name) {}
void SetDebugName(VkDevice device, VkSampler obj, const string& name) {}
void SetDebugName(VkDevice device, VkDescriptorPool obj, const string& name) {}
void SetDebugName(VkDevice device, VkDescriptorSet obj, const string& name) {}
void SetDebugName(VkDevice device, VkFramebuffer obj, const string& name) {}
void SetDebugName(VkDevice device, VkCommandPool obj, const string& name) {}
void SetDebugName(VkDevice device, VkSurfaceKHR obj, const string& name) {}
void SetDebugName(VkDevice device, VkSwapchainKHR obj, const string& name) {}
void SetDebugName(VkDevice device, VkDebugReportCallbackEXT obj, const string& name) {}
void SetDebugName(VkDevice device, VkDebugUtilsMessengerEXT obj, const string& name) {}

#endif // ENABLE_VULKAN_DEBUG_MARKERS

} // namespace Luna::VK
