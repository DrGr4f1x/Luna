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
	vkDestroySurfaceKHR(GetInstance(), m_surfaceKHR, nullptr);
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
		vmaDestroyImage(GetAllocator(), m_image, m_allocation);
		m_allocation = VK_NULL_HANDLE;
	}
	m_image = VK_NULL_HANDLE;
}


void CVkSwapchain::Destroy()
{
	auto device = GetDevice();
	vkDeviceWaitIdle(device);
	vkDestroySwapchainKHR(device, m_swapchainKHR, nullptr);
	m_swapchainKHR = VK_NULL_HANDLE;
}


void CVkSemaphore::Destroy()
{
	vkDestroySemaphore(GetDevice(), m_semaphore, nullptr);
}


void CVkDebugUtilsMessenger::Destroy()
{
	vkDestroyDebugUtilsMessengerEXT(GetInstance(), m_messenger, nullptr);
	m_messenger = VK_NULL_HANDLE;
}


void CVkFence::Destroy()
{
	vkDestroyFence(GetDevice(), m_fence, nullptr);
	m_fence = VK_NULL_HANDLE;
}


void CVkCommandPool::Destroy()
{
	vkDestroyCommandPool(GetDevice(), m_commandPool, nullptr);
	m_commandPool = VK_NULL_HANDLE;
}


void CVkImageView::Destroy()
{
	vkDestroyImageView(GetDevice(), m_imageView, nullptr);
	m_imageView = VK_NULL_HANDLE;
}


void CVkBuffer::Destroy()
{
	vmaDestroyBuffer(GetAllocator(), m_buffer, m_allocation);
	m_allocation = VK_NULL_HANDLE;
	m_buffer = VK_NULL_HANDLE;
}


void CVkBufferView::Destroy()
{
	vkDestroyBufferView(GetDevice(), m_bufferView, nullptr);
	m_bufferView = VK_NULL_HANDLE;
}


void CVkDescriptorSetLayout::Destroy()
{
	vkDestroyDescriptorSetLayout(GetDevice(), m_descriptorSetLayout, nullptr);
	m_descriptorSetLayout = VK_NULL_HANDLE;
}


void CVkPipelineLayout::Destroy()
{
	vkDestroyPipelineLayout(GetDevice(), m_pipelineLayout, nullptr);
	m_pipelineLayout = VK_NULL_HANDLE;
}


void CVkPipeline::Destroy()
{
	vkDestroyPipeline(GetDevice(), m_pipeline, nullptr);
	m_pipeline = VK_NULL_HANDLE;
}


void CVkPipelineCache::Destroy()
{
	vkDestroyPipelineCache(GetDevice(), m_pipelineCache, nullptr);
	m_pipelineCache = VK_NULL_HANDLE;
}


void CVkShaderModule::Destroy()
{
	vkDestroyShaderModule(GetDevice(), m_shaderModule, nullptr);
	m_shaderModule = VK_NULL_HANDLE;
}

} // namespace Luna::VK