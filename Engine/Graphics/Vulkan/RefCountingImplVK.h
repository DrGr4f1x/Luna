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
#include "Graphics\Vulkan\RefCountingVK.h"

using namespace Microsoft::WRL;


namespace Luna::VK
{

//
// VkInstance
//
class __declspec(uuid("D7BB5E27-8C83-4AF3-B5F6-92CEF5575057")) CVkInstance
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IVkInstance>
	, public NonCopyable
{
public:
	CVkInstance() noexcept = default;
	explicit CVkInstance(VkInstance instance) noexcept
		: m_instance{ instance }
	{
	}

	~CVkInstance() final
	{
		Destroy();
	}

	VkInstance Get() const noexcept final { return m_instance; }
	operator VkInstance() const noexcept { return Get(); }

	void Destroy();

private:
	VkInstance m_instance{ VK_NULL_HANDLE };
};


//
// VkPhysicalDevice
//
class __declspec(uuid("9DF46137-E220-42FC-BE3A-9A3695F58F83")) CVkPhysicalDevice
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IVkPhysicalDevice>
	, public NonCopyable
{
public:
	CVkPhysicalDevice() noexcept = default;
	CVkPhysicalDevice(IVkInstance* cinstance, VkPhysicalDevice physicalDevice) noexcept
		: m_instance{ cinstance }
		, m_physicalDevice{ physicalDevice }
	{
	}

	~CVkPhysicalDevice() final
	{
		Destroy();
	}

	VkPhysicalDevice Get() const noexcept final { return m_physicalDevice; }
	operator VkPhysicalDevice() const noexcept { return Get(); }

	VkInstance GetInstance() const noexcept final { return m_instance->Get(); }

	void Destroy();

private:
	wil::com_ptr<IVkInstance> m_instance;
	VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
};


//
// VkDevice
//
class __declspec(uuid("883F7812-D71F-42E1-B6D1-A9FBB842FE78")) CVkDevice
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IVkDevice>
	, public NonCopyable
{
public:
	CVkDevice() noexcept = default;
	CVkDevice(IVkPhysicalDevice* physicalDevice, VkDevice device) noexcept
		: m_physicalDevice{ physicalDevice }
		, m_device{ device }
	{
	}

	~CVkDevice() final
	{
		Destroy();
	}

	VkDevice Get() const noexcept final { return m_device; }
	operator VkDevice() const { return Get(); }

	VkPhysicalDevice GetPhysicalDevice() const noexcept final { return m_physicalDevice->Get(); }

	void Destroy();

private:
	wil::com_ptr<IVkPhysicalDevice> m_physicalDevice;
	VkDevice m_device{ VK_NULL_HANDLE };
};


//
// VkSurfaceKHR
//
class __declspec(uuid("B61E2455-F5E2-40D1-A4D7-06A7ACCAC9D9")) CVkSurface 
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IVkSurface>
	, public NonCopyable
{
public:
	CVkSurface() noexcept = default;
	CVkSurface(IVkInstance* instance, VkSurfaceKHR surface) noexcept
		: m_instance{ instance }
		, m_surfaceKHR{ surface }
	{
	}

	~CVkSurface() final
	{
		Destroy();
	}

	VkSurfaceKHR Get() const noexcept final { return m_surfaceKHR; }
	operator VkSurfaceKHR() const noexcept { return Get(); }

	VkInstance GetInstance() const noexcept final { return m_instance->Get(); }

	void Destroy();

private:
	wil::com_ptr<IVkInstance> m_instance;
	VkSurfaceKHR m_surfaceKHR{ VK_NULL_HANDLE };
};


//
// VmaAllocator
//
class __declspec(uuid("4284F64C-DB1D-4531-ADE0-15B18A4F70AD")) CVmaAllocator 
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IVmaAllocator>
	, public NonCopyable
{
public:
	CVmaAllocator() noexcept = default;
	CVmaAllocator(IVkDevice* device, VmaAllocator allocator) noexcept
		: m_device{ device }
		, m_allocator{ allocator }
	{
	}

	~CVmaAllocator() final
	{
		Destroy();
	}

	VmaAllocator Get() const noexcept final { return m_allocator; }
	operator VmaAllocator() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept final { return m_device->Get(); }

	void Destroy();

private:
	wil::com_ptr<IVkDevice> m_device;
	VmaAllocator m_allocator{ VK_NULL_HANDLE };
};


//
// VkImage
//
class __declspec(uuid("D2D93E23-1D35-4E0D-997D-035CB6BDE2A9")) CVkImage 
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IVkImage>
	, public NonCopyable
{
public:
	CVkImage(IVkDevice* cdevice, VkImage image) noexcept
		: m_device{ cdevice }
		, m_allocator{}
		, m_image{ image }
		, m_allocation{ VK_NULL_HANDLE }
	{
	}

	CVkImage(IVkDevice* device, IVmaAllocator* allocator, VkImage image, VmaAllocation allocation) noexcept
		: m_device{ device }
		, m_allocator{ allocator }
		, m_image{ image }
		, m_allocation{ allocation }
		, m_bOwnsImage{ true }
	{
	}

	~CVkImage() final
	{
		Destroy();
	}

	VkImage Get() const noexcept final { return m_image; }
	operator VkImage() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept final { return m_device->Get(); }

	void Destroy();

private:
	wil::com_ptr<IVkDevice> m_device;
	wil::com_ptr<IVmaAllocator> m_allocator;
	VkImage m_image{ VK_NULL_HANDLE };
	VmaAllocation m_allocation{ VK_NULL_HANDLE };
	bool m_bOwnsImage{ false };
};


//
// VkSwapchainKHR
//
class __declspec(uuid("CEC815F2-5037-49AC-89AA-F865D02D9CAE")) CVkSwapchain 
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IVkSwapchain>
	, public NonCopyable
{
public:
	CVkSwapchain() noexcept = default;
	CVkSwapchain(IVkDevice* device, VkSwapchainKHR swapchain) noexcept
		: m_device{ device }
		, m_swapchainKHR{ swapchain }
	{
	}

	~CVkSwapchain()
	{
		Destroy();
	}

	VkSwapchainKHR Get() const noexcept final { return m_swapchainKHR; }
	operator VkSwapchainKHR() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept final { return m_device->Get(); }

	void Destroy();

private:
	wil::com_ptr<IVkDevice> m_device;
	VkSwapchainKHR m_swapchainKHR{ VK_NULL_HANDLE };
};


//
// VkSemaphore
//
class __declspec(uuid("03D730C5-D309-4AD4-A219-7EC2281BE856")) CVkSemaphore 
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IVkSemaphore>
	, public NonCopyable
{
public:
	CVkSemaphore() noexcept = default;
	CVkSemaphore(IVkDevice* device, VkSemaphore semaphore) noexcept
		: m_device{ device }
		, m_semaphore{ semaphore }
	{
	}

	~CVkSemaphore() final
	{
		Destroy();
	}

	VkSemaphore Get() const noexcept final { return m_semaphore; }
	operator VkSemaphore() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept final { return m_device->Get(); }

	void Destroy();

private:
	wil::com_ptr<IVkDevice> m_device;
	VkSemaphore m_semaphore{ VK_NULL_HANDLE };
};


//
// VkDebugUtilsMessengerEXT
//
class __declspec(uuid("9F3D00C9-FA3B-4137-B89D-265989EE3561")) CVkDebugUtilsMessenger 
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IVkDebugUtilsMessenger>
	, public NonCopyable
{
public:
	CVkDebugUtilsMessenger() noexcept = default;
	CVkDebugUtilsMessenger(IVkInstance* instance, VkDebugUtilsMessengerEXT messenger) noexcept
		: m_instance{ instance }
		, m_messenger{ messenger }
	{
	}

	~CVkDebugUtilsMessenger() final
	{
		Destroy();
	}

	VkDebugUtilsMessengerEXT Get() const noexcept final { return m_messenger; }
	operator VkDebugUtilsMessengerEXT() const noexcept { return Get(); }

	VkInstance GetInstance() const noexcept final { return m_instance->Get(); }

	void Destroy();

private:
	wil::com_ptr<IVkInstance> m_instance;
	VkDebugUtilsMessengerEXT m_messenger{ VK_NULL_HANDLE };
};


//
// VkFence
//
class __declspec(uuid("19767FEC-831E-4D3E-B825-932B7F37A287")) CVkFence 
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IVkFence>
	, public NonCopyable
{
public:
	CVkFence() noexcept = default;
	CVkFence(IVkDevice* device, VkFence fence) noexcept
		: m_device{ device }
		, m_fence{ fence }
	{
	}

	~CVkFence() final
	{
		Destroy();
	}

	VkFence Get() const noexcept final { return m_fence; }
	operator VkFence() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept final { return m_device->Get(); }

	void Destroy();

private:
	wil::com_ptr<IVkDevice> m_device;
	VkFence m_fence{ VK_NULL_HANDLE };
};


//
// VkCommandPool
//
class __declspec(uuid("E9AAB404-DFAC-4DDE-890D-6C9BCF7695C6")) CVkCommandPool 
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IVkCommandPool>
	, public NonCopyable
{
public:
	CVkCommandPool() noexcept = default;
	CVkCommandPool(IVkDevice* device, VkCommandPool commandPool) noexcept
		: m_device{ device }
		, m_commandPool{ commandPool }
	{
	}

	~CVkCommandPool() final
	{
		Destroy();
	}

	VkCommandPool Get() const noexcept final { return m_commandPool; }
	operator VkCommandPool() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept final { return m_device->Get(); }

	void Destroy();

private:
	wil::com_ptr<IVkDevice> m_device;
	VkCommandPool m_commandPool{ VK_NULL_HANDLE };
};

} // namespace Luna::VK