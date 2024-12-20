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

#include "RefCountingImplVK.h"


namespace Luna::VK
{

void CVkInstance::Destroy()
{
	vkDestroyInstance(m_instance, nullptr);
	m_instance = VK_NULL_HANDLE;
}


void CVkPhysicalDevice::Destroy()
{
	m_physicalDevice = VK_NULL_HANDLE;
}


void CVkDevice::Destroy()
{
	if (m_device)
	{
		vkDeviceWaitIdle(m_device);
	}
	vkDestroyDevice(m_device, nullptr);
	m_device = nullptr;
}


void CVkSurface::Destroy()
{
	vkDestroySurfaceKHR(*m_instance, m_surfaceKHR, nullptr);
	m_surfaceKHR = VK_NULL_HANDLE;
}


void CVmaAllocator::Destroy()
{
	vmaDestroyAllocator(m_allocator);
	m_allocator = VK_NULL_HANDLE;
}


void CVkImage::Destroy()
{
	if (m_bOwnsImage)
	{
		vmaDestroyImage(*m_allocator, m_image, m_allocation);
		m_allocation = VK_NULL_HANDLE;
	}
	m_image = VK_NULL_HANDLE;
}


void CVkSwapchain::Destroy()
{
	vkDeviceWaitIdle(*m_device);
	vkDestroySwapchainKHR(*m_device, m_swapchainKHR, nullptr);
	m_swapchainKHR = VK_NULL_HANDLE;
}

} // namespace Luna::VK