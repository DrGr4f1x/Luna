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
// VkInstance
//
class __declspec(uuid("D7BB5E27-8C83-4AF3-B5F6-92CEF5575057")) CVkInstance
	: public RefCounted<CVkInstance>
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

	VkInstance Get() const noexcept { return m_instance; }
	operator VkInstance() const noexcept { return Get(); }

	void Destroy();

private:
	VkInstance m_instance{ VK_NULL_HANDLE };
};


//
// VkPhysicalDevice
//
class __declspec(uuid("9DF46137-E220-42FC-BE3A-9A3695F58F83")) CVkPhysicalDevice
	: public RefCounted<CVkPhysicalDevice>
	, public NonCopyable
{
public:
	CVkPhysicalDevice() noexcept = default;
	CVkPhysicalDevice(CVkInstance* cinstance, VkPhysicalDevice physicalDevice) noexcept
		: m_instance{ cinstance }
		, m_physicalDevice{ physicalDevice }
	{
	}

	~CVkPhysicalDevice() final
	{
		Destroy();
	}

	VkPhysicalDevice Get() const noexcept { return m_physicalDevice; }
	operator VkPhysicalDevice() const noexcept { return Get(); }

	VkInstance GetInstance() const noexcept { return m_instance->Get(); }

	void Destroy();

private:
	wil::com_ptr<CVkInstance> m_instance;
	VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
};


//
// VkDevice
//
class __declspec(uuid("883F7812-D71F-42E1-B6D1-A9FBB842FE78")) CVkDevice
	: public RefCounted<CVkDevice>
	, public NonCopyable
{
public:
	CVkDevice() noexcept = default;
	CVkDevice(CVkPhysicalDevice* physicalDevice, VkDevice device) noexcept
		: m_physicalDevice{ physicalDevice }
		, m_device{ device }
	{
	}

	~CVkDevice() final
	{
		Destroy();
	}

	VkDevice Get() const noexcept { return m_device; }
	operator VkDevice() const { return Get(); }

	VkPhysicalDevice GetPhysicalDevice() const noexcept { return m_physicalDevice->Get(); }

	void Destroy();

private:
	wil::com_ptr<CVkPhysicalDevice> m_physicalDevice;
	VkDevice m_device{ VK_NULL_HANDLE };
};


//
// VkSurfaceKHR
//
class __declspec(uuid("B61E2455-F5E2-40D1-A4D7-06A7ACCAC9D9")) CVkSurface 
	: public RefCounted<CVkSurface>
	, public NonCopyable
{
public:
	CVkSurface() noexcept = default;
	CVkSurface(CVkInstance* instance, VkSurfaceKHR surface) noexcept
		: m_instance{ instance }
		, m_surfaceKHR{ surface }
	{
	}

	~CVkSurface() final
	{
		Destroy();
	}

	VkSurfaceKHR Get() const noexcept { return m_surfaceKHR; }
	operator VkSurfaceKHR() const noexcept { return Get(); }

	VkInstance GetInstance() const noexcept { return m_instance->Get(); }

	void Destroy();

private:
	wil::com_ptr<CVkInstance> m_instance;
	VkSurfaceKHR m_surfaceKHR{ VK_NULL_HANDLE };
};


//
// VmaAllocator
//
class __declspec(uuid("4284F64C-DB1D-4531-ADE0-15B18A4F70AD")) CVmaAllocator 
	: public RefCounted<CVmaAllocator>
	, public NonCopyable
{
public:
	CVmaAllocator() noexcept = default;
	CVmaAllocator(CVkDevice* device, VmaAllocator allocator) noexcept
		: m_device{ device }
		, m_allocator{ allocator }
	{
	}

	~CVmaAllocator() final
	{
		Destroy();
	}

	VmaAllocator Get() const noexcept { return m_allocator; }
	operator VmaAllocator() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept { return m_device->Get(); }

	void Destroy();

private:
	wil::com_ptr<CVkDevice> m_device;
	VmaAllocator m_allocator{ VK_NULL_HANDLE };
};


