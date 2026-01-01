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

// Vulkan headers
#define FORCE_VULKAN_VALIDATION 0
#define ENABLE_VULKAN_VALIDATION (ENABLE_VALIDATION || FORCE_VULKAN_VALIDATION)

#define FORCE_VULKAN_DEBUG_MARKERS 0
#define ENABLE_VULKAN_DEBUG_MARKERS (ENABLE_DEBUG_MARKERS || FORCE_VULKAN_DEBUG_MARKERS)

#define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES 1
#include <vulkan\vulkan.h>
#pragma comment(lib, "vulkan-1.lib")

#include <External/volk/volk.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "External\VulkanMemoryAllocator\include\vk_mem_alloc.h"

#include "External/vk-bootstrap/src/VkBootstrap.h"

#define USE_DESCRIPTOR_BUFFERS 1
#define USE_LEGACY_DESCRIPTOR_SETS (!USE_DESCRIPTOR_BUFFERS)