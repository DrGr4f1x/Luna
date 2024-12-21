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

} // namespace Luna::VK