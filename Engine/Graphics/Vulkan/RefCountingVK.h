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

#include "Graphics\Vulkan\VulkanCommon.h"

using namespace Microsoft::WRL;


namespace Luna::VK
{

//
// IVkInstance
//
class __declspec(uuid("D7BB5E27-8C83-4AF3-B5F6-92CEF5575057")) IVkInstance : public IUnknown
{
public:
	virtual ~IVkInstance() = default;
	
	virtual VkInstance Get() const noexcept = 0;
};


//
// IVkPhysicalDevice
//
class __declspec(uuid("8B6A21A8-BF5F-4E50-8E86-477EC4430ACF")) IVkPhysicalDevice : public IUnknown
{
public:
	virtual ~IVkPhysicalDevice() = default;

	virtual VkPhysicalDevice Get() const noexcept = 0;
	virtual VkInstance GetInstance() const noexcept = 0;
};


//
// IVkDevice
//
class __declspec(uuid("3FD7DF71-0D71-45DA-A4E7-8DF13020435D")) IVkDevice : public IUnknown
{
public:
	virtual ~IVkDevice() = default;

	virtual VkDevice Get() const noexcept = 0;
	virtual VkPhysicalDevice GetPhysicalDevice() const noexcept = 0;
};


//
// IVkSurface
//
class __declspec(uuid("ACF9E4F7-2643-4878-AC96-E1F83D4BD499")) IVkSurface : public IUnknown
{
public:
	virtual ~IVkSurface() = default;

	virtual VkSurfaceKHR Get() const noexcept = 0;
	virtual VkInstance GetInstance() const noexcept = 0;
};


//
// IVmaAllocator
//
class __declspec(uuid("AA6FC27E-6B32-4246-8366-E4AB1FCAD230")) IVmaAllocator : public IUnknown
{
public:
	virtual ~IVmaAllocator() = default;

	virtual VmaAllocator Get() const noexcept = 0;
	virtual VkDevice GetDevice() const noexcept = 0;
};


//
// IVkImage
//
class __declspec(uuid("23AA6F3B-24A6-40CD-8851-8DDA46A7182A")) IVkImage : public IUnknown
{
public:
	virtual ~IVkImage() = default;

	virtual VkImage Get() const noexcept = 0;
	virtual VkDevice GetDevice() const noexcept = 0;
};


//
// IVkSwapchain
//
class __declspec(uuid("233E85CB-8FEF-4508-ACA2-D91EF3368F39")) IVkSwapchain : public IUnknown
{
public:
	virtual ~IVkSwapchain() = default;

	virtual VkSwapchainKHR Get() const noexcept = 0;
	virtual VkDevice GetDevice() const noexcept = 0;
};


//
// IVkSemaphore
//
class __declspec(uuid("5834F4D9-0933-4FB2-B8E8-F5EF34C19DDB")) IVkSemaphore : public IUnknown
{
public:
	virtual ~IVkSemaphore() = default;

	virtual VkSemaphore Get() const noexcept = 0;
	virtual VkDevice GetDevice() const noexcept = 0;
};


//
// IVkDebugUtilsMessenger
//
class __declspec(uuid("001FDEE9-F24A-4AEA-9078-54609845454B")) IVkDebugUtilsMessenger : public IUnknown
{
public:
	virtual ~IVkDebugUtilsMessenger() = default;

	virtual VkDebugUtilsMessengerEXT Get() const noexcept = 0;
	virtual VkInstance GetInstance() const noexcept = 0;
};


//
// IVkFence
//
class __declspec(uuid("546A4B53-9A05-430E-8F6C-1E2388330910")) IVkFence : public IUnknown
{
public:
	virtual ~IVkFence() = default;

	virtual VkFence Get() const noexcept = 0;
	virtual VkDevice GetDevice() const noexcept = 0;
};



//
// IVkCommandPool
//
class __declspec(uuid("32597150-B24B-4209-8A0E-812C7CE3396A")) IVkCommandPool : public IUnknown
{
public:
	virtual ~IVkCommandPool() = default;

	virtual VkCommandPool Get() const noexcept = 0;
	virtual VkDevice GetDevice() const noexcept = 0;
};

} // namespace Luna::VK