//
// VkImage
//
class __declspec(uuid("D2D93E23-1D35-4E0D-997D-035CB6BDE2A9")) CVkImage 
	: public RefCounted<CVkImage>
	, public NonCopyable
{
public:
	CVkImage(CVkDevice* cdevice, VkImage image) noexcept
		: m_device{ cdevice }
		, m_allocator{}
		, m_image{ image }
		, m_allocation{ VK_NULL_HANDLE }
	{
	}

	CVkImage(CVkDevice* device, CVmaAllocator* allocator, VkImage image, VmaAllocation allocation) noexcept
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

	VkImage Get() const noexcept { return m_image; }
	operator VkImage() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept { return m_device->Get(); }
	VmaAllocator GetAllocator() const noexcept { return m_allocator->Get(); }

	void Destroy();

private:
	wil::com_ptr<CVkDevice> m_device;
	wil::com_ptr<CVmaAllocator> m_allocator;
	VkImage m_image{ VK_NULL_HANDLE };
	VmaAllocation m_allocation{ VK_NULL_HANDLE };
	bool m_bOwnsImage{ false };
};


//
// VkSwapchainKHR
//
class __declspec(uuid("CEC815F2-5037-49AC-89AA-F865D02D9CAE")) CVkSwapchain 
	: public RefCounted<CVkSwapchain>
	, public NonCopyable
{
public:
	CVkSwapchain() noexcept = default;
	CVkSwapchain(CVkDevice* device, VkSwapchainKHR swapchain) noexcept
		: m_device{ device }
		, m_swapchainKHR{ swapchain }
	{
	}

	~CVkSwapchain() final
	{
		Destroy();
	}

	VkSwapchainKHR Get() const noexcept { return m_swapchainKHR; }
	operator VkSwapchainKHR() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept { return m_device->Get(); }

	void Destroy();

private:
	wil::com_ptr<CVkDevice> m_device;
	VkSwapchainKHR m_swapchainKHR{ VK_NULL_HANDLE };
};


//
// VkSemaphore
//
class __declspec(uuid("03D730C5-D309-4AD4-A219-7EC2281BE856")) CVkSemaphore 
	: public RefCounted<CVkSemaphore>
	, public NonCopyable
{
public:
	CVkSemaphore() noexcept = default;
	CVkSemaphore(CVkDevice* device, VkSemaphore semaphore) noexcept
		: m_device{ device }
		, m_semaphore{ semaphore }
	{
	}

	~CVkSemaphore() final
	{
		Destroy();
	}

	VkSemaphore Get() const noexcept { return m_semaphore; }
	operator VkSemaphore() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept { return m_device->Get(); }

	void Destroy();

private:
	wil::com_ptr<CVkDevice> m_device;
	VkSemaphore m_semaphore{ VK_NULL_HANDLE };
};


//
// VkDebugUtilsMessengerEXT
//
class __declspec(uuid("9F3D00C9-FA3B-4137-B89D-265989EE3561")) CVkDebugUtilsMessenger 
	: public RefCounted<CVkDebugUtilsMessenger>
	, public NonCopyable
{
public:
	CVkDebugUtilsMessenger() noexcept = default;
	CVkDebugUtilsMessenger(CVkInstance* instance, VkDebugUtilsMessengerEXT messenger) noexcept
		: m_instance{ instance }
		, m_messenger{ messenger }
	{
	}

	~CVkDebugUtilsMessenger() final
	{
		Destroy();
	}

	VkDebugUtilsMessengerEXT Get() const noexcept { return m_messenger; }
	operator VkDebugUtilsMessengerEXT() const noexcept { return Get(); }

	VkInstance GetInstance() const noexcept { return m_instance->Get(); }

	void Destroy();

private:
	wil::com_ptr<CVkInstance> m_instance;
	VkDebugUtilsMessengerEXT m_messenger{ VK_NULL_HANDLE };
};


//
// VkFence
//
class __declspec(uuid("19767FEC-831E-4D3E-B825-932B7F37A287")) CVkFence 
	: public RefCounted<CVkFence>
	, public NonCopyable
{
public:
	CVkFence() noexcept = default;
	CVkFence(CVkDevice* device, VkFence fence) noexcept
		: m_device{ device }
		, m_fence{ fence }
	{
	}

	~CVkFence() final
	{
		Destroy();
	}

	VkFence Get() const noexcept { return m_fence; }
	operator VkFence() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept { return m_device->Get(); }

	void Destroy();

private:
	wil::com_ptr<CVkDevice> m_device;
	VkFence m_fence{ VK_NULL_HANDLE };
};


