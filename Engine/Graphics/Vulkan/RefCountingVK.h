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
	operator VkInstance() const noexcept { return Get(); }
};


//
// IVkPhysicalDevice
//
class __declspec(uuid("8B6A21A8-BF5F-4E50-8E86-477EC4430ACF")) IVkPhysicalDevice : public IUnknown
{
public:
	virtual ~IVkPhysicalDevice() = default;

	virtual VkPhysicalDevice Get() const noexcept = 0;
	operator VkPhysicalDevice() const noexcept { return Get(); }
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
	operator VkDevice() const noexcept { return Get(); }
	virtual VkPhysicalDevice GetPhysicalDevice() const noexcept = 0;
};


//
// IVkSurfaceKHR
//
class __declspec(uuid("ACF9E4F7-2643-4878-AC96-E1F83D4BD499")) IVkSurfaceKHR : public IUnknown
{
public:
	virtual ~IVkSurfaceKHR() = default;

	virtual VkSurfaceKHR Get() const noexcept = 0;
	operator VkSurfaceKHR() const noexcept { return Get(); }
	virtual VkInstance GetInstance() const noexcept = 0;
};
} // namespace Luna::VK