//
// VkCommandPool
//
class __declspec(uuid("E9AAB404-DFAC-4DDE-890D-6C9BCF7695C6")) CVkCommandPool 
	: public RefCounted<CVkCommandPool>
	, public NonCopyable
{
public:
	CVkCommandPool() noexcept = default;
	CVkCommandPool(CVkDevice* device, VkCommandPool commandPool) noexcept
		: m_device{ device }
		, m_commandPool{ commandPool }
	{
	}

	~CVkCommandPool() final
	{
		Destroy();
	}

	VkCommandPool Get() const noexcept { return m_commandPool; }
	operator VkCommandPool() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept { return m_device->Get(); }

	void Destroy();

private:
	wil::com_ptr<CVkDevice> m_device;
	VkCommandPool m_commandPool{ VK_NULL_HANDLE };
};


//
// VkImageView
//
class __declspec(uuid("D97705D7-0380-4DF4-89B9-167EF8355085")) CVkImageView 
	: public RefCounted<CVkImageView>
	, public NonCopyable
{
public:
	CVkImageView() noexcept = default;
	CVkImageView(CVkDevice* device, CVkImage* image, VkImageView imageView) noexcept
		: m_device{ device }
		, m_image{ image }
		, m_imageView{ imageView }
	{
	}

	~CVkImageView() final
	{
		Destroy();
	}

	VkImageView Get() const noexcept { return m_imageView; }
	operator VkImageView() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept { return m_device->Get(); }
	VkImage GetImage() const noexcept { return m_image->Get(); }

	void Destroy();

private:
	wil::com_ptr<CVkDevice> m_device;
	wil::com_ptr<CVkImage> m_image;
	VkImageView m_imageView{ VK_NULL_HANDLE };
};


//
// VkBuffer
//
class __declspec(uuid("56B8DE36-FF52-40B2-A736-CD91EE48EAD0")) CVkBuffer
	: public RefCounted<CVkBuffer>
	, public NonCopyable
{
public:
	CVkBuffer() noexcept = default;
	CVkBuffer(CVkDevice* device, CVmaAllocator* allocator, VkBuffer buffer, VmaAllocation allocation) noexcept
		: m_device{ device }
		, m_allocator{ allocator }
		, m_buffer{ buffer }
		, m_allocation{ allocation }
	{}
	~CVkBuffer()
	{
		Destroy();
	}

	VkBuffer Get() const noexcept { return m_buffer; }
	operator VkBuffer() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept { return m_device->Get(); }
	VmaAllocator GetAllocator() const noexcept { return m_allocator->Get(); }
	VmaAllocation GetAllocation() const noexcept { return m_allocation; }

	void Destroy();

private:
	wil::com_ptr<CVkDevice> m_device;
	wil::com_ptr<CVmaAllocator> m_allocator;
	VkBuffer m_buffer{ VK_NULL_HANDLE };
	VmaAllocation m_allocation{ VK_NULL_HANDLE };
};


//
// VkBufferView
//
class __declspec(uuid("E5A4DE6C-365B-4B7D-8203-BE8C7F840A73")) CVkBufferView
	: public RefCounted<CVkBufferView>
	, public NonCopyable
{
public:
	CVkBufferView() noexcept = default;
	CVkBufferView(CVkDevice* device, VkBufferView bufferView)
		: m_device{ device }
		, m_bufferView{ bufferView }
	{}
	~CVkBufferView()
	{
		Destroy();
	}

	VkBufferView Get() const noexcept { return m_bufferView; }
	operator VkBufferView() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept { return m_device->Get(); }

	void Destroy();

private:
	wil::com_ptr<CVkDevice> m_device;
	VkBufferView m_bufferView{ VK_NULL_HANDLE };
};

//
// VkDescriptorSetLayout
//
class __declspec(uuid("69336CFB-EC2C-4D6D-AA13-C3EB3BEA4592")) CVkDescriptorSetLayout
	: public RefCounted<CVkDescriptorSetLayout>
	, public NonCopyable
{
public:
	CVkDescriptorSetLayout() noexcept = default;
	CVkDescriptorSetLayout(CVkDevice* device, VkDescriptorSetLayout descriptorSetLayout) noexcept
		: m_device{ device }
		, m_descriptorSetLayout{ descriptorSetLayout }
	{}
	~CVkDescriptorSetLayout()
	{
		Destroy();
	}

	VkDescriptorSetLayout Get() const noexcept { return m_descriptorSetLayout; }
	operator VkDescriptorSetLayout() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept { return m_device->Get(); }

	void Destroy();

private:
	wil::com_ptr<CVkDevice> m_device;
	VkDescriptorSetLayout m_descriptorSetLayout{ VK_NULL_HANDLE };
};


//
// VkPipelineLayout
//
class __declspec(uuid("789884E6-D174-4398-B552-A28AF8F20152")) CVkPipelineLayout
	: public RefCounted<CVkPipelineLayout>
	, public NonCopyable
{
public:
	CVkPipelineLayout() noexcept = default;
	CVkPipelineLayout(CVkDevice* device, VkPipelineLayout pipelineLayout)
		: m_device{ device }
		, m_pipelineLayout{ pipelineLayout }
	{}
	~CVkPipelineLayout()
	{
		Destroy();
	}

	VkPipelineLayout Get() const noexcept { return m_pipelineLayout; }
	operator VkPipelineLayout() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept { return m_device->Get(); }

	void Destroy();

private:
	wil::com_ptr<CVkDevice> m_device;
	VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };
};


//
// VkPipeline
//
class __declspec(uuid("ABC4D8B2-3413-41C6-AB99-5C53718B6BA0")) CVkPipeline
	: public RefCounted<CVkPipeline>
	, public NonCopyable
{
public:
	CVkPipeline() noexcept = default;
	CVkPipeline(CVkDevice* device , VkPipeline pipeline)
		: m_device{ device }
		, m_pipeline{ pipeline }
	{}
	~CVkPipeline()
	{
		Destroy();
	}

	VkPipeline Get() const noexcept { return m_pipeline; }
	operator VkPipeline() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept { return m_device->Get(); }

	void Destroy();

private:
	wil::com_ptr<CVkDevice> m_device;
	VkPipeline m_pipeline{ VK_NULL_HANDLE };
};


//
// VkPipelineCache
//
class __declspec(uuid("A2F4A397-56FC-4AE4-8221-83A50E383061")) CVkPipelineCache
	: public RefCounted<CVkPipelineCache>
	, public NonCopyable
{
public:
	CVkPipelineCache() noexcept = default;
	CVkPipelineCache(CVkDevice* device, VkPipelineCache pipelineCache)
		: m_device{ device }
		, m_pipelineCache{ pipelineCache }
	{}
	~CVkPipelineCache()
	{
		Destroy();
	}

	VkPipelineCache Get() const noexcept { return m_pipelineCache; }
	operator VkPipelineCache() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept { return m_device->Get(); }

	void Destroy();

private:
	wil::com_ptr<CVkDevice> m_device;
	VkPipelineCache m_pipelineCache{ VK_NULL_HANDLE };
};


//
// VkShaderModule
//
class __declspec(uuid("79662185-8E76-4EE0-A70F-C05FF9C2EA5F")) CVkShaderModule
	: public RefCounted<CVkShaderModule>
	, public NonCopyable
{
public:
	CVkShaderModule() noexcept = default;
	CVkShaderModule(CVkDevice* device, VkShaderModule shaderModule)
		: m_device{ device }
		, m_shaderModule{ shaderModule }
	{}

	~CVkShaderModule()
	{
		Destroy();
	}

	VkShaderModule Get() const noexcept { return m_shaderModule; }
	operator VkShaderModule() const noexcept { return Get(); }

	VkDevice GetDevice() const noexcept { return m_device->Get(); }

	void Destroy();

private:
	wil::com_ptr<CVkDevice> m_device;
	VkShaderModule m_shaderModule{ VK_NULL_HANDLE };
};

} // namespace Luna::VK